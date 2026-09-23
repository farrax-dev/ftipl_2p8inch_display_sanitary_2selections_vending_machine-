// =====================================================
//         PAYMENT - CASH (pulse-counting coin acceptor)
// =====================================================
// Coin acceptors report a coin's value as a short burst of pulses on one
// GPIO (1 pulse per unit, set via the acceptor's own DIP switches). Pulses
// are counted in a hardware interrupt so counting isn't at the mercy of the
// main loop's touch/display/network work; handlePaymentCashScreen() just
// watches for a gap after the last pulse to know a burst finished, then
// classifies the pulse count into a coin value. Adapted from the
// Scan_Pulses()/Time_Out() pulse-train logic in the original PIC16F690
// build (singleshe65.c), reworked to be non-blocking for this screen's
// polling loop instead of that build's busy-wait while() loops.
//
// A coin, once mechanically accepted by the acceptor, can't be returned —
// there's no "inhibit"/return-coin line wired here (no GPIO left to spare —
// see the earlier IO budget). So an unrecognized pulse count is simply not
// credited, same limitation the original hardware had.
//
// GPIO33 is also motor-pool slot M6 (MOTOR_PIN_POOL, Core_04_Hardware.ino) —
// behind an external dip switch rather than fixed to one job. Its default
// throw feeds this pulse line; the other throw reconnects the same pin to a
// 6th motor driver channel. See COIN_PULSE_PIN_FREE below for how firmware
// decides which one it's allowed to assume.
//
// Unlike the GPIO34-39 pins this line lived on before, GPIO33 is a regular
// GPIO with an internal pull-up available, but the code below still asks for
// a plain INPUT and leans on the external 10k pull-up to 3V3 described in
// the coin-acceptor table in docs/wiring-diagram.html, matching how the
// acceptor was already wired. If a future move ever puts this back on a
// GPIO34-39 pin, the external pull-up becomes mandatory again, not just
// carried over: those pins have no internal pull resistor at all. That gap
// was hit once already — with a bare pinMode(INPUT) and no external
// pull-up (then on GPIO34) the line floated, drifted across the ~1.65V
// threshold on ambient noise, and the ISR counted the noise as coins — the
// machine dispensed with nothing connected and nothing inserted. The pin
// itself was never the problem; an unconditioned floating input is.
//
// History: GPIO22 originally, freed when GPIO22 became the RFID reader's TX
// line (a coin pulse is a pure input signal that never needed an
// output-capable pin); then GPIO34, swapped to GPIO36 ("VP") when the RFID
// reader's RX line moved off it for wiring convenience; then off GPIO36
// again onto GPIO35 because VP proved unreliable in practice (it sits
// directly next to the header's EN pin, and a stray/adjacent connection
// there briefly holding EN low holds the whole board in reset — a real
// failure mode already hit once while wiring the RFID reader); then onto
// GPIO33/M6, which is where the dip switch replaced a permanent hand-off.
const int COIN_PIN = 33;

// Cuts power to the coin acceptor itself via an external relay/MOSFET —
// GPIO32, motor-pool slot M5 (see Core_04_Hardware.ino's MOTOR_PIN_POOL
// comment), behind its own external dip switch. Its default throw feeds
// this relay; the other throw reconnects the same pin to the M5 motor
// driver channel. See COIN_POWER_PIN_FREE below.
//
// Same active-high/low convention as the motor driver channels — it's
// wired to the same kind of driver board — so it's driven through
// MOTOR_ACTIVE_HIGH rather than a hardcoded HIGH/LOW.
const int COIN_ENABLE_PIN = 32;

// GPIO33/GPIO32 are also motor-pool slots M6/M5 (MOTOR_PIN_POOL,
// Core_04_Hardware.ino), each behind its own external dip switch rather
// than fixed to one job. Firmware has no way to sense either switch's
// physical position, so it goes by CFG_MOTOR_COUNT instead: a pin that
// falls within MOTOR_PIN_POOL[0..MAX_MOTORS-1] is assumed to actually be
// wired to a motor driver right now, and the matching half of the coin
// subsystem backs off rather than fighting initMotorPins() over the same
// pin. Below 5 motors, both are free and cash works exactly as before; at
// 5, the power relay backs off (cash still credits coins, just without
// software control over the acceptor's power); at 6, there's no pin left
// for the pulse input either, so the whole coin subsystem goes quiet — a
// 6-motor build has no cash hardware, and CFG_PAYMENT_CASH_AVAILABLE
// should be turned off in Config.h to match.
const bool COIN_POWER_PIN_FREE = (MAX_MOTORS < 5);
const bool COIN_PULSE_PIN_FREE = (MAX_MOTORS < 6);

