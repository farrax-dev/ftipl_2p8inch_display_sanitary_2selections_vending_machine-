// =====================================================
//             ADMIN UPI CONFIGURATION SCREEN
// =====================================================
// Merchant ID is the only PhonePe field visible OR editable here. The rest
// of the account (base URL, provider ID, salt key/index, store and terminal
// ID) is fixed to Config.h and deliberately isn't shown on this screen at
// all, not even read-only — those values live on a machine that sits in a
// public place, so there's no reason to display them there. Change them in
// Config.h and reflash instead. See Config.h's UPI section and
// Core_09_Storage.ino's UPI block for how they're kept out of NVS too.
const int UPI_ROW_X = 20, UPI_ROW_W = 280, UPI_ROW_H = 26;
const int UPI_ROW_Y = 70;
const int UPI_EDIT_W = 60;
const int UPI_FIELD_W = UPI_ROW_W - UPI_EDIT_W - 6;
const int UPI_EDIT_X = UPI_ROW_X + UPI_ROW_W - UPI_EDIT_W;

void drawAdminUPIScreen() {
  drawGradientBackground();
  drawScreenTitle("UPI Configuration");

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT, COL_BG_BOTTOM);
  tft.setCursor(UPI_ROW_X, UPI_ROW_Y - 14);
  tft.print("Merchant ID");

  drawCardShadow(UPI_ROW_X, UPI_ROW_Y, UPI_FIELD_W, UPI_ROW_H, 5);
  tft.fillRoundRect(UPI_ROW_X, UPI_ROW_Y, UPI_FIELD_W, UPI_ROW_H, 5, COL_CARD);
  tft.drawRoundRect(UPI_ROW_X, UPI_ROW_Y, UPI_FIELD_W, UPI_ROW_H, 5, COL_CARD_BRD);
  tft.setTextColor(COL_TEXT, COL_CARD);
  tft.setCursor(UPI_ROW_X + 8, UPI_ROW_Y + 9);
  tft.printf("%.30s", phonepeMerchantId);

  drawCardShadow(UPI_EDIT_X, UPI_ROW_Y, UPI_EDIT_W, UPI_ROW_H, 5);
  tft.fillRoundRect(UPI_EDIT_X, UPI_ROW_Y, UPI_EDIT_W, UPI_ROW_H, 5, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox("Edit", UPI_ROW_Y + 9, UPI_EDIT_X, UPI_EDIT_W);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  wrapTextInBox(
    "Every other PhonePe setting (URL, provider, salt, store/terminal ID) "
    "is fixed in Config.h and not shown here. Edit it there and reflash.",
    UPI_ROW_X, UPI_ROW_W, UPI_ROW_Y + UPI_ROW_H + 16, 1, true);

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

      if (pointInRect(sx, sy, UPI_EDIT_X, UPI_ROW_Y, UPI_EDIT_W, UPI_ROW_H)) {
        openTextEntry(TE_UPI_MERCHANT_ID);
        return;
      }
    }
  }
}
