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
const unsigned long CASH_TIMEOUT_MS   = (unsigned long)CFG_CASH_TIMEOUT_SEC * 1000UL;  // give up if nothing happens

// The acceptor emits a false pulse while its electronics power up (it was
// being counted as Rs 1 with no coin inserted). All pulses are ignored for
// this long after the relay switches on. Increase it if a phantom Rs 1 still
// appears; lower it (e.g. 500) if customers need to insert a coin sooner.
const unsigned long COIN_POWER_SETTLE_MS = 1000;

volatile uint32_t coinISRPulseCount = 0;
volatile unsigned long coinISRLastPulseMs = 0;
volatile unsigned long coinISRIgnoreUntilMs = 0;  // pulses before this time are power-up noise

// Real coin pulses are 20-100ms wide, so anything arriving within 5ms of the
// last edge is contact bounce or noise, never a second coin. Without this a
// single slow edge crossing the threshold could register a dozen counts.
const unsigned long COIN_DEBOUNCE_MS = 5;

void IRAM_ATTR coinPulseISR() {
  unsigned long now = millis();
  if ((long)(now - coinISRIgnoreUntilMs) < 0) return;  // still settling after power-on
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
  Serial.printf("coin acceptor: listening %s\n", on ? "ON" : "OFF");

  if (on) {
    // Start the settle window BEFORE power is applied, so the power-up
    // glitch pulse is never counted.
    coinISRIgnoreUntilMs = millis() + COIN_POWER_SETTLE_MS;
  }

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
unsigned long cashLastTimeoutLineUpdate = 0;
char cashStatusMsg[32] = "";
uint16_t cashStatusColor = COL_TEXT;

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
  // Still settling after power-on: throw away anything seen and report nothing
  if ((long)(millis() - coinISRIgnoreUntilMs) < 0) {
    noInterrupts();
    coinISRPulseCount = 0;
    interrupts();
    coinLastSeenISRCount = 0;
    coinPulseCount = 0;
    coinBurstActive = false;
    return 0;
  }

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
  Serial.printf("coin acceptor: %d pulses -> Rs %d\n", pulses, value);
  return value;
}

// Total / Paid chips plus the big "Remaining" card — they all change together
// every time a coin is credited, so they are wiped and redrawn as one region
// (y=36..125).
void drawCashChip(int x, int w, const char* text, uint16_t textColor) {
  tft.fillRoundRect(x, 38, w, 22, 6, COL_CARD);
  tft.drawRoundRect(x, 38, w, 22, 6, COL_CARD_BRD);
  drawSerifLine(text, x, w, 38 + 11, textColor, 1);
}

void drawCashProgressLines() {
  tft.fillRect(0, 36, tft.width(), 90, COL_BG_BOTTOM);

  int remaining = max(cashAmountDue, 0);
  int paid = orderTotal - remaining;

  char buf[24];
  snprintf(buf, sizeof(buf), "Total Rs. %d", orderTotal);
  drawCashChip(10, 148, buf, COL_TEXT);
  snprintf(buf, sizeof(buf), "Paid Rs. %d", paid);
  drawCashChip(162, 148, buf, COL_SUCCESS);

  // Remaining card: a label strip over the amount, in the accent colour that
  // marks "this is the thing to act on" everywhere else in the app.
  const int RX = 10, RY = 64, RW = 300, RH = 58;
  drawCardShadow(RX, RY, RW, RH, 10);
  tft.fillRoundRect(RX, RY, RW, RH, 10, COL_CARD);
  tft.drawRoundRect(RX, RY, RW, RH, 10, COL_CARD_BRD);
  tft.drawRoundRect(RX + 1, RY + 1, RW - 2, RH - 2, 9, COL_ACCENT);

  drawSerifLine("Remaining", RX, RW, RY + 14, COL_TEXT, 1);

  char amountBuf[16];
  snprintf(amountBuf, sizeof(amountBuf), "Rs. %d", remaining);
  drawSerifLine(amountBuf, RX, RW, RY + 41, COL_ACCENT, 0);
}

// CASH_TIMEOUT_MS is otherwise invisible to the customer — unlike the UPI
// screen's own "(Ns)" countdown, nothing here used to show how long an
// idle coin acceptor had left before the screen gave up and sent them back
// to Welcome. Sits in the gap between the status line (ends y=168) and the
// button bar (BTN_Y=190).
void drawCashTimeoutLine() {
  tft.fillRect(0, 170, tft.width(), 18, COL_BG_BOTTOM);
  int remainingSec = (int)((CASH_TIMEOUT_MS - (millis() - cashEnteredTime)) / 1000);
  if (remainingSec < 0) remainingSec = 0;
  char buf[24];
  snprintf(buf, sizeof(buf), "Time left: %ds", remainingSec);
  drawSerifLine(buf, 0, tft.width(), 179, COL_TEXT, 1);
}

