// =====================================================
//           ADMIN PAYMENTS SCREEN
// =====================================================
// (Still SCREEN_ADMIN_SETTINGS / drawAdminSettingsScreen in the code — the
// Payments tile on the Admin Panel opens it. Everything that isn't about
// taking money moved out: max quantity, machine ID, PIN and the clock are on
// the Machine screen (Screen_24), WiFi and 4G on the Network screen
// (Screen_23).)
//
// One row per thing a customer can pay with, each a switch:
//
//   Free Vend      - the machine never asks for payment (see below)
//   UPI     [Setup]   -> Screen_12_AdminUPIConfig.ino
//   Cash    [Coins]   -> Screen_15_AdminCoin.ino
//   RFID    [Reset] [Cards]  -> Screen_21 / Screen_18
//   Show Prices    - only while nobody pays (Free Vend, or RFID-only)
//
// Free Vend, when on, skips payment entirely (Screen_02_Select.ino /
// Screen_03_CartReview.ino go straight to Core_11_Dispense.ino's
// runFreeVendCheckout() instead of ever reaching SCREEN_PAYMENT_METHOD), so
// the individual method rows would have nothing left to control and are
// hidden rather than left on screen doing nothing.
//
// Which method rows exist at all is Config.h's CFG_PAYMENT_*_AVAILABLE
// (PAYMENT_AVAILABLE[], Core_09_Storage.ino) — a build without UPI never shows
// a UPI row.
const int PAY_ROW_X = 16, PAY_ROW_W = 288, PAY_ROW_Y0 = 40;
const int PAY_RIGHT = PAY_ROW_X + PAY_ROW_W - 8;     // right edge of every switch
const int PAY_CHIP_W = 56, PAY_CHIP_GAP = 6;

enum { PAYROW_FREEVEND, PAYROW_METHOD, PAYROW_PRICES };

// Fills kinds[]/args[] with the rows to show, top to bottom, and returns how
// many. args[] is the payment index for PAYROW_METHOD rows.
int payRows(int kinds[], int args[]) {
  int n = 0;
  kinds[n] = PAYROW_FREEVEND; args[n] = 0; n++;
  if (!freeVendMode) {
    for (int p = 0; p < PAYMENT_COUNT; p++) {
      if (!PAYMENT_AVAILABLE[p]) continue;
      kinds[n] = PAYROW_METHOD; args[n] = p; n++;
    }
  }
  // "Show Prices" matters whenever a customer never actually pays anything —
  // machine-wide Free Vend, or RFID being the only thing a customer can pay
  // with (free-vend-by-badge doesn't need a price on screen either).
  if (freeVendMode || isRFIDOnlyMachine()) {
    kinds[n] = PAYROW_PRICES; args[n] = 0; n++;
  }
  return n;
}

// Up to four rows get a roomy 32px; a fifth (RFID-only builds) squeezes all
// of them to 28px so the stack still ends above the bottom bar.
int payRowH(int count)   { return (count <= 4) ? 32 : 28; }
int payRowGap(int count) { return (count <= 4) ? 6 : 3; }
int payRowY(int row, int count) { return PAY_ROW_Y0 + row * (payRowH(count) + payRowGap(count)); }

int payChipX(int slot) {      // slot 0 sits against the switch, 1 to its left
  return PAY_RIGHT - ASW_W - 8 - PAY_CHIP_W - slot * (PAY_CHIP_W + PAY_CHIP_GAP);
}

