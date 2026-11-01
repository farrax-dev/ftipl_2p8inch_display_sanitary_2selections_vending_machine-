// =====================================================
//               ADMIN NETWORK SCREEN
// =====================================================
// A summary card per connection (WiFi, 4G) so the state of both is visible at
// once, and one tap away from its own setup screen. Reached from the Network
// tile on the Admin Panel; WiFi Setup (Screen_13) and 4G/LTE Setup (Screen_16)
// both come back here.
//
// A card whose radio isn't fitted at all (Config.h's CFG_WIFI_ENABLED /
// CFG_LTE_ENABLED) is shown greyed with "Not fitted" and doesn't navigate.
// A card whose radio is fitted but switched off IS still tappable — that
// setup screen is where it gets switched back on, and a technician must never
// be locked out of it by having used it to turn the radio off.
const int NET_CARD_X = 16, NET_CARD_W = 288, NET_CARD_H = 68;
const int NET_CARD_Y0 = 40, NET_CARD_GAP = 6;
const int NET_BTN_W = 70;

int netCardY(int k) { return NET_CARD_Y0 + k * (NET_CARD_H + NET_CARD_GAP); }

// k: 0 = WiFi, 1 = 4G
void drawNetCard(int k) {
  int x = NET_CARD_X, y = netCardY(k), w = NET_CARD_W, h = NET_CARD_H;
  bool fitted = (k == 0) ? CFG_WIFI_ENABLED : CFG_LTE_ENABLED;
  bool radioOn = (k == 0) ? wifiEnabled : lteEnabled;
  bool connected = fitted && radioOn && ((k == 0) ? (WiFi.status() == WL_CONNECTED) : isLTEConnected());

  drawCard(x, y, w, h, 8);
  tft.fillRoundRect(x + 8, y + 9, 4, h - 18, 2, connected ? COL_TILE_STRIPE_OK : COL_TILE_STRIPE_OFF);

  auiPut(k == 0 ? TXT_WIFI : TXT_4G_LTE, AL_LEFT, x + 22, 120, y + 17,
         fitted ? COL_CARD_TITLE : COL_CARD_CAPTION, FACE_MD);

  const char* status;
  uint16_t statusCol;
  if (!fitted)        { status = "Not fitted";    statusCol = COL_CARD_CAPTION; }
  else if (!radioOn)  { status = TXT_TURNED_OFF;    statusCol = COL_CARD_CAPTION; }
  else if (connected) { status = TXT_CONNECTED;     statusCol = COL_OK_TEXT; }
  else                { status = TXT_NOT_CONNECTED; statusCol = COL_ERR_TEXT; }
  auiPut(status, AL_RIGHT, x + 130, w - 130 - 14, y + 17, statusCol, FACE_SM);

  char l1[48], l2[48];
  if (k == 0) {
    snprintf(l1, sizeof(l1), "Network: %s", strlen(wifiSSID) > 0 ? wifiSSID : TXT_NOT_SET);
    if (connected) {
      long rssi = WiFi.RSSI();
      snprintf(l2, sizeof(l2), "Signal: %s (%ld dBm)", wifiQualityLabel(wifiBarsFromRSSI(rssi)), rssi);
    } else {
      snprintf(l2, sizeof(l2), TXT_SIGNAL);
    }
  } else {
    snprintf(l1, sizeof(l1), "APN: %s", LTE_APN);
    if (connected) snprintf(l2, sizeof(l2), "Signal: %s", wifiQualityLabel(lteSignalBars()));
    else           snprintf(l2, sizeof(l2), TXT_BACKUP_LINK_WHEN_WIFI_IS_DOWN);
  }
  int textW = w - 22 - NET_BTN_W - 24;
  auiPut(l1, AL_LEFT, x + 22, textW, y + 41, COL_CARD_TITLE, FACE_XS);
  auiPut(l2, AL_LEFT, x + 22, textW, y + 57, COL_CARD_CAPTION, FACE_XS);

  auiRowButton(x + w - 10 - NET_BTN_W, y + 48, NET_BTN_W, TXT_SETUP, fitted ? ABTN_PRIMARY : ABTN_OFF);
}

void drawAdminNetworkScreen() {
  drawGradientBackground();
  drawScreenTitle(TXT_NETWORK);

  drawNetCard(0);
  drawNetCard(1);

  auiBarButton(0, 1, TXT_BACK, ABTN_SECONDARY);
}

void handleAdminNetworkScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (auiBarHit(sx, sy, 0, 1)) {
        currentScreen = SCREEN_ADMIN_PANEL;
        drawAdminPanelScreen();
        return;
      }

      if (CFG_WIFI_ENABLED && pointInRect(sx, sy, NET_CARD_X, netCardY(0), NET_CARD_W, NET_CARD_H)) {
        currentScreen = SCREEN_ADMIN_WIFI;
        drawAdminWiFiScreen();
        return;
      }

      if (CFG_LTE_ENABLED && pointInRect(sx, sy, NET_CARD_X, netCardY(1), NET_CARD_W, NET_CARD_H)) {
        currentScreen = SCREEN_ADMIN_LTE;
        drawAdminLTEScreen();
        return;
      }
    }
  }
}
