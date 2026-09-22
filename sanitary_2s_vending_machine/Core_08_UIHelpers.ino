// ---------- Shared bottom button bar ----------
const int BTN_Y = 190;
const int BTN_H = 35;
const int BTN_BACK_X = 20;
const int BTN_BACK_W = 120;
const int BTN_PROCEED_X = 180;
const int BTN_PROCEED_W = 120;

bool pointInRect(int px, int py, int rx, int ry, int rw, int rh) {
  return (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh);
}

// ---------- Card elevation ----------
// A real ILI9341 has no alpha blending, so a soft drop shadow isn't
// achievable the way it would be on a compositor — this fakes it with a
// single solid-color card, offset down-right and peeking out from behind
// the real one, in COL_SHADOW (Core_03_Theme.ino). Cheap (one extra
// fillRoundRect) and reads as "this card sits above the background" at
// arm's length on a 2.8" screen, which is what a flat card on a flat
// gradient can't do on its own.
//
// Called before the card's own fillRoundRect+drawRoundRect, never after —
// the shadow has to be underneath, not on top.
const int CARD_SHADOW_DX = 3, CARD_SHADOW_DY = 3;

void drawCardShadow(int x, int y, int w, int h, int r) {
  tft.fillRoundRect(x + CARD_SHADOW_DX, y + CARD_SHADOW_DY, w, h, r, COL_SHADOW);
}

// Convenience for the common case: a default-styled card (COL_CARD fill,
// COL_CARD_BRD border) with its shadow. A card that needs a different fill
// (a selected state, a status color) calls drawCardShadow() directly and
// draws its own fill/border on top — see Screen_02_Select.ino's product
// cards for that pattern.
void drawCard(int x, int y, int w, int h, int r) {
  drawCardShadow(x, y, w, h, r);
  tft.fillRoundRect(x, y, w, h, r, COL_CARD);
  tft.drawRoundRect(x, y, w, h, r, COL_CARD_BRD);
}

// ---------- Screen header ----------
// Every screen except Welcome wears the same header: a white band across the
// top, the screen's name in a serif face, and a rule under it that starts
// solid accent beneath the title and fades out towards the right edge.
//
// Three things were wrong with the version this replaces, and all three came
// from the same place — the header was sized per screen instead of designed
// once:
//
//   1. It was set in FreeSerifBold9pt7b, whose caps stand about 12px. Body
//      text on these screens is the default font at setTextSize(2), 14px.
//      The heading was literally smaller than the text it headed, which is
//      what makes a screen look unfinished no matter how tidy the rest is.
//      12pt (caps ~17px) puts the heading clearly above body text again, and
//      the widest title in the app ("UPI Configuration", 220px) still clears
//      the space left for it.
//   2. Every screen passed its own band geometry — 10/30 here, 8/24 there,
//      8/26 somewhere else — so the title jumped a few pixels up or down and
//      changed size as you moved between screens. The constants below are
//      now the only geometry there is, and drawScreenTitle() takes no
//      position arguments at all.
//   3. The band had no edge. A title floating on white above a cream
//      gradient reads as stray text; the rule gives it a base and separates
//      chrome from content.
//
// Anything a screen wants to keep in the band beside the title (a counter, a
// Cancel chip, the Admin Panel's nav chips) is drawn by that screen after
// this returns, and tells drawScreenTitle() how much of the right-hand side
// to keep clear so a long title can never run underneath it.
const int HDR_Y        = 3;                        // band top
const int HDR_H        = 27;                       // band height
const int HDR_X        = 16;                       // title left margin
const int HDR_RULE_Y   = HDR_Y + HDR_H;            // 30 — rule sits on the band's base
const int HDR_RULE_H   = 2;
const int HDR_BOTTOM   = HDR_RULE_Y + HDR_RULE_H;  // 32 — first row a screen may draw on
const int HDR_STATUS_W = 30;                       // right strip left clear for the WiFi/4G icon

