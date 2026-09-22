// =====================================================
//   SALES REPORT (bodies + send orchestration)
// =====================================================
// Builds the report bodies and hands them to Core_17_Gmail.ino, which speaks
// SMTP to Gmail over whichever link is up.
//
// A Brevo HTTPS-API path used to sit behind this as a fallback, from when
// Gmail looked impossible on cellular — the modem's firmware has no SMTP
// engine ("AT+SMTPSRV=?" answers ERROR). That turned out not to matter:
// ESP_SSLClient does the TLS on the ESP32 over the modem's raw TCP socket, so
// the ESP32 speaks SMTP itself and Gmail works on 4G. Brevo was then a second
// way to do the same job, costing flash this build could not spare — the
// sketch overflowed a 1.2MB partition by 22KB — and a third-party account for
// mail the customer can already send from their own address.
//
// Three kinds of email:
//   DAILY     — one day's sales, sent automatically at up to two configured
//               times. The one the customer actually reads: what sold today,
//               so they know what to refill.
//   LOW STOCK — fired when a motor crosses the threshold.
//   FULL      — sent manually from the admin panel: 60 days of daily figures,
//               inventory, lifetime totals, plus the transaction CSV attached.
//
// The machine ID is in every subject line, since a customer may run several.

char reportLastStatus[48] = "Not sent yet";
bool reportLastOk = false;

// Upper bound on the raw CSV. Base64 inflates it by 4/3, and past a few
// thousand transactions the email stops being something anyone reads and
// becomes a database dump. SMTP streams the message rather than buffering it,
// so this is a readability limit rather than a transport one.
const size_t CSV_ATTACH_MAX_BYTES = 8192;

// Checked in one place so the send path, the scheduler and the greyed-out
// Send button can't drift apart on what "configured" means.
bool emailConfigured() {
  return strlen(gmailUser) > 0 && strlen(gmailPass) > 0 && strlen(reportTo) > 0;
}

// ---------- Subjects ----------
// "VM-A4C21F | Ladies Room 1 | Daily Sales Summary - 2026-09-06"
// The ID identifies the unit and the name says where it is; a reader glancing
// at an inbox of alerts from several machines needs both. The name is omitted
// rather than left as an empty segment when it hasn't been set.
String reportSubject(ReportKind kind, uint32_t dateNum) {
  char d[12];
  dateNumToString(dateNum, d, sizeof(d));

  String who = String(machineId);
  if (strlen(machineName) > 0) who += " | " + String(machineName);

  switch (kind) {
    case REPORT_DAILY:
      return who + " | Daily Sales Summary - " + String(d);
    case REPORT_LOWSTOCK:
      return who + " | LOW STOCK ALERT - Refill Required";
    default:
      return who + " | 60-Day Sales Report - " + String(d);
  }
}

// ---------- HTML ----------
// Attributes use single quotes throughout. That started out as a necessity —
// the body was embedded in a JSON request and every double quote needed
// escaping — and with the JSON gone it is only a convention now, kept because
// it is consistent and costs nothing.
String htmlHeader(const char* title) {
  char ts[28];
  salesTimestamp(ts, sizeof(ts));
  String h = "<html><body style='font-family:Arial,sans-serif;font-size:13px'>";
  h += "<h2>" + String(title) + "</h2>";
  h += "<p><b>Machine:</b> " + String(machineId) +
       "<br><b>Generated:</b> " + String(ts) + "</p>";
  return h;
}

