// =====================================================
//           ADMIN COIN ACCEPTOR CONFIG SCREEN
// =====================================================
// One row per denomination the acceptor can be taught, each a switch between
// Accept and Reject (a rejected coin is returned by the acceptor, never
// credited).
const int COIN_ROW_X = 16, COIN_ROW_W = 288, COIN_ROW_H = 32;
const int COIN_ROW_Y0 = 40, COIN_ROW_GAP = 6;
const int COIN_RIGHT = COIN_ROW_X + COIN_ROW_W - 8;
const int COIN_SW_W = 90;

int coinRowY(int d) {
  return COIN_ROW_Y0 + d * (COIN_ROW_H + COIN_ROW_GAP);
}

void drawAdminCoinScreen() {
  drawGradientBackground();

  drawScreenTitle("Coin Acceptor");

  for (int d = 0; d < COIN_DENOM_COUNT; d++) {
    int y = coinRowY(d), cy = y + COIN_ROW_H / 2;
    auiRow(COIN_ROW_X, y, COIN_ROW_W, COIN_ROW_H);

    char label[16];
    // "Re. 1" for the singular rupee, "Rs. N" for the rest
    if (COIN_DENOM_VALUES[d] == 1) snprintf(label, sizeof(label), "Re. 1 coin");
    else snprintf(label, sizeof(label), "Rs. %d coin", COIN_DENOM_VALUES[d]);
    auiRowLabel(COIN_ROW_X + 12, cy, 150, label);

    auiSwitch(COIN_RIGHT, cy, coinDenomEnabled[d], COIN_SW_W, "Accept", "Reject");
  }

  auiBarButton(0, 1, "Back", ABTN_SECONDARY);
}

void handleAdminCoinScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (auiBarHit(sx, sy, 0, 1)) {
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
