// =====================================================
//                ADMIN 4G/LTE SETUP SCREEN
// =====================================================
const int LTE_TEST_X = 20, LTE_TEST_Y = 128, LTE_TEST_W = 280, LTE_TEST_H = 28;

void drawAdminLTEScreen() {
  drawGradientBackground();

  drawScreenTitle("4G/LTE Setup");

  bool connected = isLTEConnected();

  tft.setTextSize(1);
  tft.setCursor(20, 46);
  if (!lteEnabled) {
    // Neutral colour, not COL_DANGER — this is a deliberate choice the admin
    // just made, not a fault. Takes priority over the other three states:
    // lteLastError is left holding whatever it last said and would
    // otherwise flash back up here the instant the radio is switched off.
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    tft.print("Status: Turned off");
  } else if (connected) {
    tft.setTextColor(COL_SUCCESS, COL_BG_BOTTOM);
    tft.print("Status: Connected");
  } else if (lteLastError[0] != '\0') {
    tft.setTextColor(COL_DANGER, COL_BG_BOTTOM);
    tft.printf("Status: %s", lteLastError);
  } else {
    tft.setTextColor(COL_DANGER, COL_BG_BOTTOM);
    tft.print("Status: Not connected");
  }

  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, 64);
  tft.printf("APN: %s", LTE_APN);
  tft.print("  (used as backup when WiFi is down)");

  int bars = lteSignalBars();
  tft.setCursor(20, 82);
  if (connected) {
    tft.printf("Signal: %s", wifiQualityLabel(bars));
  } else {
    tft.print("Signal: -");
  }
  drawCellularBars(268, 96, 2, connected ? bars : 0,
                    connected ? COL_SUCCESS : COL_TEXT_DIM);

  drawCardShadow(LTE_TEST_X, LTE_TEST_Y, LTE_TEST_W, LTE_TEST_H, 6);
  tft.fillRoundRect(LTE_TEST_X, LTE_TEST_Y, LTE_TEST_W, LTE_TEST_H, 6, COL_ACCENT);
  tft.setTextSize(2);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox("Test Internet", LTE_TEST_Y + 7, LTE_TEST_X, LTE_TEST_W);

  tft.setTextSize(1);
  tft.setCursor(20, 166);
  // !lteEnabled takes priority over whatever the last real test said —
  // otherwise turning LTE off would leave an old "Test OK" or "Test failed"
  // line sitting there looking current.
  if (!lteEnabled) {
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    tft.print("Test result: LTE is turned off");
  } else if (!ltePingAttempted) {
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    tft.print("Test result: (not tested yet)");
  } else if (ltePingSuccess) {
    tft.setTextColor(COL_SUCCESS, COL_BG_BOTTOM);
    tft.printf("Test OK - HTTP %d in %lums", ltePingHttpCode, ltePingMs);
  } else {
    tft.setTextColor(COL_DANGER, COL_BG_BOTTOM);
    tft.print("Test failed - could not reach the internet over 4G");
  }

  drawBackButton("< Back");

  // Shares the bottom bar with "< Back" — the BTN_PROCEED slot every other
  // screen uses for its one primary action was sitting empty here. Styled
  // like the Settings screen's own WiFi/4G chips (Screen_11_AdminSettings.ino):
  // accent fill when on, bordered COL_CARD with dimmed text when off.
  uint16_t toggleFill = lteEnabled ? COL_ACCENT : COL_CARD;
  drawCardShadow(BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H, 8);
  tft.fillRoundRect(BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H, 8, toggleFill);
  if (!lteEnabled) tft.drawRoundRect(BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H, 8, COL_CARD_BRD);
  tft.setTextSize(2);
  tft.setTextColor(lteEnabled ? COL_BG_TOP : COL_TEXT_DIM, toggleFill);
  centerTextInBox(lteEnabled ? "LTE: On" : "LTE: Off", BTN_Y + 9, BTN_PROCEED_X, BTN_PROCEED_W);
}

void handleAdminLTEScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (pointInRect(sx, sy, LTE_TEST_X, LTE_TEST_Y, LTE_TEST_W, LTE_TEST_H)) {
        // connectLTEIfNeeded() (which pingGoogleTestLTE() calls first) already
        // refuses instantly when lteEnabled is false, so this isn't needed for
        // correctness — it's here so a tap while off goes straight to the
        // "LTE is turned off" result line above instead of flashing the
        // "Testing 4G connection... this can take up to 15-20s" screen for
        // something that will return in microseconds.
        if (lteEnabled) {
          drawGradientBackground();
          tft.setTextSize(2);
          tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
          centerText("Testing 4G connection...", 100);
          centerText("(this can take up to 15-20s)", 130);
          pingGoogleTestLTE();
        }
        drawAdminLTEScreen();
        return;
      }

      if (pointInRect(sx, sy, BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H)) {
        setLteEnabled(!lteEnabled);  // Core_13_LTEModem.ino
        drawAdminLTEScreen();
        return;
      }

      if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_SETTINGS;
        drawAdminSettingsScreen();
        return;
      }
    }
  }

  static unsigned long lastLTEScreenRefresh = 0;
  if (millis() - lastLTEScreenRefresh > 3000) {
    lastLTEScreenRefresh = millis();
    drawAdminLTEScreen();
  }
}
