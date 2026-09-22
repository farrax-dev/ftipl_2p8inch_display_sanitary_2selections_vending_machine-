// =====================================================
//   SALES LOG — 60-DAY ROLLING HISTORY
// =====================================================
// Three layers, each answering a different question:
//
//   1. DAILY TOTALS (/daily.dat) — a fixed 60-slot table, one slot per
//      calendar day, holding that day's transaction count plus per-product
//      units and revenue. This is what the nightly email and the day-by-day
//      table in the manual report are built from. When a 61st day starts it
//      overwrites the oldest slot, which is exactly the "keep 60 days" rule
//      and needs no separate cleanup pass.
//
//      It's only ~2.6KB, so the whole table lives in RAM and is rewritten to
//      flash whenever it changes. That keeps reads free and makes a power cut
//      cost at most the transaction in progress.
//
//   2. TRANSACTION LOG (/events.csv) — one CSV line per sale, for the
//      attachment. Pruned at each day rollover so it never holds more than
//      the same 60 days.
//
//   3. LIFETIME TOTALS (NVS) — all-time units/revenue/transactions, which
//      survive the 60-day window and are what the on-screen summary shows.
//
// Restock history is deliberately not tracked: current stock on hand is what
// drives refill decisions and the low-stock alert, and that's read live from
// motorStock[] rather than reconstructed from a history.

const char* EVENT_LOG_PATH = "/events.csv";
const char* EVENT_LOG_TMP  = "/events.tmp";
const char* DAILY_DAT_PATH = "/daily.dat";

const int DAILY_HISTORY_DAYS = 60;

// One calendar day. date is YYYYMMDD (20260905); zero marks an unused slot.
//
// openStock is captured when the slot is first created — at the day rollover
// if the machine is powered, otherwise at the first sale of the day. It's what
// makes "started with 20, sold 8, 12 left" possible in the report; without it
// only the closing figure is knowable after the fact.
//
// unitsByPay breaks each product's sales down by payment method, because a
// product sold across UPI and cash in the same day has no single "mode of
// payment" — the report shows the mix instead.
struct DailyRecord {
  uint32_t date;
  uint16_t txns;
  uint16_t units[MAX_PRODUCTS];
  uint32_t revenue[MAX_PRODUCTS];
  uint16_t openStock[MAX_PRODUCTS];
  uint16_t unitsByPay[MAX_PRODUCTS][PAYMENT_COUNT];
  uint16_t txnsByPay[PAYMENT_COUNT];
  uint32_t revByPay[PAYMENT_COUNT];
  uint32_t firstSec;   // seconds past midnight of the day's first sale
  uint32_t lastSec;    // ... and its last, so the report can state the window
};

// Every column the CSV carries, defined once — it's written in four places
// (create, prune, reset, and the row writer) and they must not drift apart.
const char* CSV_HEADER =
  "MachineID,TimeStamp,Product,Unit Price,QTY,Payment Received,Payment Method,Closing Stock";

DailyRecord dailyHistory[DAILY_HISTORY_DAYS];
bool salesLogReady = false;

// ---------- Lifetime totals (NVS) ----------
long lifetimeUnitsSold[MAX_PRODUCTS];
long lifetimeRevenue[MAX_PRODUCTS];
long lifetimeTxnCount = 0;
long lifetimeRevenueTotal = 0;

void loadSalesTotals() {
  char key[16];
  for (int i = 0; i < MAX_PRODUCTS; i++) {
    snprintf(key, sizeof(key), "sq%d", i);
    lifetimeUnitsSold[i] = prefs.getLong(key, 0);
    snprintf(key, sizeof(key), "sa%d", i);
    lifetimeRevenue[i] = prefs.getLong(key, 0);
  }
  lifetimeTxnCount     = prefs.getLong("txncnt", 0);
  lifetimeRevenueTotal = prefs.getLong("revtot", 0);
}

void saveProductTotals(int i) {
  char key[16];
  snprintf(key, sizeof(key), "sq%d", i);
  prefs.putLong(key, lifetimeUnitsSold[i]);
  snprintf(key, sizeof(key), "sa%d", i);
  prefs.putLong(key, lifetimeRevenue[i]);
}

