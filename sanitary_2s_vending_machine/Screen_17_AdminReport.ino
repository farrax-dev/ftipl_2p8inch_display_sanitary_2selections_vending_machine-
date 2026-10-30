// =====================================================
//           ADMIN REPORTS SCREEN
// =====================================================
// Five tabs along the top, jump straight to any of them (it used to be a
// single "next page" button that cycled through four, so reaching the third
// meant tapping past the first two):
//
//   Sales - transactions, units, revenue, stock, today's figures (tile grid)
//   Items - a Sold / Rs / Left table, one row per product
//   Auto  - the two daily report send times and the low-stock alert
//   Email - Gmail credentials, recipient and machine name
//   Data  - how much history is stored, link in use, and the reset
//
// The bottom bar is Back, Send Today and Send 60 Days. Send 60 Days sends the full 60-day
// report with the transaction CSV attached; the automatic mails are built and
// sent by Core_16_ReportSchedule.ino without anyone standing here. The status
// line above the bar shows how the last send, manual or automatic, went.
//
// reportPage and reportResetArmed live in Core_02_AppState.ino — see the
// comment there for why.
const int RPT_PAGE_COUNT = 5;
const int RPT_PAGE_SALES = 0, RPT_PAGE_ITEMS = 1, RPT_PAGE_AUTO = 2,
          RPT_PAGE_EMAIL = 3, RPT_PAGE_DATA = 4;

// ---------- Bottom bar: Back | Send Today | Send 60 Days ----------
// Custom widths (Back is short) so both send labels fit without shrinking.
const int RPT_BAR_BACK_X = 16,  RPT_BAR_BACK_W = 56;
const int RPT_BAR_TODAY_X = 80, RPT_BAR_TODAY_W = 108;
const int RPT_BAR_FULL_X = 196, RPT_BAR_FULL_W = 108;

bool rptBarHit(int sx, int sy, int x, int w) {
  return pointInRect(sx, sy, x - 2, ABAR_Y - 3, w + 4, ABAR_H + 12);
}

// ---------- Tabs ----------
const int RPT_TAB_Y = 35, RPT_TAB_H = 22, RPT_TAB_W = 56, RPT_TAB_PITCH = 58;
int rptTabX(int k) { return 16 + k * RPT_TAB_PITCH; }

// ---------- Rows ----------
const int RPT_ROW_X = 16, RPT_ROW_W = 288, RPT_ROW_H = 28;
const int RPT_ROW_Y0 = 62, RPT_ROW_GAP = 2;
const int RPT_RIGHT = RPT_ROW_X + RPT_ROW_W - 12;
const int RPT_STATUS_CY = 188;

int reportRowY(int index) { return RPT_ROW_Y0 + index * (RPT_ROW_H + RPT_ROW_GAP); }
int reportRowCY(int index) { return reportRowY(index) + RPT_ROW_H / 2; }

// A row with a label on the left and a value on the right. tappable adds a
// chevron after the value so it reads as something that opens.
void drawReportRow(int index, const char* label, const char* value, uint16_t valCol, bool tappable) {
  int y = reportRowY(index), cy = reportRowCY(index);
  auiRow(RPT_ROW_X, y, RPT_ROW_W, RPT_ROW_H);
  int labelW = auiTextW(label, FACE_MD);
  auiPut(label, AL_LEFT, RPT_ROW_X + 12, labelW + 2, cy, COL_TEXT, FACE_MD);
  int right = RPT_RIGHT - (tappable ? 14 : 0);
  int vx = RPT_ROW_X + 12 + labelW + 12;
  auiPut(value, AL_RIGHT, vx, right - vx, cy, valCol, FACE_SM);
  if (tappable) {
    // chevron
    int cx = RPT_RIGHT - 4;
    tft.drawLine(cx - 4, cy - 5, cx, cy, COL_TEXT_DIM);
    tft.drawLine(cx - 4, cy + 5, cx, cy, COL_TEXT_DIM);
    tft.drawLine(cx - 5, cy - 5, cx - 1, cy, COL_TEXT_DIM);
    tft.drawLine(cx - 5, cy + 5, cx - 1, cy, COL_TEXT_DIM);
  }
}