String htmlInventoryTable() {
  String h = "<h3>Current Stock</h3>";
  h += "<table border=1 cellpadding=5 cellspacing=0>";
  h += "<tr style='background:#eee'><th>Motor</th><th>Product</th><th>Current Stock</th></tr>";
  for (int m = 0; m < MAX_MOTORS; m++) {
    // Unassigned motors, and motors holding a disabled product, aren't
    // sellable stock — listing them overstates what the machine can dispense.
    if (!motorIsActive(m)) continue;
    int owner = productForMotor(m);
    h += "<tr><td>M" + String(m + 1) + "</td>";
    h += "<td>" + String(products[owner].name) + "</td>";
    if (motorStock[m] <= lowStockThreshold) {
      h += "<td align=right style='color:#c00'><b>" + String(motorStock[m]) + " LOW</b></td></tr>";
    } else {
      h += "<td align=right>" + String(motorStock[m]) + "</td></tr>";
    }
  }
  // Total belongs in the table as its footer row, not stranded underneath it.
  h += "<tr style='background:#eee'><td><b>Total</b></td><td></td>";
  h += "<td align=right><b>" + String(totalStockOnHand()) + "</b></td></tr>";
  h += "</table>";
  return h;
}

// "09:14" from seconds-past-midnight, or a dash when nothing was recorded.
String hhmm(uint32_t sec, bool valid) {
  if (!valid) return "-";
  char b[8];
  snprintf(b, sizeof(b), "%02u:%02u", (unsigned)(sec / 3600), (unsigned)((sec / 60) % 60));
  return String(b);
}

// Compact "UPI 6, Cash 2" for one product's day, so a row can show the mix
// rather than pretending there was a single payment mode.
String payMixForProduct(int slot, int productIdx) {
  if (slot < 0) return "-";
  String s = "";
  for (int p = 0; p < PAYMENT_COUNT; p++) {
    uint16_t n = dailyHistory[slot].unitsByPay[productIdx][p];
    if (n == 0) continue;
    if (s.length()) s += ", ";
    s += String(PAYMENT_NAMES[p]) + " " + String(n);
  }
  return s.length() ? s : "-";
}

