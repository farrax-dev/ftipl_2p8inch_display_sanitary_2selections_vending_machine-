// =====================================================
//                  ADMIN WIFI SCREEN
// =====================================================
const int WF_SSID_BTN_X = 236, WF_SSID_BTN_Y = 40, WF_BTN_W = 66, WF_BTN_H = 20;
const int WF_PASS_BTN_Y = 64;
const int WF_CONNECT_X = 20,  WF_CONNECT_Y = 128, WF_CONNECT_W = 130, WF_CONNECT_H = 28;
const int WF_PING_X    = 170, WF_PING_Y    = 128, WF_PING_W    = 130, WF_PING_H    = 28;

void drawWiFiFieldButton(int y, const char* label) {
  drawCardShadow(WF_SSID_BTN_X, y, WF_BTN_W, WF_BTN_H, 5);
  tft.fillRoundRect(WF_SSID_BTN_X, y, WF_BTN_W, WF_BTN_H, 5, COL_ACCENT);
  tft.setTextSize(1);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox(label, y + 7, WF_SSID_BTN_X, WF_BTN_W);
}

void drawAdminWiFiScreen() {
  drawGradientBackground();

  drawScreenTitle("WiFi Setup");

  bool connected = wifiEnabled && (WiFi.status() == WL_CONNECTED);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, 46);
  tft.print("Name:");
  tft.setTextColor(COL_TEXT, COL_BG_BOTTOM);
  tft.setCursor(60, 46);
  if (strlen(wifiSSID) > 0) tft.printf("%.24s", wifiSSID);
  else tft.print("(not set)");
  drawWiFiFieldButton(WF_SSID_BTN_Y, "Edit");

  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, 70);
  tft.print("Pass:");
  tft.setTextColor(COL_TEXT, COL_BG_BOTTOM);
  tft.setCursor(60, 70);
  int plen = strlen(wifiPass);
  if (plen == 0) {
    tft.print("(none)");
  } else {
    char stars[25];
    int n = min(plen, 24);
    for (int k = 0; k < n; k++) stars[k] = '*';
    stars[n] = '\0';
    tft.print(stars);
  }
  drawWiFiFieldButton(WF_PASS_BTN_Y, "Edit");

  tft.setCursor(20, 90);
  if (!wifiEnabled) {
    // Neutral colour, not COL_DANGER — this is a deliberate choice the admin
    // just made, not a fault. Same priority LTE Setup gives its own "Turned
    // off" line (Screen_16_AdminLTE.ino).
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    tft.print("Status: Turned off");
  } else {
    tft.setTextColor(connected ? COL_SUCCESS : COL_DANGER, COL_BG_BOTTOM);
    tft.print(connected ? "Status: Connected" : "Status: Not connected");
  }

  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(180, 90);
  if (connected) tft.printf("IP: %s", WiFi.localIP().toString().c_str());
  else tft.print("IP: -");

  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, 108);
  if (connected) {
    long rssi = WiFi.RSSI();
    int bars = wifiBarsFromRSSI(rssi);
    tft.printf("Signal: %ld dBm  %s", rssi, wifiQualityLabel(bars));
    drawWiFiSymbol(275, 122, 2, bars, true);
  } else {
    tft.print("Signal: -");
    drawWiFiSymbol(275, 122, 2, 0, false);
  }

  drawCardShadow(WF_CONNECT_X, WF_CONNECT_Y, WF_CONNECT_W, WF_CONNECT_H, 6);
  tft.fillRoundRect(WF_CONNECT_X, WF_CONNECT_Y, WF_CONNECT_W, WF_CONNECT_H, 6, COL_ACCENT);
  tft.setTextSize(2);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox("Connect", WF_CONNECT_Y + 7, WF_CONNECT_X, WF_CONNECT_W);

  drawCardShadow(WF_PING_X, WF_PING_Y, WF_PING_W, WF_PING_H, 6);
  tft.fillRoundRect(WF_PING_X, WF_PING_Y, WF_PING_W, WF_PING_H, 6, COL_ACCENT);
  centerTextInBox("Ping", WF_PING_Y + 7, WF_PING_X, WF_PING_W);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, 152);
  if (timeSynced) {
    char timeBuf[16], dateBuf[24];
    if (getClockStrings(timeBuf, sizeof(timeBuf), dateBuf, sizeof(dateBuf))) {
      tft.setTextColor(COL_SUCCESS, COL_BG_BOTTOM);
      tft.printf("Clock: %s  %s", timeBuf, dateBuf);
    }
  } else {
    tft.print("Clock: not synced yet");
  }

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, 166);
  // !wifiEnabled takes priority over everything else here — same reasoning
  // as the status line above, and as LTE Setup's own test-result line.
  if (!wifiEnabled) {
    tft.print("Ping result: WiFi is turned off");
  } else if (strlen(wifiSSID) == 0) {
    tft.print("Set a network name to get started.");
  } else if (!wifiPingAttempted) {
    tft.print("Ping result: (not tested yet)");
  } else if (wifiPingSuccess) {
    tft.setTextColor(COL_SUCCESS, COL_BG_BOTTOM);
    tft.printf("Ping OK - HTTP %d in %lums", wifiPingHttpCode, wifiPingMs);
  } else {
    tft.setTextColor(COL_DANGER, COL_BG_BOTTOM);
    tft.print("Ping failed - no internet reachable");
  }

  drawBackButton("< Back");

  // Shares the bottom bar with "< Back" — the BTN_PROCEED slot every other
  // screen uses for its one primary action was sitting empty here (Connect/
  // Ping already have their own row above). Styled like the Settings
  // screen's own WiFi/4G chips (Screen_11_AdminSettings.ino), same as LTE
  // Setup's on/off toggle (Screen_16_AdminLTE.ino).
  uint16_t toggleFill = wifiEnabled ? COL_ACCENT : COL_CARD;
  drawCardShadow(BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H, 8);
  tft.fillRoundRect(BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H, 8, toggleFill);
  if (!wifiEnabled) tft.drawRoundRect(BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H, 8, COL_CARD_BRD);
  tft.setTextSize(2);
  tft.setTextColor(wifiEnabled ? COL_BG_TOP : COL_TEXT_DIM, toggleFill);
  centerTextInBox(wifiEnabled ? "WiFi: On" : "WiFi: Off", BTN_Y + 9, BTN_PROCEED_X, BTN_PROCEED_W);
}

void handleAdminWiFiScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (pointInRect(sx, sy, WF_SSID_BTN_X, WF_SSID_BTN_Y, WF_BTN_W, WF_BTN_H)) {
        openTextEntry(TE_WIFI_SSID);
        return;
      }
      if (pointInRect(sx, sy, WF_SSID_BTN_X, WF_PASS_BTN_Y, WF_BTN_W, WF_BTN_H)) {
        openTextEntry(TE_WIFI_PASS);
        return;
      }
      if (pointInRect(sx, sy, WF_CONNECT_X, WF_CONNECT_Y, WF_CONNECT_W, WF_CONNECT_H)) {
        // Guarded the same way LTE Setup's Test button guards on lteEnabled
        // — connectWiFiIfNeeded() would already refuse instantly while off,
        // but this skips the pointless disconnect/reconnect cycle too.
        if (wifiEnabled && strlen(wifiSSID) > 0) {
          WiFi.disconnect(true);
          delay(100);
          connectWiFiIfNeeded();
          ntpConfigured = false;   // re-run SNTP against the new connection
          wifiPingAttempted = false;
        }
        drawAdminWiFiScreen();
        return;
      }
      if (pointInRect(sx, sy, WF_PING_X, WF_PING_Y, WF_PING_W, WF_PING_H)) {
        if (wifiEnabled) pingGoogleTest();
        drawAdminWiFiScreen();
        return;
      }
      if (pointInRect(sx, sy, BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H)) {
        setWifiEnabled(!wifiEnabled);  // Core_06_Network.ino
        drawAdminWiFiScreen();
        return;
      }
      if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_SETTINGS;
        drawAdminSettingsScreen();
        return;
      }
    }
  }

  static unsigned long lastWiFiScreenRefresh = 0;
  if (millis() - lastWiFiScreenRefresh > 3000) {
    lastWiFiScreenRefresh = millis();
    drawAdminWiFiScreen();
  }
}
