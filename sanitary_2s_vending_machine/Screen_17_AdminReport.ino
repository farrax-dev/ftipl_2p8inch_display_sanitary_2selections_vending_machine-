// =====================================================
//           ADMIN REPORTS SCREEN
// =====================================================
// Five tabs along the top, jump straight to any of them (it used to be a
// single "next page" button that cycled through four, so reaching the third
// meant tapping past the first two):
//
//   Sales - transactions, units, revenue, stock, today's figures (tile grid)
//   Items - a Sold / Rs / Left table, one row per product
//   Auto  - the two nightly e-mail times and the low-stock alert
//   Email - Gmail credentials, recipient and machine name
//   Data  - how much history is stored, link in use, and the reset
//
// The bottom bar is Back and Send Now. Send Now always sends the full 60-day
// report with the transaction CSV attached; the automatic mails are built and
// sent by Core_16_ReportSchedule.ino without anyone standing here. The status
// line above the bar shows how the last send, manual or automatic, went.
//
// reportPage and reportResetArmed live in Core_02_AppState.ino — see the
// comment there for why.
const int RPT_PAGE_COUNT = 5;
const int RPT_PAGE_SALES = 0, RPT_PAGE_ITEMS = 1, RPT_PAGE_AUTO = 2,
          RPT_PAGE_EMAIL = 3, RPT_PAGE_DATA = 4;

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

const int RPT_MAIL_W = 46, RPT_MAIL_H = 22;

void drawReportTile(int col, int row, const char* label, const char* value, bool accent, bool mailChip) {
  int x = rptTileX(col), y = rptTileY(row);
  drawCard(x, y, RPT_TILE_W, RPT_TILE_H, 6);

  auiPut(label, AL_LEFT, x + 10, RPT_TILE_W - 20, y + 10, COL_TEXT_DIM, FACE_XS);
  int valueW = RPT_TILE_W - 20 - (mailChip ? RPT_MAIL_W + 6 : 0);
  auiPut(value, AL_LEFT, x + 10, valueW, y + 26, accent ? COL_ACCENT : COL_TEXT, FACE_MD);

  if (mailChip) {
    auiButton(x + RPT_TILE_W - 8 - RPT_MAIL_W, y + 17, RPT_MAIL_W, RPT_MAIL_H, "Mail", ABTN_PRIMARY, FACE_XS);
  }
}

// The headline figures. The "Today" tile (col 0, row 2) doubles as a tap
// target — see handleAdminReportScreen() — sending the exact numbers shown:
// today's figures are the same ones the nightly mail carries, so the operator
// can sanity-check them before they go out.
void drawReportSalesPage() {
  char buf[24];

  snprintf(buf, sizeof(buf), "%ld", lifetimeTxnCount);
  drawReportTile(0, 0, "Transactions", buf, false, false);

  snprintf(buf, sizeof(buf), "%ld", totalUnitsSoldAll());
  drawReportTile(1, 0, "Products sold", buf, false, false);

  snprintf(buf, sizeof(buf), "%ld", lifetimeRevenueTotal);
  drawReportTile(0, 1, "Total sales (Rs.)", buf, true, false);

  snprintf(buf, sizeof(buf), "%d", totalStockOnHand());
  drawReportTile(1, 1, "Units in stock", buf, false, false);

  int slot = findDailySlot(todayDateNum());
  snprintf(buf, sizeof(buf), "%ld", dayRevenue(slot));
  drawReportTile(0, 2, "Today (Rs.)", buf, true, true);

  char today[20];
  snprintf(today, sizeof(today), "%d / %d", dayUnits(slot), slot >= 0 ? dailyHistory[slot].txns : 0);
  drawReportTile(1, 2, "Today units/txns", today, false, false);
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
void drawReportSchedulePage() {
  drawReportRow(0, "Nightly email 1",
                strlen(nightlyTime1) > 0 ? nightlyTime1 : "Off", COL_ACCENT, true);
  drawReportRow(1, "Nightly email 2",
                strlen(nightlyTime2) > 0 ? nightlyTime2 : "Off", COL_ACCENT, true);

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

  auiBarButton(0, 2, "Back", ABTN_SECONDARY);
  // Greyed out until there's somewhere to send to, rather than letting the
  // tap fail with an error after a long wait. Still tappable: it jumps to the
  // Email tab to show what's missing.
  auiBarButton(1, 2, "Send Now", emailConfigured() ? ABTN_PRIMARY : ABTN_OFF);
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

  if (auiBarHit(sx, sy, 0, 2)) {
    reportResetArmed = false;
    currentScreen = SCREEN_ADMIN_PANEL;
    drawAdminPanelScreen();
    return;
  }

  if (auiBarHit(sx, sy, 1, 2)) {
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

  // Sales tab: tapping the "Today" tile emails today's report — the same
  // thing the nightly schedule sends, on demand.
  if (reportPage == RPT_PAGE_SALES &&
      pointInRect(sx, sy, rptTileX(0), rptTileY(2), RPT_TILE_W, RPT_TILE_H)) {
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

  if (reportPage == RPT_PAGE_AUTO) {
    if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(0), RPT_ROW_W, RPT_ROW_H)) {
      openTextEntry(TE_NIGHTLY_1);
    } else if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(1), RPT_ROW_W, RPT_ROW_H)) {
      openTextEntry(TE_NIGHTLY_2);
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
