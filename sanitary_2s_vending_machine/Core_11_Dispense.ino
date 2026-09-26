// Runs one motor for its normal dispense duration, then keeps watching the
// drop sensor for CFG_DROP_SENSOR_BUFFER_MS more before giving up — the beam
// break can land anywhere from partway through the motor's spin to a couple
// of seconds after it stops, not on any fixed instant, so the whole window
// is polled rather than just checked once at the end. Returns whether a drop
// was seen (pollDropSensor()'s clear->detected transition) anywhere in that
// window.
//
// CFG_IR_SENSOR_PRESENT (Config.h) is checked first: with no sensor fitted
// there is nothing to poll for, so this just runs the motor for its plain
// dispense time and reports the unit as delivered — no
// CFG_DROP_SENSOR_BUFFER_MS tacked on afterwards, no drop-sensor logic at
// all. A build with no sensor module wired in has no way to tell a real
// dispense from a jam either way, so it doesn't pretend to check.
bool runMotorPulseVerified(int m, unsigned long motorMs) {
  if (!CFG_IR_SENSOR_PRESENT) {
    Serial.printf("motor M%d ON (GPIO %d) for %lums (no drop sensor fitted)\n",
                  m + 1, MOTOR_PINS[m], motorMs);
    motorWrite(m, true);
    delay(motorMs);
    motorWrite(m, false);
    Serial.printf("motor M%d OFF (drop check skipped - no sensor)\n", m + 1);
    return true;
  }

  Serial.printf("motor M%d ON (GPIO %d) for %lums\n", m + 1, MOTOR_PINS[m], motorMs);
  motorWrite(m, true);

  bool detected = false;
  bool motorStopped = false;
  unsigned long startMs = millis();
  unsigned long windowMs = motorMs + CFG_DROP_SENSOR_BUFFER_MS;

  while (millis() - startMs < windowMs) {
    // The motor always runs its full defined time no matter when the sensor
    // fires — an early detection (or a noise spike on the sensor line) must
    // never cut the motor short. For a coil/spiral dispenser especially,
    // stopping mid-rotation can leave the mechanism out of position and jam
    // the next dispense.
    if (!motorStopped && millis() - startMs >= motorMs) {
      motorWrite(m, false);
      motorStopped = true;
    }

    if (pollDropSensor()) detected = true;

    // Only once the motor has actually stopped is a confirmed drop a reason
    // to stop waiting early — otherwise keep polling through the rest of the
    // buffer window in case the drop hasn't happened yet.
    if (motorStopped && detected) break;

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
  int failedUnits = 0;
  int unitNum = 0;
  int remaining = qty;
  for (int m = 0; m < MAX_MOTORS && remaining > 0; m++) {
    if (!(products[i].motorMask & (1 << m))) continue;
    int take = min(remaining, motorStock[m]);
    for (int k = 0; k < take; k++) {
      unitNum++;
      Serial.printf("Dispense: '%s' unit %d/%d -> M%d\n", products[i].name, unitNum, qty, m + 1);
      if (!runMotorPulseVerified(m, runTimeForProduct(i))) {
        allDetected = false;
        failedUnits++;
      }
      decrementMotorStock(m, 1);
      delay(MOTOR_GAP_MS);
    }
    remaining -= take;
  }
  Serial.printf("Dispense: '%s' done - %d/%d units confirmed%s\n",
                products[i].name, qty - failedUnits, qty,
                failedUnits ? " (JAM/EMPTY SUSPECTED)" : "");
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

  Serial.printf("Dispense: cart start, payment=%s%s\n",
                paymentMethod, freeVend ? " (free vend)" : "");

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

  Serial.printf("Dispense: cart done, payment=%s - %s\n", paymentMethod,
                allDetected ? "all units confirmed" : "ONE OR MORE UNITS NOT CONFIRMED");

  return allDetected;
}

// Machine-wide free vend (Admin > Settings > "Free Vend", Core_09_Storage.ino's
// freeVendMode) — distinct from the RFID free-vend-by-registered-card feature
// in Screen_06_PaymentOther.ino, which only waives payment for one tapped
// card while everyone else still pays. This is the "nobody pays, ever" mode:
// called directly from the two places a completed cart would otherwise move
// to SCREEN_PAYMENT_METHOD (Screen_02_Select.ino's quickVendSelect() and
// Screen_03_CartReview.ino's Pay button) whenever freeVendMode is on, so the
// customer never sees a payment method screen at all.
//
// Mirrors the Cash/RFID success screens (Screen_06_PaymentOther.ino) rather
// than introducing a new AppScreen state — dispenseCart() already blocks for
// the whole dispense, so there is nothing for a separate screen state to do
// that this synchronous sequence doesn't already cover.
void runFreeVendCheckout() {
  bool dispensedOk = dispenseCart("Free", true);  // freeVend — logged at Rs 0, see dispenseCart() above

  drawGradientBackground();
  tft.setTextSize(3);
  if (dispensedOk) {
    tft.setTextColor(COL_SUCCESS, COL_BG_BOTTOM);
    centerText("Enjoy!", 80);
    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    centerText("Please collect your item", 130);
  } else {
    tft.setTextColor(COL_DANGER, COL_BG_BOTTOM);
    centerText("Dispense", 70);
    centerText("Failed", 100);
    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    centerText("Please contact support", 140);
  }
  delay(2000);

  resetCart();
  currentScreen = SCREEN_WELCOME;
  drawWelcomeScreen();
}