// ---------- Dates ----------
// YYYYMMDD for right now, or 0 when the clock has never been set. Callers
// treat 0 as "can't date this", which is why sales still get logged to the
// CSV but can't land in a daily slot until time syncs.
uint32_t todayDateNum() {
  struct tm ti;
  if (!timeSynced || !getLocalTime(&ti, 10)) return 0;
  return (uint32_t)(ti.tm_year + 1900) * 10000u
       + (uint32_t)(ti.tm_mon + 1) * 100u
       + (uint32_t)ti.tm_mday;
}

void dateNumToString(uint32_t d, char* buf, size_t bufSize) {
  if (d == 0) { snprintf(buf, bufSize, "unknown"); return; }
  snprintf(buf, bufSize, "%02u-%02u-%04u",
           (unsigned)(d % 100), (unsigned)((d / 100) % 100), (unsigned)(d / 10000));
}

// Pulls the date out of a CSV row as a comparable YYYYMMDD number, or 0 when
// the row carries no usable date.
//
// Rows are matched numerically rather than by text prefix. The previous code
// compared the first ten characters of a line against an ISO cutoff, which
// silently stopped working the moment MachineID became the first column:
// "VM-A4C21F" sorts above any date, so every row was kept and the 60-day
// prune never actually dropped anything. Displayed dates are DD-MM-YYYY now,
// which cannot be ordered as text at all, so parsing is the only way.
uint32_t csvRowDateNum(const String &line) {
  int c1 = line.indexOf(',');
  if (c1 < 0) return 0;
  int c2 = line.indexOf(',', c1 + 1);
  if (c2 < 0) return 0;

  String ts = line.substring(c1 + 1, c2);
  int sp = ts.indexOf(' ');
  if (sp < 0) return 0;                 // "no-clock+123s" rows carry no date

  String before = ts.substring(0, sp);
  String after  = ts.substring(sp + 1);
  uint32_t dd = 0, mm = 0, yy = 0;

  if (after.length() >= 10 && after.charAt(2) == '-' && after.charAt(5) == '-') {
    // Current layout: "HH:MM:SS DD-MM-YYYY"
    dd = after.substring(0, 2).toInt();
    mm = after.substring(3, 5).toInt();
    yy = after.substring(6, 10).toInt();
  } else if (before.length() >= 10 && before.charAt(4) == '-' && before.charAt(7) == '-') {
    // Legacy layout: "YYYY-MM-DD HH:MM:SS". Rows written before the format
    // changed are still in the log for up to 60 days, and silently skipping
    // them would empty the attachment on a machine that has just been updated.
    yy = before.substring(0, 4).toInt();
    mm = before.substring(5, 7).toInt();
    dd = before.substring(8, 10).toInt();
  } else {
    return 0;
  }

  if (yy < 2020 || mm < 1 || mm > 12 || dd < 1 || dd > 31) return 0;
  return yy * 10000u + mm * 100u + dd;
}

void salesTimestamp(char* buf, size_t bufSize) {
  struct tm ti;
  if (timeSynced && getLocalTime(&ti, 10)) {
    strftime(buf, bufSize, "%H:%M:%S %d-%m-%Y", &ti);
  } else {
    snprintf(buf, bufSize, "no-clock+%lus", millis() / 1000);
  }
}

// ---------- Daily table ----------
void saveDailyHistory() {
  if (!salesLogReady) return;
  File f = LittleFS.open(DAILY_DAT_PATH, FILE_WRITE);
  if (!f) return;
  f.write((const uint8_t*)dailyHistory, sizeof(dailyHistory));
  f.close();
}