// The daily email. Answers, in order: what period is this, how did the day go
// overall, what happened to each product, and how did people pay.
String buildDailyHTML(uint32_t dateNum, bool hasCsv) {
  int slot = findDailySlot(dateNum);
  char d[12], now[28];
  dateNumToString(dateNum, d, sizeof(d));
  salesTimestamp(now, sizeof(now));

  String h;
  h.reserve(4000);
  h += htmlHeader("Daily Sales Summary");

  // 1. The window this report covers, stated explicitly so a reader never has
  //    to guess whether "today" means a calendar day or a rolling 24 hours.
  h += "<p><b>Period covered:</b> 00:00:00 " + String(d) + " &nbsp;to&nbsp; " +
       String(now) + "<br>";
  bool anySale = (slot >= 0 && dailyHistory[slot].txns > 0);
  h += "<b>First sale:</b> " + hhmm(slot >= 0 ? dailyHistory[slot].firstSec : 0, anySale) +
       " &nbsp;&nbsp;<b>Last sale:</b> " +
       hhmm(slot >= 0 ? dailyHistory[slot].lastSec : 0, anySale) + "</p>";

  // 2. Headline figures.
  h += "<h3>Financial Summary</h3><table border=1 cellpadding=5 cellspacing=0>";
  h += "<tr><td>Total Transactions</td><td align=right><b>" +
       String(slot >= 0 ? dailyHistory[slot].txns : 0) + "</b></td></tr>";
  h += "<tr><td>Units Dispensed</td><td align=right><b>" + String(dayUnits(slot)) +
       "</b></td></tr>";
  h += "<tr><td>Total Payment Received</td><td align=right><b>Rs " +
       String(dayRevenue(slot)) + "</b></td></tr>";
  h += "<tr><td>Current Stock</td><td align=right><b>" + String(totalStockOnHand()) +
       "</b></td></tr>";
  h += "</table>";

  // 3. Per-product movement: opened with, sold, left, at what price, paid how.
  h += "<h3>Product Performance</h3><table border=1 cellpadding=5 cellspacing=0>";
  h += "<tr style='background:#eee'><th>Product</th><th>Unit Price</th>"
       "<th>Opening Stock</th><th>Units Sold</th><th>Current Stock</th>"
       "<th>Payment Received</th><th>Payment Method</th></tr>";
  long totOpen = 0, totSold = 0, totLeft = 0, totRs = 0;
  for (int i = 0; i < MAX_PRODUCTS; i++) {
    bool sold = (slot >= 0 && dailyHistory[slot].units[i] > 0);
    if (!productEnabled[i] && !sold) continue;

    int open  = (slot >= 0) ? dailyHistory[slot].openStock[i] : totalStockForProduct(i);
    int units = sold ? dailyHistory[slot].units[i] : 0;
    long rs   = sold ? dailyHistory[slot].revenue[i] : 0;
    int left  = totalStockForProduct(i);
    totSold += units; totRs += rs;
    // A product that has since been disabled still shows its sales — that was
    // real money — but its stock is not on offer, so it neither shows a
    // remaining figure nor counts toward the totals.
    if (productEnabled[i]) { totOpen += open; totLeft += left; }

    h += "<tr><td>" + String(products[i].name) +
         (productEnabled[i] ? "" : " <i>(disabled)</i>") + "</td>";
    h += "<td align=right>" + String(products[i].price) + "</td>";
    h += "<td align=right>" + String(productEnabled[i] ? String(open) : String("-")) + "</td>";
    h += "<td align=right>" + String(units) + "</td>";
    if (!productEnabled[i]) {
      h += "<td align=right>-</td>";
    } else if (left <= lowStockThreshold) {
      h += "<td align=right style='color:#c00'><b>" + String(left) + "</b></td>";
    } else {
      h += "<td align=right>" + String(left) + "</td>";
    }
    h += "<td align=right>" + String(rs) + "</td>";
    h += "<td>" + payMixForProduct(slot, i) + "</td></tr>";
  }
  h += "<tr style='background:#eee'><td><b>TOTAL</b></td><td></td>";
  h += "<td align=right><b>" + String(totOpen) + "</b></td>";
  h += "<td align=right><b>" + String(totSold) + "</b></td>";
  h += "<td align=right><b>" + String(totLeft) + "</b></td>";
  h += "<td align=right><b>" + String(totRs) + "</b></td><td></td></tr>";
  h += "</table>";

  // 4. Payment split across the day.
  h += "<h3>Revenue by Payment Method</h3>";
  h += "<table border=1 cellpadding=5 cellspacing=0>";
  h += "<tr style='background:#eee'><th>Payment Method</th><th>Transactions</th>"
       "<th>Payment Received</th></tr>";
  for (int p = 0; p < PAYMENT_COUNT; p++) {
    uint16_t n = (slot >= 0) ? dailyHistory[slot].txnsByPay[p] : 0;
    uint32_t rs = (slot >= 0) ? dailyHistory[slot].revByPay[p] : 0;
    if (n == 0 && rs == 0) continue;
    h += "<tr><td>" + String(PAYMENT_NAMES[p]) + "</td>";
    h += "<td align=right>" + String(n) + "</td>";
    h += "<td align=right>" + String(rs) + "</td></tr>";
  }
  h += "<tr style='background:#eee'><td><b>TOTAL</b></td><td align=right><b>" +
       String(slot >= 0 ? dailyHistory[slot].txns : 0) + "</b></td>";
  h += "<td align=right><b>" + String(dayRevenue(slot)) + "</b></td></tr>";
  h += "</table>";

  if (!anySale) h += "<p style='color:#888'>No sales were recorded in this period.</p>";

  h += htmlInventoryTable();
  if (hasCsv) {
    h += "<p style='color:#666;font-size:11px'>The attached CSV lists every "
         "individual transaction behind these figures.</p>";
  }
  h += "</body></html>";
  return h;
}

