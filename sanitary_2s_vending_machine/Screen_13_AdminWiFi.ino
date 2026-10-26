// =====================================================
//                  ADMIN WIFI SCREEN
// =====================================================
// Top to bottom: the WiFi radio switch in the header band, the network name
// and password (tap either row to edit), a live status card (state, IP,
// signal, last ping), and the actions in the bottom bar: Back / Connect /
// Ping. Connect and Ping grey out together while the radio is off, which is
// also what the handler enforces — they used to look fully active while doing
// nothing.
//
// The status card is redrawn on its own every few seconds, in place, rather
// than the whole screen: a full redraw repaints the gradient and flickers.
const int WF_ROW_X = 16, WF_ROW_W = 288, WF_ROW_H = 30;
const int WF_ROW_Y0 = 40, WF_ROW_GAP = 4;
const int WF_BTN_W = 64;
const int WF_RIGHT = WF_ROW_X + WF_ROW_W - 8;
const int WF_BTN_X = WF_RIGHT - WF_BTN_W;

const int WF_CARD_Y = 110, WF_CARD_H = 78;

// Radio switch in the header band, clear of the status icon at x=296.
const int HDR_SW_RIGHT = 288;
const int HDR_SW_CY = HDR_Y + HDR_H / 2;

int wfRowY(int row) { return WF_ROW_Y0 + row * (WF_ROW_H + WF_ROW_GAP); }

void drawHeaderSwitch(bool on) {
  auiOnOff(HDR_SW_RIGHT, HDR_SW_CY, on);
}

bool headerSwitchHit(int sx, int sy) {
  return pointInRect(sx, sy, HDR_SW_RIGHT - ASW_W - 8, 0, ASW_W + 12, HDR_BOTTOM + 2);
}

// busy != NULL replaces the card's contents with a one-line "working" message
// while a blocking connect/ping is under way.
void drawWiFiStatusCard(const char* busy) {
  int x = WF_ROW_X, y = WF_CARD_Y, w = WF_ROW_W, h = WF_CARD_H;
  bool connected = wifiEnabled && (WiFi.status() == WL_CONNECTED);

  drawCard(x, y, w, h, 8);

  if (busy != NULL) {
    auiPut(busy, AL_CENTER, x + 12, w - 24, y + h / 2, COL_ACCENT, FACE_MD);
    return;
  }

  // L1: state. Neutral colour when switched off — a deliberate choice, not a
  // fault.
  const char* state = !wifiEnabled ? "Turned off" : connected ? "Connected" : "Not connected";
  uint16_t stateCol = !wifiEnabled ? COL_TEXT_DIM : connected ? COL_SUCCESS : COL_DANGER;
  auiPut(state, AL_LEFT, x + 12, w - 24, y + 14, stateCol, FACE_MD);

  int textW = w - 24 - 48;   // leave the signal glyph's column free
  char buf[48];
  if (connected) snprintf(buf, sizeof(buf), "IP: %s", WiFi.localIP().toString().c_str());
  else           snprintf(buf, sizeof(buf), "IP: -");
  auiPut(buf, AL_LEFT, x + 12, textW, y + 33, COL_TEXT, FACE_XS);

  if (connected) {
    long rssi = WiFi.RSSI();
    snprintf(buf, sizeof(buf), "Signal: %s (%ld dBm)", wifiQualityLabel(wifiBarsFromRSSI(rssi)), rssi);
  } else {
    snprintf(buf, sizeof(buf), "Signal: -");
  }
  auiPut(buf, AL_LEFT, x + 12, textW, y + 50, COL_TEXT, FACE_XS);

  int bars = connected ? wifiBarsFromRSSI(WiFi.RSSI()) : 0;
  drawWiFiSymbol(x + w - 34, y + 48, 2, bars, connected);

  // L4: last ping. !wifiEnabled takes priority over everything else — same
  // reasoning as the state line above, and as 4G Setup's test-result line.
  uint16_t resCol = COL_TEXT_DIM;
  if (!wifiEnabled) {
    snprintf(buf, sizeof(buf), "Ping: WiFi is turned off");
  } else if (strlen(wifiSSID) == 0) {
    snprintf(buf, sizeof(buf), "Set a network name to get started");
  } else if (!wifiPingAttempted) {
    snprintf(buf, sizeof(buf), "Ping: not tested yet");
  } else if (wifiPingSuccess) {
    snprintf(buf, sizeof(buf), "Ping OK - HTTP %d in %lums", wifiPingHttpCode, wifiPingMs);
    resCol = COL_SUCCESS;
  } else {
    snprintf(buf, sizeof(buf), "Ping failed - no internet reachable");
    resCol = COL_DANGER;
  }
  auiPut(buf, AL_LEFT, x + 12, w - 24, y + 67, resCol, FACE_XS);
}

