// =====================================================
//             ADMIN UPI CONFIGURATION SCREEN
// =====================================================
const int UPI_ROW_H = 20;
const int UPI_ROW_Y0 = 38;
const int UPI_ROW_GAP = 2;

int upiRowY(int r) {
  return UPI_ROW_Y0 + r * (UPI_ROW_H + UPI_ROW_GAP);
}

void drawAdminUPIScreen() {
  drawGradientBackground();

  drawScreenTitle("UPI Configuration");

  struct UPIRowDef {
    const char* label;
    const char* val;
  } rows[7] = {
    {"URL", phonepeBaseUrl},
    {"Prov", phonepeProviderId},
    {"Merch", phonepeMerchantId},
    {"Salt", phonepeSaltKey},
    {"Idx", ""}, // Salt index handled separately
    {"Store", phonepeStoreId},
    {"Term", phonepeTerminalId}
  };

  for (int r = 0; r < 7; r++) {
    int y = upiRowY(r);
    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    tft.setCursor(10, y + 6);
    tft.print(rows[r].label);

    if (r == 4) { // Salt Index Stepper
      drawCardShadow(180, y, 22, 20, 4);
      tft.fillRoundRect(180, y, 22, 20, 4, COL_ACCENT);
      tft.setTextColor(COL_BG_TOP, COL_ACCENT);
      tft.setTextSize(2);
      centerTextInBox("-", y + 3, 180, 22);

      // Rounded to match the flanking -/+ buttons; no shadow, read-only figure.
      tft.fillRoundRect(206, y, 42, 20, 4, COL_CARD);
      tft.setTextColor(COL_TEXT, COL_CARD);
      char idxBuf[6];
      snprintf(idxBuf, sizeof(idxBuf), "%d", phonepeSaltIndex);
      tft.setTextSize(1);
      centerTextInBox(idxBuf, y + 6, 206, 42);

      drawCardShadow(252, y, 22, 20, 4);
      tft.fillRoundRect(252, y, 22, 20, 4, COL_ACCENT);
      tft.setTextColor(COL_BG_TOP, COL_ACCENT);
      tft.setTextSize(2);
      centerTextInBox("+", y + 3, 252, 22);
    } else { // Standard Text Field
      // drawCardShadow()+fillRoundRect rather than the old fillRect — a
      // sharp-cornered fill under the rounded border left corner artifacts.
      drawCardShadow(52, y, 198, 20, 4);
      tft.fillRoundRect(52, y, 198, 20, 4, COL_CARD);
      tft.drawRoundRect(52, y, 198, 20, 4, COL_CARD_BRD);
      tft.setTextColor(COL_TEXT, COL_CARD);
      tft.setCursor(56, y + 6);
      tft.printf("%.26s", rows[r].val);

      drawCardShadow(254, y, 56, 20, 4);
      tft.fillRoundRect(254, y, 56, 20, 4, COL_ACCENT);
      tft.setTextColor(COL_BG_TOP, COL_ACCENT);
      centerTextInBox("Edit", y + 6, 254, 56);
    }
  }

  drawBackButton("< Back");
}

void handleAdminUPIScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_SETTINGS;
        drawAdminSettingsScreen();
        return;
      }

      if (pointInRect(sx, sy, 254, upiRowY(0), 56, 20)) { openTextEntry(TE_UPI_BASE_URL); return; }
      if (pointInRect(sx, sy, 254, upiRowY(1), 56, 20)) { openTextEntry(TE_UPI_PROVIDER_ID); return; }
      if (pointInRect(sx, sy, 254, upiRowY(2), 56, 20)) { openTextEntry(TE_UPI_MERCHANT_ID); return; }
      if (pointInRect(sx, sy, 254, upiRowY(3), 56, 20)) { openTextEntry(TE_UPI_SALT_KEY); return; }

      // Salt Index -
      if (pointInRect(sx, sy, 180, upiRowY(4), 22, 20)) {
        if (phonepeSaltIndex > 1) phonepeSaltIndex--;
        saveUPISettings();
        drawAdminUPIScreen();
        return;
      }
      // Salt Index +
      if (pointInRect(sx, sy, 252, upiRowY(4), 22, 20)) {
        if (phonepeSaltIndex < 99) phonepeSaltIndex++;
        saveUPISettings();
        drawAdminUPIScreen();
        return;
      }

      if (pointInRect(sx, sy, 254, upiRowY(5), 56, 20)) { openTextEntry(TE_UPI_STORE_ID); return; }
      if (pointInRect(sx, sy, 254, upiRowY(6), 56, 20)) { openTextEntry(TE_UPI_TERMINAL_ID); return; }
    }
  }
}
