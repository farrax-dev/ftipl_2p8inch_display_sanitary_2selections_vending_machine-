// =====================================================
//             ADMIN UPI CONFIGURATION SCREEN
// =====================================================
// Merchant ID is the only field here a technician can change on-screen; the
// rest of the PhonePe account (URL, provider ID, salt key/index, store and
// terminal ID) is fixed to Config.h and shown read-only for reference —
// change it there and reflash instead. See Config.h's UPI section and
// Core_09_Storage.ino's UPI block for why.
const int UPI_ROW_H = 20;
const int UPI_ROW_Y0 = 38;
const int UPI_ROW_GAP = 2;
const int UPI_MERCHANT_ROW = 2;

int upiRowY(int r) {
  return UPI_ROW_Y0 + r * (UPI_ROW_H + UPI_ROW_GAP);
}

void drawAdminUPIScreen() {
  drawGradientBackground();

  drawScreenTitle("UPI Configuration");

  char idxBuf[6];
  snprintf(idxBuf, sizeof(idxBuf), "%d", phonepeSaltIndex);

  struct UPIRowDef {
    const char* label;
    const char* val;
  } rows[7] = {
    {"URL", phonepeBaseUrl},
    {"Prov", phonepeProviderId},
    {"Merch", phonepeMerchantId},
    {"Salt", phonepeSaltKey},
    {"Idx", idxBuf},
    {"Store", phonepeStoreId},
    {"Term", phonepeTerminalId}
  };

  for (int r = 0; r < 7; r++) {
    int y = upiRowY(r);
    bool editable = (r == UPI_MERCHANT_ROW);

    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    tft.setCursor(10, y + 6);
    tft.print(rows[r].label);

    // drawCardShadow()+fillRoundRect rather than the old fillRect — a
    // sharp-cornered fill under the rounded border left corner artifacts.
    // The read-only rows take the full width freed by having no Edit
    // button, so a longer value (like the base URL) shows more of itself.
    int fieldW = editable ? 198 : 254;
    drawCardShadow(52, y, fieldW, 20, 4);
    tft.fillRoundRect(52, y, fieldW, 20, 4, COL_CARD);
    tft.drawRoundRect(52, y, fieldW, 20, 4, COL_CARD_BRD);
    tft.setTextColor(editable ? COL_TEXT : COL_TEXT_DIM, COL_CARD);
    tft.setCursor(56, y + 6);
    tft.printf(editable ? "%.26s" : "%.35s", rows[r].val);

    if (editable) {
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

      if (pointInRect(sx, sy, 254, upiRowY(UPI_MERCHANT_ROW), 56, 20)) {
        openTextEntry(TE_UPI_MERCHANT_ID);
        return;
      }
    }
  }
}
