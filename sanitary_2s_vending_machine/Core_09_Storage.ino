// ---------- Persistent motor stock storage (NVS via Preferences) ----------
Preferences prefs;
int motorStock[MAX_MOTORS];

void saveMotorStockSlot(int m) {
  char key[8];
  snprintf(key, sizeof(key), "mstock%d", m);
  prefs.putInt(key, motorStock[m]);
}

void decrementMotorStock(int m, int qty) {
  motorStock[m] -= qty;
  if (motorStock[m] < 0) motorStock[m] = 0;
  saveMotorStockSlot(m);
}

// ---------- Global settings (Admin Settings screen) ----------
int maxCartQty = CFG_MAX_CART_QTY;

void saveMaxCartQty() {
  prefs.putInt("maxcartqty", maxCartQty);
}

// ---------- Machine ID ----------
// Shown on the Welcome screen and editable from Admin > Settings, so a site
// with several machines can be identified from the front panel without
// opening anything up.
char machineId[17] = "";

// Human-readable location/name, e.g. "Ladies Room 1". Goes in every email
// subject alongside the ID: the ID is unique but says nothing about where the
// machine is, which is what someone reading a low-stock alert actually needs.
char machineName[40] = "";   // long enough for a full product/location name

void saveMachineId() {
  prefs.putString("machineid", machineId);
}

void saveMachineName() {
  prefs.putString("machname", machineName);
}

// ---------- Admin PIN ----------
// CFG_ADMIN_PIN (Config.h) is only the seed value, same as everything else
// in that file — this buffer is what handleAdminLoginScreen() actually
// compares against, and Admin > Settings > PIN (Screen_11_AdminSettings.ino,
// via Screen_14_AdminTextEntry.ino's TE_ADMIN_PIN) is how it's changed
// without a reflash. Sized to match pinBuffer (Screen_07_AdminLogin.ino) —
// the login pad can't type more than 8 digits, so a longer PIN could never
// be entered back in.
char adminPin[9] = "";

void saveAdminPin() {
  prefs.putString("adminpin", adminPin);
}

// ---------- Email settings (Admin Report screen) ----------
// Compiled-in defaults, same arrangement as DEFAULT_PHONEPE_SALT_KEY in
// Core_06_Network.ino: these seed the values on a board that has never had
// them entered, and the admin screen still overrides them on-site without a
// reflash.
const char* DEFAULT_REPORT_TO = CFG_REPORT_TO;

// ---------- Gmail SMTP (WiFi and 4G) ----------
// The only sender. It reaches Gmail on cellular as well as on WiFi despite
// the modem having no SMTP engine of its own, because the TLS runs on the
// ESP32 over the modem's raw TCP socket — see Core_17_Gmail.ino.
//
// The password is a Google App Password, not the account password — Google
// stopped accepting account passwords for SMTP in 2022. Stored without the
// spaces Google displays it with.
const char* DEFAULT_GMAIL_USER = CFG_GMAIL_USER;
const char* DEFAULT_GMAIL_PASS = CFG_GMAIL_APP_PASSWORD;

char gmailUser[64] = "";
char gmailPass[32] = "";

void saveGmailSettings() {
  prefs.putString("gmailuser", gmailUser);
  prefs.putString("gmailpass", gmailPass);
}

char reportTo[64]     = "";

// Nightly summary times as "HH:MM". An empty string disables that slot, so
// the admin can run one, two, or no automatic sends per day.
char nightlyTime1[6] = "";
char nightlyTime2[6] = "";

bool lowStockAlertOn   = CFG_LOW_STOCK_ALERT;
int  lowStockThreshold = CFG_LOW_STOCK_LEVEL;   // units at or below this trigger the alert

void saveEmailSettings() {
  prefs.putString("reportto", reportTo);
}

void saveScheduleSettings() {
  prefs.putString("nightly1", nightlyTime1);
  prefs.putString("nightly2", nightlyTime2);
  prefs.putInt("lowon", lowStockAlertOn ? 1 : 0);
  prefs.putInt("lowthr", lowStockThreshold);
}

// Default for a machine that's never been given an ID: the last three bytes
// of the ESP32's factory MAC, which is unique per board. That way the ID is
// meaningful out of the box instead of blank, and two units on a bench are
// still tellable apart before anyone configures them.
void defaultMachineId(char* buf, size_t bufSize) {
  uint64_t mac = ESP.getEfuseMac();
  snprintf(buf, bufSize, "VM-%02X%02X%02X",
           (uint8_t)(mac >> 16), (uint8_t)(mac >> 8), (uint8_t)mac);
}

