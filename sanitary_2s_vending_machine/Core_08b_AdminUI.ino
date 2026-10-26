// =====================================================
//        ADMIN UI KIT  (shared by every admin screen)
// =====================================================
// Every admin screen is built from the handful of pieces below so the whole
// area reads as one design: the same four type sizes, the same button
// styles, the same bottom bar, the same row/stepper/switch controls.
//
// Type. The Welcome screen's face (FreeSerifBold) at its smaller sizes, plus
// one regular-weight face for dense secondary text. The default 5x7 bitmap
// font these screens used before is gone: at size 1 it is 7px tall and
// unreadable at arm's length, and at size 2 it is 12px wide per character,
// which is why labels kept running out of room. A proportional serif is
// narrower per character AND taller, so text got bigger without costing width.
//
//   FACE_XS  FreeSerif 9pt        cap 12px  captions, long values, hints
//   FACE_SM  FreeSerifBold 9pt    cap 12px  secondary labels, small buttons
//   FACE_MD  FreeSerifBold 12pt   cap 16px  primary labels, buttons, values
//   FACE_LG  FreeSerifBold 18pt   cap 24px  big numbers (keypad, PIN)
//
// Custom GFX fonts have three traps, all handled here so no screen has to:
//   - setTextSize() scales them, so size is pinned to 1 before every draw.
//   - setCursor() takes the BASELINE, not the glyph top, so text is placed by
//     its cap height (auiCapH) to centre on a given line.
//   - They are transparent: setTextColor's background colour is ignored, so
//     whatever sits behind the text must already be painted.
//
// auiPut() is the one text routine. It shrinks the face and finally clips with
// ".." until the string fits its box, so a long machine name or e-mail address
// can never run off a card.
//
// Vertical budget (240px): header to y=32, content y=38..192, bottom bar
// y=196..232.
const int FACE_XS = 0;
const int FACE_SM = 1;
const int FACE_MD = 2;
const int FACE_LG = 3;

#define AUI_FONT(face) ((face) == FACE_LG ? &FreeSerifBold18pt7b : \
                        (face) == FACE_MD ? &FreeSerifBold12pt7b : \
                        (face) == FACE_SM ? &FreeSerifBold9pt7b  : &FreeSerif9pt7b)

const int AL_LEFT = 0, AL_CENTER = 1, AL_RIGHT = 2;

int auiCapH(int face) {
  return (face == FACE_LG) ? 24 : (face == FACE_MD) ? 16 : 12;
}

void auiFace(int face) {
  tft.setTextSize(1);
  tft.setFont(AUI_FONT(face));
}

int auiTextW(const char* s, int face) {
  int16_t x1, y1;
  uint16_t w, h;
  auiFace(face);
  tft.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  tft.setFont();
  return (int)w;
}

// Largest face (stepping LG -> MD -> SM -> XS) at which s fits maxW.
int auiFitFace(const char* s, int maxW, int face) {
  while (face > FACE_XS && auiTextW(s, face) > maxW) face--;
  return face;
}

// Copies s into dst, chopping characters off the end and adding ".." until the
// result fits maxW at the given face. n must be at least 4.
void auiClip(char* dst, size_t n, const char* s, int maxW, int face) {
  strncpy(dst, s, n - 1);
  dst[n - 1] = '\0';
  if (auiTextW(dst, face) <= maxW) return;
  int len = (int)strlen(dst);
  if (len > (int)n - 3) len = (int)n - 3;
  for (; len > 0; len--) {
    dst[len] = '.';
    dst[len + 1] = '.';
    dst[len + 2] = '\0';
    if (auiTextW(dst, face) <= maxW) return;
  }
  strcpy(dst, "..");
}

// The one text routine. Draws s inside the horizontal box [bx, bx+bw], with its
// cap height centred on cy. AL_LEFT starts at bx, AL_RIGHT ends at bx+bw,
// AL_CENTER centres in the box. Shrinks/clips so it never leaves the box.
void auiPut(const char* s, int align, int bx, int bw, int cy, uint16_t col, int face) {
  if (s == NULL || s[0] == '\0') return;
  face = auiFitFace(s, bw, face);
  char buf[72];
  auiClip(buf, sizeof(buf), s, bw, face);

  int16_t x1, y1;
  uint16_t w, h;
  auiFace(face);
  tft.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);

  int x;
  if (align == AL_CENTER)     x = bx + (bw - (int)w) / 2;
  else if (align == AL_RIGHT) x = bx + bw - (int)w;
  else                        x = bx;

  tft.setTextColor(col);
  tft.setCursor(x - x1, cy + auiCapH(face) / 2);
  tft.print(buf);
  tft.setFont();
}

