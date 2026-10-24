// =====================================================
//                  WELCOME SCREEN
// =====================================================
const int WELCOME_STRIP_H  = 30;
const int WELCOME_CLOCK_W  = 292;   // stop short of the WiFi icon at x=298

const int WELCOME_PILL_X = 40, WELCOME_PILL_Y = 190;
const int WELCOME_PILL_W = 240, WELCOME_PILL_H = 36;

int lastClockMinute = -2;
unsigned long lastClockRefresh = 0;

void drawWelcomeClock() {
  tft.fillRect(0, 0, WELCOME_CLOCK_W, WELCOME_STRIP_H, COL_BG_TOP);

  char timeBuf[16], dateBuf[24];
  if (getClockStrings(timeBuf, sizeof(timeBuf), dateBuf, sizeof(dateBuf))) {
    tft.setTextSize(2);
    tft.setTextColor(COL_TEXT, COL_BG_TOP);
    tft.setCursor(10, 8);
    tft.print(timeBuf);

    tft.setTextSize(2);
    tft.setTextColor(COL_ACCENT, COL_BG_TOP);
    rightText(dateBuf, WELCOME_CLOCK_W - 8, 8);

    lastClockMinute = currentMinuteStamp();
  } else {
    tft.setTextSize(2);
    tft.setTextColor(COL_TEXT_DIM, COL_BG_TOP);
    tft.setCursor(10, 8);
    tft.print("--:--");

    tft.setTextSize(2);
    tft.setTextColor(COL_TEXT_DIM, COL_BG_TOP);
    rightText((WiFi.status() == WL_CONNECTED) ? "Syncing..." : "No network",
              WELCOME_CLOCK_W - 8, 8);

    lastClockMinute = -1;
  }
}

// Centers one line of the welcome title (FreeSerifBold18pt7b, already active)
// horizontally on screen with its TOP at topY. Same getTextBounds()-based
// top-to-baseline conversion as drawHeaderTitle() (Core_08_UIHelpers.ino),
// just centered on the full screen width instead of a given box — there's
// no dedicated centerTextInBox()-for-custom-fonts helper since this is the
// only place that needs one.
void drawBigTitleLine(const char* text, int topY) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((tft.width() - (int)w) / 2, topY - y1);
  tft.print(text);
}

void drawWelcomeCard() {
  int x = 16, y = 40, w = tft.width() - 32, h = 138;

  drawCardShadow(x, y, w, h, 16);
  tft.fillRoundRect(x, y, w, h, 16, COL_CARD);
  tft.drawRoundRect(x, y, w, h, 16, COL_CARD_BRD);
  tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 15, COL_ACCENT);

  // Title comes from Config.h. Any of the three lines may be blank, so the
  // block is centred on whatever is actually set rather than at fixed
  // positions — a one-line title would otherwise sit high on the card with
  // an obvious empty gap beneath it.
  bool has1 = (strlen(CFG_TITLE_LINE_1) > 0);
  bool has2 = (strlen(CFG_TITLE_LINE_2) > 0);
  bool hasSub = (strlen(CFG_TITLE_SUBTITLE) > 0);

  // Serif, not sans — same family as the screen title strips
  // (Core_08_UIHelpers.ino's drawHeaderTitle()) so every heading in the app
  // reads as one typographic voice. 18pt, not 24pt — 24pt ran a line like
  // "ALL PURPOSE" past the card's edge; 18pt is the next size down in the
  // bundled Free* set.
  // Size pinned to 1 before the face is set, for the reason spelled out in
  // drawHeaderTitle() (Core_08_UIHelpers.ino): setTextSize() scales a custom
  // font too, so 18pt at a leftover size 2 is 36pt and off the card. This
  // card has always rendered right, but only because drawWelcomeClock() runs
  // first and happens to leave the size at 1 — that is luck, not a contract.
  tft.setTextSize(1);
  tft.setFont(&FreeSerifBold18pt7b);
  // The font's own line-height metric, not a guess — replaces the old
  // "line height at text size 4" constant, which was tuned for the default
  // bitmap font and would be wrong for this one.
  const int BIG_H = FreeSerifBold18pt7b.yAdvance;
  const int SUB_H = 30;    // subtitle line plus the gap above it (default font, unchanged)
  int blockH = (has1 ? BIG_H : 0) + (has2 ? BIG_H : 0) + (hasSub ? SUB_H : 0);
  int cursorY = y + (h - blockH) / 2;

  // Single draw, not the old double-offset faux-bold — FreeSerifBold is
  // already a bold weight, so stacking the faux-bold trick on top of it
  // over-thickens the strokes at this size.
  tft.setTextColor(COL_TEXT, COL_CARD);
  if (has1) { drawBigTitleLine(CFG_TITLE_LINE_1, cursorY); cursorY += BIG_H; }
  if (has2) { drawBigTitleLine(CFG_TITLE_LINE_2, cursorY); cursorY += BIG_H; }
  tft.setFont();  // back to the default font before the subtitle

  if (hasSub) {
    tft.setTextSize(2);
    tft.setTextColor(COL_ACCENT, COL_CARD);
    centerText(CFG_TITLE_SUBTITLE, cursorY + 10);
  }
}