const int PAYMENT_COUNT = 3;
const char* PAYMENT_NAMES[PAYMENT_COUNT] = { "UPI", "Cash", "RFID" };
// Indices into PAYMENT_NAMES/PAYMENT_AVAILABLE/paymentEnabled — named so call
// sites don't have to remember the array order.
const int PAY_IDX_UPI = 0, PAY_IDX_CASH = 1, PAY_IDX_RFID = 2;

// Build-time capability, from Config.h — a method left out here never shows
// up anywhere (admin toggle list or customer payment screen), regardless of
// what a stale paymentEnabled[] NVS value says. This is the "only these
// should be present in the admin menu" list; paymentEnabled[] below is the
// separate on-site ON/OFF within whatever this allows.
const bool PAYMENT_AVAILABLE[PAYMENT_COUNT] = {
  CFG_PAYMENT_UPI_AVAILABLE, CFG_PAYMENT_CASH_AVAILABLE,
  CFG_PAYMENT_RFID_AVAILABLE
};

bool paymentEnabled[PAYMENT_COUNT];

void savePaymentEnabled(int p) {
  char key[10];
  snprintf(key, sizeof(key), "payEnab%d", p);
  prefs.putInt(key, paymentEnabled[p] ? 1 : 0);
}

int getEnabledPaymentMethods(int outIndices[]) {
  int count = 0;
  for (int p = 0; p < PAYMENT_COUNT; p++) {
    if (PAYMENT_AVAILABLE[p] && paymentEnabled[p]) outIndices[count++] = p;
  }
  return count;
}

// True when RFID is the only payment method this machine can actually offer
// right now — the customer never sees a payment method screen at all, every
// sale is "tap a registered card." Admin > Settings uses this to decide
// whether the price-visibility toggle is worth showing.
bool isRFIDOnlyMachine() {
  int idx[PAYMENT_COUNT];
  return getEnabledPaymentMethods(idx) == 1 && idx[0] == PAY_IDX_RFID;
}

// ---------- Product price visibility (RFID-only machines) ----------
// A registered-card machine doesn't charge anything, so the price line on
// the Select screen can be more confusing than useful. Off by default
// everywhere else.
bool hideProductPrices = false;

void saveHideProductPrices() {
  prefs.putInt("hideprices", hideProductPrices ? 1 : 0);
}

void saveClock24Hour() {
  prefs.putInt("clock24h", clock24Hour ? 1 : 0);
}

// ---------- WiFi on/off (Admin > WiFi Setup) ----------
// setWifiEnabled() (Core_06_Network.ino) is what actually calls this — see
// saveLteEnabled() just below for why the live-state side of a toggle isn't
// handled in this storage-only file.
void saveWifiEnabled() {
  prefs.putInt("wifion", wifiEnabled ? 1 : 0);
}

// ---------- LTE on/off (Admin > 4G/LTE Setup) ----------
// setLteEnabled() (Core_13_LTEModem.ino) is what actually calls this — it
// also updates the live connection state, which has no business living in
// this storage-only file.
void saveLteEnabled() {
  prefs.putInt("lteon", lteEnabled ? 1 : 0);
}

// ---------- Registered RFID cards ----------
// Presence on this list is the base payment check for RFID: a match vends
// for free (subject to that card's withdrawal limit below), no match is
// refused. UIDs are stored as uppercase hex strings (however many bytes the
// card/reader reports), compared case-insensitively.
//
// Each card also carries an admin-set name (for telling cards apart on the
// list) and an optional withdrawal limit — the total units that card may
// dispense before it's refused. withdrawLimit == 0 means unlimited.
// withdrawUsed only ever goes up here; it's zeroed either by
// Screen_19_AdminRFIDCardEdit.ino's per-card "Reset Usage" button, or
// automatically for every card at once by the monthly schedule further down
// this file (rfidResetEnabled et al. / maintainRFIDResetSchedule()).
const int MAX_RFID_CARDS = 50;
const int RFID_NAME_MAXLEN = 16;

