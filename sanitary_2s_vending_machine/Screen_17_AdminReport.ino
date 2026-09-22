// =====================================================
//           ADMIN SALES REPORT SCREEN
// =====================================================
// Four pages:
//   0 Sales      - transactions, products sold, money taken, stock remaining
//   1 By product - one row per product slot: units, revenue, stock left
//   2 Schedule   - the two nightly email times and the low-stock alert
//   3 Email      - Gmail credentials, recipient and machine name
//
// Sales and By-product are separate pages because seven 18px rows is all that
// fits between the title and the button bar, and the headline figures plus
// six product rows plus a column header need eleven.
//
// The Send button always sends the full 60-day report with the transaction
// CSV attached; the automatic mails are built and sent by
// Core_16_ReportSchedule.ino without anyone standing here.
//
// reportPage and reportResetArmed live in Core_02_AppState.ino — see the
// comment there for why.
const int RPT_PAGE_COUNT = 4;

const int RPT_ROW_X = 14, RPT_ROW_W = 292, RPT_ROW_H = 18;
const int RPT_ROW_Y0 = 36, RPT_ROW_GAP = 2;
const int RPT_MAX_ROWS = 7;   // row 6 ends at y=174, just above the status line

const int RPT_BTN_BACK_X = 8,   RPT_BTN_BACK_W = 76;
const int RPT_BTN_PAGE_X = 92,  RPT_BTN_PAGE_W = 76;
const int RPT_BTN_SEND_X = 176, RPT_BTN_SEND_W = 133;

int reportRowY(int index) {
  return RPT_ROW_Y0 + index * (RPT_ROW_H + RPT_ROW_GAP);
}

void drawReportRow(int index, const char* label, const char* value, bool dim) {
  if (index >= RPT_MAX_ROWS) return;
  int y = reportRowY(index);
  drawCard(RPT_ROW_X, y, RPT_ROW_W, RPT_ROW_H, 4);

  tft.setTextSize(1);
  tft.setTextColor(dim ? COL_TEXT_DIM : COL_TEXT, COL_CARD);
  tft.setCursor(RPT_ROW_X + 6, y + 5);
  tft.print(label);

  tft.setTextColor(COL_ACCENT, COL_CARD);
  rightText(value, RPT_ROW_X + RPT_ROW_W - 6, y + 5);
}

// The headline figures. Per-product detail is on its own page.
void drawReportSalesPage() {
  char buf[24];
  int row = 0;

  snprintf(buf, sizeof(buf), "%ld", lifetimeTxnCount);
  drawReportRow(row++, "Transactions", buf, false);

  snprintf(buf, sizeof(buf), "%ld", totalUnitsSoldAll());
  drawReportRow(row++, "Products sold", buf, false);

  snprintf(buf, sizeof(buf), "Rs %ld", lifetimeRevenueTotal);
  drawReportRow(row++, "Total sales", buf, false);

  snprintf(buf, sizeof(buf), "%d units", totalStockOnHand());
  drawReportRow(row++, "Products remaining", buf, false);

  // Today's figures are the same ones the nightly mail will carry, so the
  // operator can sanity-check them before they go out — and tapping the row
  // sends that exact report immediately. Emailing what you're looking at
  // beats hunting for a separate button, and it leaves the bottom bar free
  // for the 60-day report.
  int slot = findDailySlot(todayDateNum());
  snprintf(buf, sizeof(buf), "%d sold  Rs %ld", dayUnits(slot), dayRevenue(slot));
  drawReportRow(row++, "Today - tap to email", buf, false);

  snprintf(buf, sizeof(buf), "%d txns today", slot >= 0 ? dailyHistory[slot].txns : 0);
  drawReportRow(row++, "Today's transactions", buf, true);
}

// One row per product slot — MAX_PRODUCTS is 5, plus the column header
// that's six of the seven rows available (RPT_MAX_ROWS). Disabled slots that
// have never sold are skipped so the page shows what's actually in service,
// but a disabled slot with past sales still appears (that history is real
// money).
void drawReportProductPage() {
  char buf[26], label[22];
  int row = 0;

  drawReportRow(row++, "Product", "Sold / Rs / Left", true);

  for (int i = 0; i < MAX_PRODUCTS && row < RPT_MAX_ROWS; i++) {
    if (!productEnabled[i] && lifetimeUnitsSold[i] == 0) continue;
    snprintf(label, sizeof(label), "%.14s", products[i].name);
    snprintf(buf, sizeof(buf), "%ld / %ld / %d",
             lifetimeUnitsSold[i], lifetimeRevenue[i], totalStockForProduct(i));
    drawReportRow(row++, label, buf, false);
  }

  if (row == 1) drawReportRow(row, "(no products enabled)", "", true);
}