void loadDailyHistory() {
  memset(dailyHistory, 0, sizeof(dailyHistory));
  if (!salesLogReady || !LittleFS.exists(DAILY_DAT_PATH)) return;

  File f = LittleFS.open(DAILY_DAT_PATH, FILE_READ);
  if (!f) return;
  // Size-checked rather than trusted: a firmware change to MAX_PRODUCTS
  // changes the record layout, and reading the old file into the new struct
  // would silently produce nonsense numbers.
  if (f.size() == sizeof(dailyHistory)) {
    f.read((uint8_t*)dailyHistory, sizeof(dailyHistory));
  } else {
    Serial.println("SalesLog: daily.dat layout changed, starting fresh");
  }
  f.close();
}

// Returns the slot for `date`, creating it if needed. When every slot is in
// use the oldest is recycled — that IS the 60-day retention.
int dailySlotFor(uint32_t date) {
  if (date == 0) return -1;

  int oldestIdx = 0;
  uint32_t oldestDate = 0xFFFFFFFF;
  for (int i = 0; i < DAILY_HISTORY_DAYS; i++) {
    if (dailyHistory[i].date == date) return i;
    if (dailyHistory[i].date == 0) {           // free slot, take it
      startDailySlot(i, date);
      return i;
    }
    if (dailyHistory[i].date < oldestDate) {
      oldestDate = dailyHistory[i].date;
      oldestIdx = i;
    }
  }

  startDailySlot(oldestIdx, date);
  return oldestIdx;
}

// Initialises a slot and snapshots the stock the day is starting with, which
// is the only moment that figure is available.
void startDailySlot(int idx, uint32_t date) {
  memset(&dailyHistory[idx], 0, sizeof(DailyRecord));
  dailyHistory[idx].date = date;
  for (int i = 0; i < MAX_PRODUCTS; i++) {
    dailyHistory[idx].openStock[i] = (uint16_t)totalStockForProduct(i);
  }
}

// Opens today's slot before any sale happens, so the opening stock recorded
// is genuinely the morning figure rather than whatever is left after the
// first dispense. Called at the day rollover from the schedule tick and again
// at the top of every dispense, so it is right whether the machine was
// powered overnight or switched on mid-morning.
void ensureTodaySlot() {
  uint32_t today = todayDateNum();
  if (today == 0) return;               // clock not set yet; nothing datable
  if (findDailySlot(today) >= 0) return;
  dailySlotFor(today);
  saveDailyHistory();
}

// Slot indices ordered oldest-to-newest, skipping empties. Used by the
// report builders so the day-by-day table reads chronologically even though
// the slots are recycled in arbitrary order.
int dailySortedIndices(int* out) {
  int n = 0;
  for (int i = 0; i < DAILY_HISTORY_DAYS; i++) {
    if (dailyHistory[i].date != 0) out[n++] = i;
  }
  for (int a = 0; a < n - 1; a++) {
    for (int b = 0; b < n - 1 - a; b++) {
      if (dailyHistory[out[b]].date > dailyHistory[out[b + 1]].date) {
        int t = out[b]; out[b] = out[b + 1]; out[b + 1] = t;
      }
    }
  }
  return n;
}

int findDailySlot(uint32_t date) {
  for (int i = 0; i < DAILY_HISTORY_DAYS; i++) {
    if (dailyHistory[i].date == date) return i;
  }
  return -1;
}

// ---------- Transaction CSV ----------
void csvSanitize(const char* in, char* out, size_t outSize) {
  size_t j = 0;
  for (size_t i = 0; in[i] != '\0' && j < outSize - 1; i++) {
    char c = in[i];
    if (c == ',' || c == '"' || c == '\r' || c == '\n') c = ' ';
    out[j++] = c;
  }
  out[j] = '\0';
}

size_t salesLogSize() {
  if (!salesLogReady) return 0;
  File f = LittleFS.open(EVENT_LOG_PATH, FILE_READ);
  if (!f) return 0;
  size_t n = f.size();
  f.close();
  return n;
}