// ---------- Buttons ----------
const int ABTN_PRIMARY   = 0;   // accent fill — the one thing to do on a screen
const int ABTN_SECONDARY = 1;   // card fill with border — navigation, cancel
const int ABTN_DANGER    = 2;   // destructive
const int ABTN_OFF       = 3;   // unavailable: flat, dimmed, still bordered

void auiButton(int x, int y, int w, int h, const char* label, int style, int face) {
  int r = (h >= 30) ? 8 : 6;
  uint16_t fill = COL_CARD, txt = COL_TEXT;
  if (style == ABTN_PRIMARY)      { fill = COL_ACCENT; txt = COL_BG_TOP; }
  else if (style == ABTN_DANGER)  { fill = COL_DANGER; txt = COL_BG_TOP; }
  else if (style == ABTN_OFF)     { fill = COL_CARD;   txt = COL_TEXT_DIM; }

  if (style != ABTN_OFF) drawCardShadow(x, y, w, h, r);
  tft.fillRoundRect(x, y, w, h, r, fill);
  if (style == ABTN_SECONDARY || style == ABTN_OFF) tft.drawRoundRect(x, y, w, h, r, COL_CARD_BRD);
  auiPut(label, AL_CENTER, x + 4, w - 8, y + h / 2, txt, face);
}

// ---------- Bottom bar ----------
// 1 to 3 equal buttons across the foot of the screen. Slot numbering is left
// to right; hit-testing uses the same geometry as drawing.
const int ABAR_Y = 196, ABAR_H = 36, ABAR_X = 16, ABAR_W = 288, ABAR_GAP = 8;

int auiBarSlotW(int slots) { return (ABAR_W - (slots - 1) * ABAR_GAP) / slots; }
int auiBarSlotX(int slot, int slots) { return ABAR_X + slot * (auiBarSlotW(slots) + ABAR_GAP); }

void auiBarButton(int slot, int slots, const char* label, int style) {
  auiButton(auiBarSlotX(slot, slots), ABAR_Y, auiBarSlotW(slots), ABAR_H, label, style, FACE_MD);
}

bool auiBarHit(int sx, int sy, int slot, int slots) {
  return pointInRect(sx, sy, auiBarSlotX(slot, slots) - 2, ABAR_Y - 3,
                     auiBarSlotW(slots) + 4, ABAR_H + 12);
}

// ---------- Rows and cards ----------
void auiRow(int x, int y, int w, int h) {
  drawCard(x, y, w, h, 6);
}

// A row's left-hand label. cy is the row's vertical centre.
void auiRowLabel(int x, int cy, int maxW, const char* label) {
  auiPut(label, AL_LEFT, x, maxW, cy, COL_TEXT, FACE_MD);
}

// ---------- Switch ----------
// A pill with a sliding knob; the state word sits on the side opposite the
// knob. rightX is the pill's right edge, cy its vertical centre. The whole
// row is the tap target, never just the pill.
const int ASW_W = 66, ASW_H = 24;

void auiSwitch(int rightX, int cy, bool on, int w, const char* onText, const char* offText) {
  int x = rightX - w, y = cy - ASW_H / 2;
  int knobR = 9;
  uint16_t track = on ? COL_ACCENT : COL_SHADOW;

  tft.fillRoundRect(x, y, w, ASW_H, ASW_H / 2, track);
  tft.drawRoundRect(x, y, w, ASW_H, ASW_H / 2, on ? COL_ACCENT : COL_CARD_BRD);

  int kx = on ? (x + w - 3 - knobR) : (x + 3 + knobR);
  tft.fillCircle(kx, cy, knobR, COL_BG_TOP);
  tft.drawCircle(kx, cy, knobR, on ? COL_ACCENT : COL_CARD_BRD);

  int textW = w - 2 * knobR - 12;
  if (on) auiPut(onText,  AL_CENTER, x + 6,                   textW, cy, COL_BG_TOP,   FACE_SM);
  else    auiPut(offText, AL_CENTER, x + 2 * knobR + 6,       textW, cy, COL_TEXT_DIM, FACE_SM);
}

void auiOnOff(int rightX, int cy, bool on) {
  auiSwitch(rightX, cy, on, ASW_W, "ON", "OFF");
}

// ---------- Stepper:  [-]  value  [+] ----------
const int ASTEP_BTN_W = 34, ASTEP_VAL_W = 58, ASTEP_GAP = 4, ASTEP_H = 28;

int auiStepPlusX(int rightX)  { return rightX - ASTEP_BTN_W; }
int auiStepValX(int rightX)   { return auiStepPlusX(rightX) - ASTEP_GAP - ASTEP_VAL_W; }
int auiStepMinusX(int rightX) { return auiStepValX(rightX) - ASTEP_GAP - ASTEP_BTN_W; }
int auiStepLeftX(int rightX)  { return auiStepMinusX(rightX); }