String buildLowStockHTML() {
  String h;
  h.reserve(1800);
  h += htmlHeader("Low Stock Alert");
  h += "<p>The following are at or below " + String(lowStockThreshold) +
       " unit(s) and need refilling:</p>";
  h += "<table border=1 cellpadding=5 cellspacing=0>";
  h += "<tr style='background:#eee'><th>Motor</th><th>Product</th>"
       "<th>Current Stock</th></tr>";
  for (int m = 0; m < MAX_MOTORS; m++) {
    int owner = productForMotor(m);
    if (!motorIsActive(m) || motorStock[m] > lowStockThreshold) continue;
    h += "<tr><td>M" + String(m + 1) + "</td>";
    h += "<td>" + String(products[owner].name) + "</td>";
    h += "<td align=right style='color:#c00'><b>" + String(motorStock[m]) + "</b></td></tr>";
  }
  h += "</table>";
  h += htmlInventoryTable();
  h += "</body></html>";
  return h;
}

String buildFullHTML(bool csvTruncated) {
  char now[28];
  salesTimestamp(now, sizeof(now));

  int order[DAILY_HISTORY_DAYS];
  int n = dailySortedIndices(order);

  // Aggregate the stored window once, so the summary below mirrors the daily
  // email's shape instead of quietly mixing in all-time figures.
  long winTxns = 0, winUnits = 0, winRs = 0;
  long winUnitsProd[MAX_PRODUCTS], winRsProd[MAX_PRODUCTS];
  long winTxnsPay[PAYMENT_COUNT], winRsPay[PAYMENT_COUNT];
  long winUnitsProdPay[MAX_PRODUCTS][PAYMENT_COUNT];
  for (int i = 0; i < MAX_PRODUCTS; i++) {
    winUnitsProd[i] = 0; winRsProd[i] = 0;
    for (int p = 0; p < PAYMENT_COUNT; p++) winUnitsProdPay[i][p] = 0;
  }
  for (int p = 0; p < PAYMENT_COUNT; p++) { winTxnsPay[p] = 0; winRsPay[p] = 0; }

  for (int k = 0; k < n; k++) {
    DailyRecord &r = dailyHistory[order[k]];
    winTxns += r.txns;
    for (int i = 0; i < MAX_PRODUCTS; i++) {
      winUnits += r.units[i];
      winRs    += r.revenue[i];
      winUnitsProd[i] += r.units[i];
      winRsProd[i]    += r.revenue[i];
      for (int p = 0; p < PAYMENT_COUNT; p++) winUnitsProdPay[i][p] += r.unitsByPay[i][p];
    }
    for (int p = 0; p < PAYMENT_COUNT; p++) {
      winTxnsPay[p] += r.txnsByPay[p];
      winRsPay[p]   += r.revByPay[p];
    }
  }

  char firstDay[12], lastDay[12];
  dateNumToString(n ? dailyHistory[order[0]].date : 0, firstDay, sizeof(firstDay));
  dateNumToString(n ? dailyHistory[order[n - 1]].date : 0, lastDay, sizeof(lastDay));

  String h;
  h.reserve(9000);
  h += htmlHeader("Sales Report - Last 60 Days");

  h += "<p><b>Period covered:</b> 00:00:00 " + String(firstDay) + " &nbsp;to&nbsp; " +
       String(now) + "<br>";
  h += "<b>Trading days recorded:</b> " + String(n) + " of 60";
  if (n) h += " (most recent " + String(lastDay) + ")";
  h += "</p>";

  h += "<h3>Financial Summary</h3><table border=1 cellpadding=5 cellspacing=0>";
  h += "<tr><td>Total Transactions</td><td align=right><b>" + String(winTxns) +
       "</b></td></tr>";
  h += "<tr><td>Units Dispensed</td><td align=right><b>" + String(winUnits) +
       "</b></td></tr>";
  h += "<tr><td>Total Payment Received</td><td align=right><b>Rs " + String(winRs) +
       "</b></td></tr>";
  h += "<tr><td>Average Daily Revenue</td><td align=right>Rs " +
       String(n ? winRs / n : 0) + "</td></tr>";
  h += "<tr><td>Current Stock</td><td align=right><b>" + String(totalStockOnHand()) +
       "</b></td></tr>";
  h += "</table>";

  // Same columns as the daily table. "Start" is the stock the oldest stored
  // day opened with, so Start - Sold != Remaining whenever the machine has
  // been refilled during the window — which is expected, and why the refill
  // total is called out underneath.
  h += "<h3>Product Performance</h3>";
  h += "<table border=1 cellpadding=5 cellspacing=0>";
  h += "<tr style='background:#eee'><th>Product</th><th>Unit Price</th>"
       "<th>Opening Stock</th><th>Units Sold</th><th>Current Stock</th>"
       "<th>Payment Received</th><th>Payment Method</th></tr>";
  long totOpen = 0, totSold = 0, totLeft = 0, totRs = 0;
  for (int i = 0; i < MAX_PRODUCTS; i++) {
    if (!productEnabled[i] && winUnitsProd[i] == 0) continue;
    int open = n ? dailyHistory[order[0]].openStock[i] : totalStockForProduct(i);
    int left = totalStockForProduct(i);
    totSold += winUnitsProd[i]; totRs += winRsProd[i];
    // As in the daily table: a disabled product's past sales still count, its
    // shelf stock does not.
    if (productEnabled[i]) { totOpen += open; totLeft += left; }

    String mix = "";
    for (int p = 0; p < PAYMENT_COUNT; p++) {
      if (winUnitsProdPay[i][p] == 0) continue;
      if (mix.length()) mix += ", ";
      mix += String(PAYMENT_NAMES[p]) + " " + String(winUnitsProdPay[i][p]);
    }

    h += "<tr><td>" + String(products[i].name) +
         (productEnabled[i] ? "" : " <i>(disabled)</i>") + "</td>";
    h += "<td align=right>" + String(products[i].price) + "</td>";
    h += "<td align=right>" + String(productEnabled[i] ? String(open) : String("-")) + "</td>";
    h += "<td align=right>" + String(winUnitsProd[i]) + "</td>";
    if (!productEnabled[i]) {
      h += "<td align=right>-</td>";
    } else if (left <= lowStockThreshold) {
      h += "<td align=right style='color:#c00'><b>" + String(left) + "</b></td>";
    } else {
      h += "<td align=right>" + String(left) + "</td>";
    }
    h += "<td align=right>" + String(winRsProd[i]) + "</td>";
    h += "<td>" + (mix.length() ? mix : String("-")) + "</td></tr>";
  }
  h += "<tr style='background:#eee'><td><b>TOTAL</b></td><td></td>";
  h += "<td align=right><b>" + String(totOpen) + "</b></td>";
  h += "<td align=right><b>" + String(totSold) + "</b></td>";
  h += "<td align=right><b>" + String(totLeft) + "</b></td>";
  h += "<td align=right><b>" + String(totRs) + "</b></td><td></td></tr>";
  h += "</table>";

  h += "<h3>Revenue by Payment Method</h3>";
  h += "<table border=1 cellpadding=5 cellspacing=0>";
  h += "<tr style='background:#eee'><th>Payment Method</th><th>Transactions</th>"
       "<th>Payment Received</th></tr>";
  for (int p = 0; p < PAYMENT_COUNT; p++) {
    if (winTxnsPay[p] == 0 && winRsPay[p] == 0) continue;
    h += "<tr><td>" + String(PAYMENT_NAMES[p]) + "</td>";
    h += "<td align=right>" + String(winTxnsPay[p]) + "</td>";
    h += "<td align=right>" + String(winRsPay[p]) + "</td></tr>";
  }
  h += "<tr style='background:#eee'><td><b>TOTAL</b></td><td align=right><b>" +
       String(winTxns) + "</b></td><td align=right><b>" + String(winRs) + "</b></td></tr>";
  h += "</table>";


  h += "<h3>Daily Breakdown</h3>";
  h += "<table border=1 cellpadding=4 cellspacing=0>";
  h += "<tr style='background:#eee'><th>Date</th><th>Transactions</th>"
       "<th>Units Dispensed</th><th>Payment Received</th></tr>";
  // Newest first — recent days are what anyone actually scans for.
  for (int k = n - 1; k >= 0; k--) {
    int s = order[k];
    char d[12];
    dateNumToString(dailyHistory[s].date, d, sizeof(d));
    h += "<tr><td>" + String(d) + "</td>";
    h += "<td align=right>" + String(dailyHistory[s].txns) + "</td>";
    h += "<td align=right>" + String(dayUnits(s)) + "</td>";
    h += "<td align=right>" + String(dayRevenue(s)) + "</td></tr>";
  }
  h += "</table>";
  if (n == 0) h += "<p style='color:#888'>No dated sales recorded yet.</p>";

  h += htmlInventoryTable();
  h += "<p style='color:#666;font-size:11px'>";
  if (csvTruncated) {
    h += "The attached CSV was too large to send in full, so it holds only the "
         "most recent transactions. ";
  } else {
    h += "The attached CSV holds every individual transaction from the same "
         "60-day window. ";
  }
  h += "Every figure in this report covers the 60-day period stated above.</p>";
  h += "</body></html>";
  return h;
}