struct RFIDCard {
  char uid[21];                       // 20 hex chars (10-byte UID) + null
  char name[RFID_NAME_MAXLEN + 1];
  int  withdrawLimit;                 // 0 = unlimited
  int  withdrawUsed;
};
RFIDCard rfidCards[MAX_RFID_CARDS];
int  rfidCardCount = 0;

void loadRFIDCards() {
  rfidCardCount = prefs.getInt("rfidcount", 0);
  if (rfidCardCount < 0) rfidCardCount = 0;
  if (rfidCardCount > MAX_RFID_CARDS) rfidCardCount = MAX_RFID_CARDS;
  for (int i = 0; i < rfidCardCount; i++) {
    char key[10];
    snprintf(key, sizeof(key), "rfid%d", i);
    prefs.getString(key, "").toCharArray(rfidCards[i].uid, sizeof(rfidCards[i].uid));
    snprintf(key, sizeof(key), "rfidn%d", i);
    prefs.getString(key, "").toCharArray(rfidCards[i].name, sizeof(rfidCards[i].name));
    snprintf(key, sizeof(key), "rfidl%d", i);
    rfidCards[i].withdrawLimit = prefs.getInt(key, 0);
    snprintf(key, sizeof(key), "rfidu%d", i);
    rfidCards[i].withdrawUsed = prefs.getInt(key, 0);
  }
}

void saveRFIDCardSlot(int i) {
  char key[10];
  snprintf(key, sizeof(key), "rfid%d", i);
  prefs.putString(key, rfidCards[i].uid);
  snprintf(key, sizeof(key), "rfidn%d", i);
  prefs.putString(key, rfidCards[i].name);
  snprintf(key, sizeof(key), "rfidl%d", i);
  prefs.putInt(key, rfidCards[i].withdrawLimit);
  snprintf(key, sizeof(key), "rfidu%d", i);
  prefs.putInt(key, rfidCards[i].withdrawUsed);
  prefs.putInt("rfidcount", rfidCardCount);
}

// -1 when no card matches — callers that need the row (edit, limit checks)
// use this instead of isCardRegistered()'s plain bool.
int findRFIDCardIndex(const char* uid) {
  for (int i = 0; i < rfidCardCount; i++) {
    if (strcasecmp(rfidCards[i].uid, uid) == 0) return i;
  }
  return -1;
}

bool isCardRegistered(const char* uid) {
  return findRFIDCardIndex(uid) != -1;
}

// Returns false if the card is already registered or the list is full.
bool addRFIDCard(const char* uid) {
  if (strlen(uid) == 0 || rfidCardCount >= MAX_RFID_CARDS) return false;
  if (isCardRegistered(uid)) return false;
  RFIDCard &c = rfidCards[rfidCardCount];
  strncpy(c.uid, uid, sizeof(c.uid) - 1);
  c.uid[sizeof(c.uid) - 1] = '\0';
  c.name[0] = '\0';
  c.withdrawLimit = 0;
  c.withdrawUsed = 0;
  rfidCardCount++;
  saveRFIDCardSlot(rfidCardCount - 1);
  return true;
}

// Shifts every later card down one slot so the stored list never has a gap
// in the middle — loadRFIDCards() only reads rfid0..rfid(count-1).
void removeRFIDCard(int index) {
  if (index < 0 || index >= rfidCardCount) return;
  for (int i = index; i < rfidCardCount - 1; i++) {
    rfidCards[i] = rfidCards[i + 1];
    saveRFIDCardSlot(i);
  }
  rfidCardCount--;
  char key[10];
  snprintf(key, sizeof(key), "rfid%d", rfidCardCount);
  prefs.remove(key);
  snprintf(key, sizeof(key), "rfidn%d", rfidCardCount);
  prefs.remove(key);
  snprintf(key, sizeof(key), "rfidl%d", rfidCardCount);
  prefs.remove(key);
  snprintf(key, sizeof(key), "rfidu%d", rfidCardCount);
  prefs.remove(key);
  prefs.putInt("rfidcount", rfidCardCount);
}

void setRFIDCardName(int i, const char* name) {
  if (i < 0 || i >= rfidCardCount) return;
  strncpy(rfidCards[i].name, name, sizeof(rfidCards[i].name) - 1);
  rfidCards[i].name[sizeof(rfidCards[i].name) - 1] = '\0';
  saveRFIDCardSlot(i);
}

