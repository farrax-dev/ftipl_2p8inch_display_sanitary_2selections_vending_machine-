// ---------- Pin Config (ESP32 DevKit, hardware VSPI defaults) ----------
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TOUCH_CS 21

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS);

// Every GPIO available for a motor, in the order they are handed out. A build
// with CFG_MOTOR_COUNT of 4 uses the first four; the rest stay free for other
// use. Ordering is fixed so M1-M5 keep the same pins they always had and
// existing wiring is unaffected by a change to the count.
//
// Chosen to avoid the display SPI bus (18/19/23), the TFT and touch selects
// (2/4/5/21), the flash SPI pins (6-11), the LTE modem UART (16/17), and
// GPIO12, which sets flash voltage at reset and caused a real upload failure
// when something external was wired to it.
// GPIO22 was in this pool but is now the RFID reader's UART1 TX line
// (Core_19_RFID.ino). GPIO33 was M6's pin but went to the coin acceptor
// (Screen_06_PaymentOther.ino) when CFG_MOTOR_COUNT dropped from 6 to 5,
// freeing GPIO35 (now spare). The RFID reader's RX sits on GPIO34
// (Core_19_RFID.ino), also outside this pool, since both only ever need an
// input pin.
//
// Capped at 5 entries, not 8: GPIO14/15 (what would have been M7/M8) are the
// RTC's I2C SDA/SCL (Core_18_RTC.ino) full time now, and GPIO33 (what would
// have been M6) belongs to the coin acceptor full time — neither is a pin
// shared with a hypothetical extra motor, so there's no reason to keep a
// pool entry that would silently collide the moment CFG_MOTOR_COUNT went
// past 5. See the motor-channel table in docs/wiring-diagram.html.
const int MOTOR_PIN_POOL[] = { 13, 25, 26, 27, 32 };
const int MOTOR_PINS_AVAILABLE = sizeof(MOTOR_PIN_POOL) / sizeof(MOTOR_PIN_POOL[0]);

// Caught at compile time rather than as a mystery reboot from reading past
// the end of the pool.
static_assert(CFG_MOTOR_COUNT >= 1 && CFG_MOTOR_COUNT <= 5,
              "CFG_MOTOR_COUNT must be 1-5: GPIO14/15 belong to the RTC and GPIO33 to the coin acceptor now, not a 6th motor");
static_assert(CFG_PRODUCT_COUNT >= 1 && CFG_PRODUCT_COUNT <= 6,
              "CFG_PRODUCT_COUNT must be 1-6: the default product table has 6 entries");

// Indexed 0..MAX_MOTORS-1 everywhere else; the pool simply holds more than a
// given build uses.
const int* MOTOR_PINS = MOTOR_PIN_POOL;
const bool MOTOR_ACTIVE_HIGH = CFG_MOTOR_ACTIVE_HIGH;

// Single value from Config.h, applied to every product/motor.
const unsigned long MOTOR_RUN_MS = CFG_MOTOR_RUN_MS;

const unsigned long MOTOR_TEST_MS = 2500;
const unsigned long MOTOR_GAP_MS = 400;

void motorWrite(int m, bool on) {
  if (m < 0 || m >= MAX_MOTORS) return;
  digitalWrite(MOTOR_PINS[m], (on == MOTOR_ACTIVE_HIGH) ? HIGH : LOW);
}

void allMotorsOff() {
  for (int m = 0; m < MAX_MOTORS; m++) motorWrite(m, false);
}

void initMotorPins() {
  for (int m = 0; m < MAX_MOTORS; m++) {
    digitalWrite(MOTOR_PINS[m], MOTOR_ACTIVE_HIGH ? LOW : HIGH);
    pinMode(MOTOR_PINS[m], OUTPUT);
    motorWrite(m, false);
  }
}

void runMotorPulse(int m, unsigned long ms) {
  Serial.printf("motor M%d ON (GPIO %d) for %lums\n", m + 1, MOTOR_PINS[m], ms);
  motorWrite(m, true);
  delay(ms);
  motorWrite(m, false);
  Serial.printf("motor M%d OFF\n", m + 1);
}
