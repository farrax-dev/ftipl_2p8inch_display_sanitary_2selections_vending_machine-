// =====================================================
//                  ADMIN PANEL SCREEN
// =====================================================
// The hub every admin task starts from. Six tiles, each a section with a live
// one-line summary, so the state of the machine is visible before anything is
// opened:
//
//   Products  - names, prices, motors, visibility      -> Screen_22 / Screen_09
//   Stock     - per-motor quantities, motor test       -> Screen_10
//   Payments  - Free Vend, UPI/Cash/RFID, coins, cards -> Screen_11 and below
//   Network   - WiFi and 4G                            -> Screen_23 / 13 / 16
//   Reports   - sales, e-mail schedule, credentials    -> Screen_17
//   Machine   - max quantity, machine ID, PIN, clock   -> Screen_24 / 20
//
// This used to be a six-line product list with three tiny chips for
// Clock/Report/Setup in the header, and everything else buried under one
// overloaded Settings screen.
const int PANEL_TILE_W = 141, PANEL_TILE_H = 44, PANEL_TILE_GAP = 6;
const int PANEL_TILE_X0 = 16, PANEL_TILE_Y0 = 38;
const int PANEL_TILE_COUNT = 6;

int panelTileX(int k) { return PANEL_TILE_X0 + (k % 2) * (PANEL_TILE_W + PANEL_TILE_GAP); }
int panelTileY(int k) { return PANEL_TILE_Y0 + (k / 2) * (PANEL_TILE_H + PANEL_TILE_GAP); }

// What each tile says about its section right now.
void panelCaption(int k, char* out, size_t n) {
  out[0] = '\0';
  switch (k) {
    case 0: {
      int on = 0;
      for (int i = 0; i < MAX_PRODUCTS; i++) if (productEnabled[i]) on++;
      snprintf(out, n, "%d of %d on sale", on, MAX_PRODUCTS);
      break;
    }
    case 1:
      snprintf(out, n, "%d units left", totalStockOnHand());
      break;
    case 2: {
      if (freeVendMode) { snprintf(out, n, "Free vend"); break; }
      for (int p = 0; p < PAYMENT_COUNT; p++) {
        if (!PAYMENT_AVAILABLE[p] || !paymentEnabled[p]) continue;
        if (out[0]) strncat(out, "/", n - strlen(out) - 1);
        strncat(out, PAYMENT_NAMES[p], n - strlen(out) - 1);
      }
      if (!out[0]) snprintf(out, n, "None enabled");
      break;
    }
    case 3:
      if (wifiEnabled && WiFi.status() == WL_CONNECTED) snprintf(out, n, "WiFi connected");
      else if (lteEnabled && isLTEConnected())          snprintf(out, n, "4G connected");
      else                                              snprintf(out, n, "Offline");
      break;
    case 4:
      snprintf(out, n, "Today Rs.%ld", dayRevenue(findDailySlot(todayDateNum())));
      break;
    default:
      if (strlen(machineId) > 0) snprintf(out, n, "ID %s", machineId);
      else                       snprintf(out, n, "ID, PIN, clock");
      break;
  }
}

void drawAdminPanelScreen() {
  drawGradientBackground();

  drawScreenTitle("Admin Panel");

  const char* titles[PANEL_TILE_COUNT] = {
    "Products", "Stock", "Payments", "Network", "Reports", "Machine"
  };

  for (int k = 0; k < PANEL_TILE_COUNT; k++) {
    int x = panelTileX(k), y = panelTileY(k);
    drawCard(x, y, PANEL_TILE_W, PANEL_TILE_H, 8);
    tft.fillRoundRect(x + 8, y + 9, 4, PANEL_TILE_H - 18, 2, COL_ACCENT);

    char cap[28];
    panelCaption(k, cap, sizeof(cap));
    auiPut(titles[k], AL_LEFT, x + 20, PANEL_TILE_W - 28, y + 16, COL_TEXT, FACE_MD);
    auiPut(cap,       AL_LEFT, x + 20, PANEL_TILE_W - 26, y + 33, COL_TEXT_DIM, FACE_XS);
  }

  auiBarButton(0, 1, "Exit Admin", ABTN_SECONDARY);
}

void handleAdminPanelScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (auiBarHit(sx, sy, 0, 1)) {
        currentScreen = SCREEN_WELCOME;
        drawWelcomeScreen();
        return;
      }

      for (int k = 0; k < PANEL_TILE_COUNT; k++) {
        if (!pointInRect(sx, sy, panelTileX(k), panelTileY(k), PANEL_TILE_W, PANEL_TILE_H)) continue;

        switch (k) {
          case 0:
            currentScreen = SCREEN_ADMIN_PRODUCTS;
            drawAdminProductsScreen();
            break;
          case 1:
            motorStockPage = 0;
            motorEditing = -1;
            currentScreen = SCREEN_ADMIN_MOTOR_STOCK;
            drawAdminMotorStockScreen();
            break;
          case 2:
            currentScreen = SCREEN_ADMIN_SETTINGS;
            drawAdminSettingsScreen();
            break;
          case 3:
            currentScreen = SCREEN_ADMIN_NETWORK;
            drawAdminNetworkScreen();
            break;
          case 4:
            reportPage = 0;
            reportResetArmed = false;
            currentScreen = SCREEN_ADMIN_REPORT;
            drawAdminReportScreen();
            break;
          default:
            currentScreen = SCREEN_ADMIN_MACHINE;
            drawAdminMachineScreen();
            break;
        }
        return;
      }
    }
  }
}