// One row per product line of an order. The machine id is a column rather
// than a header note so logs from several machines can be concatenated and
// still be told apart.
void appendTxnLine(const char* product, int unitPrice, int qty,
                   int amount, const char* paymentMethod, int stockLeft) {
  if (!salesLogReady) return;
  File f = LittleFS.open(EVENT_LOG_PATH, FILE_APPEND);
  if (!f) return;

  char ts[28], safeId[20], safeProduct[20], safeMethod[16];
  salesTimestamp(ts, sizeof(ts));
  csvSanitize(machineId, safeId, sizeof(safeId));
  csvSanitize(product, safeProduct, sizeof(safeProduct));
  csvSanitize(paymentMethod, safeMethod, sizeof(safeMethod));

  f.printf("%s,%s,%s,%d,%d,%d,%s,%d\n",
           safeId, ts, safeProduct, unitPrice, qty, amount,
           safeMethod, stockLeft);
  f.close();
}

// Maps "UPI"/"Cash"/... back to its index in PAYMENT_NAMES so the daily
// totals can be broken down by method. -1 when the name isn't recognised,
// which keeps an unexpected caller from corrupting the per-method arrays.
int paymentIndexFromName(const char* name) {
  for (int p = 0; p < PAYMENT_COUNT; p++) {
    if (strcmp(name, PAYMENT_NAMES[p]) == 0) return p;
  }
  return -1;
}

// Seconds past midnight, for the "first sale / last sale" window in the
// report. Returns -1 when the clock isn't set.
long secondsIntoDay() {
  struct tm ti;
  if (!timeSynced || !getLocalTime(&ti, 10)) return -1;
  return ti.tm_hour * 3600L + ti.tm_min * 60L + ti.tm_sec;
}

// Rewrites the CSV keeping only lines dated on or after `cutoffDate`. Called
// once per day rollover, not per write — it costs a full file copy.
//
// Lines are timestamped "YYYY-MM-DD ...", so the date compares as a string
// prefix without parsing. Rows written before the clock synced start with
// "no-clock" and are kept: they're recent by definition (the clock syncs
// within a minute of boot) and dropping them would hide real sales.
void pruneEventLog(uint32_t cutoffDate) {
  if (!salesLogReady || cutoffDate == 0) return;

  char cutoffStr[12];
  dateNumToString(cutoffDate, cutoffStr, sizeof(cutoffStr));

  File src = LittleFS.open(EVENT_LOG_PATH, FILE_READ);
  if (!src) return;
  File dst = LittleFS.open(EVENT_LOG_TMP, FILE_WRITE);
  if (!dst) { src.close(); return; }

  dst.println(CSV_HEADER);
  src.readStringUntil('\n');   // drop the old header

  int kept = 0, dropped = 0;
  while (src.available()) {
    String line = src.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    // Undated rows (written before the clock synced) are kept: they are recent
    // by definition, and dropping them would hide real sales.
    uint32_t rowDate = csvRowDateNum(line);
    bool keep = (rowDate == 0) || (rowDate >= cutoffDate);
    if (keep) { dst.println(line); kept++; }
    else dropped++;
  }
  src.close();
  dst.close();

  LittleFS.remove(EVENT_LOG_PATH);
  LittleFS.rename(EVENT_LOG_TMP, EVENT_LOG_PATH);
  Serial.printf("SalesLog: pruned to %s — kept %d rows, dropped %d\n",
                cutoffStr, kept, dropped);
}

// ---------- Init ----------
void initSalesLog() {
  if (!LittleFS.begin(true)) {   // true = format if not yet a LittleFS volume
    Serial.println("SalesLog: LittleFS mount failed - logging disabled");
    salesLogReady = false;
    return;
  }
  salesLogReady = true;

  if (!LittleFS.exists(EVENT_LOG_PATH)) {
    File f = LittleFS.open(EVENT_LOG_PATH, FILE_WRITE);
    if (f) {
      f.println(CSV_HEADER);
      f.close();
    }
  }
  loadDailyHistory();

  int used = 0;
  for (int i = 0; i < DAILY_HISTORY_DAYS; i++) if (dailyHistory[i].date) used++;
  Serial.printf("SalesLog: ready, %d days of history, log %u bytes\n",
                used, (unsigned)salesLogSize());
}