const unsigned long COIN_TRAIN_GAP_MS = 200;    // silence -> burst is complete
const unsigned long COIN_TRAIN_MAX_MS = 3000;   // safety cap on a stuck line
const unsigned long CASH_TIMEOUT_MS   = 90000;  // give up if nothing happens

volatile uint32_t coinISRPulseCount = 0;
volatile unsigned long coinISRLastPulseMs = 0;

// Real coin pulses are 20-100ms wide, so anything arriving within 5ms of the
// last edge is contact bounce or noise, never a second coin. Without this a
// single slow edge crossing the threshold could register a dozen counts.
const unsigned long COIN_DEBOUNCE_MS = 5;

void IRAM_ATTR coinPulseISR() {
  unsigned long now = millis();
  if (now - coinISRLastPulseMs < COIN_DEBOUNCE_MS) return;
  coinISRLastPulseMs = now;
  coinISRPulseCount++;
}

// Same HIGH/LOW-energises convention as motorWrite() in Core_04_Hardware.ino
// — COIN_ENABLE_PIN drives the same kind of relay/MOSFET driver board.
void coinAcceptorPower(bool on) {
  if (!COIN_POWER_PIN_FREE) return;  // GPIO32 is M5's driver output in this build
  digitalWrite(COIN_ENABLE_PIN, (on == MOTOR_ACTIVE_HIGH) ? HIGH : LOW);
}

void initCoinAcceptor() {
  if (COIN_PULSE_PIN_FREE) {
    // Plain INPUT, not INPUT_PULLUP — kept consistent with how this line was
    // already wired (external 10k pull-up to 3V3, see the comment on
    // COIN_PIN). GPIO33 does have an internal pull-up available if that
    // external resistor is ever removed, unlike the GPIO34-39 pins this
    // line used to live on.
    pinMode(COIN_PIN, INPUT);
  }

  if (COIN_POWER_PIN_FREE) {
    // Written before pinMode(OUTPUT), same order as initMotorPins() — so
    // the pin never glitches HIGH/energised for the instant between the two
    // calls. Powered down until a cash screen actually needs it.
    digitalWrite(COIN_ENABLE_PIN, MOTOR_ACTIVE_HIGH ? LOW : HIGH);
    pinMode(COIN_ENABLE_PIN, OUTPUT);
    coinAcceptorPower(false);
  }
}

// The interrupt is live only while the cash screen is showing. Nothing else
// in the machine cares about coins, and leaving it attached means motor
// switching during a dispense can inject counts into the next sale.
// Acceptor power follows the same lifecycle: it's only powered up while
// something is actually listening for its pulses, so a coin dropped in
// outside of the cash screen physically can't register.
void coinAcceptorListen(bool on) {
  coinAcceptorPower(on);
  if (!COIN_PULSE_PIN_FREE) return;  // GPIO33 is M6's driver output in this build
  if (on) attachInterrupt(digitalPinToInterrupt(COIN_PIN), coinPulseISR, FALLING);
  else    detachInterrupt(digitalPinToInterrupt(COIN_PIN));
}

int coinPulseCount = 0;  // pulses seen so far in the burst being timed
bool coinBurstActive = false;
unsigned long coinBurstStartMs = 0;
uint32_t coinLastSeenISRCount = 0;

int cashAmountDue = 0;
bool cashAnyCoinAccepted = false;
unsigned long cashEnteredTime = 0;
char cashStatusMsg[32] = "";
uint16_t cashStatusColor = COL_TEXT_DIM;

void resetCoinAcceptorState() {
  noInterrupts();
  coinISRPulseCount = 0;
  coinISRLastPulseMs = 0;
  interrupts();
  coinPulseCount = 0;
  coinBurstActive = false;
  coinLastSeenISRCount = 0;
}