// ---------- Sales tab: 2x3 tile grid ----------
// Each figure gets its own card with a caption over a large number, so the
// one an operator actually came for stands out instead of blending into a
// stack of similar thin rows.
const int RPT_TILE_GAP = 4, RPT_TILE_ROW_GAP = 3;
const int RPT_TILE_Y0 = 62;
const int RPT_TILE_W = (RPT_ROW_W - RPT_TILE_GAP) / 2;
const int RPT_TILE_H = 36;

int rptTileX(int col) { return RPT_ROW_X + col * (RPT_TILE_W + RPT_TILE_GAP); }
int rptTileY(int row) { return RPT_TILE_Y0 + row * (RPT_TILE_H + RPT_TILE_ROW_GAP); }

void drawReportTile(int col, int row, const char* label, const char* value, bool accent) {
  int x = rptTileX(col), y = rptTileY(row);
  drawCard(x, y, RPT_TILE_W, RPT_TILE_H, 6);

  auiPut(label, AL_LEFT, x + 10, RPT_TILE_W - 20, y + 10, COL_TEXT_DIM, FACE_XS);
  auiPut(value, AL_LEFT, x + 10, RPT_TILE_W - 20, y + 26, accent ? COL_ACCENT : COL_TEXT, FACE_MD);
}

// The headline figures. Sending is done from the bottom bar (Send Today /
// Send 60 Days), not from these tiles.
void drawReportSalesPage() {
  char buf[24];

  snprintf(buf, sizeof(buf), "%ld", lifetimeTxnCount);
  drawReportTile(0, 0, "Transactions", buf, false);

  snprintf(buf, sizeof(buf), "%ld", totalUnitsSoldAll());
  drawReportTile(1, 0, "Products sold", buf, false);

  snprintf(buf, sizeof(buf), "%ld", lifetimeRevenueTotal);
  drawReportTile(0, 1, "Total sales (Rs.)", buf, true);

  snprintf(buf, sizeof(buf), "%d", totalStockOnHand());
  drawReportTile(1, 1, "Units in stock", buf, false);

  int slot = findDailySlot(todayDateNum());
  snprintf(buf, sizeof(buf), "%ld", dayRevenue(slot));
  drawReportTile(0, 2, "Today (Rs.)", buf, true);

  char today[20];
  snprintf(today, sizeof(today), "%d / %d", dayUnits(slot), slot >= 0 ? dailyHistory[slot].txns : 0);
  drawReportTile(1, 2, "Today units/txns", today, false);
}

// ---------- Items tab: a real table ----------
// One row per product slot, right-aligned into its own Sold / Rs / Left column.
// Row height adapts to how many products there are: with two it is 28px and
// the text is bold 9pt; with six it is 16px and drops to the regular face.
// Disabled slots that have never sold are skipped so the page shows what's
// actually in service, but a disabled slot with past sales still appears
// (that history is real money).
const int RPT_TBL_Y0 = 62;
const int RPT_TBL_COL_NAME = 118;
const int RPT_TBL_COL_SOLD = 54;
const int RPT_TBL_COL_RS   = 62;
const int RPT_TBL_COL_LEFT = RPT_ROW_W - RPT_TBL_COL_NAME - RPT_TBL_COL_SOLD - RPT_TBL_COL_RS;

// header: accent fill, column captions. Data row: zebra-striped (alternating
// COL_CARD/COL_BG_TOP) so rows of numbers stay scannable with no ruled line
// needed under every single one.
void drawReportTableRow(int y, int h, int row, const char* name, const char* sold, const char* rs,
                        const char* left, bool header) {
  uint16_t fill = header ? COL_ACCENT : ((row % 2) ? COL_CARD : COL_BG_TOP);
  uint16_t txt  = header ? COL_BG_TOP : COL_TEXT;
  int face = (h >= 24) ? FACE_SM : FACE_XS;
  int cy = y + h / 2;

  int colSoldX = RPT_ROW_X + RPT_TBL_COL_NAME;
  int colRsX   = colSoldX + RPT_TBL_COL_SOLD;
  int colLeftX = colRsX + RPT_TBL_COL_RS;
  int rowRightX = colLeftX + RPT_TBL_COL_LEFT;

  tft.fillRect(RPT_ROW_X, y, RPT_ROW_W, h, fill);
  auiPut(name, AL_LEFT, RPT_ROW_X + 8, RPT_TBL_COL_NAME - 12, cy, txt, face);
  auiPut(sold, AL_RIGHT, colSoldX, RPT_TBL_COL_SOLD - 8, cy, txt, face);
  auiPut(rs,   AL_RIGHT, colRsX,   RPT_TBL_COL_RS - 8,   cy, txt, face);
  auiPut(left, AL_RIGHT, colLeftX, RPT_TBL_COL_LEFT - 8, cy, txt, face);
}

