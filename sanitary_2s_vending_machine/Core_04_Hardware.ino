// ---------- Pin Config (ESP32 DevKit, hardware VSPI defaults) ----------
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TOUCH_CS 21

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS);

// Every GPIO available for a motor, in the order they are handed out. A build
// with CFG_MOTOR_COUNT of 4 uses the first four; the rest stay free for other
// use. Ordering is fixed so M1-M6 keep the same pins they always had and
// existing wiring is unaffected by a change to the count.
//
// Chosen to avoid the display SPI bus (18/19/23), the TFT and touch selects
// (2/4/5/21), the flash SPI pins (6-11), the LTE modem UART (16/17), and
// GPIO12, which sets flash voltage at reset and caused a real upload failure
// when something external was wired to it.
// GPIO22 was in this pool but is now the RFID reader's UART1 TX line
// (Core_19_RFID.ino), which also took over the input-only GPIO34 for that
// module's RX line.
//
// Capped at 6 entries, not 8: GPIO14/15 (what would have been M7/M8) are the
// RTC's I2C SDA/SCL (Core_18_RTC.ino) full time, with no dip switch or other
// escape hatch — there's no reason to keep a pool entry that would silently
// collide with the RTC the moment CFG_MOTOR_COUNT went past 6. See the
// motor-channel table in docs/wiring-diagram.html.
//
// pool[4] (GPIO32, "M5") and pool[5] (GPIO33, "M6") are dual-role by design,
// each behind its own external dip switch rather than fixed to one job:
//   - GPIO32: dip switch B's default throw feeds the coin acceptor's
//     ON/OFF power relay (COIN_ENABLE_PIN, Screen_06_PaymentOther.ino); the
//     other throw reconnects the same terminal to the M5 motor driver
//     channel.
//   - GPIO33: dip switch A's default throw feeds the coin acceptor's pulse
//     input (COIN_PIN, Screen_06_PaymentOther.ino); the other throw
//     reconnects it to the M6 motor driver channel.
// Both switches are manual, reflash-mode selectors — firmware can't sense
// either one's physical position, so it goes by CFG_MOTOR_COUNT instead:
// below 5, neither pin is claimed by a motor and the coin subsystem inits
// normally; at 5, GPIO32 is claimed and the coin power relay is skipped; at
// 6, GPIO33 is claimed too and the coin subsystem has no pin left to run on
// at all. See the guards in Screen_06_PaymentOther.ino's initCoinAcceptor()
// and coinAcceptorListen().
//
// Sanitary-napkin build note: CFG_MOTOR_COUNT is 2 here, so only pool[0]
// (GPIO13, M1) and pool[1] (GPIO25, M2) are ever touched by initMotorPins()
// below. pool[2]/pool[3] (GPIO26/27, "M3"/"M4") are simply idle/reserved.
// pool[4]/pool[5] are on their coin throw, per the dip-switch behaviour
// above. None of this needs a code change to work; raising CFG_MOTOR_COUNT
// past 4 just needs the matching dip switch(es) moved to their motor throw
// first, and a reflash either way.
const int MOTOR_PIN_POOL[] = { 13, 25, 26, 27, 32, 33 };
const int MOTOR_PINS_AVAILABLE = sizeof(MOTOR_PIN_POOL) / sizeof(MOTOR_PIN_POOL[0]);

// Caught at compile time rather than as a mystery reboot from reading past
// the end of the pool.
static_assert(CFG_MOTOR_COUNT >= 1 && CFG_MOTOR_COUNT <= 6,
              "CFG_MOTOR_COUNT must be 1-6: GPIO14/15 belong to the RTC full-time, not a 7th/8th motor");
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