// The two header faces. 12pt is the intended one; 9pt exists only as a
// fallback for a title that would otherwise overrun whatever the screen has
// reserved on the right — no current title needs it, but textEntryTitle and
// the "Edit Product N" style titles are built at runtime, so the guard stays.
//
// Two catches with a custom GFX font, and both have bitten this header:
//
// setTextSize() still applies. A custom font is not a fixed size — the
// multiplier scales it exactly the way it scales the built-in 5x7 bitmap
// font. Every screen signs off with setTextSize(2) (drawBackButton() and
// friends leave it there), so a header that only calls setFont() draws its
// title at DOUBLE size, measures it at double size too, and runs straight
// through whatever the screen reserved on its right — the Admin Panel's
// title disappearing under the Clock/Report/Setup chips, the Select
// Products title colliding with its counter. The Welcome screen escaped
// this only by luck: drawWelcomeClock() happens to leave the size at 1
// before drawWelcomeCard() sets its 18pt face. So size is pinned to 1 here
// explicitly, before the measurement, not left to whatever ran last.
//
// setCursor(x,y) means something different. The default bitmap font treats
// y as roughly the glyph TOP; a custom font treats it as the BASELINE, so
// the same y is a different vertical position depending on which is active.
// getTextBounds() measures the actual glyph box and the baseline is derived
// from that, which centers correctly whichever face is picked.
//
// Returns the x of the title's right edge, which is where the rule's solid
// section ends.
int drawHeaderTitle(const char* title, int availW) {
  int16_t x1, y1;
  uint16_t w, h;

  tft.setTextSize(1);
  tft.setFont(&FreeSerifBold12pt7b);
  tft.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  if ((int)w > availW) {
    tft.setFont(&FreeSerifBold9pt7b);
    tft.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  }

  tft.setTextColor(COL_TEXT, COL_BG_TOP);
  // y1 is the glyph top's offset from the baseline (negative — glyphs sit
  // above it), so subtracting it turns "desired top" into "baseline to pass
  // setCursor", the conversion centerTextInBox() doesn't need for the
  // default font's top-left convention.
  tft.setCursor(HDR_X, HDR_Y + (HDR_H - (int)h) / 2 - y1);
  tft.print(title);

  tft.setFont();       // back to the default bitmap font for the rest of
  tft.setTextSize(2);  // this screen — every other call site still expects it
  return HDR_X + (int)w;
}

// Solid accent from the left edge to just past the title, then a fade out
// across whatever is left. A rule of one flat color all the way across a
// 320px screen reads as a border — something boxing the content in; tapering
// it reads as an underline that belongs to the title, and it leaves the
// right-hand side of the band visually quiet for the counters and chips that
// live there.
//
// Fades to COL_BG_TOP, not COL_BG_BOTTOM: the rule sits 30px down a 240px
// gradient, where the background is still within a shade of white, so fading
// towards the cream at the bottom of that gradient would end the taper on a
// faint tan streak instead of on nothing.
void drawHeaderRule(int titleRight) {
  int w = tft.width();
  int solid = min(titleRight + 10, w);
  tft.fillRect(0, HDR_RULE_Y, solid, HDR_RULE_H, COL_ACCENT);

  uint8_t r1, g1, b1, r2, g2, b2;
  color565toRGB(COL_ACCENT, r1, g1, b1);
  color565toRGB(COL_BG_TOP, r2, g2, b2);

  int fadeW = w - solid;
  for (int k = 0; k < fadeW; k++) {
    float t = smoothstep01((float)k / (float)fadeW);
    uint8_t r = r1 + t * (r2 - r1);
    uint8_t g = g1 + t * (g2 - g1);
    uint8_t b = b1 + t * (b2 - b1);
    tft.drawFastVLine(solid + k, HDR_RULE_Y, HDR_RULE_H, tft.color565(r, g, b));
  }
}