void drawReportProductPage() {
  int listed[16];
  int n = 0;
  for (int i = 0; i < MAX_PRODUCTS && n < 16; i++) {
    if (!productEnabled[i] && lifetimeUnitsSold[i] == 0) continue;
    listed[n++] = i;
  }

  int lines = n + 1;                            // + header
  int h = min(28, 116 / lines);

  drawReportTableRow(RPT_TBL_Y0, h, 0, "Product", "Sold", "Rs.", "Left", true);

  for (int k = 0; k < n; k++) {
    int i = listed[k];
    char sold[10], rs[12], left[8];
    snprintf(sold, sizeof(sold), "%ld", lifetimeUnitsSold[i]);
    snprintf(rs,   sizeof(rs),   "%ld", lifetimeRevenue[i]);
    snprintf(left, sizeof(left), "%d", totalStockForProduct(i));
    drawReportTableRow(RPT_TBL_Y0 + (k + 1) * h, h, k + 1, products[i].name, sold, rs, left, false);
  }

  if (n == 0) {
    drawReportTableRow(RPT_TBL_Y0 + h, h, 1, "(no products enabled)", "", "", "", false);
    lines = 2;
  }

  // Frame the whole block once rather than a border per row — reads as one
  // table instead of a stack of separate cards.
  tft.drawRect(RPT_ROW_X, RPT_TBL_Y0, RPT_ROW_W, lines * h, COL_CARD_BRD);
}

// ---------- Auto tab ----------
// Two independent daily send times (the report always goes to the single
// "Send to" address on the Email tab — these are times, not recipients). Each
// row shows its time, or Off, and opens the time picker below.
char* reportTimePtr(int slot) { return (slot == 0) ? nightlyTime1 : nightlyTime2; }

// "14:05" -> "2:05 PM" (or unchanged in 24-hour mode); blank/invalid -> "Off".
void formatReportTime(const char* hhmm, char* out, size_t n) {
  int m = parseScheduleTime(hhmm);
  if (m < 0) { snprintf(out, n, "Off"); return; }
  int h = m / 60, mi = m % 60;
  if (clock24Hour) { snprintf(out, n, "%02d:%02d", h, mi); return; }
  int h12 = (h % 12 == 0) ? 12 : h % 12;
  snprintf(out, n, "%d:%02d %s", h12, mi, h >= 12 ? "PM" : "AM");
}

// ---------- Time picker (modal over the Reports screen) ----------
// rptTimeSlot is the slot being edited (0 or 1), or -1 when the tabs show.
// Nothing is stored until Save. Minutes move in steps of 5 and both fields
// wrap around, so any time is a few taps away and nothing can be typed wrong.
int  rptTimeSlot = -1;
bool rptTimeOn = false;
int  rptTimeH = 22, rptTimeM = 0;

const int RTP_SW_Y = 40, RTP_SW_H = 32;
const int RTP_LABEL_CY = 86;
const int RTP_PLUS_Y = 94,  RTP_PLUS_H = 30;
const int RTP_VAL_Y  = 127, RTP_VAL_H  = 28;
const int RTP_MINUS_Y = 158, RTP_MINUS_H = 30;
const int RTP_COL_W = 80, RTP_COLON_W = 24, RTP_AMPM_W = 56, RTP_AMPM_GAP = 14;

