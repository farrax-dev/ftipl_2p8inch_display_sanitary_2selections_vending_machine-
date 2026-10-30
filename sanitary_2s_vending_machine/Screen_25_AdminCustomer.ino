// =====================================================
//          ADMIN CUSTOMER SCREENS SETTINGS
// =====================================================
// Settings that shape what a shopper sees (Admin > Machine > Customer):
//
//   Max items          - most units one order may hold          [-] n [+]
//   Auto timeout       - send an idle customer back to Welcome   switch
//   Timeout (seconds)  - how long "idle" is                      [-] n [+]
//   Show countdown     - draw the remaining seconds on screen    switch
//
// The timeout covers Select Products, Your Cart and Payment Method only; the
// payment screens keep their own (Core_21_IdleTimeout.ino). "Show countdown"
// off hides the number but the timer still runs and still times the customer
// out. Every change is saved the moment it is made.
const int CUS_ROW_X = 16, CUS_ROW_W = 288, CUS_ROW_H = 32;
const int CUS_ROW_Y0 = 40, CUS_ROW_GAP = 6;
const int CUS_RIGHT = CUS_ROW_X + CUS_ROW_W - 8;
const int CUS_TIMEOUT_STEP = 5;

int cusRowY(int row) { return CUS_ROW_Y0 + row * (CUS_ROW_H + CUS_ROW_GAP); }
int cusRowCY(int row) { return cusRowY(row) + CUS_ROW_H / 2; }

void drawAdminCustomerScreen() {
  drawGradientBackground();
  drawScreenTitle("Customer Screens");

  for (int r = 0; r < 4; r++) auiRow(CUS_ROW_X, cusRowY(r), CUS_ROW_W, CUS_ROW_H);

  int stepLabelW = auiStepLeftX(CUS_RIGHT) - CUS_ROW_X - 20;
  char buf[8];

  snprintf(buf, sizeof(buf), "%d", maxCartQty);
  auiRowLabel(CUS_ROW_X + 12, cusRowCY(0), stepLabelW, "Max items");
  auiStepper(CUS_RIGHT, cusRowCY(0), buf, COL_ACCENT);

  auiRowLabel(CUS_ROW_X + 12, cusRowCY(1), 170, "Auto timeout");
  auiOnOff(CUS_RIGHT, cusRowCY(1), idleTimeoutOn);

  snprintf(buf, sizeof(buf), "%d", idleTimeoutSec);
  auiRowLabel(CUS_ROW_X + 12, cusRowCY(2), stepLabelW, "Seconds");
  auiStepper(CUS_RIGHT, cusRowCY(2), buf, idleTimeoutOn ? COL_ACCENT : COL_TEXT_DIM);

  auiRowLabel(CUS_ROW_X + 12, cusRowCY(3), 170, "Show countdown");
  auiOnOff(CUS_RIGHT, cusRowCY(3), idleShowTimer);

  auiBarButton(0, 1, "Back", ABTN_SECONDARY);
}

void handleAdminCustomerScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (auiBarHit(sx, sy, 0, 1)) {
        currentScreen = SCREEN_ADMIN_MACHINE;
        drawAdminMachineScreen();
        return;
      }

      int step = auiStepTouch(sx, sy, CUS_RIGHT, cusRowCY(0));
      if (step < 0) {
        if (maxCartQty > 1) maxCartQty--;
        saveMaxCartQty();
        drawAdminCustomerScreen();
        return;
      }
      if (step > 0) {
        // Capped at Config.h's seed, not an arbitrary 99 — CFG_MAX_CART_QTY
        // is the ceiling a technician flashed for this machine, and the
        // admin screen shouldn't be able to exceed what was commissioned.
        if (maxCartQty < CFG_MAX_CART_QTY) maxCartQty++;
        saveMaxCartQty();
        drawAdminCustomerScreen();
        return;
      }

      if (pointInRect(sx, sy, CUS_ROW_X, cusRowY(1), CUS_ROW_W, CUS_ROW_H)) {
        idleTimeoutOn = !idleTimeoutOn;
        saveIdleSettings();
        drawAdminCustomerScreen();
        return;
      }

      step = auiStepTouch(sx, sy, CUS_RIGHT, cusRowCY(2));
      if (step != 0) {
        idleTimeoutSec = constrain(idleTimeoutSec + step * CUS_TIMEOUT_STEP,
                                   IDLE_TIMEOUT_MIN_SEC, IDLE_TIMEOUT_MAX_SEC);
        saveIdleSettings();
        drawAdminCustomerScreen();
        return;
      }

      if (pointInRect(sx, sy, CUS_ROW_X, cusRowY(3), CUS_ROW_W, CUS_ROW_H)) {
        idleShowTimer = !idleShowTimer;
        saveIdleSettings();
        drawAdminCustomerScreen();
        return;
      }
    }
  }
}
