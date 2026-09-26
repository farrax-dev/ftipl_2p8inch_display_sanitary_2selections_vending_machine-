// =====================================================
//           ADMIN SETTINGS SCREEN
// =====================================================
// Rows tightened from 22h/3gap to 20h/2gap so up to three payment toggles end
// at y=164 instead of 175, leaving room for the Machine ID row above the
// button bar at BTN_Y=190.
const int PAY_ROW_X = 20, PAY_ROW_W = 280, PAY_ROW_H = 20;
const int PAY_ROW_Y0 = 78, PAY_ROW_GAP = 2;

int paymentRowY(int row) {
  return PAY_ROW_Y0 + row * (PAY_ROW_H + PAY_ROW_GAP);
}

// Which payment rows this build even has to draw — Config.h's
// CFG_PAYMENT_*_AVAILABLE (PAYMENT_AVAILABLE[], Core_09_Storage.ino) decides
// this once, so a build with UPI/Cash left out never shows a row for them at
// all, and the freed vertical space is what lets the RFID-only "Show
// Prices" row below fit without redesigning the whole screen.
int visiblePaymentRows(int outIndices[]) {
  int count = 0;
  for (int p = 0; p < PAYMENT_COUNT; p++) {
    if (PAYMENT_AVAILABLE[p]) outIndices[count++] = p;
  }
  return count;
}

// Machine ID + Admin PIN share this row, split into two tap targets — the
// screen is already full down to BTN_Y=190, with no vertical room left for
// a row each. MID_ROW_Y/H stays the shared geometry both halves use; the
// showPriceToggleRow check below still measures against MID_ROW_Y as its
// "is there room above the button bar" bound.
const int MID_ROW_Y = 167, MID_ROW_H = 20;
const int MACHID_ROW_X = 20,  MACHID_ROW_W = 168;
const int ADMINPIN_ROW_X = 196, ADMINPIN_ROW_W = 104;  // 8px gap from MACHID_ROW

// Small chips inside the RFID row, left of its ON/OFF toggle — "Cards" opens
// card registration, "Reset" opens the automatic monthly usage-reset
// schedule (Screen_21_AdminRFIDReset.ino). Only drawn/hit-tested for that one
// row.
const int RFID_CARDS_CHIP_W = 46, RFID_CARDS_CHIP_H = 16;
const int RFID_RESET_CHIP_W = 46;

// Custom 5-button layout at bottom of Settings screen
const int SET_BTN_BACK_X = 8,   SET_BTN_BACK_W = 57;
const int SET_BTN_WIFI_X = 69,  SET_BTN_WIFI_W = 57;
const int SET_BTN_UPI_X  = 130, SET_BTN_UPI_W  = 57;
const int SET_BTN_COIN_X = 191, SET_BTN_COIN_W = 57;
const int SET_BTN_LTE_X  = 252, SET_BTN_LTE_W  = 57;