void setRFIDCardLimit(int i, int limit) {
  if (i < 0 || i >= rfidCardCount) return;
  rfidCards[i].withdrawLimit = limit;
  saveRFIDCardSlot(i);
}

void resetRFIDCardUsage(int i) {
  if (i < 0 || i >= rfidCardCount) return;
  rfidCards[i].withdrawUsed = 0;
  saveRFIDCardSlot(i);
}

// Booked after a successful RFID dispense — see handlePaymentRFIDScreen() in
// Screen_06_PaymentOther.ino.
void addRFIDCardUsage(int i, int qty) {
  if (i < 0 || i >= rfidCardCount) return;
  rfidCards[i].withdrawUsed += qty;
  saveRFIDCardSlot(i);
}

// ---------- Automatic monthly usage reset ----------
// withdrawUsed only ever went up before this — Screen_19_AdminRFIDCardEdit.ino's
// per-card "Reset Usage" button was the only way to zero it. This adds a
// machine-wide schedule (Screen_21_AdminRFIDReset.ino) that zeroes every
// registered card's usage on one configured day of the month, at one
// configured time — e.g. "reset on the 1st at 01:00" for a monthly quota.
// Capped to day 1-28 so it always falls inside every month, including
// February.
bool rfidResetEnabled = false;
int  rfidResetDay = 1;     // 1-28
int  rfidResetHour = 1;    // 0-23
int  rfidResetMinute = 0;  // 0-59

// YYYYMM of the last month this actually fired, so a reboot or a machine left
// on past the target minute can't refire it twice in the same month — same
// "already done" marker shape as nightlySentDate (Core_16_ReportSchedule.ino).
// 0 means "never".
uint32_t rfidResetFiredMonth = 0;

void saveRFIDResetSchedule() {
  prefs.putInt("rfidrston", rfidResetEnabled ? 1 : 0);
  prefs.putInt("rfidrstday", rfidResetDay);
  prefs.putInt("rfidrsthr", rfidResetHour);
  prefs.putInt("rfidrstmin", rfidResetMinute);
}

void saveRFIDResetFired() {
  prefs.putULong("rfidrstfired", rfidResetFiredMonth);
}

void loadRFIDResetSchedule() {
  rfidResetEnabled = prefs.getInt("rfidrston", 0) != 0;
  rfidResetDay     = prefs.getInt("rfidrstday", 1);
  rfidResetHour    = prefs.getInt("rfidrsthr", 1);
  rfidResetMinute  = prefs.getInt("rfidrstmin", 0);
  if (rfidResetDay < 1 || rfidResetDay > 28) rfidResetDay = 1;
  if (rfidResetHour < 0 || rfidResetHour > 23) rfidResetHour = 1;
  if (rfidResetMinute < 0 || rfidResetMinute > 59) rfidResetMinute = 0;
  rfidResetFiredMonth = (uint32_t)prefs.getULong("rfidrstfired", 0);
}

// Called once a second from the idle Welcome screen, same call site and
// cadence as maintainReportSchedule() (Core_16_ReportSchedule.ino). Fires at
// most once per calendar month: on rfidResetDay, once the clock reaches
// rfidResetHour:rfidResetMinute, every registered card's withdrawUsed is
// zeroed and the month is marked done. Uses ">=" on the time rather than an
// exact match, so a reset still happens if the machine was off or busy at
// the exact minute — same reasoning as the nightly email slots.
void maintainRFIDResetSchedule() {
  if (!rfidResetEnabled || rfidCardCount == 0) return;

  uint32_t today = todayDateNum();   // Core_14_SalesLog.ino; 0 if clock not synced
  if (today == 0) return;

  uint32_t yyyymm = today / 100;
  int dayOfMonth = (int)(today % 100);
  if (dayOfMonth != rfidResetDay) return;
  if (yyyymm == rfidResetFiredMonth) return;   // already run this month

  int nowMin = currentMinutesOfDay();          // Core_16_ReportSchedule.ino
  if (nowMin < 0 || nowMin < rfidResetHour * 60 + rfidResetMinute) return;

  for (int i = 0; i < rfidCardCount; i++) resetRFIDCardUsage(i);

  rfidResetFiredMonth = yyyymm;
  saveRFIDResetFired();
  Serial.printf("RFID: monthly usage reset fired for %d card(s) (day %d, %02d:%02d)\n",
                rfidCardCount, rfidResetDay, rfidResetHour, rfidResetMinute);
}