// Classic pulse-count -> coin value table (tune to your acceptor's DIP-
// switch setup). 3-5 pulses are all treated as Rs 5 to match the +/-2 pulse
// jitter the original hardware/logic tolerated.
int coinValueForPulseCount(int pulses) {
  if (pulses == 1) return 1;
  if (pulses == 2) return 2;
  if (pulses >= 3 && pulses <= 5) return 5;
  if (pulses >= 10) return 10;
  return 0;  // unrecognized count -> not credited
}

// Returns the coin value just credited (>0), 0 if no burst has resolved yet,
// -1 if a burst finished but didn't match a known coin value, or -2 if it
// matched a real denomination the admin has disabled (Admin > Settings >
// Coin). Either way the coin is already mechanically retained by the
// acceptor — there's no path to return it, just to decline crediting it.
int pollCoinAcceptor() {
  noInterrupts();
  uint32_t isrCount = coinISRPulseCount;
  unsigned long lastPulseMs = coinISRLastPulseMs;
  interrupts();

  if (isrCount != coinLastSeenISRCount) {
    if (!coinBurstActive) {
      coinBurstActive = true;
      coinBurstStartMs = millis();
    }
    coinPulseCount += (int)(isrCount - coinLastSeenISRCount);
    coinLastSeenISRCount = isrCount;
  }

  if (!coinBurstActive) return 0;

  bool gapElapsed = (millis() - lastPulseMs) >= COIN_TRAIN_GAP_MS;
  bool tooLong    = (millis() - coinBurstStartMs) >= COIN_TRAIN_MAX_MS;
  if (!gapElapsed && !tooLong) return 0;

  int pulses = coinPulseCount;
  int value = coinValueForPulseCount(pulses);
  coinPulseCount = 0;
  coinBurstActive = false;

  if (value <= 0) {
    Serial.printf("coin acceptor: %d pulses -> unrecognized\n", pulses);
    return -1;
  }
  if (!isCoinValueEnabled(value)) {
    Serial.printf("coin acceptor: %d pulses -> Rs %d (disabled by admin)\n", pulses, value);
    return -2;
  }
  Serial.printf("coin acceptor: %d pulses -> Rs %d\n", pulses, value);
  return value;
}

// Total/Paid sub-header plus the big "how much is left" number — both change
// together every time a coin is credited, so they're wiped and redrawn as
// one region.
void drawCashProgressLines() {
  // Taller than the old 60px band — the amount below now renders at size 4
  // instead of 3, and this has to fully clear the bigger glyphs on every
  // redraw or old digits ghost through the new ones.
  tft.fillRect(0, 40, tft.width(), 72, COL_BG_BOTTOM);

  int remaining = max(cashAmountDue, 0);
  int paid = orderTotal - remaining;

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  char headerBuf[32];
  snprintf(headerBuf, sizeof(headerBuf), "Total Rs %d   Paid Rs %d", orderTotal, paid);
  centerText(headerBuf, 44);

  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  centerText("Remaining", 58);

  tft.setTextSize(4);
  tft.setTextColor(COL_ACCENT, COL_BG_BOTTOM);
  char amountBuf[16];
  snprintf(amountBuf, sizeof(amountBuf), "Rs %d", remaining);
  centerText(amountBuf, 80);
}

// Built once when the screen opens — the admin's accepted-denomination list
// doesn't change mid-transaction, so this doesn't need to be in the redraw
// path that runs on every coin.
void drawAcceptedCoinsLine() {
  char buf[48];
  buf[0] = '\0';
  strncat(buf, "Accepts: ", sizeof(buf) - strlen(buf) - 1);
  bool first = true, any = false;
  for (int d = 0; d < COIN_DENOM_COUNT; d++) {
    if (!coinDenomEnabled[d]) continue;
    any = true;
    char tmp[8];
    snprintf(tmp, sizeof(tmp), first ? "Rs%d" : ", Rs%d", COIN_DENOM_VALUES[d]);
    strncat(buf, tmp, sizeof(buf) - strlen(buf) - 1);
    first = false;
  }
  if (!any) strncat(buf, "none", sizeof(buf) - strlen(buf) - 1);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  centerText(buf, 122);
}

