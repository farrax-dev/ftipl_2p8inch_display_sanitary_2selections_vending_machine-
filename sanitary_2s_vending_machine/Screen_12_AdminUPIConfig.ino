// =====================================================
//             ADMIN UPI CONFIGURATION SCREEN
// =====================================================
// Merchant ID and Store ID are the only PhonePe account fields visible OR
// editable here, plus the payment timeout (a machine-side setting, not a
// PhonePe field). The rest of the account (base URL, provider ID, salt
// key/index, terminal ID) is fixed to Config.h and deliberately isn't shown
// on this screen at all, not even read-only — those values live on a
// machine that sits in a public place, so there's no reason to display them
// there. Change them in Config.h and reflash instead. See Config.h's UPI
// section and Core_09_Storage.ino's UPI block for how they're kept out of
// NVS too.
const int UPI_ROW_X = 20, UPI_ROW_W = 280, UPI_ROW_H = 24;
const int UPI_ROW_Y0 = 48, UPI_ROW_GAP = 40;
const int UPI_EDIT_W = 60;
const int UPI_FIELD_W = UPI_ROW_W - UPI_EDIT_W - 6;
const int UPI_EDIT_X = UPI_ROW_X + UPI_ROW_W - UPI_EDIT_W;

int upiRowY(int index) { return UPI_ROW_Y0 + index * UPI_ROW_GAP; }

// Timeout stepper geometry — same right-anchored -/value/+ layout as the
// Admin > Date & Time screen's fields (Screen_20_AdminDateTime.ino).
const int UPI_TO_BTN_W = 30, UPI_TO_VAL_W = 60;
const int UPI_TO_PLUS_X  = UPI_ROW_X + UPI_ROW_W - 8 - UPI_TO_BTN_W;
const int UPI_TO_VAL_X   = UPI_TO_PLUS_X - 6 - UPI_TO_VAL_W;
const int UPI_TO_MINUS_X = UPI_TO_VAL_X - 6 - UPI_TO_BTN_W;

// Whole minutes only. 1 is the shortest a customer could plausibly still be
// scanning/paying inside; 10 is UPI_TIMEOUT_MAX_MS (Core_06_Network.ino),
// the ceiling every step here respects.
const int UPI_TIMEOUT_MIN_MINUTES = 1;
const int UPI_TIMEOUT_MAX_MINUTES = 10;

