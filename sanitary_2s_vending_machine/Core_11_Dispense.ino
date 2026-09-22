void dispenseProduct(int i, int qty) {
  int remaining = qty;
  for (int m = 0; m < MAX_MOTORS && remaining > 0; m++) {
    if (!(products[i].motorMask & (1 << m))) continue;
    int take = min(remaining, motorStock[m]);
    for (int k = 0; k < take; k++) {
      runMotorPulse(m, runTimeForProduct(i));
      decrementMotorStock(m, 1);
      delay(MOTOR_GAP_MS);
    }
    remaining -= take;
  }
}

void drawDispensingScreen() {
  drawGradientBackground();
  tft.setTextSize(3);
  tft.setTextColor(COL_ACCENT, COL_BG_BOTTOM);
  centerText("Dispensing", 90);
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  centerText("Please wait...", 130);
}

// paymentMethod is recorded against every line of the order in the sales log
// (Core_14_SalesLog.ino). Every payment flow funnels through here, so this is
// the one place a completed sale can be booked without duplicating it per
// payment screen.
//
// freeVend is for the RFID free-vend flow: a registered card authorises the
// dispense but no money changes hands, so every line is logged at Rs 0
// instead of the catalog price — units sold and the transaction count still
// go up, so "N free vends today" stays visible in reports, it just never
// contributes to revenue.
void dispenseCart(const char* paymentMethod, bool freeVend) {
  drawDispensingScreen();
  allMotorsOff();

  // Open today's record before any motor runs, so the day's "stock at start"
  // is the figure from before this sale rather than after it.
  ensureTodaySlot();

  for (int i = 0; i < MAX_PRODUCTS; i++) {
    if (cartQty[i] > 0) {
      dispenseProduct(i, cartQty[i]);
      // Logged after the motors have run, so a jam that halts dispensing
      // doesn't book revenue for product that never came out.
      recordSale(i, cartQty[i], paymentMethod, freeVend ? 0 : -1);
    }
  }
  allMotorsOff();

  // Counts the order as one transaction and flushes the daily table to flash.
  recordTransaction(paymentMethod);

  // Stock just dropped, so this is the moment a motor can cross the low-stock
  // line (Core_16_ReportSchedule.ino decides whether that warrants an email).
  checkLowStockAlert();
}