// reservedRightW is how many pixels at the RIGHT edge of the band the caller
// intends to draw into itself — measured from x = tft.width() leftwards, so a
// chip whose left edge is at x=160 reserves 320-160 = 160. Never less than
// the status-icon strip, which every screen carries whether it asks or not.
void drawScreenTitle(const char* title, int reservedRightW) {
  // Cleared from y=0, not from HDR_Y — the few rows above the band would
  // otherwise keep the gradient and leave a faint seam along the top edge.
  tft.fillRect(0, 0, tft.width(), HDR_RULE_Y, COL_BG_TOP);
  // That clear runs over the bottom half of the WiFi/4G icon, and screens
  // that repaint only their header (Screen_02_Select.ino's refusal messages)
  // would otherwise leave it sliced in half until the next signal change.
  indicatorDirty = true;

  int reserved = max(reservedRightW, HDR_STATUS_W);
  drawHeaderRule(drawHeaderTitle(title, tft.width() - reserved - HDR_X));
}

void drawScreenTitle(const char* title) {
  drawScreenTitle(title, HDR_STATUS_W);
}

// Vertical position for a plain line of default-font text sitting in the
// header band beside the title — the counter on Select Products, the total on
// Your Cart. textSize is the setTextSize() value it will be drawn at.
int headerTextY(int textSize) {
  return HDR_Y + (HDR_H - 8 * textSize) / 2;
}

// Right edge to pass rightText() for anything in the band. Stops short of the
// status icon rather than at the screen edge — a right-aligned counter taken
// all the way to tft.width() - 12 runs its last character under the WiFi fan,
// and the icon repaints on its own poll so the two flicker over each other.
int headerRightX() {
  return tft.width() - HDR_STATUS_W - 4;
}

// ---------- Word-wrapped label text ----------
// The default font's own newline handling sends the cursor back to x=0 — the
// SCREEN's left edge, not the box's — so a label with a break in it lands its
// second line outside the card it belongs to, over whatever is to the left.
// This splits the string instead: it breaks on spaces, honours an explicit
// newline, and centers each line within the box.
//
// draw=false counts the lines without drawing any, which is what lets a
// caller center the whole block vertically before committing to a top edge.
// Text size is passed in; color is whatever the caller last set.
const int WRAP_LINE_GAP = 2;

int wrapTextInBox(const char* text, int boxX, int boxW, int topY, int size, bool draw) {
  int maxChars = boxW / (6 * size);
  if (maxChars < 1) maxChars = 1;
  int lineH = 8 * size + WRAP_LINE_GAP;

  int lines = 0;
  int i = 0;
  while (text[i] != '\0') {
    while (text[i] == ' ') i++;              // a line never starts on a space
    if (text[i] == '\n') { i++; continue; }
    if (text[i] == '\0') break;

    int start = i, lastSpace = -1, n = 0;
    while (text[i] != '\0' && text[i] != '\n' && n < maxChars) {
      if (text[i] == ' ') lastSpace = i;
      i++;
      n++;
    }
    int end = i;
    // Back up to the last space only when the cut landed mid-word — not when
    // the line ended at the string's end, at an explicit break, or on a space
    // that the next pass will skip anyway.
    if (text[i] != '\0' && text[i] != '\n' && text[i] != ' ' && lastSpace > start) {
      end = lastSpace;
      i = lastSpace;
    }
    while (end > start && text[end - 1] == ' ') end--;   // nothing to center around

    if (draw) {
      char buf[32];
      int len = end - start;
      if (len > (int)sizeof(buf) - 1) len = sizeof(buf) - 1;
      memcpy(buf, text + start, len);
      buf[len] = '\0';
      centerTextInBox(buf, topY + lines * lineH, boxX, boxW);
    }
    lines++;
    if (text[i] == '\n') i++;
  }
  return lines;
}

int wrapTextHeight(int lines, int size) {
  return (lines > 0) ? lines * (8 * size + WRAP_LINE_GAP) - WRAP_LINE_GAP : 0;
}