// Status line on Screen_18_AdminRFIDCards.ino ("Card added" / "Already
// registered"). Declared here rather than in that tab because
// Screen_14_AdminTextEntry.ino's commitTextEntry() — an earlier tab — sets
// these too, after a manually-typed card number.
char rfidCardsMsg[32] = "";
uint16_t rfidCardsMsgColor = COL_TEXT_DIM;
unsigned long rfidCardsMsgUntil = 0;

// ---------- Coin acceptor denomination config (Admin Coin screen) ----------
const int COIN_DENOM_COUNT = 4;
const int COIN_DENOM_VALUES[COIN_DENOM_COUNT] = { 1, 2, 5, 10 };
bool coinDenomEnabled[COIN_DENOM_COUNT];

void saveCoinDenomEnabled(int d) {
  char key[10];
  snprintf(key, sizeof(key), "coinEnab%d", d);
  prefs.putInt(key, coinDenomEnabled[d] ? 1 : 0);
}

// -1 if value isn't one of COIN_DENOM_VALUES, else true/false for whether
// the admin currently accepts that denomination.
bool isCoinValueEnabled(int value) {
  for (int d = 0; d < COIN_DENOM_COUNT; d++) {
    if (COIN_DENOM_VALUES[d] == value) return coinDenomEnabled[d];
  }
  return false;
}

// ---------- Product data ----------
Product products[MAX_PRODUCTS];
char productNames[MAX_PRODUCTS][13];
bool productEnabled[MAX_PRODUCTS];

// Sized to the full six-slot pool rather than to MAX_PRODUCTS, so lowering
// CFG_PRODUCT_COUNT does not leave the initialiser with more entries than the
// array can hold. Only the first MAX_PRODUCTS are ever read.
const char* DEFAULT_NAMES[]   = { "Regular", "Regular+Wings", "XL Overnight", "Product 4", "Product 5", "Product 6" };
const int   DEFAULT_PRICES[]  = { 10, 15, 20, 10, 10, 10 };
const bool  DEFAULT_ENABLED[] = { true, true, true, false, false, false };
const uint16_t DEFAULT_MASK[] = { 0b000001, 0b000010, 0b000100, 0, 0, 0 };

// A hash of everything in Config.h. Comparing it against the stored value is
// how an edited config is detected without a version number to remember: the
// file's contents ARE the version. FNV-1a, chosen for being three lines long
// rather than for cryptographic strength — the only thing it has to do is
// differ when the text differs.
uint32_t configFingerprint() {
  // CFG_ADMIN_PIN is in this hash specifically so a locked-out admin has a
  // way back in: editing it here and reflashing is the *reset* path (no
  // login needed, just physical access to the unit), separate from the
  // on-screen *change* path (Admin > Settings > PIN, needs the current PIN).
  // Leaving it out of the fingerprint would mean the stored NVS value keeps
  // silently winning even after Config.h is edited — the exact bug this
  // whole seed/fingerprint mechanism exists to avoid, just for this one field.
  String all = String(CFG_MACHINE_NAME) + CFG_MACHINE_ID + CFG_ADMIN_PIN +
               CFG_WIFI_SSID +
               CFG_WIFI_PASSWORD + CFG_GMAIL_USER + CFG_GMAIL_APP_PASSWORD +
               CFG_REPORT_TO + CFG_NIGHTLY_TIME_1 + CFG_NIGHTLY_TIME_2 +
               CFG_UPI_BASE_URL + CFG_UPI_PROVIDER_ID + CFG_UPI_MERCHANT_ID +
               CFG_UPI_SALT_KEY + CFG_UPI_STORE_ID + CFG_UPI_TERMINAL_ID +
               String(CFG_UPI_SALT_INDEX) + String(CFG_UPI_TIMEOUT_MIN) +
               String(CFG_MAX_CART_QTY) +
               String(CFG_LOW_STOCK_LEVEL) + String(CFG_LOW_STOCK_ALERT ? 1 : 0) +
               String(CFG_MOTOR_COUNT) + String(CFG_PRODUCT_COUNT);

  uint32_t h = 2166136261u;
  for (size_t i = 0; i < all.length(); i++) {
    h ^= (uint8_t)all[i];
    h *= 16777619u;
  }
  return h;
}