void drawCashStatusLine() {
  // Size 2 instead of 1 — this is the one line that reports what just
  // happened to the coin you dropped in, so it earns more presence than an
  // ordinary caption. Every message it carries ("+Rs N credited", "Coin not
  // recognized", "That coin is not accepted") comfortably fits 320px at
  // this size.
  tft.fillRect(0, 148, tft.width(), 20, COL_BG_BOTTOM);
  if (cashStatusMsg[0] != '\0') {
    tft.setTextSize(2);
    tft.setTextColor(cashStatusColor, COL_BG_BOTTOM);
    centerText(cashStatusMsg, 152);
  }
}

// Swaps in a "no refund" note once coins start arriving, since there is no
// hardware path to give a partial payment back.
void drawCashBackButtonArea() {
  if (cashAnyCoinAccepted) {
    tft.fillRoundRect(BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H, 8, COL_BG_TOP);
    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT_DIM, COL_BG_TOP);
    centerTextInBox("Paying, no refund", BTN_Y + 13, BTN_BACK_X, BTN_BACK_W);
  } else {
    drawBackButton("< Back");
  }
}

void drawPaymentCashScreen() {
  cashAmountDue = orderTotal;
  cashAnyCoinAccepted = false;
  cashStatusMsg[0] = '\0';
  cashStatusColor = COL_TEXT_DIM;
  cashEnteredTime = millis();
  resetCoinAcceptorState();
  coinAcceptorListen(true);

  drawGradientBackground();
  drawScreenTitle("Insert Coins");

  drawCashProgressLines();
  drawAcceptedCoinsLine();

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  centerText("No change given - pay exact amount", 136);
  drawCashStatusLine();
  drawCashBackButtonArea();
}

void handlePaymentCashScreen() {
  int credited = pollCoinAcceptor();

  if (credited > 0) {
    cashAmountDue -= credited;
    bool firstCoin = !cashAnyCoinAccepted;
    cashAnyCoinAccepted = true;
    cashEnteredTime = millis();  // coins are still coming in, keep the session alive
    snprintf(cashStatusMsg, sizeof(cashStatusMsg), "+Rs %d credited", credited);
    cashStatusColor = COL_SUCCESS;

    if (cashAmountDue <= 0) {
      dispenseCart("Cash", false);

      drawGradientBackground();
      tft.setTextSize(3);
      tft.setTextColor(COL_SUCCESS, COL_BG_BOTTOM);
      centerText("Payment", 70);
      centerText("Successful!", 100);
      tft.setTextSize(1);
      tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
      centerText("Please collect your item", 140);
      delay(2500);

      resetCart();
      coinAcceptorListen(false);
      currentScreen = SCREEN_WELCOME;
      drawWelcomeScreen();
      return;
    }

    drawCashProgressLines();
    drawCashStatusLine();
    if (firstCoin) drawCashBackButtonArea();
  } else if (credited == -2) {
    // A real denomination, just not one the admin currently accepts.
    snprintf(cashStatusMsg, sizeof(cashStatusMsg), "That coin is not accepted");
    cashStatusColor = COL_DANGER;
    drawCashStatusLine();
  } else if (credited < 0) {
    // Burst finished but didn't match a known coin value; the acceptor has
    // already mechanically kept the coin, there's nothing to credit it as.
    snprintf(cashStatusMsg, sizeof(cashStatusMsg), "Coin not recognized");
    cashStatusColor = COL_DANGER;
    drawCashStatusLine();
  }

  // Fires whether or not a coin has been credited yet — cashEnteredTime is
  // bumped on every accepted coin (see above), so this is really an
  // inactivity timeout: no coins at all for CASH_TIMEOUT_MS, or a customer
  // who paid partway then walked off. Without the second case the "no
  // refund" Back-button swap above left the screen stuck forever once a
  // single coin had been credited.
  if (millis() - cashEnteredTime > CASH_TIMEOUT_MS) {
    coinAcceptorListen(false);
    currentScreen = SCREEN_PAYMENT_METHOD;
    drawPaymentMethodScreen();
    return;
  }

  if (isRealTouch() && millis() - lastTouchTime > TOUCH_DEBOUNCE) {
    lastTouchTime = millis();
    TS_Point raw = ts.getPoint();
    int sx, sy;
    mapTouchToScreen(raw, sx, sy);
    if (!cashAnyCoinAccepted && pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
      coinAcceptorListen(false);
      currentScreen = SCREEN_PAYMENT_METHOD;
      drawPaymentMethodScreen();
    }
  }
}

