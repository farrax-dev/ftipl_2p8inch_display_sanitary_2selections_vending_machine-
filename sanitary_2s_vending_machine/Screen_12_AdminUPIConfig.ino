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
//
// Three 44px rows. The two ID rows put a caption over the value (long IDs get
// the full width of the card) with an Edit button at the right; the whole row
// is the tap target. The timeout row is a stepper.
const int UPI_ROW_X = 16, UPI_ROW_W = 288, UPI_ROW_H = 44;
const int UPI_ROW_Y0 = 40, UPI_ROW_GAP = 6;
const int UPI_EDIT_W = 64;
const int UPI_RIGHT = UPI_ROW_X + UPI_ROW_W - 8;
const int UPI_EDIT_X = UPI_RIGHT - UPI_EDIT_W;

int upiRowY(int index) { return UPI_ROW_Y0 + index * (UPI_ROW_H + UPI_ROW_GAP); }
int upiRowCY(int index) { return upiRowY(index) + UPI_ROW_H / 2; }

// Whole minutes only. 1 is the shortest a customer could plausibly still be
// scanning/paying inside; 10 is UPI_TIMEOUT_MAX_MS (Core_06_Network.ino),
// the ceiling every step here respects.
const int UPI_TIMEOUT_MIN_MINUTES = 1;
const int UPI_TIMEOUT_MAX_MINUTES = 10;

void drawUPIFieldRow(int index, const char* caption, const char* value) {
  int y = upiRowY(index);
  auiRow(UPI_ROW_X, y, UPI_ROW_W, UPI_ROW_H);
  auiPut(caption, AL_LEFT, UPI_ROW_X + 12, 150, y + 12, COL_TEXT_DIM, FACE_XS);
  auiPut(strlen(value) > 0 ? value : "(not set)", AL_LEFT, UPI_ROW_X + 12,
         UPI_EDIT_X - 8 - (UPI_ROW_X + 12), y + 30, COL_TEXT, FACE_MD);
  auiRowButton(UPI_EDIT_X, upiRowCY(index), UPI_EDIT_W, "Edit", ABTN_PRIMARY);
}

void drawAdminUPIScreen() {
  drawGradientBackground();
  drawScreenTitle("UPI Configuration");

  drawUPIFieldRow(0, "Merchant ID", phonepeMerchantId);
  drawUPIFieldRow(1, "Store ID", phonepeStoreId);

  int y2 = upiRowY(2), cy2 = upiRowCY(2);
  auiRow(UPI_ROW_X, y2, UPI_ROW_W, UPI_ROW_H);
  auiRowLabel(UPI_ROW_X + 12, cy2, auiStepLeftX(UPI_RIGHT) - UPI_ROW_X - 20, "Timeout (min)");
  char tbuf[8];
  snprintf(tbuf, sizeof(tbuf), "%d", (int)(upiTimeoutMs / 60000));
  auiStepper(UPI_RIGHT, cy2, tbuf, COL_ACCENT);

  auiBarButton(0, 1, "Back", ABTN_SECONDARY);
}

void handleAdminUPIScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (auiBarHit(sx, sy, 0, 1)) {
        currentScreen = SCREEN_ADMIN_SETTINGS;
        drawAdminSettingsScreen();
        return;
      }

      if (pointInRect(sx, sy, UPI_ROW_X, upiRowY(0), UPI_ROW_W, UPI_ROW_H)) {
        openTextEntry(TE_UPI_MERCHANT_ID);
        return;
      }
      if (pointInRect(sx, sy, UPI_ROW_X, upiRowY(1), UPI_ROW_W, UPI_ROW_H)) {
        openTextEntry(TE_UPI_STORE_ID);
        return;
      }

      int step = auiStepTouch(sx, sy, UPI_RIGHT, upiRowCY(2));
      if (step != 0) {
        int mins = (int)(upiTimeoutMs / 60000);
        if (step < 0 && mins > UPI_TIMEOUT_MIN_MINUTES) mins--;
        if (step > 0 && mins < UPI_TIMEOUT_MAX_MINUTES) mins++;
        upiTimeoutMs = (unsigned long)mins * 60000UL;
        saveUPISettings();
        drawAdminUPIScreen();
        return;
      }
    }
  }
}