// ---------- Word-wrapped label text for a custom GFX font ----------
// wrapTextInBox() above assumes the default bitmap font's fixed 6px-wide
// cell (maxChars = boxW / (6*size)) — wrong for a proportional face like the
// FreeSerif set, where "W" and "i" aren't the same width. This measures each
// candidate line for real with getTextBounds() instead of counting
// characters, the same way drawHeaderTitle()/drawBigTitleLine() measure a
// single line.
//
// Always setTextSize(1) before touching the font — a custom GFXfont is one
// fixed point size; letting a leftover setTextSize() scale it multiplies the
// point size right along with it (see drawHeaderTitle()'s comment on that
// exact trap). Leaves the default bitmap font active on return, same
// contract as every other call site in this file.
//
// draw=false just counts lines/measures height, same as wrapTextInBox(),
// so a caller can decide whether a font choice fits before committing to it.
int wrapFontInBox(const GFXfont* font, const char* text, int boxX, int boxW, int topY, bool draw) {
  tft.setTextSize(1);
  tft.setFont(font);

  int lineH = font->yAdvance;
  int lines = 0;
  int len = strlen(text);
  int i = 0;

  while (i < len) {
    while (text[i] == ' ') i++;
    if (i >= len) break;

    int lineStart = i;
    int lineEnd = i;     // end of the best-fitting line found so far
    int probe = i;

    while (probe < len) {
      int wordEnd = probe;
      while (wordEnd < len && text[wordEnd] != ' ') wordEnd++;

      char tryBuf[40];
      int tryLen = wordEnd - lineStart;
      if (tryLen > (int)sizeof(tryBuf) - 1) tryLen = sizeof(tryBuf) - 1;
      memcpy(tryBuf, text + lineStart, tryLen);
      tryBuf[tryLen] = '\0';

      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds(tryBuf, 0, 0, &x1, &y1, &w, &h);

      // Adding this word would overflow the box, and the line already has
      // at least one word on it — stop before this word, not mid-word. A
      // single word too wide for the box on its own still gets drawn (and
      // overflows), same as a bitmap-font line that hits wrapTextInBox()'s
      // maxChars with nowhere to back up to.
      if ((int)w > boxW && lineEnd > lineStart) break;
      lineEnd = wordEnd;

      probe = wordEnd;
      while (probe < len && text[probe] == ' ') probe++;
    }

    if (draw) {
      char buf[40];
      int lineLen = lineEnd - lineStart;
      if (lineLen > (int)sizeof(buf) - 1) lineLen = sizeof(buf) - 1;
      memcpy(buf, text + lineStart, lineLen);
      buf[lineLen] = '\0';

      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
      // y1 is the glyph top's offset from the baseline (negative), same
      // top-to-baseline conversion as drawBigTitleLine()/drawHeaderTitle().
      tft.setCursor(boxX + (boxW - (int)w) / 2, topY + lines * lineH - y1);
      tft.print(buf);
    }

    lines++;
    i = probe;
  }

  tft.setFont();  // back to the default bitmap font, same contract as callers expect
  return lines;
}

int wrapFontHeight(int lines, const GFXfont* font) {
  return (lines > 0) ? lines * font->yAdvance : 0;
}

// Longer labels ("Exit Admin", "Motor Stock") don't fit BTN_*_W at a fixed
// text size 2 — a size-2 char cell is 12px, so anything past 8-ish
// characters runs past the button edge. fitTextSize() (below) picks the
// largest size that still fits, and centerTextInBox() centers it instead of
// the old fixed cursor offset, which assumed size 2.
void drawBackButton(const char* label) {
  drawCardShadow(BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H, 8);
  tft.fillRoundRect(BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H, 8, COL_CARD);
  tft.drawRoundRect(BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H, 8, COL_CARD_BRD);
  int sz = fitTextSize(label, BTN_BACK_W - 10, 2);
  tft.setTextSize(sz);
  tft.setTextColor(COL_TEXT, COL_CARD);
  centerTextInBox(label, BTN_Y + (BTN_H - 8 * sz) / 2, BTN_BACK_X, BTN_BACK_W);
}

void drawProceedButton(const char* label) {
  drawCardShadow(BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H, 8);
  tft.fillRoundRect(BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H, 8, COL_ACCENT);
  int sz = fitTextSize(label, BTN_PROCEED_W - 10, 2);
  tft.setTextSize(sz);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox(label, BTN_Y + (BTN_H - 8 * sz) / 2, BTN_PROCEED_X, BTN_PROCEED_W);
}

