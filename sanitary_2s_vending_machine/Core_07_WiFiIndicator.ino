// =====================================================
//         GLOBAL WIFI SIGNAL INDICATOR
// =====================================================
const int WIFI_ICON_X = 296, WIFI_ICON_Y = 2;
const int WIFI_ICON_W = 24,  WIFI_ICON_H = 16;
const unsigned long WIFI_POLL_INTERVAL_MS = 1500;

// indicatorDirty lives in Core_02_AppState.ino — see the comment there for
// why (Core_06_Network.ino's setWifiEnabled() needs to set it too, and loads
// before this tab in the concatenated build).
unsigned long lastWiFiPoll = 0;
int  lastDrawnBars = -99;
bool lastDrawnConnected = false;
bool lastDrawnViaLTE = false;  // which glyph (WiFi fan vs cellular bars) was drawn last

int wifiBarsFromRSSI(long rssi) {
  if (rssi >= -55) return 4;
  if (rssi >= -65) return 3;
  if (rssi >= -75) return 2;
  if (rssi >= -85) return 1;
  return 0;
}

const char* wifiQualityLabel(int bars) {
  switch (bars) {
    case 4: return "Excellent";
    case 3: return "Good";
    case 2: return "Fair";
    case 1: return "Weak";
    default: return "Very weak";
  }
}

// One arc of the WiFi fan: a 110-degree sweep centred on straight up.
void drawWiFiArc(int cx, int cyApex, int r, int thickness, uint16_t color) {
  for (int a = -55; a <= 55; a++) {
    float rad = a * 0.0174533f;
    float s = sin(rad), c = cos(rad);
    for (int t = 0; t < thickness; t++) {
      int px = cx + (int)lround(s * (r - t));
      int py = cyApex - (int)lround(c * (r - t));
      tft.drawPixel(px, py, color);
    }
  }
}

// Classic WiFi glyph: a dot at the apex plus three arcs above it.
// bars 1 lights the dot, 2/3/4 light each further arc. scale 1 = status-bar
// size (about 20x11 px), scale 2 = the larger version on the WiFi Setup screen.
void drawWiFiSymbol(int cx, int cyApex, int scale, int bars, bool connected) {
  uint16_t onColor  = (bars >= 3) ? COL_SUCCESS : (bars == 2) ? COL_WARNING : COL_DANGER;
  uint16_t offColor = COL_TEXT_DIM;
  int radii[3] = { 4 * scale, 7 * scale, 10 * scale };
  int thickness = scale + 1;

  tft.fillCircle(cx, cyApex - scale, scale,
                 (connected && bars >= 1) ? onColor : offColor);

  for (int k = 0; k < 3; k++) {
    bool lit = connected && (bars >= k + 2);
    drawWiFiArc(cx, cyApex, radii[k], thickness, lit ? onColor : offColor);
  }

  if (!connected) {
    tft.drawLine(cx - 5 * scale, cyApex - scale,
                 cx + 6 * scale, cyApex - 11 * scale, COL_DANGER);
    if (scale > 1) {
      tft.drawLine(cx - 5 * scale + 1, cyApex - scale,
                   cx + 6 * scale + 1, cyApex - 11 * scale, COL_DANGER);
    }
  }
}

// Shows the WiFi fan glyph whenever WiFi is connected; otherwise, if the
// 4G/LTE modem (Core_13_LTEModem.ino) is carrying the connection instead,
// shows cellular signal bars in the same spot so the icon always reflects
// whichever path is actually online.
void drawWiFiIndicator() {
  bool wifiConnected = (WiFi.status() == WL_CONNECTED);

  if (!wifiConnected && isLTEConnected()) {
    drawLTEIndicator();
    return;
  }

  int bars = wifiConnected ? wifiBarsFromRSSI(WiFi.RSSI()) : 0;
  tft.fillRect(WIFI_ICON_X, WIFI_ICON_Y, WIFI_ICON_W, WIFI_ICON_H, COL_BG_TOP);
  drawWiFiSymbol(WIFI_ICON_X + WIFI_ICON_W / 2,
                 WIFI_ICON_Y + WIFI_ICON_H - 1, 1, bars, wifiConnected);
}

void maintainWiFiIndicator() {
  if (!indicatorDirty && millis() - lastWiFiPoll < WIFI_POLL_INTERVAL_MS) return;
  lastWiFiPoll = millis();

  bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  bool viaLTE = !wifiConnected && isLTEConnected();

  int bars;
  bool connected;
  if (viaLTE) {
    // Cached — see maintainLTEStatus() in Core_13_LTEModem.ino. This used to
    // be a live AT+CSQ, called twice per refresh.
    bars = lteSignalBars();
    connected = true;
  } else {
    bars = wifiConnected ? wifiBarsFromRSSI(WiFi.RSSI()) : -1;
    connected = wifiConnected;
  }

  if (indicatorDirty || viaLTE != lastDrawnViaLTE || bars != lastDrawnBars || connected != lastDrawnConnected) {
    drawWiFiIndicator();
    lastDrawnViaLTE = viaLTE;
    lastDrawnBars = bars;
    lastDrawnConnected = connected;
  }
  indicatorDirty = false;
}
