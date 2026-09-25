// Runs one motor for its normal dispense duration, then keeps watching the
// drop sensor for CFG_DROP_SENSOR_BUFFER_MS more before giving up — the beam
// break can land anywhere from partway through the motor's spin to a couple
// of seconds after it stops, not on any fixed instant, so the whole window
// is polled rather than just checked once at the end. Returns whether a drop
// was seen (pollDropSensor()'s clear->detected transition) anywhere in that
// window.
bool runMotorPulseVerified(int m, unsigned long motorMs) {
  Serial.printf("motor M%d ON (GPIO %d) for %lums\n", m + 1, MOTOR_PINS[m], motorMs);
  motorWrite(m, true);

  bool detected = false;
  unsigned long startMs = millis();
  unsigned long windowMs = motorMs + CFG_DROP_SENSOR_BUFFER_MS;
  bool motorStopped = false;
  while (millis() - startMs < windowMs) {
    if (!motorStopped && millis() - startMs >= motorMs) {
      motorWrite(m, false);
      motorStopped = true;
    }
    if (pollDropSensor()) {
      detected = true;
      break;
    }
    delay(5);
  }
  motorWrite(m, false);
  Serial.printf("motor M%d OFF (%s)\n", m + 1, detected ? "drop detected" : "NO drop detected");
  return detected;
}

// Returns true only if every unit dispensed for this product registered a
// drop-sensor detection — one missed unit fails the whole product so the
// customer-facing message (Core_11's callers) can say so rather than
// reporting success on a partially-jammed order.
bool dispenseProduct(int i, int qty) {
  bool allDetected = true;
  int remaining = qty;
  for (int m = 0; m < MAX_MOTORS && remaining > 0; m++) {
    if (!(products[i].motorMask & (1 << m))) continue;
    int take = min(remaining, motorStock[m]);
    for (int k = 0; k < take; k++) {
      if (!runMotorPulseVerified(m, runTimeForProduct(i))) allDetected = false;
      decrementMotorStock(m, 1);
      delay(MOTOR_GAP_MS);
    }
    remaining -= take;
  }
  return allDetected;
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
//
// Returns true only if every unit in the cart registered a drop-sensor
// detection. The sale is still logged either way — payment was already
// taken (or, for freeVend, authorised) before this ran, and the motors did
// fire, so the transaction is real regardless of whether the sensor caught
// the product landing; the return value is purely for the caller's
// dispensed/failed message to the customer.
bool dispenseCart(const char* paymentMethod, bool freeVend) {
  drawDispensingScreen();
  allMotorsOff();

  // Open today's record before any motor runs, so the day's "stock at start"
  // is the figure from before this sale rather than after it.
  ensureTodaySlot();

  bool allDetected = true;
  for (int i = 0; i < MAX_PRODUCTS; i++) {
    if (cartQty[i] > 0) {
      if (!dispenseProduct(i, cartQty[i])) allDetected = false;
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

  return allDetected;
}