// ---------- Recording ----------
// One call per product line of a completed order. unitPriceOverride lets a
// free vend (RFID) book Rs 0 of revenue for a real dispense instead of the
// catalog price — pass -1 (the normal case) to use products[productIdx].price.
void recordSale(int productIdx, int qty, const char* paymentMethod, int unitPriceOverride) {
  if (productIdx < 0 || productIdx >= MAX_PRODUCTS || qty <= 0) return;

  int unitPrice = (unitPriceOverride >= 0) ? unitPriceOverride : products[productIdx].price;
  int amount = unitPrice * qty;
  int payIdx = paymentIndexFromName(paymentMethod);

  lifetimeUnitsSold[productIdx] += qty;
  lifetimeRevenue[productIdx]   += amount;
  lifetimeRevenueTotal          += amount;
  saveProductTotals(productIdx);
  prefs.putLong("revtot", lifetimeRevenueTotal);

  int slot = dailySlotFor(todayDateNum());
  if (slot >= 0) {
    dailyHistory[slot].units[productIdx]   += qty;
    dailyHistory[slot].revenue[productIdx] += amount;
    if (payIdx >= 0) {
      dailyHistory[slot].unitsByPay[productIdx][payIdx] += qty;
      dailyHistory[slot].revByPay[payIdx] += amount;
    }
    long sec = secondsIntoDay();
    if (sec >= 0) {
      if (dailyHistory[slot].firstSec == 0) dailyHistory[slot].firstSec = (uint32_t)sec;
      dailyHistory[slot].lastSec = (uint32_t)sec;
    }
  }

  appendTxnLine(products[productIdx].name, unitPrice,
                qty, amount, paymentMethod, totalStockForProduct(productIdx));
}

// One per completed order — this is the "number of transactions" figure.
void recordTransaction(const char* paymentMethod) {
  lifetimeTxnCount++;
  prefs.putLong("txncnt", lifetimeTxnCount);

  int slot = dailySlotFor(todayDateNum());
  if (slot >= 0) {
    dailyHistory[slot].txns++;
    int payIdx = paymentIndexFromName(paymentMethod);
    if (payIdx >= 0) dailyHistory[slot].txnsByPay[payIdx]++;
  }

  saveDailyHistory();   // one flash write per order, not per product line
}

void resetSalesTotals() {
  for (int i = 0; i < MAX_PRODUCTS; i++) {
    lifetimeUnitsSold[i] = 0;
    lifetimeRevenue[i] = 0;
    saveProductTotals(i);
  }
  lifetimeTxnCount = 0;
  lifetimeRevenueTotal = 0;
  prefs.putLong("txncnt", 0);
  prefs.putLong("revtot", 0);

  memset(dailyHistory, 0, sizeof(dailyHistory));
  saveDailyHistory();
  if (salesLogReady) {
    LittleFS.remove(EVENT_LOG_PATH);
    File f = LittleFS.open(EVENT_LOG_PATH, FILE_WRITE);
    if (f) { f.println(CSV_HEADER); f.close(); }
  }
}

// ---------- Derived figures ----------
long totalUnitsSoldAll() {
  long t = 0;
  for (int i = 0; i < MAX_PRODUCTS; i++) t += lifetimeUnitsSold[i];
  return t;
}

// A motor counts toward stock only when it's assigned to a product that is
// actually on sale. An unassigned motor, or one holding a disabled product,
// isn't stock anyone can buy — including it in "stock remaining" overstates
// what the machine can dispense and makes refill planning wrong.
bool motorIsActive(int m) {
  int owner = productForMotor(m);
  return owner >= 0 && productEnabled[owner];
}

int totalStockOnHand() {
  int t = 0;
  for (int m = 0; m < MAX_MOTORS; m++) {
    if (motorIsActive(m)) t += motorStock[m];
  }
  return t;
}

int dayUnits(int slot) {
  if (slot < 0) return 0;
  int t = 0;
  for (int i = 0; i < MAX_PRODUCTS; i++) t += dailyHistory[slot].units[i];
  return t;
}

long dayRevenue(int slot) {
  if (slot < 0) return 0;
  long t = 0;
  for (int i = 0; i < MAX_PRODUCTS; i++) t += dailyHistory[slot].revenue[i];
  return t;
}