// Reads the tail of the transaction log, up to maxBytes, starting at a line
// boundary so the CSV never begins mid-record. Sets truncated when older rows
// had to be dropped, which the email then says out loud.
String readCSVTail(size_t maxBytes, bool &truncated) {
  truncated = false;
  if (!salesLogReady) return "";

  File f = LittleFS.open(EVENT_LOG_PATH, FILE_READ);
  if (!f) return "";
  size_t size = f.size();
  if (size == 0) { f.close(); return ""; }

  String out;
  if (size > maxBytes) {
    truncated = true;
    f.seek(size - maxBytes);
    f.readStringUntil('\n');   // discard the partial line at the seek point
    out += String(CSV_HEADER) + "\n";
  }
  out.reserve(maxBytes + 64);

  char buf[257];
  while (f.available()) {
    size_t n = f.readBytes(buf, sizeof(buf) - 1);
    if (n == 0) break;
    buf[n] = '\0';
    out += buf;
  }
  f.close();
  return out;
}

// Just one day's rows, for the daily email's attachment — the whole log tail
// would mostly be older days the reader has already had. Rows are matched by
// parsing the date out of the timestamp column: it now reads
// "HH:MM:SS DD-MM-YYYY", so the old text-prefix comparison would never match.
String readCSVForDate(uint32_t date, size_t maxBytes, bool &truncated) {
  truncated = false;
  if (!salesLogReady || date == 0) return "";

  File f = LittleFS.open(EVENT_LOG_PATH, FILE_READ);
  if (!f) return "";

  String out = String(CSV_HEADER) + "\n";
  f.readStringUntil('\n');            // skip the stored header
  int rows = 0;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    if (csvRowDateNum(line) != date) continue;

    if (out.length() + line.length() + 1 > maxBytes) { truncated = true; break; }
    out += line + "\n";
    rows++;
  }
  f.close();

  // Returned even with no matching rows, so the daily email always carries a
  // CSV with its header. A consistent attachment is easier to feed into a
  // spreadsheet than one that appears only on days with sales.
  Serial.printf("Report: daily CSV matched %d rows for %08u\n", rows, (unsigned)date);
  return out;
}

