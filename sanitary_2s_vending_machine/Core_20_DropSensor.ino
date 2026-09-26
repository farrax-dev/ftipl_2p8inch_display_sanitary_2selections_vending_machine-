// =====================================================
//        PRODUCT-DROP IR SENSOR (jam/dispense detection)
// =====================================================
// A break-beam pair or a single reflective module across the dispense chute
// — either way, a digital output that changes state while a product is
// passing through it. This file only reads that raw signal; the decision
// about what a missed drop means (dispenseProduct()'s per-unit verification
// window, Core_11_Dispense.ino) lives with the dispense flow, using
// pollDropSensor() below plus CFG_DROP_SENSOR_BUFFER_MS (Config.h).
//
// GPIO35 — the pin this whole project has kept in reserve for exactly this:
// "preferred spare for a new input" (see the motor-pool/spare tables in
// docs/wiring-diagram.html). Input-only, like every pin in the GPIO34-39
// range: no internal pull-up/down, no output driver — and unlike GPIO33
// (COIN_PIN, Screen_06_PaymentOther.ino), there's no internal pull-up to
// fall back on here at all.
//
// External 10k pull-up to 3V3 required. The module idles HIGH on its own
// (confirmed — see CFG_IR_SENSOR_ACTIVE_LOW's comment in Config.h), so this
// isn't strictly load-bearing if its output stage turns out to be push-pull
// — but COIN_PIN's own history is the reason to fit it anyway: that line
// once floated on an unconditioned input, drifted across the threshold on
// ambient noise, and the ISR counted phantom coins with nothing inserted.
// A missing pull-up here would fail the same way — phantom "product
// dropped" events off noise alone — for the cost of one resistor.
const int IR_SENSOR_PIN = 35;

// Debounce window for the drop event itself, not contact bounce — a real
// product passing through a beam takes tens of milliseconds, so anything
// shorter is sensor noise (dust, a fluttering edge at the beam boundary),
// never a second product.
const unsigned long DROP_SENSOR_DEBOUNCE_MS = 30;

// dispenseProduct() (Core_11_Dispense.ino) reads CFG_DROP_SENSOR_BUFFER_MS
// straight from Config.h rather than a derived const here — Core_11 sorts
// before this file in the concatenated build, so a const defined here
// wouldn't exist yet at its point of use (see Core_02_AppState.ino's
// motorStockPage comment for the same cross-tab ordering rule). The macro
// itself is fine everywhere: Config.h is #included at the top of the main
// sketch file, ahead of every tab.

void initDropSensor() {
  if (!CFG_IR_SENSOR_PRESENT) return;  // no module wired in — leave GPIO35 untouched
  pinMode(IR_SENSOR_PIN, INPUT);
}

// Instantaneous, undebounced read: true if the sensor is reporting a
// product in the beam right now. Confirmed on the module in hand: idles
// HIGH, reads LOW while the beam is cut. Read through Config.h's
// CFG_IR_SENSOR_ACTIVE_LOW rather than a hardcoded LOW, for the same reason
// CFG_MOTOR_ACTIVE_HIGH exists — so a future module swap that idles the
// other way is a Config.h edit, not a code change.
bool dropSensorRawDetected() {
  int level = digitalRead(IR_SENSOR_PIN);
  return CFG_IR_SENSOR_ACTIVE_LOW ? (level == LOW) : (level == HIGH);
}

bool dropSensorStableState = false;  // debounced state: true = beam currently broken
bool dropSensorRawLast = false;      // last raw reading, to notice a fresh transition
unsigned long dropSensorLastEdgeMs = 0;

// Call on every tick of whatever loop is watching it — the main loop() when
// idle, or dispenseProduct()'s tight verification loop while a motor is
// running (polling is plenty fast for a mechanical drop — no interrupt
// needed here the way the coin pulse train needed one).
// Returns true exactly once per product: on the poll where the debounced
// state settles from clear to detected, not on every tick a product happens
// to still be in the beam. A product that never breaks the beam at all
// simply never returns true — that's the jam/empty-slot case a future
// caller would want to notice.
bool pollDropSensor() {
  // Belt-and-suspenders: Core_11_Dispense.ino's runMotorPulseVerified() is
  // the one place that actually decides whether to call this at all when no
  // sensor is fitted, but a stray future caller should still get "no drop"
  // rather than a floating/unread GPIO35 read.
  if (!CFG_IR_SENSOR_PRESENT) return false;

  bool raw = dropSensorRawDetected();
  if (raw != dropSensorRawLast) {
    dropSensorRawLast = raw;
    dropSensorLastEdgeMs = millis();
  }

  if (raw != dropSensorStableState && millis() - dropSensorLastEdgeMs >= DROP_SENSOR_DEBOUNCE_MS) {
    dropSensorStableState = raw;
    return raw;
  }
  return false;
}
