// =====================================================
//                  ADMIN EDIT SCREEN
// =====================================================
// Motor run time is one value for every product, set in Config.h
// (CFG_MOTOR_RUN_MS), and is deliberately not editable here — see
// runTimeForProduct() in Core_09_Storage.ino.

void drawAdminEditScreen() {
  int i = adminEditingSlot;
  drawGradientBackground();

  char titleBuf[20];
  snprintf(titleBuf, sizeof(titleBuf), "Edit Product %d", i + 1);
  drawScreenTitle(titleBuf);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, 50);
  tft.print("Name:");

  // drawCard() rather than the old fillRect+drawRoundRect pair — a
  // sharp-cornered fill under a rounded border left tiny fill-color corner
  // artifacts poking past the border's curve.
  drawCard(70, 44, 150, 26, 6);
  tft.setTextColor(COL_TEXT, COL_CARD);
  centerTextInBox(products[i].name, 52, 70, 150);

  drawCardShadow(230, 44, 70, 26, 6);
  tft.fillRoundRect(230, 44, 70, 26, 6, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  tft.setCursor(248, 52);
  tft.print("Edit");

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, 90);
  tft.print("Price:");

  drawCardShadow(75, 80, 28, 28, 6);
  tft.fillRoundRect(75, 80, 28, 28, 6, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  tft.setTextSize(2);
  tft.setCursor(85, 87);
  tft.print("-");

  drawCardShadow(163, 80, 28, 28, 6);
  tft.fillRoundRect(163, 80, 28, 28, 6, COL_ACCENT);
  tft.setCursor(173, 87);
  tft.print("+");

  // Rounded to match the radius of the two accent buttons flanking it —
  // was a sharp-cornered fillRect, the one shape on the row that didn't
  // match its neighbours. Read-only, so no shadow (nothing to "raise").
  tft.fillRoundRect(108, 80, 50, 28, 6, COL_CARD);
  tft.setTextColor(COL_TEXT, COL_CARD);
  char pbuf[8];
  snprintf(pbuf, sizeof(pbuf), "Rs%d", products[i].price);
  centerTextInBox(pbuf, 88, 108, 50);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, 130);
  tft.print("Motors:");

  drawCard(70, 116, 150, 28, 6);
  tft.setTextColor(COL_TEXT, COL_CARD);
  char motorBuf[16];  // worst case "1,2,3,4,5" across all MAX_MOTORS
  formatMotorList(products[i].motorMask, motorBuf, sizeof(motorBuf));
  centerTextInBox(motorBuf, 124, 70, 150);

  drawCardShadow(230, 116, 70, 28, 6);
  tft.fillRoundRect(230, 116, 70, 28, 6, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  tft.setCursor(238, 124);
  tft.print("Assign");

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, 163);
  tft.print("Visible:");

  uint16_t toggleColor = productEnabled[i] ? COL_ACCENT : COL_BG_TOP;
  // Flat (no shadow) when disabled, same reasoning as the "no refund" pill
  // and the login keypad's "C" key: a shadow under a background-colored
  // fill reads as a smudge, not a raised control.
  if (productEnabled[i]) drawCardShadow(100, 150, 110, 30, 6);
  tft.fillRoundRect(100, 150, 110, 30, 6, toggleColor);
  tft.drawRoundRect(100, 150, 110, 30, 6, COL_CARD_BRD);
  tft.setTextSize(2);
  tft.setTextColor(productEnabled[i] ? COL_BG_TOP : COL_DANGER, toggleColor);
  centerTextInBox(productEnabled[i] ? "ENABLED" : "DISABLED", 159, 100, 110);

  drawBackButton("< Back");
}

void handleAdminEditScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);
      int i = adminEditingSlot;

      if (pointInRect(sx, sy, 230, 44, 70, 26)) {
        openTextEntry(TE_PRODUCT_NAME);
        return;
      }
      if (pointInRect(sx, sy, 75, 80, 28, 28)) {
        if (products[i].price > 1) products[i].price--;
        savePriceSlot(i);
        drawAdminEditScreen();
        return;
      }
      if (pointInRect(sx, sy, 163, 80, 28, 28)) {
        if (products[i].price < 999) products[i].price++;
        savePriceSlot(i);
        drawAdminEditScreen();
        return;
      }
      if (pointInRect(sx, sy, 230, 116, 70, 28)) {
        currentScreen = SCREEN_ADMIN_MOTOR_ASSIGN;
        drawAdminMotorAssignScreen();
        return;
      }
      if (pointInRect(sx, sy, 100, 150, 110, 30)) {
        productEnabled[i] = !productEnabled[i];
        saveEnabledSlot(i);
        drawAdminEditScreen();
        return;
      }
      if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_PANEL;
        drawAdminPanelScreen();
        return;
      }
    }
  }
}
