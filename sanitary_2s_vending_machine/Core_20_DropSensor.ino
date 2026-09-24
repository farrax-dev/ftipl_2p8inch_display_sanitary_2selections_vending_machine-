// =====================================================
//        PRODUCT-DROP IR SENSOR (jam/dispense detection)
// =====================================================
// A break-beam pair or a single reflective module across the dispense chute
// — either way, a digital output that changes state while a product is
// passing through it. This file only reads that signal; nothing in the
// dispense flow (Core_11_Dispense.ino's dispenseProduct()/dispenseCart())
// acts on it yet. pollDropSensor() below is ready to be called from there
// once there's a decision about what a missed drop should actually do
// (retry the motor, flag the sale, alert the admin, ...) — that's a product
// behaviour choice, not a wiring one, so it's left for when that's decided.
//
// GPIO35 — the pin this whole project has kept in reserve for exactly this:
// "preferred spare for a new input" (see the motor-pool/spare tables in
// docs/wiring-diagram.html). Input-only, like every pin in the GPIO34-39
// range: no internal pull-up/down, no output driver. That's fine for a
// sensor module with its own actively-driven digital output (the normal
// case — an LM393-comparator IR module drives HIGH and LOW itself, it
// doesn't need a pull resistor), but if the module in hand turns out to be
// open-collector instead, GPIO35 needs an external pull-up to work at all —
// unlike GPIO33 (COIN_PIN, Screen_06_PaymentOther.ino), there is no internal
// pull-up here to fall back on if that gets missed.
const int IR_SENSOR_PIN = 35;

// Debounce window for the drop event itself, not contact bounce — a real
// product passing through a beam takes tens of milliseconds, so anything
// shorter is sensor noise (dust, a fluttering edge at the beam boundary),
// never a second product.
const unsigned long DROP_SENSOR_DEBOUNCE_MS = 30;

void initDropSensor() {
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

// Call every loop() tick (polling is plenty fast for a mechanical drop —
// no interrupt needed here the way the coin pulse train needed one).
// Returns true exactly once per product: on the poll where the debounced
// state settles from clear to detected, not on every tick a product happens
// to still be in the beam. A product that never breaks the beam at all
// simply never returns true — that's the jam/empty-slot case a future
// caller would want to notice.
bool pollDropSensor() {
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