// ---------- Persistent storage loader & functions ----------
void loadPersistedProductData() {
  prefs.begin("vending", false);

  // Config.h values are seeds, not permanent settings. They are pushed into
  // NVS whenever the file's contents change, and the admin screens own them
  // between flashes. Without this, editing Config.h would appear to do
  // nothing on a machine that already has settings stored, because
  // getString() prefers the stored value.
  uint32_t fp = configFingerprint();
  if (prefs.getULong("cfgfp", 0) != fp) {
    Serial.printf("Config: contents changed, seeding defaults (fingerprint %08X)\n",
                  (unsigned)fp);
    prefs.putString("machname", CFG_MACHINE_NAME);
    if (strlen(CFG_MACHINE_ID) > 0) prefs.putString("machineid", CFG_MACHINE_ID);
    prefs.putString("adminpin", CFG_ADMIN_PIN);
    prefs.putString("wifissid", CFG_WIFI_SSID);
    prefs.putString("wifipass", CFG_WIFI_PASSWORD);
    prefs.putString("gmailuser", CFG_GMAIL_USER);
    prefs.putString("gmailpass", CFG_GMAIL_APP_PASSWORD);
    prefs.putString("reportto", CFG_REPORT_TO);
    prefs.putString("nightly1", CFG_NIGHTLY_TIME_1);
    prefs.putString("nightly2", CFG_NIGHTLY_TIME_2);
    prefs.putInt("lowon", CFG_LOW_STOCK_ALERT ? 1 : 0);
    prefs.putInt("lowthr", CFG_LOW_STOCK_LEVEL);
    prefs.putInt("maxcartqty", CFG_MAX_CART_QTY);
    prefs.putInt("wifion", CFG_WIFI_ENABLED ? 1 : 0);
    prefs.putInt("lteon", CFG_LTE_ENABLED ? 1 : 0);
    // Merchant ID, Store ID and the payment timeout are admin-editable
    // (Screen_12_AdminUPIConfig.ino), so those alone get the seed-then-owned
    // NVS treatment. Every other UPI field is fixed to Config.h below in
    // loadPersistedProductData() and never stored, so there's nothing to seed
    // here for them.
    prefs.putString("upimerchantid", CFG_UPI_MERCHANT_ID);
    prefs.putString("upistoreid", CFG_UPI_STORE_ID);
    prefs.putULong("upitimeoutms", (unsigned long)CFG_UPI_TIMEOUT_MIN * 60000UL);
    prefs.putULong("cfgfp", fp);
  }

  maxCartQty = prefs.getInt("maxcartqty", CFG_MAX_CART_QTY);
  for (int p = 0; p < PAYMENT_COUNT; p++) {
    char key[10];
    snprintf(key, sizeof(key), "payEnab%d", p);
    // A method Config.h doesn't make available is forced off here regardless
    // of what's stored — otherwise a build that later removes a payment
    // method would silently keep honouring an old NVS value for it.
    paymentEnabled[p] = PAYMENT_AVAILABLE[p] && (prefs.getInt(key, 1) != 0);
  }

  hideProductPrices = prefs.getInt("hideprices", 0) != 0;
  clock24Hour = prefs.getInt("clock24h", 0) != 0;

  // Force-ANDed with the compile-time switch, the same way paymentEnabled[]
  // is force-ANDed with PAYMENT_AVAILABLE[] above — a build with no WiFi
  // radio meant to be used at all must never re-enable itself from a stale
  // NVS value, e.g. one carried over from an earlier Config.h that did have
  // WiFi on.
  wifiEnabled = CFG_WIFI_ENABLED && (prefs.getInt("wifion", 1) != 0);

  // Same reasoning, LTE's modem instead of WiFi's radio.
  lteEnabled = CFG_LTE_ENABLED && (prefs.getInt("lteon", 1) != 0);

  loadRFIDCards();
  loadRFIDResetSchedule();

  for (int d = 0; d < COIN_DENOM_COUNT; d++) {
    char key[12];
    snprintf(key, sizeof(key), "coinEnab%d", d);
    coinDenomEnabled[d] = prefs.getInt(key, 1) != 0;
  }

  char idFallback[sizeof(machineId)];
  defaultMachineId(idFallback, sizeof(idFallback));
  prefs.getString("machineid", idFallback).toCharArray(machineId, sizeof(machineId));
  prefs.getString("machname", CFG_MACHINE_NAME).toCharArray(machineName, sizeof(machineName));
  prefs.getString("adminpin", CFG_ADMIN_PIN).toCharArray(adminPin, sizeof(adminPin));

  prefs.getString("reportto", DEFAULT_REPORT_TO).toCharArray(reportTo, sizeof(reportTo));
  prefs.getString("gmailuser", DEFAULT_GMAIL_USER).toCharArray(gmailUser, sizeof(gmailUser));
  prefs.getString("gmailpass", DEFAULT_GMAIL_PASS).toCharArray(gmailPass, sizeof(gmailPass));
  prefs.getString("nightly1", CFG_NIGHTLY_TIME_1).toCharArray(nightlyTime1, sizeof(nightlyTime1));
  prefs.getString("nightly2", CFG_NIGHTLY_TIME_2).toCharArray(nightlyTime2, sizeof(nightlyTime2));
  lowStockAlertOn   = prefs.getInt("lowon", CFG_LOW_STOCK_ALERT ? 1 : 0) != 0;
  lowStockThreshold = prefs.getInt("lowthr", CFG_LOW_STOCK_LEVEL);

  prefs.getString("wifissid", CFG_WIFI_SSID).toCharArray(wifiSSID, sizeof(wifiSSID));
  prefs.getString("wifipass", CFG_WIFI_PASSWORD).toCharArray(wifiPass, sizeof(wifiPass));

  // UPI: Merchant ID, Store ID and the payment timeout are the fields
  // Admin > UPI Configuration can change, so those alone come from NVS
  // (seed-then-owned, same as everything else in this file). Every other
  // PhonePe field is fixed to Config.h — no admin override exists for them,
  // so they're just assigned straight from the CFG_ constants rather than
  // round-tripped through Preferences.
  prefs.getString("upimerchantid", DEFAULT_PHONEPE_MERCHANT_ID).toCharArray(phonepeMerchantId, sizeof(phonepeMerchantId));
  prefs.getString("upistoreid", DEFAULT_PHONEPE_STORE_ID).toCharArray(phonepeStoreId, sizeof(phonepeStoreId));
  strncpy(phonepeBaseUrl, DEFAULT_PHONEPE_BASE_URL, sizeof(phonepeBaseUrl) - 1);
  phonepeBaseUrl[sizeof(phonepeBaseUrl) - 1] = '\0';
  strncpy(phonepeProviderId, DEFAULT_PHONEPE_PROVIDER_ID, sizeof(phonepeProviderId) - 1);
  phonepeProviderId[sizeof(phonepeProviderId) - 1] = '\0';
  strncpy(phonepeSaltKey, DEFAULT_PHONEPE_SALT_KEY, sizeof(phonepeSaltKey) - 1);
  phonepeSaltKey[sizeof(phonepeSaltKey) - 1] = '\0';
  phonepeSaltIndex = DEFAULT_PHONEPE_SALT_INDEX;
  strncpy(phonepeTerminalId, DEFAULT_PHONEPE_TERMINAL_ID, sizeof(phonepeTerminalId) - 1);
  phonepeTerminalId[sizeof(phonepeTerminalId) - 1] = '\0';

  upiTimeoutMs = prefs.getULong("upitimeoutms", (unsigned long)CFG_UPI_TIMEOUT_MIN * 60000UL);
  // Defensive clamp, not a normal path: guards against a value left over from
  // an older build that allowed a wider range than the 1-10 minute one the
  // admin screen enforces today.
  if (upiTimeoutMs < UPI_TIMEOUT_MIN_MS) upiTimeoutMs = UPI_TIMEOUT_MIN_MS;
  if (upiTimeoutMs > UPI_TIMEOUT_MAX_MS) upiTimeoutMs = UPI_TIMEOUT_MAX_MS;

  for (int m = 0; m < MAX_MOTORS; m++) {
    char keyMStock[9];
    snprintf(keyMStock, sizeof(keyMStock), "mstock%d", m);
    motorStock[m] = prefs.getInt(keyMStock, STOCK_MAX);
  }

  for (int i = 0; i < MAX_PRODUCTS; i++) {
    char keyPrice[8], keyName[8], keyEnab[8], keyMask[8];
    snprintf(keyPrice, sizeof(keyPrice), "price%d", i);
    snprintf(keyName,  sizeof(keyName),  "name%d",  i);
    snprintf(keyEnab,  sizeof(keyEnab),  "enab%d",  i);
    snprintf(keyMask,  sizeof(keyMask),  "mask%d",  i);

    products[i].price = prefs.getInt(keyPrice, DEFAULT_PRICES[i]);
    products[i].motorMask = (uint16_t)prefs.getInt(keyMask, DEFAULT_MASK[i]);

    String storedName = prefs.getString(keyName, DEFAULT_NAMES[i]);
    storedName.toCharArray(productNames[i], sizeof(productNames[i]));
    products[i].name = productNames[i];

    productEnabled[i] = prefs.getInt(keyEnab, DEFAULT_ENABLED[i] ? 1 : 0) != 0;
  }

  // Read here while prefs is already open, alongside everything else
  // NVS-backed: lifetime sales counters (Core_14_SalesLog.ino) and the
  // already-sent markers that stop a reboot re-firing a scheduled email
  // (Core_16_ReportSchedule.ino).
  loadSalesTotals();
  loadScheduleState();
}