// ---------- The send ----------
// Retries a failed send. Gmail failures on a cellular link are usually the
// link rather than the mail — a dropped TLS handshake or a stalled socket —
// so a couple of attempts with a short backoff recovers most of them. An
// authentication rejection would fail identically every time, but it also
// leaves a clear reason in the Serial log.
const int REPORT_MAX_TRIES = 3;

int sendViaGmail(ReportKind kind, uint32_t dateNum, const String &html,
                 const String &csv, const String &csvName) {
  int code = -1;
  for (int t = 0; t < REPORT_MAX_TRIES; t++) {
    code = gmailSend(reportSubject(kind, dateNum), html, csv, csvName);
    if (code == 250) return code;
    if (t < REPORT_MAX_TRIES - 1) {
      Serial.printf("Report: send failed (%d), attempt %d/%d, retrying\n",
                    code, t + 1, REPORT_MAX_TRIES);
      delay(3000 * (t + 1));
    }
  }
  return code;
}

bool sendReportEmail(ReportKind kind, uint32_t dateNum, bool withAttachment) {
  reportLastOk = false;

  if (!emailConfigured()) {
    strncpy(reportLastStatus, "Email not configured", sizeof(reportLastStatus) - 1);
    return false;
  }
  if (WiFi.status() != WL_CONNECTED && !connectLTEIfNeeded()) {
    strncpy(reportLastStatus, "No internet connection", sizeof(reportLastStatus) - 1);
    return false;
  }

  // The attachment is read before the body, so the body only claims a CSV
  // when one is really going.
  String csv;
  bool csvTruncated = false;
  if (withAttachment) {
    if (kind == REPORT_DAILY)      csv = readCSVForDate(dateNum, CSV_ATTACH_MAX_BYTES, csvTruncated);
    else if (kind == REPORT_FULL)  csv = readCSVTail(CSV_ATTACH_MAX_BYTES, csvTruncated);
  }

  String html;
  if (kind == REPORT_DAILY)         html = buildDailyHTML(dateNum, csv.length() > 0);
  else if (kind == REPORT_LOWSTOCK) html = buildLowStockHTML();
  else                              html = buildFullHTML(csvTruncated);

  char csvName[48], ds[12];
  dateNumToString(dateNum, ds, sizeof(ds));
  snprintf(csvName, sizeof(csvName), "%s_%s.csv", machineId, ds);

  Serial.printf("Report: sending %s, %u byte body, %u byte CSV\n",
                (kind == REPORT_DAILY) ? "daily" :
                (kind == REPORT_LOWSTOCK) ? "low-stock alert" : "full report",
                (unsigned)html.length(), (unsigned)csv.length());

  int code = sendViaGmail(kind, dateNum, html, csv, String(csvName));

  // Last resort: drop the attachment. A large base64 body over software TLS
  // on a weak cellular link is the one part of this that can genuinely be too
  // much, and the figures matter more than the raw rows.
  if (code != 250 && csv.length() > 0) {
    Serial.println("Report: retrying without the attachment");
    csv = "";
    if (kind == REPORT_DAILY)     html = buildDailyHTML(dateNum, false);
    else if (kind == REPORT_FULL) html = buildFullHTML(false);
    code = sendViaGmail(kind, dateNum, html, csv, String(csvName));
    if (code == 250) {
      strncpy(reportLastStatus, "Sent (no attachment)", sizeof(reportLastStatus) - 1);
      reportLastOk = true;
      return true;
    }
  }

  if (code == 250) {
    const char* what = (kind == REPORT_DAILY) ? "Daily" :
                       (kind == REPORT_LOWSTOCK) ? "Alert" : "Full report";
    snprintf(reportLastStatus, sizeof(reportLastStatus), "%s sent%s", what,
             csvTruncated ? " (CSV trimmed)" : "");
    reportLastOk = true;
    return true;
  }

  if (code == 0) {
    // The conversation got going but a step was refused — nearly always the
    // app password.
    strncpy(reportLastStatus, "Rejected - check app password", sizeof(reportLastStatus) - 1);
  } else {
    strncpy(reportLastStatus, "Send failed - no connection", sizeof(reportLastStatus) - 1);
  }
  return false;
}