// ---------- Shared card-grid layout (Select Products + Payment Method) ----------
// Starts at 40, not the old 45: the header now ends at a known HDR_BOTTOM=32
// for every screen instead of wherever that screen's own band happened to
// stop, so the cards can take the slack that used to absorb the difference.
const int CARD_AREA_X = 15, CARD_AREA_Y = 40, CARD_AREA_W = 290, CARD_AREA_H = 145;
const int CARD_GAP = 8;

// Three columns makes a 91px card, and 91px leaves 79px of text width —
// six characters at text size 2. No real product name fits in six
// characters, which is why every name on the select screen was falling back
// to size 1 and ending up no bigger than the caption under it. Two columns
// makes a 141px card: ten characters a line at size 2, and a 68px card is
// tall enough for a name to take two of those lines when it needs to.
//
// Only up to 4 products, though. Five in two columns needs three rows, and
// a 43px row has no space for a two-line name above a price and a status
// line — so 5+ keeps the three-column grid it has today rather than trading
// a cramped name for a cramped card.
void getSelectCardRect(int k, int count, int &x, int &y, int &w, int &h) {
  int columns = (count <= 2) ? max(1, count) : (count <= 4) ? 2 : 3;
  int rows = (count + columns - 1) / columns;
  w = (CARD_AREA_W - (columns - 1) * CARD_GAP) / columns;
  h = (CARD_AREA_H - (rows - 1) * CARD_GAP) / rows;

  int row = k / columns;
  int col = k % columns;
  int itemsInRow = min(columns, count - row * columns);
  int rowW = itemsInRow * w + (itemsInRow - 1) * CARD_GAP;
  int rowX0 = CARD_AREA_X + (CARD_AREA_W - rowW) / 2;

  x = rowX0 + col * (w + CARD_GAP);
  y = CARD_AREA_Y + row * (h + CARD_GAP);
}

// ---------- Drawing helpers ----------
// A straight linear lerp changes color at a constant rate top to bottom,
// which on a screen this small reads as a visible, slightly mechanical
// ramp. Smoothstep eases in and out of the transition — nearly flat near
// COL_BG_TOP, nearly flat near COL_BG_BOTTOM, doing most of the change
// through the middle — so the same two colors read as a soft wash instead
// of a ruler-straight fade. Same cost as the linear version: one extra
// multiply per scanline.
float smoothstep01(float t) {
  return t * t * (3.0f - 2.0f * t);
}

void drawGradientBackground() {
  int h = tft.height();
  int w = tft.width();

  uint8_t r1, g1, b1, r2, g2, b2;
  color565toRGB(COL_BG_TOP, r1, g1, b1);
  color565toRGB(COL_BG_BOTTOM, r2, g2, b2);

  for (int y = 0; y < h; y++) {
    float t = smoothstep01((float)y / (float)h);
    uint8_t r = r1 + t * (r2 - r1);
    uint8_t g = g1 + t * (g2 - g1);
    uint8_t b = b1 + t * (b2 - b1);
    uint16_t color = tft.color565(r, g, b);
    tft.drawFastHLine(0, y, w, color);
  }

  indicatorDirty = true;
}

void rightText(const char* text, int rightX, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  tft.setCursor(rightX - (int)w, y);
  tft.print(text);
}

void centerText(const char* text, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  int x = (tft.width() - w) / 2;
  tft.setCursor(x, y);
  tft.print(text);
}

void centerTextInBox(const char* text, int y, int boxX, int boxW) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  int x = boxX + (boxW - (int)w) / 2;
  tft.setCursor(x, y);
  tft.print(text);
}

// Largest text size (1..maxSize) at which the string still fits in availW.
int fitTextSize(const char* s, int availW, int maxSize) {
  int len = (int)strlen(s);
  if (len == 0) return 1;
  for (int sz = maxSize; sz > 1; sz--) {
    if (len * 6 * sz <= availW) return sz;
  }
  return 1;
}

void color565toRGB(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = (color >> 11) & 0x1F;
  g = (color >> 5) & 0x3F;
  b = color & 0x1F;
  r = (r * 255) / 31;
  g = (g * 255) / 63;
  b = (b * 255) / 31;
}