void drawReportSchedulePage() {
  char buf[24];
  int row = 0;

  drawReportRow(row++, "Nightly email 1",
                strlen(nightlyTime1) > 0 ? nightlyTime1 : "off", false);
  drawReportRow(row++, "Nightly email 2",
                strlen(nightlyTime2) > 0 ? nightlyTime2 : "off", false);

  drawReportRow(row++, "Low stock alert", lowStockAlertOn ? "ON" : "OFF", false);

  snprintf(buf, sizeof(buf), "%d units", lowStockThreshold);
  drawReportRow(row++, "Alert below (tap)", buf, false);

  // Which motors are already flagged, so it's obvious why an alert has or
  // hasn't gone out.
  char lowList[24] = "";
  int lowCount = 0;
  for (int m = 0; m < MAX_MOTORS; m++) {
    if (!motorIsActive(m) || motorStock[m] > lowStockThreshold) continue;
    lowCount++;
    char tmp[6];
    snprintf(tmp, sizeof(tmp), lowList[0] ? ",M%d" : "M%d", m + 1);
    if (strlen(lowList) + strlen(tmp) < sizeof(lowList) - 1) strcat(lowList, tmp);
  }
  drawReportRow(row++, "Currently low", lowCount ? lowList : "none", lowCount == 0);

  int days = 0;
  for (int i = 0; i < DAILY_HISTORY_DAYS; i++) if (dailyHistory[i].date) days++;
  snprintf(buf, sizeof(buf), "%d of 60 days", days);
  drawReportRow(row++, "History stored", buf, true);

  int resetRow = row;
  drawReportRow(row++, "Reset all data",
                reportResetArmed ? "TAP AGAIN" : "tap to clear", true);
  // Second tap is unrecoverable, so the armed state gets called out in
  // danger red rather than blending in as just another accent-colored value.
  if (reportResetArmed) {
    tft.setTextColor(COL_DANGER, COL_CARD);
    rightText("TAP AGAIN", RPT_ROW_X + RPT_ROW_W - 6, reportRowY(resetRow) + 5);
  }
}

void drawReportEmailPage() {
  char buf[24];
  int row = 0;

  drawReportRow(row++, "Sends via", "Gmail SMTP", true);

  drawReportRow(row++, "Gmail address",
                strlen(gmailUser) > 0 ? gmailUser : "(not set)", false);

  // Only the length is shown, not the characters: this is a live credential
  // on a machine that lives in a public place. Tap the row and use Show on
  // the keyboard to check it.
  if (strlen(gmailPass) > 0) snprintf(buf, sizeof(buf), "%d chars set", (int)strlen(gmailPass));
  else                       snprintf(buf, sizeof(buf), "(not set)");
  drawReportRow(row++, "App password", buf, false);

  drawReportRow(row++, "Send report to",
                strlen(reportTo) > 0 ? reportTo : "(not set)", false);

  // Appears in every email subject next to the ID, which is why it lives on
  // this page rather than with the ID on the Settings screen — that screen
  // has no room left above its button bar.
  drawReportRow(row++, "Machine name",
                strlen(machineName) > 0 ? machineName : "(not set)", false);

  drawReportRow(row++, "Link",
                WiFi.status() == WL_CONNECTED ? "WiFi" :
                isLTEConnected() ? "4G" : "offline", true);
}

void drawAdminReportScreen() {
  drawGradientBackground();

  const char* titles[RPT_PAGE_COUNT] = {
    "Sales Report", "By Product", "Auto Emails", "Report Email"
  };
  drawScreenTitle(titles[reportPage]);

  if (reportPage == 0)      drawReportSalesPage();
  else if (reportPage == 1) drawReportProductPage();
  else if (reportPage == 2) drawReportSchedulePage();
  else                      drawReportEmailPage();

  tft.fillRect(0, BTN_Y - 14, tft.width(), 12, COL_BG_BOTTOM);
  tft.setTextSize(1);
  tft.setTextColor(reportLastOk ? COL_SUCCESS : COL_TEXT_DIM, COL_BG_BOTTOM);
  centerText(reportLastStatus, BTN_Y - 12);

  drawCard(RPT_BTN_BACK_X, BTN_Y, RPT_BTN_BACK_W, BTN_H, 8);
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_CARD);
  centerTextInBox("Back", BTN_Y + 9, RPT_BTN_BACK_X, RPT_BTN_BACK_W);

  const char* nextLabels[RPT_PAGE_COUNT] = { "Items", "Auto", "Email", "Sales" };
  drawCard(RPT_BTN_PAGE_X, BTN_Y, RPT_BTN_PAGE_W, BTN_H, 8);
  tft.setTextColor(COL_TEXT, COL_CARD);
  centerTextInBox(nextLabels[reportPage], BTN_Y + 9, RPT_BTN_PAGE_X, RPT_BTN_PAGE_W);

  // Greyed out until there's somewhere to send to, rather than letting the
  // tap fail with an error after a long wait.
  bool canSend = emailConfigured();
  uint16_t sendFill = canSend ? COL_ACCENT : COL_CARD;
  drawCardShadow(RPT_BTN_SEND_X, BTN_Y, RPT_BTN_SEND_W, BTN_H, 8);
  tft.fillRoundRect(RPT_BTN_SEND_X, BTN_Y, RPT_BTN_SEND_W, BTN_H, 8, sendFill);
  tft.drawRoundRect(RPT_BTN_SEND_X, BTN_Y, RPT_BTN_SEND_W, BTN_H, 8, COL_CARD_BRD);
  tft.setTextColor(canSend ? COL_BG_TOP : COL_TEXT_DIM, sendFill);
  centerTextInBox("Send Now", BTN_Y + 9, RPT_BTN_SEND_X, RPT_BTN_SEND_W);
}