int rtpTotalW() { return RTP_COL_W * 2 + RTP_COLON_W + (clock24Hour ? 0 : RTP_AMPM_GAP + RTP_AMPM_W); }
int rtpHourX()  { return (320 - rtpTotalW()) / 2; }
int rtpMinX()   { return rtpHourX() + RTP_COL_W + RTP_COLON_W; }
int rtpAmpmX()  { return rtpMinX() + RTP_COL_W + RTP_AMPM_GAP; }

void openReportTimePicker(int slot) {
  int m = parseScheduleTime(reportTimePtr(slot));
  rptTimeSlot = slot;
  rptTimeOn = (m >= 0);
  rptTimeH = (m >= 0) ? m / 60 : 22;
  rptTimeM = (m >= 0) ? (m % 60) : 0;
  drawAdminReportScreen();
}

void drawRtpButton(int x, int y, int h, bool plus) {
  if (rptTimeOn) {
    drawCardShadow(x, y, RTP_COL_W, h, 8);
    tft.fillRoundRect(x, y, RTP_COL_W, h, 8, COL_ACCENT);
  } else {
    tft.fillRoundRect(x, y, RTP_COL_W, h, 8, COL_CARD);
    tft.drawRoundRect(x, y, RTP_COL_W, h, 8, COL_CARD_BRD);
  }
  uint16_t g = rptTimeOn ? COL_BG_TOP : COL_TEXT_DIM;
  int cx = x + RTP_COL_W / 2, cy = y + h / 2;
  tft.fillRect(cx - 7, cy - 1, 15, 3, g);
  if (plus) tft.fillRect(cx - 1, cy - 7, 3, 15, g);
}

void drawRtpColumn(int x, const char* label, const char* value) {
  uint16_t col = rptTimeOn ? COL_TEXT : COL_TEXT_DIM;
  auiPut(label, AL_CENTER, x, RTP_COL_W, RTP_LABEL_CY, col, FACE_SM);
  drawRtpButton(x, RTP_PLUS_Y, RTP_PLUS_H, true);
  tft.fillRoundRect(x, RTP_VAL_Y, RTP_COL_W, RTP_VAL_H, 6, COL_CARD);
  tft.drawRoundRect(x, RTP_VAL_Y, RTP_COL_W, RTP_VAL_H, 6, COL_CARD_BRD);
  auiPut(value, AL_CENTER, x + 2, RTP_COL_W - 4, RTP_VAL_Y + RTP_VAL_H / 2,
         rptTimeOn ? COL_ACCENT : COL_TEXT_DIM, FACE_LG);
  drawRtpButton(x, RTP_MINUS_Y, RTP_MINUS_H, false);
}

void drawReportTimePicker() {
  drawGradientBackground();
  char title[24];
  snprintf(title, sizeof(title), "Daily Report %d", rptTimeSlot + 1);
  drawScreenTitle(title);

  auiRow(RPT_ROW_X, RTP_SW_Y, RPT_ROW_W, RTP_SW_H);
  auiRowLabel(RPT_ROW_X + 12, RTP_SW_Y + RTP_SW_H / 2, 170, "Send this report");
  auiOnOff(RPT_RIGHT, RTP_SW_Y + RTP_SW_H / 2, rptTimeOn);

  int h = rptTimeH;
  if (!clock24Hour) h = (h % 12 == 0) ? 12 : h % 12;
  char hb[6], mb[6];
  snprintf(hb, sizeof(hb), "%02d", h);
  snprintf(mb, sizeof(mb), "%02d", rptTimeM);
  drawRtpColumn(rtpHourX(), "Hour", hb);
  drawRtpColumn(rtpMinX(), "Minute", mb);
  auiPut(":", AL_CENTER, rtpHourX() + RTP_COL_W, RTP_COLON_W, RTP_VAL_Y + RTP_VAL_H / 2 - 2,
         rptTimeOn ? COL_TEXT : COL_TEXT_DIM, FACE_LG);

  if (!clock24Hour) {
    auiButton(rtpAmpmX(), RTP_VAL_Y, RTP_AMPM_W, RTP_VAL_H, rptTimeH >= 12 ? "PM" : "AM",
              rptTimeOn ? ABTN_SECONDARY : ABTN_OFF, FACE_SM);
  }

  auiBarButton(0, 2, "Cancel", ABTN_SECONDARY);
  auiBarButton(1, 2, "Save", ABTN_PRIMARY);
}