void auiStepGlyph(int x, int cy, bool plus) {
  int cx = x + ASTEP_BTN_W / 2;
  tft.fillRect(cx - 7, cy - 1, 15, 3, COL_BG_TOP);
  if (plus) tft.fillRect(cx - 1, cy - 7, 3, 15, COL_BG_TOP);
}

void auiStepper(int rightX, int cy, const char* value, uint16_t valCol) {
  int y = cy - ASTEP_H / 2;
  int mx = auiStepMinusX(rightX), vx = auiStepValX(rightX), px = auiStepPlusX(rightX);

  drawCardShadow(mx, y, ASTEP_BTN_W, ASTEP_H, 6);
  tft.fillRoundRect(mx, y, ASTEP_BTN_W, ASTEP_H, 6, COL_ACCENT);
  auiStepGlyph(mx, cy, false);

  tft.fillRoundRect(vx, y, ASTEP_VAL_W, ASTEP_H, 6, COL_BG_TOP);
  tft.drawRoundRect(vx, y, ASTEP_VAL_W, ASTEP_H, 6, COL_CARD_BRD);
  auiPut(value, AL_CENTER, vx + 3, ASTEP_VAL_W - 6, cy, valCol, FACE_MD);

  drawCardShadow(px, y, ASTEP_BTN_W, ASTEP_H, 6);
  tft.fillRoundRect(px, y, ASTEP_BTN_W, ASTEP_H, 6, COL_ACCENT);
  auiStepGlyph(px, cy, true);
}

// -1 for the minus button, +1 for plus, 0 for neither. The hit areas reach a
// few pixels past the drawn buttons so a fingertip doesn't have to be exact.
int auiStepTouch(int sx, int sy, int rightX, int cy) {
  int y = cy - ASTEP_H / 2 - 4, h = ASTEP_H + 8;
  if (pointInRect(sx, sy, auiStepMinusX(rightX) - 4, y, ASTEP_BTN_W + 6, h)) return -1;
  if (pointInRect(sx, sy, auiStepPlusX(rightX) - 2, y, ASTEP_BTN_W + 6, h)) return 1;
  return 0;
}

// ---------- Small in-row button (Edit / Setup / Reset ...) ----------
const int AROWBTN_H = 26;

void auiRowButton(int x, int cy, int w, const char* label, int style) {
  auiButton(x, cy - AROWBTN_H / 2, w, AROWBTN_H, label, style, FACE_SM);
}

bool auiRowButtonHit(int sx, int sy, int x, int cy, int w) {
  return pointInRect(sx, sy, x - 3, cy - AROWBTN_H / 2 - 3, w + 6, AROWBTN_H + 6);
}

// ---------- Hint / status line on the gradient ----------
// COL_TEXT, not COL_TEXT_DIM: this sits directly on the gradient, which is a
// saturated colour by the lower half of the screen.
void auiHint(const char* s, int cy) {
  auiPut(s, AL_CENTER, 12, 296, cy, COL_TEXT, FACE_XS);
}

// ---------- Tab strip (Reports) ----------
void auiTab(int x, int y, int w, int h, const char* label, bool active) {
  if (active) {
    tft.fillRoundRect(x, y, w, h, 6, COL_ACCENT);
    auiPut(label, AL_CENTER, x + 2, w - 4, y + h / 2, COL_BG_TOP, FACE_SM);
  } else {
    tft.fillRoundRect(x, y, w, h, 6, COL_CARD);
    tft.drawRoundRect(x, y, w, h, 6, COL_CARD_BRD);
    auiPut(label, AL_CENTER, x + 2, w - 4, y + h / 2, COL_TEXT, FACE_SM);
  }
}

// ---------- Status dot ----------
void auiDot(int cx, int cy, bool on) {
  if (on) {
    tft.fillCircle(cx, cy, 6, COL_SUCCESS);
  } else {
    tft.fillCircle(cx, cy, 6, COL_BG_TOP);
    tft.drawCircle(cx, cy, 6, COL_TEXT_DIM);
  }
}

// ---------- Full-screen message (long operations) ----------
void auiMessageScreen(const char* title, const char* line1, const char* line2) {
  drawGradientBackground();
  auiPut(title, AL_CENTER, 10, 300, 92, COL_ACCENT, FACE_MD);
  if (line1) auiPut(line1, AL_CENTER, 10, 300, 124, COL_TEXT, FACE_SM);
  if (line2) auiPut(line2, AL_CENTER, 10, 300, 146, COL_TEXT, FACE_XS);
}