void saveWiFiCredentials() {
  prefs.putString("wifissid", wifiSSID);
  prefs.putString("wifipass", wifiPass);
}

// Merchant ID, Store ID and the payment timeout only — see
// loadPersistedProductData()'s UPI block above for why the rest of the
// PhonePe fields never reach here.
void saveUPISettings() {
  prefs.putString("upimerchantid", phonepeMerchantId);
  prefs.putString("upistoreid", phonepeStoreId);
  prefs.putULong("upitimeoutms", upiTimeoutMs);
}

void savePriceSlot(int i) {
  char key[8];
  snprintf(key, sizeof(key), "price%d", i);
  prefs.putInt(key, products[i].price);
}

void saveNameSlot(int i) {
  char key[8];
  snprintf(key, sizeof(key), "name%d", i);
  prefs.putString(key, productNames[i]);
}

void saveEnabledSlot(int i) {
  char key[8];
  snprintf(key, sizeof(key), "enab%d", i);
  prefs.putInt(key, productEnabled[i] ? 1 : 0);
}

void saveMaskSlot(int i) {
  char key[8];
  snprintf(key, sizeof(key), "mask%d", i);
  prefs.putInt(key, products[i].motorMask);
}

unsigned long runTimeForProduct(int i) {
  return MOTOR_RUN_MS;
}