void handleReportTimePicker(int sx, int sy) {
  if (auiBarHit(sx, sy, 0, 2)) {
    rptTimeSlot = -1;
    drawAdminReportScreen();
    return;
  }
  if (auiBarHit(sx, sy, 1, 2)) {
    char* t = reportTimePtr(rptTimeSlot);
    if (rptTimeOn) snprintf(t, 6, "%02d:%02d", rptTimeH, rptTimeM);
    else           t[0] = '\0';
    saveScheduleSettings();
    rearmNightlySlot(rptTimeSlot);   // a new time applies today, even if this slot already sent
    rptTimeSlot = -1;
    drawAdminReportScreen();
    return;
  }
  if (pointInRect(sx, sy, RPT_ROW_X, RTP_SW_Y, RPT_ROW_W, RTP_SW_H)) {
    rptTimeOn = !rptTimeOn;
    drawReportTimePicker();
    return;
  }
  if (!rptTimeOn) return;

  if (!clock24Hour && pointInRect(sx, sy, rtpAmpmX(), RTP_VAL_Y, RTP_AMPM_W, RTP_VAL_H)) {
    rptTimeH = (rptTimeH + 12) % 24;
    drawReportTimePicker();
    return;
  }

  for (int c = 0; c < 2; c++) {
    int x = (c == 0) ? rtpHourX() : rtpMinX();
    int dir = 0;
    if (pointInRect(sx, sy, x, RTP_PLUS_Y - 4, RTP_COL_W, RTP_PLUS_H + 4)) dir = 1;
    else if (pointInRect(sx, sy, x, RTP_MINUS_Y, RTP_COL_W, RTP_MINUS_H + 3)) dir = -1;
    if (dir == 0) continue;
    if (c == 0) {
      rptTimeH = (rptTimeH + dir + 24) % 24;
    } else {
      // Snap to the 5-minute grid first so an old odd value (e.g. :03) lands
      // on :05 / :00 rather than skipping.
      int m = (dir > 0) ? (rptTimeM / 5 + 1) * 5 : ((rptTimeM + 4) / 5 - 1) * 5;
      rptTimeM = (m + 60) % 60;
    }
    drawReportTimePicker();
    return;
  }
}

void drawReportSchedulePage() {
  char tb[12];
  formatReportTime(nightlyTime1, tb, sizeof(tb));
  drawReportRow(0, "Report time 1", tb, strlen(nightlyTime1) > 0 ? COL_ACCENT : COL_TEXT_DIM, true);
  formatReportTime(nightlyTime2, tb, sizeof(tb));
  drawReportRow(1, "Report time 2", tb, strlen(nightlyTime2) > 0 ? COL_ACCENT : COL_TEXT_DIM, true);

  int cy = reportRowCY(2);
  auiRow(RPT_ROW_X, reportRowY(2), RPT_ROW_W, RPT_ROW_H);
  auiRowLabel(RPT_ROW_X + 12, cy, 160, "Low-stock alert");
  auiOnOff(RPT_RIGHT, cy, lowStockAlertOn);

  // A motor at or below this figure is flagged — and drawn red on the Stock
  // screen.
  cy = reportRowCY(3);
  auiRow(RPT_ROW_X, reportRowY(3), RPT_ROW_W, RPT_ROW_H);
  auiRowLabel(RPT_ROW_X + 12, cy, auiStepLeftX(RPT_RIGHT) - RPT_ROW_X - 20, "Alert below");
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", lowStockThreshold);
  auiStepper(RPT_RIGHT, cy, buf, COL_ACCENT);
}