void drawAdminUPIScreen() {
  drawGradientBackground();
  drawScreenTitle("UPI Configuration");

  // ---- Merchant ID (tap Edit to change) ----
  int y0 = upiRowY(0);
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT, COL_BG_BOTTOM);
  tft.setCursor(UPI_ROW_X, y0 - 12);
  tft.print("Merchant ID");

  drawCardShadow(UPI_ROW_X, y0, UPI_FIELD_W, UPI_ROW_H, 5);
  tft.fillRoundRect(UPI_ROW_X, y0, UPI_FIELD_W, UPI_ROW_H, 5, COL_CARD);
  tft.drawRoundRect(UPI_ROW_X, y0, UPI_FIELD_W, UPI_ROW_H, 5, COL_CARD_BRD);
  tft.setTextColor(COL_TEXT, COL_CARD);
  tft.setCursor(UPI_ROW_X + 8, y0 + 8);
  tft.printf("%.30s", phonepeMerchantId);

  drawCardShadow(UPI_EDIT_X, y0, UPI_EDIT_W, UPI_ROW_H, 5);
  tft.fillRoundRect(UPI_EDIT_X, y0, UPI_EDIT_W, UPI_ROW_H, 5, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox("Edit", y0 + 8, UPI_EDIT_X, UPI_EDIT_W);

  // ---- Store ID (tap Edit to change) ----
  int y1 = upiRowY(1);
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT, COL_BG_BOTTOM);
  tft.setCursor(UPI_ROW_X, y1 - 12);
  tft.print("Store ID");

  drawCardShadow(UPI_ROW_X, y1, UPI_FIELD_W, UPI_ROW_H, 5);
  tft.fillRoundRect(UPI_ROW_X, y1, UPI_FIELD_W, UPI_ROW_H, 5, COL_CARD);
  tft.drawRoundRect(UPI_ROW_X, y1, UPI_FIELD_W, UPI_ROW_H, 5, COL_CARD_BRD);
  tft.setTextColor(COL_TEXT, COL_CARD);
  tft.setCursor(UPI_ROW_X + 8, y1 + 8);
  tft.printf("%.30s", phonepeStoreId);

  drawCardShadow(UPI_EDIT_X, y1, UPI_EDIT_W, UPI_ROW_H, 5);
  tft.fillRoundRect(UPI_EDIT_X, y1, UPI_EDIT_W, UPI_ROW_H, 5, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox("Edit", y1 + 8, UPI_EDIT_X, UPI_EDIT_W);

  // ---- Payment timeout, whole minutes (stepper) ----
  int y2 = upiRowY(2);
  drawCard(UPI_ROW_X, y2, UPI_ROW_W, UPI_ROW_H, 5);
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT, COL_CARD);
  tft.setCursor(UPI_ROW_X + 8, y2 + 8);
  tft.print("Timeout (min)");

  drawCardShadow(UPI_TO_MINUS_X, y2, UPI_TO_BTN_W, UPI_ROW_H, 4);
  tft.fillRoundRect(UPI_TO_MINUS_X, y2, UPI_TO_BTN_W, UPI_ROW_H, 4, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  tft.setTextSize(2);
  centerTextInBox("-", y2 + 4, UPI_TO_MINUS_X, UPI_TO_BTN_W);

  // Rounded to match the flanking -/+ buttons; no shadow, read-only figure.
  tft.fillRoundRect(UPI_TO_VAL_X, y2, UPI_TO_VAL_W, UPI_ROW_H, 4, COL_BG_TOP);
  tft.setTextColor(COL_ACCENT, COL_BG_TOP);
  char tbuf[8];
  snprintf(tbuf, sizeof(tbuf), "%d", (int)(upiTimeoutMs / 60000));
  centerTextInBox(tbuf, y2 + 4, UPI_TO_VAL_X, UPI_TO_VAL_W);

  drawCardShadow(UPI_TO_PLUS_X, y2, UPI_TO_BTN_W, UPI_ROW_H, 4);
  tft.fillRoundRect(UPI_TO_PLUS_X, y2, UPI_TO_BTN_W, UPI_ROW_H, 4, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox("+", y2 + 4, UPI_TO_PLUS_X, UPI_TO_BTN_W);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  wrapTextInBox(
    "Base URL, provider, salt and terminal ID are fixed in Config.h "
    "and not shown here. Edit it there and reflash.",
    UPI_ROW_X, UPI_ROW_W, y2 + UPI_ROW_H + 10, 1, true);

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

      int y0 = upiRowY(0);
      if (pointInRect(sx, sy, UPI_EDIT_X, y0, UPI_EDIT_W, UPI_ROW_H)) {
        openTextEntry(TE_UPI_MERCHANT_ID);
        return;
      }

      int y1 = upiRowY(1);
      if (pointInRect(sx, sy, UPI_EDIT_X, y1, UPI_EDIT_W, UPI_ROW_H)) {
        openTextEntry(TE_UPI_STORE_ID);
        return;
      }

      int y2 = upiRowY(2);
      if (pointInRect(sx, sy, UPI_TO_MINUS_X, y2, UPI_TO_BTN_W, UPI_ROW_H)) {
        int mins = (int)(upiTimeoutMs / 60000);
        if (mins > UPI_TIMEOUT_MIN_MINUTES) mins--;
        upiTimeoutMs = (unsigned long)mins * 60000UL;
        saveUPISettings();
        drawAdminUPIScreen();
        return;
      }
      if (pointInRect(sx, sy, UPI_TO_PLUS_X, y2, UPI_TO_BTN_W, UPI_ROW_H)) {
        int mins = (int)(upiTimeoutMs / 60000);
        if (mins < UPI_TIMEOUT_MAX_MINUTES) mins++;
        upiTimeoutMs = (unsigned long)mins * 60000UL;
        saveUPISettings();
        drawAdminUPIScreen();
        return;
      }
    }
  }
}
