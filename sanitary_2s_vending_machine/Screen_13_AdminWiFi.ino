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

  bool connected = (WiFi.status() == WL_CONNECTED);

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

  tft.setTextColor(connected ? COL_SUCCESS : COL_DANGER, COL_BG_BOTTOM);
  tft.setCursor(20, 90);
  tft.print(connected ? "Status: Connected" : "Status: Not connected");

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
  if (strlen(wifiSSID) == 0) {
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
        if (strlen(wifiSSID) == 0) return;
        WiFi.disconnect(true);
        delay(100);
        connectWiFiIfNeeded();
        ntpConfigured = false;   // re-run SNTP against the new connection
        wifiPingAttempted = false;
        drawAdminWiFiScreen();
        return;
      }
      if (pointInRect(sx, sy, WF_PING_X, WF_PING_Y, WF_PING_W, WF_PING_H)) {
        pingGoogleTest();
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