// ---------- Email tab ----------
void drawReportEmailPage() {
  char buf[24];

  drawReportRow(0, "Gmail",
                strlen(gmailUser) > 0 ? gmailUser : "(not set)", COL_ACCENT, true);

  // Only the length is shown, not the characters: this is a live credential
  // on a machine that lives in a public place. Tap the row and use Show on
  // the keyboard to check it.
  if (strlen(gmailPass) > 0) snprintf(buf, sizeof(buf), "%d chars set", (int)strlen(gmailPass));
  else                       snprintf(buf, sizeof(buf), "(not set)");
  drawReportRow(1, "Password", buf, COL_ACCENT, true);

  drawReportRow(2, "Send to",
                strlen(reportTo) > 0 ? reportTo : "(not set)", COL_ACCENT, true);

  // Appears in every email subject next to the machine ID.
  drawReportRow(3, "Name",
                strlen(machineName) > 0 ? machineName : "(not set)", COL_ACCENT, true);
}

// ---------- Data tab ----------
void drawReportDataPage() {
  char buf[24];

  int days = 0;
  for (int i = 0; i < DAILY_HISTORY_DAYS; i++) if (dailyHistory[i].date) days++;
  snprintf(buf, sizeof(buf), "%d of %d days", days, DAILY_HISTORY_DAYS);
  drawReportRow(0, "History stored", buf, COL_TEXT_DIM, false);

  drawReportRow(1, "Sending via",
                WiFi.status() == WL_CONNECTED ? "WiFi" :
                isLTEConnected() ? "4G" : "Offline", COL_TEXT_DIM, false);

  // The second tap is unrecoverable, so it takes two deliberate taps, and the
  // armed state is called out in danger red rather than blending in.
  int cy = reportRowCY(2);
  auiRow(RPT_ROW_X, reportRowY(2), RPT_ROW_W, RPT_ROW_H);
  auiPut(reportResetArmed ? "Tap again to erase" : "Clear sales data", AL_LEFT, RPT_ROW_X + 12, 190, cy,
         reportResetArmed ? COL_DANGER : COL_TEXT, FACE_MD);
  auiRowButton(RPT_RIGHT - 70 + 4, cy, 70, reportResetArmed ? "Confirm" : "Clear", ABTN_DANGER);
}

void drawAdminReportScreen() {
  if (rptTimeSlot >= 0) { drawReportTimePicker(); return; }
  drawGradientBackground();
  drawScreenTitle("Reports");

  const char* tabs[RPT_PAGE_COUNT] = { "Sales", "Items", "Auto", "Email", "Data" };
  for (int k = 0; k < RPT_PAGE_COUNT; k++) {
    auiTab(rptTabX(k), RPT_TAB_Y, RPT_TAB_W, RPT_TAB_H, tabs[k], k == reportPage);
  }

  if (reportPage == RPT_PAGE_SALES)      drawReportSalesPage();
  else if (reportPage == RPT_PAGE_ITEMS) drawReportProductPage();
  else if (reportPage == RPT_PAGE_AUTO)  drawReportSchedulePage();
  else if (reportPage == RPT_PAGE_EMAIL) drawReportEmailPage();
  else                                   drawReportDataPage();

  char status[64];
  snprintf(status, sizeof(status), "Last send: %s", reportLastStatus);
  auiPut(status, AL_CENTER, 12, 296, RPT_STATUS_CY, reportLastOk ? COL_SUCCESS : COL_TEXT, FACE_XS);

  auiButton(RPT_BAR_BACK_X, ABAR_Y, RPT_BAR_BACK_W, ABAR_H, "Back", ABTN_SECONDARY, FACE_MD);
  // Greyed out until there's somewhere to send to, rather than letting the
  // tap fail with an error after a long wait. Still tappable: it jumps to the
  // Email tab to show what's missing.
  int sendStyle = emailConfigured() ? ABTN_PRIMARY : ABTN_OFF;
  auiButton(RPT_BAR_TODAY_X, ABAR_Y, RPT_BAR_TODAY_W, ABAR_H, "Send Today", sendStyle, FACE_XS);
  auiButton(RPT_BAR_FULL_X, ABAR_Y, RPT_BAR_FULL_W, ABAR_H, "Send 60 Days", sendStyle, FACE_XS);
}

// Sending blocks for as long as the modem takes (up to ~90s with an
// attachment on a slow uplink), so say so rather than leaving a frozen
// screen that invites repeated taps.
void drawReportSendingOverlay(const char* what) {
  auiMessageScreen("Sending report...", what, "This can take a minute over 4G. Please wait.");
}

