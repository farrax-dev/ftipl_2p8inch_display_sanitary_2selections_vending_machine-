// =====================================================
//     CUSTOMER-SCREEN IDLE TIMEOUT
// =====================================================
// A customer who walks away part-way through choosing is sent back to the
// Welcome screen after idleTimeoutSec seconds without a touch. Applies to the
// three customer screens that have a Back button:
//
//   Select Products, Your Cart, Payment Method
//
// and deliberately NOT to the Cash / UPI / RFID payment screens, which have
// their own timeouts (and their own reasons: a half-paid session, a QR code
// with a bank-side expiry), nor to Welcome or any admin screen.
//
// Admin > Machine > Customer Screens controls it (settings live in
// Core_09_Storage.ino):
//   idleTimeoutOn   - master switch. Off = customers are never timed out.
//   idleTimeoutSec  - seconds without a touch before the warning and the
//                     return to Welcome.
//   idleShowTimer   - whether the remaining seconds are drawn on screen. The
//                     clock runs and times the customer out either way; this
//                     only hides or shows the number.
//
// "Activity" is lastTouchTime (Core_05_TouchInput.ino), which every screen
// handler already stamps on each debounced touch, plus entering a new screen.

unsigned long idleScreenSince = 0;     // when the current screen was entered
AppScreen     idleLastScreen = SCREEN_WELCOME;
unsigned long idleSeenTouch = 0;       // last lastTouchTime value noticed
int           idleShownSec = -1;       // countdown value currently on screen

bool idleScreenApplies(int s) {
  return s == SCREEN_SELECT || s == SCREEN_CART_REVIEW || s == SCREEN_PAYMENT_METHOD;
}

// The gradient's colour at one scanline — the same maths as
// drawGradientBackground(), so the countdown's patch of background can be
// repainted without redrawing the whole screen.
uint16_t idleGradientColorAt(int y) {
  uint8_t r1, g1, b1, r2, g2, b2;
  color565toRGB(COL_BG_TOP, r1, g1, b1);
  color565toRGB(COL_BG_BOTTOM, r2, g2, b2);
  float t = smoothstep01((float)y / (float)tft.height());
  return tft.color565(r1 + t * (r2 - r1), g1 + t * (g2 - g1), b1 + t * (b2 - b1));
}

// Seconds remaining, drawn in the gap between the Back and Proceed buttons
// (x=144..177), which every one of the three screens leaves empty.
void drawIdleCountdown(int remainingSec) {
  const int gx = 144, gw = 34;
  for (int y = BTN_Y; y <= BTN_Y + BTN_H + 2; y++) {
    tft.drawFastHLine(gx, y, gw, idleGradientColorAt(y));
  }
  char buf[8];
  snprintf(buf, sizeof(buf), "%ds", remainingSec);
  auiPut(buf, AL_CENTER, gx, gw, BTN_Y + BTN_H / 2, remainingSec <= 10 ? COL_DANGER : COL_TEXT, FACE_XS);
}

void showIdleTimeoutWarning() {
  drawGradientBackground();
  drawSerifLine("Time Out!", 0, tft.width(), 75, COL_DANGER, 0);
  drawSerifLine("No activity for a while", 0, tft.width(), 125, COL_TEXT, 1);
  drawSerifLine("Returning to the home screen", 0, tft.width(), 155, COL_TEXT, 1);
  delay(2000);
}

void maintainIdleTimeout() {
  if (currentScreen != idleLastScreen) {
    idleLastScreen = currentScreen;
    idleScreenSince = millis();
    idleShownSec = -1;
  }
  if (!idleScreenApplies(currentScreen)) return;

  // A touch also restarts the countdown, and any redraw it caused has wiped
  // the number, so it must be drawn again even if the value is unchanged.
  if (lastTouchTime != idleSeenTouch) {
    idleSeenTouch = lastTouchTime;
    idleShownSec = -1;
  }

  if (!idleTimeoutOn) return;

  unsigned long activity = max(idleScreenSince, lastTouchTime);
  unsigned long elapsedSec = (millis() - activity) / 1000UL;
  int remaining = idleTimeoutSec - (int)elapsedSec;

  if (remaining <= 0) {
    Serial.printf("Idle timeout: %s screen idle for %ds, returning to Welcome\n",
                  currentScreen == SCREEN_SELECT ? "Select" :
                  currentScreen == SCREEN_CART_REVIEW ? "Cart" : "Payment Method",
                  idleTimeoutSec);
    showIdleTimeoutWarning();
    resetCart();
    currentScreen = SCREEN_WELCOME;
    idleLastScreen = SCREEN_WELCOME;
    drawWelcomeScreen();
    return;
  }

  if (idleShowTimer && remaining != idleShownSec) {
    idleShownSec = remaining;
    drawIdleCountdown(remaining);
  }
}
