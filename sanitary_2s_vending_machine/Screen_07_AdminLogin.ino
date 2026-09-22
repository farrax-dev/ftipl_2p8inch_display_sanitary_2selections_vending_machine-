// =====================================================
//                  ADMIN LOGIN SCREEN
// =====================================================
// The PIN compared against here is adminPin (Core_09_Storage.ino) — a
// persisted, admin-changeable value seeded from Config.h's CFG_ADMIN_PIN,
// not that macro directly. See Core_09_Storage.ino's own comment for the
// change-it-on-screen vs. reset-it-by-reflashing distinction.
char pinBuffer[9] = "";

void enterAdminLogin() {
  pinBuffer[0] = '\0';
  currentScreen = SCREEN_ADMIN_LOGIN;
  drawAdminLoginScreen();
}

// Sits inside the header band (HDR_Y..HDR_Y+HDR_H, Core_08_UIHelpers.ino)
// beside the title, not below it — centred on the band so the chip and the
// title share a middle line.
const int LOGIN_CANCEL_W = 56,  LOGIN_CANCEL_H = 20;
const int LOGIN_CANCEL_X = 236, LOGIN_CANCEL_Y = HDR_Y + (HDR_H - LOGIN_CANCEL_H) / 2;

void drawAdminLoginScreen() {
  drawGradientBackground();

  drawScreenTitle("Admin Login", tft.width() - LOGIN_CANCEL_X);

  drawCardShadow(LOGIN_CANCEL_X, LOGIN_CANCEL_Y, LOGIN_CANCEL_W, LOGIN_CANCEL_H, 6);
  tft.fillRoundRect(LOGIN_CANCEL_X, LOGIN_CANCEL_Y, LOGIN_CANCEL_W, LOGIN_CANCEL_H, 6, COL_CARD);
  tft.drawRoundRect(LOGIN_CANCEL_X, LOGIN_CANCEL_Y, LOGIN_CANCEL_W, LOGIN_CANCEL_H, 6, COL_CARD_BRD);
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT, COL_CARD);
  centerTextInBox("Cancel", LOGIN_CANCEL_Y + (LOGIN_CANCEL_H - 8) / 2,
                  LOGIN_CANCEL_X, LOGIN_CANCEL_W);

  drawPinLine();

  const char* labels[4][3] = {
    {"1", "2", "3"},
    {"4", "5", "6"},
    {"7", "8", "9"},
    {"C", "0", "OK"}
  };
  int x0 = 55, y0 = 70, bw = 66, bh = 32, gx = 6, gy = 6;
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 3; col++) {
      int x = x0 + col * (bw + gx);
      int y = y0 + row * (bh + gy);
      const char* lbl = labels[row][col];
      // OK is the one button that actually submits the PIN, so it gets the
      // accent treatment; C just clears the entry and is dimmed instead of
      // matching the plain digit keys.
      bool isOK = (strcmp(lbl, "OK") == 0);
      bool isClear = (strcmp(lbl, "C") == 0);
      uint16_t fill = isOK ? COL_ACCENT : (isClear ? COL_BG_TOP : COL_CARD);
      uint16_t txt  = isOK ? COL_BG_TOP : (isClear ? COL_TEXT_DIM : COL_TEXT);
      // "C" stays flush with the background on purpose (a quieter, lesser
      // key), so it skips the shadow the same way the "no refund" pill does
      // — a shadow under a background-colored fill reads as a smudge, not a
      // raised key.
      if (!isClear) drawCardShadow(x, y, bw, bh, 6);
      tft.fillRoundRect(x, y, bw, bh, 6, fill);
      tft.drawRoundRect(x, y, bw, bh, 6, COL_CARD_BRD);
      tft.setTextSize(2);
      tft.setTextColor(txt, fill);
      centerTextInBox(lbl, y + 7, x, bw);
    }
  }
}

void drawPinLine() {
  tft.fillRect(0, 40, tft.width(), 25, COL_BG_TOP);
  tft.setTextSize(2);
  int len = strlen(pinBuffer);
  if (len == 0) {
    tft.setTextColor(COL_TEXT_DIM, COL_BG_TOP);
    centerText("Enter PIN", 48);
  } else {
    char mask[9];
    for (int k = 0; k < len; k++) mask[k] = '*';
    mask[len] = '\0';
    tft.setTextColor(COL_ACCENT, COL_BG_TOP);
    centerText(mask, 48);
  }
}

void handleAdminLoginScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (pointInRect(sx, sy, LOGIN_CANCEL_X, LOGIN_CANCEL_Y, LOGIN_CANCEL_W, LOGIN_CANCEL_H)) {
        currentScreen = SCREEN_WELCOME;
        drawWelcomeScreen();
        return;
      }

      const char* labels[4][3] = {
        {"1", "2", "3"},
        {"4", "5", "6"},
        {"7", "8", "9"},
        {"C", "0", "OK"}
      };
      int x0 = 55, y0 = 70, bw = 66, bh = 32, gx = 6, gy = 6;
      for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
          int x = x0 + col * (bw + gx);
          int y = y0 + row * (bh + gy);
          if (pointInRect(sx, sy, x, y, bw, bh)) {
            const char* lbl = labels[row][col];

            if (strcmp(lbl, "C") == 0) {
              pinBuffer[0] = '\0';
              drawPinLine();
            } else if (strcmp(lbl, "OK") == 0) {
              if (strcmp(pinBuffer, adminPin) == 0) {
                pinBuffer[0] = '\0';
                currentScreen = SCREEN_ADMIN_PANEL;
                drawAdminPanelScreen();
              } else {
                tft.fillRect(0, 40, tft.width(), 25, COL_BG_TOP);
                tft.setTextSize(2);
                tft.setTextColor(COL_DANGER, COL_BG_TOP);
                centerText("Wrong PIN", 48);
                delay(900);
                pinBuffer[0] = '\0';
                drawPinLine();
              }
            } else {
              int len = strlen(pinBuffer);
              if (len < 8) {
                pinBuffer[len] = lbl[0];
                pinBuffer[len + 1] = '\0';
              }
              drawPinLine();
            }
            return;
          }
        }
      }
    }
  }
}