// Sending blocks for as long as the modem takes (up to ~90s with an
// attachment on a slow uplink), so say so rather than leaving a frozen
// screen that invites repeated taps.
void drawReportSendingOverlay(const char* what) {
  drawGradientBackground();
  tft.setTextSize(2);
  tft.setTextColor(COL_ACCENT, COL_BG_BOTTOM);
  centerText("Sending report...", 90);
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  centerText(what, 120);
  centerText("This can take a minute over 4G. Please wait.", 135);
}

void handleAdminReportScreen() {
  if (!isRealTouch()) return;
  if (millis() - lastTouchTime <= TOUCH_DEBOUNCE) return;

  lastTouchTime = millis();
  TS_Point raw = ts.getPoint();
  int sx, sy;
  mapTouchToScreen(raw, sx, sy);

  if (pointInRect(sx, sy, RPT_BTN_BACK_X, BTN_Y, RPT_BTN_BACK_W, BTN_H)) {
    reportResetArmed = false;
    currentScreen = SCREEN_ADMIN_PANEL;
    drawAdminPanelScreen();
    return;
  }

  if (pointInRect(sx, sy, RPT_BTN_PAGE_X, BTN_Y, RPT_BTN_PAGE_W, BTN_H)) {
    reportPage = (reportPage + 1) % RPT_PAGE_COUNT;
    reportResetArmed = false;
    drawAdminReportScreen();
    return;
  }

  if (pointInRect(sx, sy, RPT_BTN_SEND_X, BTN_Y, RPT_BTN_SEND_W, BTN_H)) {
    if (!emailConfigured()) {
      reportPage = 3;           // send is impossible; show what's missing
      drawAdminReportScreen();
      return;
    }
    drawReportSendingOverlay("60 days of sales plus the CSV attachment.");
    sendReportEmail(REPORT_FULL, todayDateNum(), true);
    lastTouchTime = millis();   // swallow taps made while it was blocked
    drawAdminReportScreen();
    return;
  }

  // Sales page: tapping the "Today" row emails today's report — the same
  // thing the nightly schedule sends, on demand.
  if (reportPage == 0 &&
      pointInRect(sx, sy, RPT_ROW_X, reportRowY(4), RPT_ROW_W, RPT_ROW_H)) {
    if (!emailConfigured()) {
      reportPage = 3;           // send is impossible; show what's missing
      drawAdminReportScreen();
      return;
    }
    drawReportSendingOverlay("Today's sales plus today's transactions CSV.");
    sendReportEmail(REPORT_DAILY, todayDateNum(), true);
    lastTouchTime = millis();
    drawAdminReportScreen();
    return;
  }

  if (reportPage == 2) {
    if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(0), RPT_ROW_W, RPT_ROW_H)) {
      openTextEntry(TE_NIGHTLY_1);
    } else if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(1), RPT_ROW_W, RPT_ROW_H)) {
      openTextEntry(TE_NIGHTLY_2);
    } else if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(2), RPT_ROW_W, RPT_ROW_H)) {
      lowStockAlertOn = !lowStockAlertOn;
      saveScheduleSettings();
      drawAdminReportScreen();
    } else if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(3), RPT_ROW_W, RPT_ROW_H)) {
      // 1..5 covers every sensible warning point for a machine this size.
      lowStockThreshold = (lowStockThreshold >= 5) ? 1 : lowStockThreshold + 1;
      saveScheduleSettings();
      drawAdminReportScreen();
    } else if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(6), RPT_ROW_W, RPT_ROW_H)) {
      // Unrecoverable, so it takes two deliberate taps.
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
    return;
  }

  if (reportPage != 3) return;

  // Row 0 ("Sends via") and row 5 (link status) are read-only labels.
  if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(1), RPT_ROW_W, RPT_ROW_H)) {
    openTextEntry(TE_GMAIL_USER);
  } else if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(2), RPT_ROW_W, RPT_ROW_H)) {
    openTextEntry(TE_GMAIL_PASS);
  } else if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(3), RPT_ROW_W, RPT_ROW_H)) {
    openTextEntry(TE_REPORT_TO);
  } else if (pointInRect(sx, sy, RPT_ROW_X, reportRowY(4), RPT_ROW_W, RPT_ROW_H)) {
    openTextEntry(TE_MACHINE_NAME);
  }
}