void drawCashStatusLine() {
  // Size 2 instead of 1 — this is the one line that reports what just
  // happened to the coin you dropped in, so it earns more presence than an
  // ordinary caption. Every message it carries ("+Rs N credited", "Coin not
  // recognized", "That coin is not accepted") comfortably fits 320px at
  // this size.
  tft.fillRect(0, 148, tft.width(), 20, COL_BG_BOTTOM);
  if (cashStatusMsg[0] != '\0') {
    // A pill behind the message so the green/red status reads clearly against
    // the pink background.
    // Measured in the same face the text is drawn in, so the pill hugs it.
    int16_t mx, my; uint16_t mw, mh;
    tft.setTextSize(1);
    tft.setFont(&FreeSerifBold12pt7b);
    tft.getTextBounds(cashStatusMsg, 0, 0, &mx, &my, &mw, &mh);
    tft.setFont();
    int tw = min((int)mw + 20, (int)tft.width() - 4);
    int tx = (tft.width() - tw) / 2;
    tft.fillRoundRect(tx, 147, tw, 22, 6, COL_CARD);
    tft.drawRoundRect(tx, 147, tw, 22, 6, cashStatusColor);
    drawSerifLine(cashStatusMsg, tx, tw, 158, cashStatusColor, 1);
  }
}

// Swaps in a "no refund" note once coins start arriving, since there is no
// hardware path to give a partial payment back.
void drawCashBackButtonArea() {
  if (cashAnyCoinAccepted) {
    tft.fillRoundRect(BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H, 8, COL_BG_TOP);
    drawSerifLine("Paying, no refund", BTN_BACK_X, BTN_BACK_W, BTN_Y + BTN_H / 2, COL_TEXT, 2);
  } else {
    drawBackButton("< Back");
  }
}

void drawPaymentCashScreen() {
  cashAmountDue = orderTotal;
  cashAnyCoinAccepted = false;
  cashStatusMsg[0] = '\0';
  cashStatusColor = COL_TEXT;
  cashEnteredTime = millis();
  cashLastTimeoutLineUpdate = millis();
  resetCoinAcceptorState();
  coinAcceptorListen(true);

  drawGradientBackground();
  drawScreenTitle("Insert Coins");

  drawCashProgressLines();

  drawCashStatusLine();
  drawCashTimeoutLine();
  drawCashBackButtonArea();
}

