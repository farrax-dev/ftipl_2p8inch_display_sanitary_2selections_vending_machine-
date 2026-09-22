// =====================================================
//           ADMIN COIN ACCEPTOR CONFIG SCREEN
// =====================================================
const int COIN_ROW_X = 20, COIN_ROW_W = 280, COIN_ROW_H = 26;
const int COIN_ROW_Y0 = 50, COIN_ROW_GAP = 8;

int coinRowY(int d) {
  return COIN_ROW_Y0 + d * (COIN_ROW_H + COIN_ROW_GAP);
}

void drawAdminCoinScreen() {
  drawGradientBackground();

  drawScreenTitle("Coin Acceptor");

  for (int d = 0; d < COIN_DENOM_COUNT; d++) {
    int y = coinRowY(d);
    drawCard(COIN_ROW_X, y, COIN_ROW_W, COIN_ROW_H, 6);

    tft.setTextSize(2);
    tft.setTextColor(COL_TEXT, COL_CARD);
    char label[12];
    snprintf(label, sizeof(label), "Rs %d coin", COIN_DENOM_VALUES[d]);
    tft.setCursor(COIN_ROW_X + 10, y + 5);
    tft.print(label);

    int toggleW = 90, toggleH = 20;
    int toggleX = COIN_ROW_X + COIN_ROW_W - toggleW - 8;
    int toggleY = y + (COIN_ROW_H - toggleH) / 2;
    uint16_t toggleColor = coinDenomEnabled[d] ? COL_ACCENT : COL_BG_TOP;
    tft.fillRoundRect(toggleX, toggleY, toggleW, toggleH, 5, toggleColor);
    tft.drawRoundRect(toggleX, toggleY, toggleW, toggleH, 5, COL_CARD_BRD);
    tft.setTextSize(1);
    tft.setTextColor(coinDenomEnabled[d] ? COL_BG_TOP : COL_TEXT_DIM, toggleColor);
    centerTextInBox(coinDenomEnabled[d] ? "ACCEPTED" : "REJECTED", toggleY + 6, toggleX, toggleW);
  }

  drawBackButton("< Back");
}

void handleAdminCoinScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_SETTINGS;
        drawAdminSettingsScreen();
        return;
      }

      for (int d = 0; d < COIN_DENOM_COUNT; d++) {
        if (pointInRect(sx, sy, COIN_ROW_X, coinRowY(d), COIN_ROW_W, COIN_ROW_H)) {
          coinDenomEnabled[d] = !coinDenomEnabled[d];
          saveCoinDenomEnabled(d);
          drawAdminCoinScreen();
          return;
        }
      }
    }
  }
}
