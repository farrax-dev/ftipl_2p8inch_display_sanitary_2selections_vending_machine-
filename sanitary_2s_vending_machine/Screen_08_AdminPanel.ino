// =====================================================
//                  ADMIN PANEL SCREEN
// =====================================================
const int ADMIN_ROW_X = 20, ADMIN_ROW_W = 280, ADMIN_ROW_H = 21;
const int ADMIN_ROW_Y0 = 44, ADMIN_ROW_GAP = 3;

// Three nav chips (Clock/Report/Setup) sharing the header band with the
// title. 42px each now rather than 44, and starting 10px further right:
// "Admin Panel" in the header face (Core_08_UIHelpers.ino) is 135px wide and
// ends at x=151, where at the old text size 2 it ended around x=147. The
// three still finish at 294, clear of the status icon's strip at x=296.
//
// Centred on the band, so the chips and the title sit on one middle line.
const int PANEL_CHIP_W = 42, PANEL_CHIP_H = 20;
const int PANEL_CHIP_Y = HDR_Y + (HDR_H - PANEL_CHIP_H) / 2;

const int PANEL_CLK_X = 160, PANEL_CLK_Y = PANEL_CHIP_Y;
const int PANEL_CLK_W = PANEL_CHIP_W, PANEL_CLK_H = PANEL_CHIP_H;

const int PANEL_RPT_X = 206, PANEL_RPT_Y = PANEL_CHIP_Y;
const int PANEL_RPT_W = PANEL_CHIP_W, PANEL_RPT_H = PANEL_CHIP_H;

const int PANEL_SET_X = 252, PANEL_SET_Y = PANEL_CHIP_Y;
const int PANEL_SET_W = PANEL_CHIP_W, PANEL_SET_H = PANEL_CHIP_H;

int adminRowY(int index) {
  return ADMIN_ROW_Y0 + index * (ADMIN_ROW_H + ADMIN_ROW_GAP);
}

void drawAdminPanelScreen() {
  drawGradientBackground();

  drawScreenTitle("Admin Panel", tft.width() - PANEL_CLK_X);

  int chipTextY = PANEL_CHIP_Y + (PANEL_CHIP_H - 8) / 2;

  drawCard(PANEL_CLK_X, PANEL_CLK_Y, PANEL_CLK_W, PANEL_CLK_H, 5);
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT, COL_CARD);
  centerTextInBox("Clock", chipTextY, PANEL_CLK_X, PANEL_CLK_W);

  drawCard(PANEL_RPT_X, PANEL_RPT_Y, PANEL_RPT_W, PANEL_RPT_H, 5);
  tft.setTextColor(COL_TEXT, COL_CARD);
  centerTextInBox("Report", chipTextY, PANEL_RPT_X, PANEL_RPT_W);

  drawCard(PANEL_SET_X, PANEL_SET_Y, PANEL_SET_W, PANEL_SET_H, 5);
  tft.setTextColor(COL_TEXT, COL_CARD);
  centerTextInBox("Setup", chipTextY, PANEL_SET_X, PANEL_SET_W);

  for (int i = 0; i < MAX_PRODUCTS; i++) {
    int y = adminRowY(i);
    drawCard(ADMIN_ROW_X, y, ADMIN_ROW_W, ADMIN_ROW_H, 5);

    char motorBuf[16];  // worst case "1,2,3,4,5" across all MAX_MOTORS
    formatMotorList(products[i].motorMask, motorBuf, sizeof(motorBuf));

    tft.setTextSize(1);
    tft.setTextColor(productEnabled[i] ? COL_TEXT : COL_TEXT_DIM, COL_CARD);
    tft.setCursor(ADMIN_ROW_X + 6, y + 6);
    tft.printf("%d:%-9.9s R%-3d M:%-6s %s",
                i + 1, products[i].name, products[i].price, motorBuf,
                productEnabled[i] ? "ON" : "OFF");
  }

  drawBackButton("Exit Admin");
  drawProceedButton("Motor Stock");
}

void handleAdminPanelScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      for (int i = 0; i < MAX_PRODUCTS; i++) {
        if (pointInRect(sx, sy, ADMIN_ROW_X, adminRowY(i), ADMIN_ROW_W, ADMIN_ROW_H)) {
          adminEditingSlot = i;
          currentScreen = SCREEN_ADMIN_EDIT;
          drawAdminEditScreen();
          return;
        }
      }

      if (pointInRect(sx, sy, PANEL_CLK_X, PANEL_CLK_Y, PANEL_CLK_W, PANEL_CLK_H)) {
        openAdminDateTimeScreen();
        return;
      }

      if (pointInRect(sx, sy, PANEL_RPT_X, PANEL_RPT_Y, PANEL_RPT_W, PANEL_RPT_H)) {
        reportPage = 0;
        currentScreen = SCREEN_ADMIN_REPORT;
        drawAdminReportScreen();
        return;
      }

      if (pointInRect(sx, sy, PANEL_SET_X, PANEL_SET_Y, PANEL_SET_W, PANEL_SET_H)) {
        currentScreen = SCREEN_ADMIN_SETTINGS;
        drawAdminSettingsScreen();
        return;
      }

      if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
        currentScreen = SCREEN_WELCOME;
        drawWelcomeScreen();
        return;
      }

      if (pointInRect(sx, sy, BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H)) {
        motorStockPage = 0;
        currentScreen = SCREEN_ADMIN_MOTOR_STOCK;
        drawAdminMotorStockScreen();
        return;
      }
    }
  }
}