void drawAdminSettingsScreen() {
  drawGradientBackground();

  drawScreenTitle("Settings");

  // Max cart quantity
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_BG_BOTTOM);
  tft.setCursor(20, 54);
  tft.print("Max Qty:");

  drawCardShadow(150, 46, 28, 28, 6);
  tft.fillRoundRect(150, 46, 28, 28, 6, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  tft.setTextSize(2);
  tft.setCursor(160, 53);
  tft.print("-");

  drawCardShadow(238, 46, 28, 28, 6);
  tft.fillRoundRect(238, 46, 28, 28, 6, COL_ACCENT);
  tft.setCursor(248, 53);
  tft.print("+");

  // Rounded to match the flanking -/+ buttons; no shadow, read-only figure.
  tft.fillRoundRect(183, 46, 50, 28, 6, COL_CARD);
  tft.setTextColor(COL_ACCENT, COL_CARD);
  tft.setTextSize(3);
  char qbuf[8];
  snprintf(qbuf, sizeof(qbuf), "%d", maxCartQty);
  centerTextInBox(qbuf, 48, 183, 50);

  // Free Vend toggle — always the first payment-related row, row 0. When
  // on, the machine never shows a payment method to the customer at all
  // (Screen_02_Select.ino / Screen_03_CartReview.ino skip straight to
  // Core_11_Dispense.ino's runFreeVendCheckout() instead of ever reaching
  // SCREEN_PAYMENT_METHOD), so the individual payment method toggles below
  // would have nothing left to control and are hidden rather than left on
  // screen doing nothing.
  int freeVendY = paymentRowY(0);
  drawCard(PAY_ROW_X, freeVendY, PAY_ROW_W, PAY_ROW_H, 5);
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT, COL_CARD);
  tft.setCursor(PAY_ROW_X + 8, freeVendY + 7);
  tft.print("Free Vend");

  {
    int toggleW = 50, toggleH = 16;
    int toggleX = PAY_ROW_X + PAY_ROW_W - toggleW - 6;
    int toggleY = freeVendY + (PAY_ROW_H - toggleH) / 2;
    uint16_t toggleColor = freeVendMode ? COL_ACCENT : COL_BG_TOP;
    tft.fillRoundRect(toggleX, toggleY, toggleW, toggleH, 4, toggleColor);
    tft.setTextColor(freeVendMode ? COL_BG_TOP : COL_TEXT_DIM, toggleColor);
    centerTextInBox(freeVendMode ? "ON" : "OFF", toggleY + 4, toggleX, toggleW);
  }

  // Payment method toggles — only the ones Config.h makes available, and
  // only while Free Vend is off (see the comment above).
  int payIdx[PAYMENT_COUNT];
  int payCount = 0;
  if (!freeVendMode) {
    payCount = visiblePaymentRows(payIdx);

    for (int row = 0; row < payCount; row++) {
      int p = payIdx[row];
      int y = paymentRowY(row + 1);
      drawCard(PAY_ROW_X, y, PAY_ROW_W, PAY_ROW_H, 5);

      tft.setTextSize(1);
      tft.setTextColor(COL_TEXT, COL_CARD);
      tft.setCursor(PAY_ROW_X + 8, y + 7);
      tft.print(PAYMENT_NAMES[p]);

      int toggleW = 50, toggleH = 16;
      int toggleX = PAY_ROW_X + PAY_ROW_W - toggleW - 6;
      int toggleY = y + (PAY_ROW_H - toggleH) / 2;

      // RFID gets "Cards" (registration) and "Reset" (automatic monthly usage
      // reset) shortcuts, sitting just left of its own toggle — there's nowhere
      // else on this packed screen to put extra nav buttons, and they only
      // need to exist on this one row.
      if (p == PAY_IDX_RFID) {
        int cardsChipX = toggleX - RFID_CARDS_CHIP_W - 6;
        int chipY = y + (PAY_ROW_H - RFID_CARDS_CHIP_H) / 2;
        tft.fillRoundRect(cardsChipX, chipY, RFID_CARDS_CHIP_W, RFID_CARDS_CHIP_H, 4, COL_ACCENT);
        tft.setTextColor(COL_BG_TOP, COL_ACCENT);
        centerTextInBox("Cards", chipY + 4, cardsChipX, RFID_CARDS_CHIP_W);

        int resetChipX = cardsChipX - RFID_RESET_CHIP_W - 6;
        tft.fillRoundRect(resetChipX, chipY, RFID_RESET_CHIP_W, RFID_CARDS_CHIP_H, 4, COL_ACCENT);
        tft.setTextColor(COL_BG_TOP, COL_ACCENT);
        centerTextInBox("Reset", chipY + 4, resetChipX, RFID_RESET_CHIP_W);
      }

      uint16_t toggleColor = paymentEnabled[p] ? COL_ACCENT : COL_BG_TOP;
      tft.fillRoundRect(toggleX, toggleY, toggleW, toggleH, 4, toggleColor);
      tft.setTextColor(paymentEnabled[p] ? COL_BG_TOP : COL_TEXT_DIM, toggleColor);
      centerTextInBox(paymentEnabled[p] ? "ON" : "OFF", toggleY + 4, toggleX, toggleW);
    }
  }

  int settingsNextRow = 1 + payCount;

  // "Show Prices" matters whenever a customer never actually pays anything —
  // machine-wide Free Vend, or the older case of RFID being the only thing a
  // customer can pay with (free-vend-by-badge doesn't need a price on screen
  // either). Guarded on room too, so a build with every payment row showing
  // never overflows into the Machine ID row below.
  bool showPriceToggleRow = (freeVendMode || isRFIDOnlyMachine()) &&
                            (paymentRowY(settingsNextRow) + PAY_ROW_H + PAY_ROW_GAP <= MID_ROW_Y);
  if (showPriceToggleRow) {
    int y = paymentRowY(settingsNextRow);
    drawCard(PAY_ROW_X, y, PAY_ROW_W, PAY_ROW_H, 5);
    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT, COL_CARD);
    tft.setCursor(PAY_ROW_X + 8, y + 7);
    tft.print("Show Prices");

    int toggleW = 50, toggleH = 16;
    int toggleX = PAY_ROW_X + PAY_ROW_W - toggleW - 6;
    int toggleY = y + (PAY_ROW_H - toggleH) / 2;
    uint16_t toggleColor = !hideProductPrices ? COL_ACCENT : COL_BG_TOP;
    tft.fillRoundRect(toggleX, toggleY, toggleW, toggleH, 4, toggleColor);
    tft.setTextColor(!hideProductPrices ? COL_BG_TOP : COL_TEXT_DIM, toggleColor);
    centerTextInBox(!hideProductPrices ? "ON" : "OFF", toggleY + 4, toggleX, toggleW);
  }

  // Machine ID (tap to edit)
  drawCard(MACHID_ROW_X, MID_ROW_Y, MACHID_ROW_W, MID_ROW_H, 5);
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT, COL_CARD);
  tft.setCursor(MACHID_ROW_X + 8, MID_ROW_Y + 6);
  tft.print("Machine ID");
  tft.setTextColor(COL_ACCENT, COL_CARD);
  rightText(strlen(machineId) > 0 ? machineId : "(not set)",
            MACHID_ROW_X + MACHID_ROW_W - 8, MID_ROW_Y + 6);

  // Admin PIN (tap to change) — unlike Machine ID, the value itself is never
  // shown here, only a fixed placeholder: it's a credential, not something
  // to glance at.
  drawCard(ADMINPIN_ROW_X, MID_ROW_Y, ADMINPIN_ROW_W, MID_ROW_H, 5);
  tft.setTextColor(COL_TEXT, COL_CARD);
  tft.setCursor(ADMINPIN_ROW_X + 6, MID_ROW_Y + 6);
  tft.print("PIN");
  tft.setTextColor(COL_ACCENT, COL_CARD);
  rightText("****", ADMINPIN_ROW_X + ADMINPIN_ROW_W - 6, MID_ROW_Y + 6);

  // Bottom action bar. WiFi/4G grey out and stop navigating only when their
  // hardware isn't fitted at all (CFG_WIFI_ENABLED / CFG_LTE_ENABLED) — never
  // when the admin has merely switched the radio off from its own setup
  // screen. Gating this on the runtime lteEnabled flag instead would lock a
  // technician out of the one screen (4G/LTE Setup) that can turn it back on
  // the moment they used it to turn LTE off.
  drawCard(SET_BTN_BACK_X, BTN_Y, SET_BTN_BACK_W, BTN_H, 8);
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_CARD);
  centerTextInBox("Back", BTN_Y + 9, SET_BTN_BACK_X, SET_BTN_BACK_W);

  uint16_t wifiFill = CFG_WIFI_ENABLED ? COL_ACCENT : COL_CARD;
  drawCardShadow(SET_BTN_WIFI_X, BTN_Y, SET_BTN_WIFI_W, BTN_H, 8);
  tft.fillRoundRect(SET_BTN_WIFI_X, BTN_Y, SET_BTN_WIFI_W, BTN_H, 8, wifiFill);
  if (!CFG_WIFI_ENABLED) tft.drawRoundRect(SET_BTN_WIFI_X, BTN_Y, SET_BTN_WIFI_W, BTN_H, 8, COL_CARD_BRD);
  tft.setTextColor(CFG_WIFI_ENABLED ? COL_BG_TOP : COL_TEXT_DIM, wifiFill);
  centerTextInBox("WiFi", BTN_Y + 9, SET_BTN_WIFI_X, SET_BTN_WIFI_W);

  drawCardShadow(SET_BTN_UPI_X, BTN_Y, SET_BTN_UPI_W, BTN_H, 8);
  tft.fillRoundRect(SET_BTN_UPI_X, BTN_Y, SET_BTN_UPI_W, BTN_H, 8, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox("UPI", BTN_Y + 9, SET_BTN_UPI_X, SET_BTN_UPI_W);

  drawCardShadow(SET_BTN_COIN_X, BTN_Y, SET_BTN_COIN_W, BTN_H, 8);
  tft.fillRoundRect(SET_BTN_COIN_X, BTN_Y, SET_BTN_COIN_W, BTN_H, 8, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox("Coin", BTN_Y + 9, SET_BTN_COIN_X, SET_BTN_COIN_W);

  uint16_t lteFill = CFG_LTE_ENABLED ? COL_ACCENT : COL_CARD;
  drawCardShadow(SET_BTN_LTE_X, BTN_Y, SET_BTN_LTE_W, BTN_H, 8);
  tft.fillRoundRect(SET_BTN_LTE_X, BTN_Y, SET_BTN_LTE_W, BTN_H, 8, lteFill);
  if (!CFG_LTE_ENABLED) tft.drawRoundRect(SET_BTN_LTE_X, BTN_Y, SET_BTN_LTE_W, BTN_H, 8, COL_CARD_BRD);
  tft.setTextColor(CFG_LTE_ENABLED ? COL_BG_TOP : COL_TEXT_DIM, lteFill);
  centerTextInBox("4G", BTN_Y + 9, SET_BTN_LTE_X, SET_BTN_LTE_W);
}

void handleAdminSettingsScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (pointInRect(sx, sy, 150, 46, 28, 28)) {
        if (maxCartQty > 1) maxCartQty--;
        saveMaxCartQty();
        drawAdminSettingsScreen();
        return;
      }
      if (pointInRect(sx, sy, 238, 46, 28, 28)) {
        // Capped at Config.h's seed, not an arbitrary 99 — CFG_MAX_CART_QTY
        // is the ceiling a technician flashed for this machine, and the
        // admin screen shouldn't be able to exceed what was commissioned.
        if (maxCartQty < CFG_MAX_CART_QTY) maxCartQty++;
        saveMaxCartQty();
        drawAdminSettingsScreen();
        return;
      }

      int freeVendY = paymentRowY(0);
      if (pointInRect(sx, sy, PAY_ROW_X, freeVendY, PAY_ROW_W, PAY_ROW_H)) {
        freeVendMode = !freeVendMode;
        saveFreeVendMode();
        drawAdminSettingsScreen();
        return;
      }

      int payIdx[PAYMENT_COUNT];
      int payCount = 0;
      if (!freeVendMode) {
        payCount = visiblePaymentRows(payIdx);

        for (int row = 0; row < payCount; row++) {
          int p = payIdx[row];
          int y = paymentRowY(row + 1);

          if (p == PAY_IDX_RFID) {
            int toggleW = 50;
            int toggleX = PAY_ROW_X + PAY_ROW_W - toggleW - 6;
            int cardsChipX = toggleX - RFID_CARDS_CHIP_W - 6;
            int chipY = y + (PAY_ROW_H - RFID_CARDS_CHIP_H) / 2;
            if (pointInRect(sx, sy, cardsChipX, chipY, RFID_CARDS_CHIP_W, RFID_CARDS_CHIP_H)) {
              currentScreen = SCREEN_ADMIN_RFID_CARDS;
              rfidCardsPage = 0;
              drawAdminRFIDCardsScreen();
              return;
            }

            int resetChipX = cardsChipX - RFID_RESET_CHIP_W - 6;
            if (pointInRect(sx, sy, resetChipX, chipY, RFID_RESET_CHIP_W, RFID_CARDS_CHIP_H)) {
              currentScreen = SCREEN_ADMIN_RFID_RESET;
              drawAdminRFIDResetScreen();
              return;
            }
          }

          if (pointInRect(sx, sy, PAY_ROW_X, y, PAY_ROW_W, PAY_ROW_H)) {
            paymentEnabled[p] = !paymentEnabled[p];
            savePaymentEnabled(p);
            drawAdminSettingsScreen();
            return;
          }
        }
      }

      int settingsNextRow = 1 + payCount;
      bool showPriceToggleRow = (freeVendMode || isRFIDOnlyMachine()) &&
                                (paymentRowY(settingsNextRow) + PAY_ROW_H + PAY_ROW_GAP <= MID_ROW_Y);
      if (showPriceToggleRow && pointInRect(sx, sy, PAY_ROW_X, paymentRowY(settingsNextRow), PAY_ROW_W, PAY_ROW_H)) {
        hideProductPrices = !hideProductPrices;
        saveHideProductPrices();
        drawAdminSettingsScreen();
        return;
      }

      if (pointInRect(sx, sy, MACHID_ROW_X, MID_ROW_Y, MACHID_ROW_W, MID_ROW_H)) {
        openTextEntry(TE_MACHINE_ID);
        return;
      }

      if (pointInRect(sx, sy, ADMINPIN_ROW_X, MID_ROW_Y, ADMINPIN_ROW_W, MID_ROW_H)) {
        openTextEntry(TE_ADMIN_PIN);
        return;
      }

      if (pointInRect(sx, sy, SET_BTN_BACK_X, BTN_Y, SET_BTN_BACK_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_PANEL;
        drawAdminPanelScreen();
        return;
      }

      if (CFG_WIFI_ENABLED && pointInRect(sx, sy, SET_BTN_WIFI_X, BTN_Y, SET_BTN_WIFI_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_WIFI;
        drawAdminWiFiScreen();
        return;
      }

      if (pointInRect(sx, sy, SET_BTN_UPI_X, BTN_Y, SET_BTN_UPI_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_UPI;
        drawAdminUPIScreen();
        return;
      }

      if (pointInRect(sx, sy, SET_BTN_COIN_X, BTN_Y, SET_BTN_COIN_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_COIN;
        drawAdminCoinScreen();
        return;
      }

      if (CFG_LTE_ENABLED && pointInRect(sx, sy, SET_BTN_LTE_X, BTN_Y, SET_BTN_LTE_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_LTE;
        drawAdminLTEScreen();
        return;
      }
    }
  }
}