void handlePaymentCashScreen() {
  int credited = pollCoinAcceptor();

  if (millis() - cashLastTimeoutLineUpdate > 1000) {
    cashLastTimeoutLineUpdate = millis();
    drawCashTimeoutLine();
  }

  if (credited > 0) {
    cashAmountDue -= credited;
    bool firstCoin = !cashAnyCoinAccepted;
    cashAnyCoinAccepted = true;
    cashEnteredTime = millis();  // coins are still coming in, keep the session alive
    snprintf(cashStatusMsg, sizeof(cashStatusMsg), "+Rs. %d credited", credited);
    cashStatusColor = COL_SUCCESS;

    if (cashAmountDue <= 0) {
      Serial.printf("coin acceptor: payment complete, Rs %d order paid (Rs %d overpaid, no change given)\n",
                    orderTotal, -cashAmountDue);
      // Power the acceptor down before dispensing so it cannot take any
      // further coins once the amount due has been met.
      coinAcceptorListen(false);
      bool dispensedOk = dispenseCart("Cash", false);

      if (dispensedOk) {
        drawDispensedScreen();
      } else {
        drawGradientBackground();
        drawSerifLine("Dispense", 0, tft.width(), 70, COL_DANGER, 0);
        drawSerifLine("Failed", 0, tft.width(), 108, COL_DANGER, 0);
        drawSerifLine("Please contact support", 0, tft.width(), 150, COL_TEXT, 1);
      }
      delay(2500);

      resetCart();
      currentScreen = SCREEN_WELCOME;
      drawWelcomeScreen();
      return;
    }

    drawCashProgressLines();
    drawCashStatusLine();
    if (firstCoin) drawCashBackButtonArea();
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
    Serial.printf("coin acceptor: cash screen timed out, Rs %d of Rs %d still due\n",
                  cashAmountDue, orderTotal);
    coinAcceptorListen(false);

    // Same "say what happened, then go home" shape as a successful dispense
    // above — any coins already credited are forfeited either way (no
    // hardware refund path), so there's nothing left for the customer to do
    // here but see why the screen is clearing.
    // Drawn exactly like drawUPITimeoutScreen() (Screen_05_PaymentUPI.ino) so
    // every payment timeout looks the same: the Welcome screen's serif face.
    drawGradientBackground();
    drawSerifLine("Payment", 0, tft.width(), 70, COL_DANGER, 0);
    drawSerifLine("Timed Out", 0, tft.width(), 108, COL_DANGER, 0);
    drawSerifLine("No payment was received in time.", 0, tft.width(), 150, COL_TEXT, 1);
    delay(2000);

    resetCart();
    currentScreen = SCREEN_WELCOME;
    drawWelcomeScreen();
    return;
  }

  if (isRealTouch() && millis() - lastTouchTime > TOUCH_DEBOUNCE) {
    lastTouchTime = millis();
    TS_Point raw = ts.getPoint();
    int sx, sy;
    mapTouchToScreen(raw, sx, sy);
    if (!cashAnyCoinAccepted && pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
      Serial.println("coin acceptor: cash payment cancelled by user (Back, no coins accepted)");
      coinAcceptorListen(false);
      backFromPayment();
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
// How long the Tap RFID Card screen waits before giving up. Follows the
// admin's Customer Screens timeout (Admin > Machine > Customer, seconds and
// show-countdown) so every customer screen times out by the same rule; when
// the admin has switched the timeout off, Config.h's CFG_RFID_TIMEOUT_SEC
// still applies so this screen can never sit open forever.
unsigned long rfidPayTimeoutMs() {
  int sec = idleTimeoutOn ? idleTimeoutSec : CFG_RFID_TIMEOUT_SEC;
  return (unsigned long)sec * 1000UL;
}

// Independent of the idle timeout above, which an actual tap — even a
// rejected one — resets (see "an actual tap counts as activity" below).
// That's fine for a customer genuinely trying different cards, but it means
// repeated rejections, or a reader misreading noise on its UART line as a
// phantom card present (the same class of floating-input failure this
// project already hit once on the coin pulse line before a pull-up was
// added), could keep this screen open indefinitely with the idle clock
// never allowed to expire. This caps the session at a fixed wall-clock
// ceiling no amount of activity can push back.
unsigned long rfidPayHardCapMs() { return rfidPayTimeoutMs() * 3; }

unsigned long rfidPayEnteredMs = 0;
unsigned long rfidPaySessionStartMs = 0;   // set once on entry, never reset by activity
unsigned long rfidLastTimeoutLineUpdate = 0;
char rfidStatusMsg[32] = "";
uint16_t rfidStatusColor = COL_TEXT;
unsigned long rfidStatusUntilMs = 0;  // rejection message clears itself, same as the coin screen

void drawRFIDStatusLine() {
  tft.fillRect(0, 146, tft.width(), 22, COL_BG_BOTTOM);
  if (rfidStatusMsg[0] != '\0') {
    // Pill sized to the message in the same serif face it is drawn in.
    int16_t mx, my; uint16_t mw, mh;
    tft.setTextSize(1);
    tft.setFont(&FreeSerifBold12pt7b);
    tft.getTextBounds(rfidStatusMsg, 0, 0, &mx, &my, &mw, &mh);
    tft.setFont();
    int tw = min((int)mw + 20, (int)tft.width() - 4);
    int tx = (tft.width() - tw) / 2;
    tft.fillRoundRect(tx, 147, tw, 22, 6, COL_CARD);
    tft.drawRoundRect(tx, 147, tw, 22, 6, rfidStatusColor);
    drawSerifLine(rfidStatusMsg, tx, tw, 158, rfidStatusColor, 1);
  }
}

// Same visibility the cash screen's own countdown gives CASH_TIMEOUT_MS —
// without it, a customer (or anyone testing this) has no way to see the
// idle timeout actually counting down, or to tell "it's about to time out"
// apart from "this is broken and never will."
void drawRFIDTimeoutLine() {
  tft.fillRect(0, 170, tft.width(), 18, COL_BG_BOTTOM);
  if (!idleShowTimer) return;   // hidden by the admin; the timeout itself still runs
  int remainingSec = (int)((rfidPayTimeoutMs() - (millis() - rfidPayEnteredMs)) / 1000);
  if (remainingSec < 0) remainingSec = 0;
  char buf[24];
  snprintf(buf, sizeof(buf), "Time left: %ds", remainingSec);
  drawSerifLine(buf, 0, tft.width(), 179, COL_TEXT, 1);
}

void drawPaymentRFIDScreen() {
  rfidPayEnteredMs = millis();
  rfidPaySessionStartMs = millis();
  rfidLastTimeoutLineUpdate = millis();
  rfidStatusMsg[0] = '\0';

  drawGradientBackground();
  drawScreenTitle("Tap RFID Card");

  // One card holding the message, in the same style as the cash screen's
  // Remaining card: accent heading lines over a dark instruction line.
  const int CX = 10, CY = 44, CW = 300, CH = 96;
  drawCardShadow(CX, CY, CW, CH, 10);
  tft.fillRoundRect(CX, CY, CW, CH, 10, COL_CARD);
  tft.drawRoundRect(CX, CY, CW, CH, 10, COL_CARD_BRD);
  tft.drawRoundRect(CX + 1, CY + 1, CW - 2, CH - 2, 9, COL_ACCENT);

  drawSerifLine("No charge for", CX, CW, CY + 22, COL_ACCENT, 0);
  drawSerifLine("registered cards", CX, CW, CY + 51, COL_ACCENT, 0);
  drawSerifLine("Hold your card near the reader", CX, CW, CY + 80, COL_TEXT, 1);
  drawRFIDStatusLine();
  drawRFIDTimeoutLine();
  drawBackButton("< Back");
}

void handlePaymentRFIDScreen() {
  if (rfidStatusMsg[0] != '\0' && millis() >= rfidStatusUntilMs) {
    rfidStatusMsg[0] = '\0';
    drawRFIDStatusLine();
  }

  if (millis() - rfidLastTimeoutLineUpdate > 1000) {
    rfidLastTimeoutLineUpdate = millis();
    drawRFIDTimeoutLine();
  }

  char uid[21];
  if (pollRFIDCard(uid, sizeof(uid))) {
    int cardIdx = findRFIDCardIndex(uid);
    // withdrawLimit == 0 is unlimited (the default for a freshly-registered
    // card); otherwise this cart must fit in what's left of the card's
    // running allowance — see Screen_19_AdminRFIDCardEdit.ino.
    //
    // hasLimit is checked separately from remaining so that an admin
    // lowering withdrawLimit below the card's already-used total doesn't
    // flip it into looking unlimited: a raw negative "remaining" and the
    // true unlimited sentinel (-1 below) used to both satisfy
    // `remaining >= 0` as false, so overLimit went false either way.
    // Clamping remaining to 0 in that case blocks the card instead of
    // waving it through.
    bool hasLimit = (cardIdx >= 0 && rfidCards[cardIdx].withdrawLimit > 0);
    int remaining = hasLimit
                       ? rfidCards[cardIdx].withdrawLimit - rfidCards[cardIdx].withdrawUsed
                       : -1;
    bool overLimit = hasLimit && cartTotalQty() > max(remaining, 0);

    if (cardIdx >= 0 && !overLimit) {
      int qty = cartTotalQty();
      bool dispensedOk = dispenseCart("RFID", true);  // freeVend — see Core_11_Dispense.ino
      // Only a successful dispense actually consumes the card's allowance —
      // a jam/sensor failure already shows "Dispense Failed" below; debiting
      // the allowance too would charge the customer's limit for product
      // they never got, with no way to refund it back onto the card.
      if (dispensedOk) addRFIDCardUsage(cardIdx, qty);

      if (dispensedOk) {
        drawDispensedScreen();
      } else {
        drawGradientBackground();
        drawSerifLine("Dispense Failed", 0, tft.width(), 80, COL_DANGER, 0);
        drawSerifLine("Please contact support", 0, tft.width(), 125, COL_TEXT, 1);
      }
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

  if (millis() - rfidPayEnteredMs > rfidPayTimeoutMs()
      || millis() - rfidPaySessionStartMs > rfidPayHardCapMs()) {
    drawGradientBackground();
    drawSerifLine("Payment", 0, tft.width(), 70, COL_DANGER, 0);
    drawSerifLine("Timed Out", 0, tft.width(), 108, COL_DANGER, 0);
    drawSerifLine("No card was tapped in time.", 0, tft.width(), 150, COL_TEXT, 1);
    delay(2000);

    resetCart();
    currentScreen = SCREEN_WELCOME;
    drawWelcomeScreen();
    return;
  }

  if (isRealTouch() && millis() - lastTouchTime > TOUCH_DEBOUNCE) {
    lastTouchTime = millis();
    TS_Point raw = ts.getPoint();
    int sx, sy;
    mapTouchToScreen(raw, sx, sy);
    if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
      backFromPayment();
    }
  }
}