void handleAdminReportScreen() {
  if (!isRealTouch()) return;
  if (millis() - lastTouchTime <= TOUCH_DEBOUNCE) return;

  lastTouchTime = millis();
  TS_Point raw = ts.getPoint();
  int sx, sy;
  mapTouchToScreen(raw, sx, sy);

  if (rptTimeSlot >= 0) { handleReportTimePicker(sx, sy); return; }

  if (rptBarHit(sx, sy, RPT_BAR_BACK_X, RPT_BAR_BACK_W)) {
    reportResetArmed = false;
    currentScreen = SCREEN_ADMIN_PANEL;
    drawAdminPanelScreen();
    return;
  }

  if (rptBarHit(sx, sy, RPT_BAR_TODAY_X, RPT_BAR_TODAY_W)) {
    if (!emailConfigured()) {
      reportPage = RPT_PAGE_EMAIL;
      drawAdminReportScreen();
      return;
    }
    drawReportSendingOverlay("Today's sales plus today's transactions CSV.");
    sendReportEmail(REPORT_DAILY, todayDateNum(), true);
    lastTouchTime = millis();
    drawAdminReportScreen();
    return;
  }

  if (rptBarHit(sx, sy, RPT_BAR_FULL_X, RPT_BAR_FULL_W)) {
    if (!emailConfigured()) {
      reportPage = RPT_PAGE_EMAIL;   // send is impossible; show what's missing
      drawAdminReportScreen();
      return;
    }
    drawReportSendingOverlay("60 days of sales plus the CSV attachment.");
    sendReportEmail(REPORT_FULL, todayDateNum(), true);
    lastTouchTime = millis();   // swallow taps made while it was blocked
    drawAdminReportScreen();
    return;
  }

  for (int k = 0; k < RPT_PAGE_COUNT; k++) {
    if (pointInRect(sx, sy, rptTabX(k) - 1, RPT_TAB_Y - 3, RPT_TAB_W + 2, RPT_TAB_H + 6)) {
      if (reportPage != k) {
        reportPage = k;
        reportResetArmed = false;
        drawAdminReportScreen();
      }
      return;
    }
  }

  if (reportPage == RPT_PAGE_AUTO) {
    if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(0), RPT_ROW_W, RPT_ROW_H)) {
      openReportTimePicker(0);
    } else if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(1), RPT_ROW_W, RPT_ROW_H)) {
      openReportTimePicker(1);
    } else if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(2), RPT_ROW_W, RPT_ROW_H)) {
      lowStockAlertOn = !lowStockAlertOn;
      saveScheduleSettings();
      drawAdminReportScreen();
    } else {
      // 1..5 covers every sensible warning point for a machine this size.
      int step = auiStepTouch(sx, sy, RPT_RIGHT, reportRowCY(3));
      if (step != 0) {
        lowStockThreshold = constrain(lowStockThreshold + step, 1, 5);
        saveScheduleSettings();
        drawAdminReportScreen();
      }
    }
    return;
  }

  if (reportPage == RPT_PAGE_EMAIL) {
    if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(0), RPT_ROW_W, RPT_ROW_H)) {
      openTextEntry(TE_GMAIL_USER);
    } else if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(1), RPT_ROW_W, RPT_ROW_H)) {
      openTextEntry(TE_GMAIL_PASS);
    } else if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(2), RPT_ROW_W, RPT_ROW_H)) {
      openTextEntry(TE_REPORT_TO);
    } else if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(3), RPT_ROW_W, RPT_ROW_H)) {
      openTextEntry(TE_MACHINE_NAME);
    }
    return;
  }

  if (reportPage == RPT_PAGE_DATA &&
      pointInRect(sx, sy, RPT_ROW_X, reportRowY(2), RPT_ROW_W, RPT_ROW_H)) {
    if (reportResetArmed) {
      resetSalesTotals();
      reportResetArmed = false;
      strncpy(reportLastStatus, "All sales data cleared", sizeof(reportLastStatus) - 1);
      reportLastOk = false;
    } else {
      reportResetArmed = true;
    }
    drawAdminReportScreen();
  }
}