void drawAdminSettingsScreen() {
  drawGradientBackground();
  drawScreenTitle("Payments");

  int kinds[PAYMENT_COUNT + 2], args[PAYMENT_COUNT + 2];
  int count = payRows(kinds, args);
  int h = payRowH(count);

  for (int r = 0; r < count; r++) {
    int y = payRowY(r, count);
    int cy = y + h / 2;
    auiRow(PAY_ROW_X, y, PAY_ROW_W, h);

    if (kinds[r] == PAYROW_FREEVEND) {
      auiRowLabel(PAY_ROW_X + 12, cy, 150, "Free Vend");
      auiOnOff(PAY_RIGHT, cy, freeVendMode);
    } else if (kinds[r] == PAYROW_PRICES) {
      auiRowLabel(PAY_ROW_X + 12, cy, 150, "Show Prices");
      auiOnOff(PAY_RIGHT, cy, !hideProductPrices);
    } else {
      int p = args[r];
      auiRowLabel(PAY_ROW_X + 12, cy, 70, PAYMENT_NAMES[p]);
      auiOnOff(PAY_RIGHT, cy, paymentEnabled[p]);

      if (p == PAY_IDX_UPI)  auiRowButton(payChipX(0), cy, PAY_CHIP_W, "Setup", ABTN_SECONDARY);
      if (p == PAY_IDX_CASH) auiRowButton(payChipX(0), cy, PAY_CHIP_W, "Coins", ABTN_SECONDARY);
      if (p == PAY_IDX_RFID) {
        auiRowButton(payChipX(0), cy, PAY_CHIP_W, "Cards", ABTN_SECONDARY);
        auiRowButton(payChipX(1), cy, PAY_CHIP_W, "Reset", ABTN_SECONDARY);
      }
    }
  }

  if (freeVendMode) auiHint("Free vend: customers are never charged", payRowY(count, count) + 6);

  auiBarButton(0, 1, "Back", ABTN_SECONDARY);
}

void handleAdminSettingsScreen() {
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

      int kinds[PAYMENT_COUNT + 2], args[PAYMENT_COUNT + 2];
      int count = payRows(kinds, args);
      int h = payRowH(count);

      for (int r = 0; r < count; r++) {
        int y = payRowY(r, count);
        int cy = y + h / 2;
        if (!pointInRect(sx, sy, PAY_ROW_X, y, PAY_ROW_W, h)) continue;

        if (kinds[r] == PAYROW_FREEVEND) {
          freeVendMode = !freeVendMode;
          saveFreeVendMode();
          drawAdminSettingsScreen();
          return;
        }

        if (kinds[r] == PAYROW_PRICES) {
          hideProductPrices = !hideProductPrices;
          saveHideProductPrices();
          drawAdminSettingsScreen();
          return;
        }

        int p = args[r];

        if (p == PAY_IDX_UPI && auiRowButtonHit(sx, sy, payChipX(0), cy, PAY_CHIP_W)) {
          currentScreen = SCREEN_ADMIN_UPI;
          drawAdminUPIScreen();
          return;
        }
        if (p == PAY_IDX_CASH && auiRowButtonHit(sx, sy, payChipX(0), cy, PAY_CHIP_W)) {
          currentScreen = SCREEN_ADMIN_COIN;
          drawAdminCoinScreen();
          return;
        }
        if (p == PAY_IDX_RFID) {
          if (auiRowButtonHit(sx, sy, payChipX(0), cy, PAY_CHIP_W)) {
            currentScreen = SCREEN_ADMIN_RFID_CARDS;
            rfidCardsPage = 0;
            drawAdminRFIDCardsScreen();
            return;
          }
          if (auiRowButtonHit(sx, sy, payChipX(1), cy, PAY_CHIP_W)) {
            currentScreen = SCREEN_ADMIN_RFID_RESET;
            drawAdminRFIDResetScreen();
            return;
          }
        }

        paymentEnabled[p] = !paymentEnabled[p];
        savePaymentEnabled(p);

        // hideProductPrices only ever gets its toggle shown while the
        // machine is RFID-only or free-vend — a machine that genuinely
        // charges money has no business hiding the amount. Turning Cash or
        // UPI on is exactly the moment that stops being true, so clear it
        // here rather than leaving it stuck true in NVS from an earlier
        // RFID-only configuration, where it would silently blank the Cart
        // Review total while Cash/UPI still show the real amount due.
        if ((p == PAY_IDX_CASH || p == PAY_IDX_UPI) && paymentEnabled[p] && hideProductPrices) {
          hideProductPrices = false;
          saveHideProductPrices();
        }

        drawAdminSettingsScreen();
        return;
      }
    }
  }
}
