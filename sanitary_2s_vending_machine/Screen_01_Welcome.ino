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
  tft.fillRect(0, 0, WELCOME_CLOCK_W, WELCOME_STRIP_H, COL_STRIP_BG);

  char timeBuf[16], dateBuf[24];
  if (getClockStrings(timeBuf, sizeof(timeBuf), dateBuf, sizeof(dateBuf))) {
    tft.setTextSize(2);
    tft.setTextColor(COL_STRIP_TIME, COL_STRIP_BG);
    leftText(timeBuf, 10, 8);

    tft.setTextSize(2);
    tft.setTextColor(COL_STRIP_DATE, COL_STRIP_BG);
    rightText(dateBuf, WELCOME_CLOCK_W - 8, 8);

    lastClockMinute = currentMinuteStamp();
  } else {
    tft.setTextSize(2);
    tft.setTextColor(COL_STRIP_TIME, COL_STRIP_BG);
    leftText("--:--", 10, 8);

    tft.setTextSize(2);
    tft.setTextColor(COL_STRIP_TIME, COL_STRIP_BG);
    rightText((WiFi.status() == WL_CONNECTED) ? TXT_SYNCING : TXT_NO_NETWORK,
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
  tft.fillRoundRect(x, y, w, h, 16, COL_CARD_FILL);
  tft.drawRoundRect(x, y, w, h, 16, COL_CARD_BORDER);
  tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 15, COL_CARD_RING);

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
  // All lines (title and subtitle) share ONE font and ONE colour. The large face is used unless
  // some line is too wide for the card, in which case every line steps down together.
  const char* lines[3];
  int n = 0;
  if (has1)   lines[n++] = CFG_TITLE_LINE_1;
  if (has2)   lines[n++] = CFG_TITLE_LINE_2;
  if (hasSub) lines[n++] = CFG_TITLE_SUBTITLE;

  tft.setTextSize(1);
  const GFXfont* titleFont = &UI_FONT_LG;
  for (int pass = 0; pass < 2; pass++) {
    tft.setFont(titleFont);
    bool fits = true;
    for (int k = 0; k < n; k++) {
      int16_t bx, by; uint16_t bw, bh;
      tft.getTextBounds(lines[k], 0, 0, &bx, &by, &bw, &bh);
      if ((int)bw > w - 16) fits = false;
    }
    if (fits || titleFont == &UI_FONT_MD) break;
    titleFont = &UI_FONT_MD;
  }
  // The font's own line-height metric, not a guess.
  const int BIG_H = titleFont->yAdvance;
  int cursorY = y + (h - n * BIG_H) / 2;

  tft.setTextColor(COL_CARD_TITLE, COL_CARD_FILL);
  for (int k = 0; k < n; k++) { drawBigTitleLine(lines[k], cursorY); cursorY += BIG_H; }
  tft.setFont();  // back to the default font
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
  uint16_t accent = outOfStock ? COL_BTN_DANGER_FILL : COL_PILL_FILL;
  uint16_t fill = (outOfStock || visible) ? accent : COL_PILL_HOLLOW;
  tft.fillRoundRect(WELCOME_PILL_X, WELCOME_PILL_Y, WELCOME_PILL_W, WELCOME_PILL_H, 18, fill);
  tft.drawRoundRect(WELCOME_PILL_X, WELCOME_PILL_Y, WELCOME_PILL_W, WELCOME_PILL_H, 18, accent);
  tft.setTextSize(2);
  tft.setTextColor((outOfStock || visible) ? COL_PILL_TEXT : accent, fill);
  centerTextInBox(outOfStock ? TXT_OUT_OF_STOCK : TXT_TOUCH_TO_CONTINUE,
                  WELCOME_PILL_Y + 11, WELCOME_PILL_X, WELCOME_PILL_W);
}

// ---------- Blink control (footer prompt pulse) ----------
unsigned long lastBlinkTime = 0;
bool blinkVisible = true;
const unsigned long BLINK_INTERVAL = 600;

void drawWelcomeScreen() {
  drawGradientBackground();

  tft.fillRect(0, 0, tft.width(), WELCOME_STRIP_H, COL_STRIP_BG);
  tft.drawFastHLine(0, WELCOME_STRIP_H + 1, tft.width(), COL_STRIP_LINE);

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