// =====================================================
//               RFID PAYMENT (registered-card free vend)
// =====================================================
// Not really a "payment" — a tap that matches the registered list (Admin >
// Settings > Cards, Screen_18_AdminRFIDCards.ino) vends for free; anything
// else is refused. See dispenseCart()'s freeVend flag in Core_11_Dispense.ino
// for how that's kept out of revenue reporting.
const unsigned long RFID_PAY_TIMEOUT_MS = 60000;  // give up and return to Payment Method if nothing's tapped

unsigned long rfidPayEnteredMs = 0;
char rfidStatusMsg[32] = "";
uint16_t rfidStatusColor = COL_TEXT_DIM;
unsigned long rfidStatusUntilMs = 0;  // rejection message clears itself, same as the coin screen

void drawRFIDStatusLine() {
  tft.fillRect(0, 140, tft.width(), 16, COL_BG_BOTTOM);
  if (rfidStatusMsg[0] != '\0') {
    tft.setTextSize(1);
    tft.setTextColor(rfidStatusColor, COL_BG_BOTTOM);
    centerText(rfidStatusMsg, 144);
  }
}

void drawPaymentRFIDScreen() {
  rfidPayEnteredMs = millis();
  rfidStatusMsg[0] = '\0';

  drawGradientBackground();
  drawScreenTitle("Tap RFID Card");

  tft.setTextSize(2);
  tft.setTextColor(COL_ACCENT, COL_BG_BOTTOM);
  centerText("No charge for", 65);
  centerText("registered cards", 88);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  centerText("Hold your card near the reader", 118);
  drawRFIDStatusLine();
  drawBackButton("< Back");
}

void handlePaymentRFIDScreen() {
  if (rfidStatusMsg[0] != '\0' && millis() >= rfidStatusUntilMs) {
    rfidStatusMsg[0] = '\0';
    drawRFIDStatusLine();
  }

  char uid[21];
  if (pollRFIDCard(uid, sizeof(uid))) {
    int cardIdx = findRFIDCardIndex(uid);
    // withdrawLimit == 0 is unlimited (the default for a freshly-registered
    // card); otherwise this cart must fit in what's left of the card's
    // running allowance — see Screen_19_AdminRFIDCardEdit.ino.
    int remaining = (cardIdx >= 0 && rfidCards[cardIdx].withdrawLimit > 0)
                       ? rfidCards[cardIdx].withdrawLimit - rfidCards[cardIdx].withdrawUsed
                       : -1;
    bool overLimit = (remaining >= 0 && cartTotalQty() > remaining);

    if (cardIdx >= 0 && !overLimit) {
      int qty = cartTotalQty();
      dispenseCart("RFID", true);  // freeVend — see Core_11_Dispense.ino
      addRFIDCardUsage(cardIdx, qty);

      drawGradientBackground();
      tft.setTextSize(3);
      tft.setTextColor(COL_SUCCESS, COL_BG_BOTTOM);
      centerText("Card Accepted", 70);
      tft.setTextSize(1);
      tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
      centerText("Please collect your item", 110);
      delay(2000);

      resetCart();
      currentScreen = SCREEN_WELCOME;
      drawWelcomeScreen();
      return;
    }

    if (cardIdx < 0) {
      snprintf(rfidStatusMsg, sizeof(rfidStatusMsg), "Card not registered");
    } else {
      snprintf(rfidStatusMsg, sizeof(rfidStatusMsg), "Limit reached (%d left)", remaining < 0 ? 0 : remaining);
    }
    rfidStatusColor = COL_DANGER;
    rfidStatusUntilMs = millis() + 1800;
    rfidPayEnteredMs = millis();  // an actual tap counts as activity, same as a coin
    drawRFIDStatusLine();
  }

  if (millis() - rfidPayEnteredMs > RFID_PAY_TIMEOUT_MS) {
    currentScreen = SCREEN_PAYMENT_METHOD;
    drawPaymentMethodScreen();
    return;
  }

  if (isRealTouch() && millis() - lastTouchTime > TOUCH_DEBOUNCE) {
    lastTouchTime = millis();
    TS_Point raw = ts.getPoint();
    int sx, sy;
    mapTouchToScreen(raw, sx, sy);
    if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
      currentScreen = SCREEN_PAYMENT_METHOD;
      drawPaymentMethodScreen();
    }
  }
}

