// =====================================================
//               ADMIN PRODUCTS SCREEN
// =====================================================
// One tile per product slot (up to six, two columns by three rows — the same
// tile grid as the Admin Panel). Each tile shows the name, price and motors,
// and a status dot: filled green when the product is on sale, hollow when it
// is hidden from customers. Tap a tile to edit that product
// (Screen_09_AdminEdit.ino, adminEditingSlot).

void drawAdminProductsScreen() {
  drawGradientBackground();
  drawScreenTitle("Products");

  int shown = min(MAX_PRODUCTS, PANEL_TILE_COUNT);
  for (int i = 0; i < shown; i++) {
    int x = panelTileX(i), y = panelTileY(i);
    bool on = productEnabled[i];

    drawCard(x, y, PANEL_TILE_W, PANEL_TILE_H, 8);
    tft.fillRoundRect(x + 8, y + 9, 4, PANEL_TILE_H - 18, 2, on ? COL_ACCENT : COL_TEXT_DIM);

    char motorBuf[16];
    formatMotorList(products[i].motorMask, motorBuf, sizeof(motorBuf));
    char cap[32];
    if (on) snprintf(cap, sizeof(cap), "Rs. %d | M%s", products[i].price, motorBuf);
    else    snprintf(cap, sizeof(cap), "Hidden | Rs. %d", products[i].price);

    auiPut(products[i].name, AL_LEFT, x + 20, PANEL_TILE_W - 44, y + 16,
           on ? COL_TEXT : COL_TEXT_DIM, FACE_MD);
    auiPut(cap, AL_LEFT, x + 20, PANEL_TILE_W - 28, y + 33, COL_TEXT_DIM, FACE_XS);
    auiDot(x + PANEL_TILE_W - 16, y + 16, on);
  }

  auiBarButton(0, 1, "Back", ABTN_SECONDARY);
}

void handleAdminProductsScreen() {
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

      int shown = min(MAX_PRODUCTS, PANEL_TILE_COUNT);
      for (int i = 0; i < shown; i++) {
        if (pointInRect(sx, sy, panelTileX(i), panelTileY(i), PANEL_TILE_W, PANEL_TILE_H)) {
          adminEditingSlot = i;
          currentScreen = SCREEN_ADMIN_EDIT;
          drawAdminEditScreen();
          return;
        }
      }
    }
  }
}