// Pulses between a filled and an outlined pill instead of blinking on/off.
//
// Once every enabled product has run out (allProductsOutOfStock(),
// Core_09_Storage.ino), this stops pulsing and says so instead — a steady
// danger-coloured "Out of Stock" rather than a blinking "come try me" that
// would otherwise keep inviting a touch into a purchase flow that has
// nothing left to sell. Still called on the same 600ms timer as before
// (handleWelcomeScreen()); it just draws the same steady state every time
// while this is true; the visible parameter only matters again once stock
// is back and it resumes pulsing.
void drawFooterPrompt(bool visible) {
  // No drawCardShadow() here on purpose: this redraws in place on every
  // blink (600ms) without clearing a bounding box first, and the shadow
  // sits a few px outside the pill's own footprint — a shadow drawn on the
  // filled phase would leave a stale sliver uncovered once the hollow phase
  // redraws only the pill's own rect. Safe only for a draw-once card, like
  // drawWelcomeCard()'s.
  bool outOfStock = allProductsOutOfStock();
  uint16_t accent = outOfStock ? COL_DANGER : COL_ACCENT;
  uint16_t fill = (outOfStock || visible) ? accent : COL_BG_TOP;
  tft.fillRoundRect(WELCOME_PILL_X, WELCOME_PILL_Y, WELCOME_PILL_W, WELCOME_PILL_H, 18, fill);
  tft.drawRoundRect(WELCOME_PILL_X, WELCOME_PILL_Y, WELCOME_PILL_W, WELCOME_PILL_H, 18, accent);
  tft.setTextSize(2);
  tft.setTextColor((outOfStock || visible) ? COL_BG_TOP : accent, fill);
  centerTextInBox(outOfStock ? "Out of Stock" : "Touch to Continue",
                  WELCOME_PILL_Y + 11, WELCOME_PILL_X, WELCOME_PILL_W);
}

// ---------- Blink control (footer prompt pulse) ----------
unsigned long lastBlinkTime = 0;
bool blinkVisible = true;
const unsigned long BLINK_INTERVAL = 600;

void drawWelcomeScreen() {
  drawGradientBackground();

  tft.fillRect(0, 0, tft.width(), WELCOME_STRIP_H, COL_BG_TOP);
  tft.drawFastHLine(0, WELCOME_STRIP_H + 1, tft.width(), COL_CARD_BRD);

  drawWelcomeClock();
  drawWelcomeCard();
  drawFooterPrompt(true);

  lastBlinkTime    = millis();
  lastClockRefresh = millis();
  blinkVisible     = true;
}

// ---------- Hold-in-corner gesture to reach Admin Login ----------
const int ADMIN_CORNER_SIZE = 40;
const unsigned long ADMIN_HOLD_MS = 2000;
bool cornerPressActive = false;
unsigned long cornerPressStartTime = 0;

bool isInCorner(int sx, int sy) {
  int w = tft.width();
  int h = tft.height();
  bool left   = sx < ADMIN_CORNER_SIZE;
  bool right  = sx > w - ADMIN_CORNER_SIZE;
  bool top    = sy < ADMIN_CORNER_SIZE;
  bool bottom = sy > h - ADMIN_CORNER_SIZE;
  return (left || right) && (top || bottom);
}

void onWelcomeTouched() {
  // Everything is sold out: the screen stays locked on Welcome.
  if (allProductsOutOfStock()) return;
  resetCart();
  currentScreen = SCREEN_SELECT;
  drawSelectScreen();
}

void handleWelcomeScreen() {
  if (millis() - lastBlinkTime > BLINK_INTERVAL) {
    blinkVisible = !blinkVisible;
    drawFooterPrompt(blinkVisible);
    lastBlinkTime = millis();
  }

  if (millis() - lastClockRefresh > 1000) {
    lastClockRefresh = millis();
    if (currentMinuteStamp() != lastClockMinute) drawWelcomeClock();

    // Scheduled emails are checked only here, on the idle screen. Sending
    // blocks for up to ~90s, which would be unacceptable mid-purchase but is
    // harmless while the machine is sitting waiting for a customer.
    maintainReportSchedule();

    // Same idle-only reasoning, though this one wouldn't actually block: the
    // RFID monthly usage reset (Core_09_Storage.ino) just checked here for
    // consistency with the rest of this machine's scheduled maintenance.
    maintainRFIDResetSchedule();
  }

  if (isRealTouch()) {
    TS_Point raw = ts.getPoint();
    int sx, sy;
    mapTouchToScreen(raw, sx, sy);

    if (isInCorner(sx, sy)) {
      if (!cornerPressActive) {
        cornerPressActive = true;
        cornerPressStartTime = millis();
      } else if (millis() - cornerPressStartTime >= ADMIN_HOLD_MS) {
        cornerPressActive = false;
        lastTouchTime = millis();
        enterAdminLogin();
      }
      return;
    }

    cornerPressActive = false;
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      onWelcomeTouched();
    }
  } else if (cornerPressActive) {
    cornerPressActive = false;
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      onWelcomeTouched();
    }
  }
}