int getEnabledProducts(int outIndices[]) {
  int count = 0;
  for (int i = 0; i < MAX_PRODUCTS; i++) {
    if (productEnabled[i]) outIndices[count++] = i;
  }
  return count;
}

int totalStockForProduct(int i) {
  int total = 0;
  for (int m = 0; m < MAX_MOTORS; m++) {
    if (products[i].motorMask & (1 << m)) total += motorStock[m];
  }
  return total;
}

// True once every enabled product has run dry — the point where the machine
// has nothing left to sell at all. Screen_01_Welcome.ino uses this to say so
// on the idle screen itself, rather than letting a customer walk all the way
// to Select Products only to find every card marked OUT OF STOCK.
//
// A build with zero products enabled returns false, not true: that is a
// commissioning problem ("nothing configured"), not a stock problem
// ("everything sold out"), and Select Products already has its own message
// for that case ("No products available").
bool allProductsOutOfStock() {
  int idx[MAX_PRODUCTS];
  int count = getEnabledProducts(idx);
  if (count == 0) return false;
  for (int k = 0; k < count; k++) {
    if (totalStockForProduct(idx[k]) > 0) return false;
  }
  return true;
}

int productForMotor(int m) {
  for (int p = 0; p < MAX_PRODUCTS; p++) {
    if (products[p].motorMask & (1 << m)) return p;
  }
  return -1;
}

void formatMotorList(uint16_t mask, char* buf, size_t bufSize) {
  buf[0] = '\0';
  bool first = true;
  for (int m = 0; m < MAX_MOTORS; m++) {
    if (mask & (1 << m)) {
      char tmp[4];
      snprintf(tmp, sizeof(tmp), first ? "%d" : ",%d", m + 1);
      strncat(buf, tmp, bufSize - strlen(buf) - 1);
      first = false;
    }
  }
  if (buf[0] == '\0') strncpy(buf, "-", bufSize);
}
