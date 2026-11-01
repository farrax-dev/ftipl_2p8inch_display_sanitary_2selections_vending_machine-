// =====================================================
//                ADMIN 4G/LTE SETUP SCREEN
// =====================================================
// Same arrangement as WiFi Setup (Screen_13): the radio switch in the header
// band, one status card, Back / Test in the bottom bar. The card is redrawn on
// its own every few seconds, in place.
const int LTE_CARD_X = 16, LTE_CARD_Y = 40, LTE_CARD_W = 288, LTE_CARD_H = 140;

// busy != NULL replaces the card's contents with a "working" message while the
// blocking connectivity test runs.
void drawLTEStatusCard(const char* busy) {
  int x = LTE_CARD_X, y = LTE_CARD_Y, w = LTE_CARD_W, h = LTE_CARD_H;
  drawCard(x, y, w, h, 8);

  if (busy != NULL) {
    auiPut(busy, AL_CENTER, x + 12, w - 24, y + h / 2 - 10, COL_MSG_TITLE, FACE_MD);
    auiPut(TXT_THIS_CAN_TAKE_20_SECONDS, AL_CENTER, x + 12, w - 24, y + h / 2 + 16, COL_CARD_CAPTION, FACE_XS);
    return;
  }

  bool connected = isLTEConnected();

  // L1: state. Priority order matters: lteLastError is left holding whatever
  // it last said and would otherwise flash back up the instant the radio is
  // switched off. Neutral colour when off — a deliberate choice, not a fault.
  char state[48];
  uint16_t stateCol;
  if (!lteEnabled)                { snprintf(state, sizeof(state), TXT_TURNED_OFF);     stateCol = COL_CARD_CAPTION; }
  else if (connected)             { snprintf(state, sizeof(state), TXT_CONNECTED);      stateCol = COL_OK_TEXT; }
  else if (lteLastError[0] != '\0') { snprintf(state, sizeof(state), "%s", lteLastError); stateCol = COL_ERR_TEXT; }
  else                            { snprintf(state, sizeof(state), TXT_NOT_CONNECTED);  stateCol = COL_ERR_TEXT; }
  auiPut(state, AL_LEFT, x + 14, w - 28, y + 18, stateCol, FACE_MD);

  char buf[56];
  snprintf(buf, sizeof(buf), "APN: %s", LTE_APN);
  auiPut(buf, AL_LEFT, x + 14, w - 28, y + 46, COL_CARD_TITLE, FACE_XS);
  auiPut(TXT_USED_AS_THE_BACKUP_LINK_WHEN_W, AL_LEFT, x + 14, w - 28, y + 66, COL_CARD_CAPTION, FACE_XS);

  int bars = lteSignalBars();
  if (connected) snprintf(buf, sizeof(buf), "Signal: %s", wifiQualityLabel(bars));
  else           snprintf(buf, sizeof(buf), TXT_SIGNAL);
  auiPut(buf, AL_LEFT, x + 14, w - 28 - 48, y + 92, COL_CARD_TITLE, FACE_XS);
  drawCellularBars(x + w - 14 - 36, y + 100, 2, connected ? bars : 0,
                   connected ? COL_WIFI_GOOD : COL_WIFI_OFF);

  // L5: last test. !lteEnabled takes priority over whatever the last real
  // test said — otherwise turning LTE off would leave an old "Test OK" or
  // "Test failed" line sitting there looking current.
  uint16_t resCol = COL_CARD_CAPTION;
  if (!lteEnabled) {
    snprintf(buf, sizeof(buf), TXT_TEST_4G_IS_TURNED_OFF);
  } else if (!ltePingAttempted) {
    snprintf(buf, sizeof(buf), TXT_TEST_NOT_TESTED_YET);
  } else if (ltePingSuccess) {
    snprintf(buf, sizeof(buf), "Test OK - HTTP %d in %lums", ltePingHttpCode, ltePingMs);
    resCol = COL_OK_TEXT;
  } else {
    snprintf(buf, sizeof(buf), TXT_TEST_FAILED_COULD_NOT_REACH_TH);
    resCol = COL_ERR_TEXT;
  }
  auiPut(buf, AL_LEFT, x + 14, w - 28, y + 124, resCol, FACE_SM);
}

void drawAdminLTEScreen() {
  drawGradientBackground();

  drawScreenTitle(TXT_4G_LTE_SETUP, tft.width() - (HDR_SW_RIGHT - ASW_W - 6));
  drawHeaderSwitch(lteEnabled);

  drawLTEStatusCard(NULL);

  // Greyed out while the radio is off — the handler already no-ops the tap in
  // that state, but until now it still looked fully active.
  auiBarButton(0, 2, TXT_BACK, ABTN_SECONDARY);
  auiBarButton(1, 2, TXT_TEST_4G, lteEnabled ? ABTN_PRIMARY : ABTN_OFF);
}

void handleAdminLTEScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (headerSwitchHit(sx, sy)) {
        setLteEnabled(!lteEnabled);  // Core_13_LTEModem.ino
        drawAdminLTEScreen();
        return;
      }

      if (auiBarHit(sx, sy, 1, 2)) {
        // connectLTEIfNeeded() (which pingGoogleTestLTE() calls first) already
        // refuses instantly when lteEnabled is false, so this isn't needed for
        // correctness — it's here so a tap while off goes straight to the
        // "4G is turned off" result line instead of flashing a "Testing..."
        // message for something that will return in microseconds.
        if (lteEnabled) {
          drawLTEStatusCard("Testing 4G connection...");
          pingGoogleTestLTE();
        }
        lastTouchTime = millis();
        drawAdminLTEScreen();
        return;
      }

      if (auiBarHit(sx, sy, 0, 2)) {
        currentScreen = SCREEN_ADMIN_NETWORK;
        drawAdminNetworkScreen();
        return;
      }
    }
  }

  static unsigned long lastLTEScreenRefresh = 0;
  if (millis() - lastLTEScreenRefresh > 3000) {
    lastLTEScreenRefresh = millis();
    drawLTEStatusCard(NULL);
  }
}