void drawAdminWiFiScreen() {
  drawGradientBackground();

  drawScreenTitle("WiFi Setup", tft.width() - (HDR_SW_RIGHT - ASW_W - 6));
  drawHeaderSwitch(wifiEnabled);

  // ---- Network name ----
  int cy = wfRowY(0) + WF_ROW_H / 2;
  auiRow(WF_ROW_X, wfRowY(0), WF_ROW_W, WF_ROW_H);
  auiRowLabel(WF_ROW_X + 12, cy, 90, "Network");
  int vx = WF_ROW_X + 12 + auiTextW("Network", FACE_MD) + 12;
  auiPut(strlen(wifiSSID) > 0 ? wifiSSID : "(not set)", AL_LEFT, vx, WF_BTN_X - 8 - vx, cy,
         COL_ACCENT, FACE_MD);
  auiRowButton(WF_BTN_X, cy, WF_BTN_W, "Edit", ABTN_PRIMARY);

  // ---- Password (never shown, only its length) ----
  cy = wfRowY(1) + WF_ROW_H / 2;
  auiRow(WF_ROW_X, wfRowY(1), WF_ROW_W, WF_ROW_H);
  auiRowLabel(WF_ROW_X + 12, cy, 100, "Password");
  vx = WF_ROW_X + 12 + auiTextW("Password", FACE_MD) + 12;
  int plen = strlen(wifiPass);
  if (plen == 0) {
    auiPut("(none)", AL_LEFT, vx, WF_BTN_X - 8 - vx, cy, COL_TEXT_DIM, FACE_MD);
  } else {
    int n = min(plen, 12);
    for (int k = 0; k < n; k++) tft.fillCircle(vx + 5 + k * 12, cy, 4, COL_ACCENT);
  }
  auiRowButton(WF_BTN_X, cy, WF_BTN_W, "Edit", ABTN_PRIMARY);

  drawWiFiStatusCard(NULL);

  int actionStyle = wifiEnabled ? ABTN_PRIMARY : ABTN_OFF;
  auiBarButton(0, 3, "Back", ABTN_SECONDARY);
  auiBarButton(1, 3, "Connect", actionStyle);
  auiBarButton(2, 3, "Ping", actionStyle);
}

void handleAdminWiFiScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (headerSwitchHit(sx, sy)) {
        setWifiEnabled(!wifiEnabled);  // Core_06_Network.ino
        drawAdminWiFiScreen();
        return;
      }
      if (pointInRect(sx, sy, WF_ROW_X, wfRowY(0), WF_ROW_W, WF_ROW_H)) {
        openTextEntry(TE_WIFI_SSID);
        return;
      }
      if (pointInRect(sx, sy, WF_ROW_X, wfRowY(1), WF_ROW_W, WF_ROW_H)) {
        openTextEntry(TE_WIFI_PASS);
        return;
      }
      if (auiBarHit(sx, sy, 1, 3)) {
        // Guarded the same way LTE Setup's Test button guards on lteEnabled
        // — connectWiFiIfNeeded() would already refuse instantly while off,
        // but this skips the pointless disconnect/reconnect cycle too.
        if (wifiEnabled && strlen(wifiSSID) > 0) {
          drawWiFiStatusCard("Connecting...");
          WiFi.disconnect(true);
          delay(100);
          connectWiFiIfNeeded();
          ntpConfigured = false;   // re-run SNTP against the new connection
          wifiPingAttempted = false;
        }
        lastTouchTime = millis();
        drawAdminWiFiScreen();
        return;
      }
      if (auiBarHit(sx, sy, 2, 3)) {
        if (wifiEnabled) {
          drawWiFiStatusCard("Testing connection...");
          pingGoogleTest();
        }
        lastTouchTime = millis();
        drawAdminWiFiScreen();
        return;
      }
      if (auiBarHit(sx, sy, 0, 3)) {
        currentScreen = SCREEN_ADMIN_NETWORK;
        drawAdminNetworkScreen();
        return;
      }
    }
  }

  static unsigned long lastWiFiScreenRefresh = 0;
  if (millis() - lastWiFiScreenRefresh > 3000) {
    lastWiFiScreenRefresh = millis();
    drawWiFiStatusCard(NULL);
  }
}
