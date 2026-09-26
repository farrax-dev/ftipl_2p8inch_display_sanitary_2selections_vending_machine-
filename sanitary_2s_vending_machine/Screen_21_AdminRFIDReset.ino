// =====================================================
//        ADMIN RFID AUTO-RESET SCHEDULE SCREEN
// =====================================================
// Configures the one thing Screen_19_AdminRFIDCardEdit.ino's per-card
// "Reset Usage" button couldn't: a machine-wide schedule that zeroes every
// registered card's usage automatically — e.g. "reset on the 1st of every
// month at 01:00" for a monthly free-vend quota. See
// maintainRFIDResetSchedule() (Core_09_Storage.ino) for the actual firing
// logic; this screen only edits rfidResetEnabled/Day/Hour/Minute, saving
// each change immediately (no separate Save step — the same "tap = committed"
// shape as the payment toggles on Admin > Settings, not the Date & Time
// screen's edit-a-copy-then-Save shape, since nothing here is as
// consequential as setting the system clock).
//
// Reached from the RFID row on Admin > Settings (Screen_11_AdminSettings.ino)
// via its "Reset" chip, next to that row's existing "Cards" chip.
const int RRS_ROW_X = 14, RRS_ROW_W = 292, RRS_ROW_H = 26;
const int RRS_ROW_Y0 = 36, RRS_ROW_GAP = 3;
const int RRS_ROW_COUNT = 4;   // on/off + day + hour + minute

int rrsRowY(int row) { return RRS_ROW_Y0 + row * (RRS_ROW_H + RRS_ROW_GAP); }

// Stepper geometry — same right-anchored -/value/+ layout as
// Screen_20_AdminDateTime.ino's fields.
const int RRS_BTN_W = 24, RRS_VAL_W = 60;
const int RRS_PLUS_X  = RRS_ROW_X + RRS_ROW_W - 8 - RRS_BTN_W;
const int RRS_VAL_X   = RRS_PLUS_X - 6 - RRS_VAL_W;
const int RRS_MINUS_X = RRS_VAL_X - 6 - RRS_BTN_W;

// Toggle geometry, row 0 only — same shape as the payment method ON/OFF
// toggles on Admin > Settings.
const int RRS_TOGGLE_W = 60, RRS_TOGGLE_H = 20;
const int RRS_TOGGLE_X = RRS_ROW_X + RRS_ROW_W - RRS_TOGGLE_W - 8;

// day/hour/minute share one row shape and one +/- range, so the draw and
// touch code both drive off this table instead of three near-identical
// blocks each.
struct RRSField { const char* label; int* value; int lo, hi; };
RRSField rrsFields[3] = {
  { "Day of Month", &rfidResetDay,    1,  28 },
  { "Hour (24h)",   &rfidResetHour,   0,  23 },
  { "Minute",       &rfidResetMinute, 0,  59 },
};

void drawRRSStepperRow(int row, const char* label, int value) {
  int y = rrsRowY(row);
  drawCard(RRS_ROW_X, y, RRS_ROW_W, RRS_ROW_H, 5);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT, COL_CARD);
  tft.setCursor(RRS_ROW_X + 10, y + (RRS_ROW_H - 8) / 2);
  tft.print(label);

  int ctrlY = y + 3, ctrlH = RRS_ROW_H - 6;
  int textY = ctrlY + (ctrlH - 16) / 2;

  drawCardShadow(RRS_MINUS_X, ctrlY, RRS_BTN_W, ctrlH, 4);
  tft.fillRoundRect(RRS_MINUS_X, ctrlY, RRS_BTN_W, ctrlH, 4, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  tft.setTextSize(2);
  centerTextInBox("-", textY, RRS_MINUS_X, RRS_BTN_W);

  // Rounded to match the flanking -/+ buttons; no shadow, read-only figure.
  tft.fillRoundRect(RRS_VAL_X, ctrlY, RRS_VAL_W, ctrlH, 4, COL_BG_TOP);
  tft.setTextColor(COL_ACCENT, COL_BG_TOP);
  tft.setTextSize(2);
  char vbuf[8];
  snprintf(vbuf, sizeof(vbuf), "%02d", value);
  centerTextInBox(vbuf, textY, RRS_VAL_X, RRS_VAL_W);

  drawCardShadow(RRS_PLUS_X, ctrlY, RRS_BTN_W, ctrlH, 4);
  tft.fillRoundRect(RRS_PLUS_X, ctrlY, RRS_BTN_W, ctrlH, 4, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox("+", textY, RRS_PLUS_X, RRS_BTN_W);
}

void drawAdminRFIDResetScreen() {
  drawGradientBackground();
  drawScreenTitle("RFID Auto-Reset");

  // ---- Row 0: on/off ----
  int y0 = rrsRowY(0);
  drawCard(RRS_ROW_X, y0, RRS_ROW_W, RRS_ROW_H, 5);
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT, COL_CARD);
  tft.setCursor(RRS_ROW_X + 10, y0 + (RRS_ROW_H - 8) / 2);
  tft.print("Monthly Reset");

  int toggleY = y0 + (RRS_ROW_H - RRS_TOGGLE_H) / 2;
  uint16_t toggleColor = rfidResetEnabled ? COL_ACCENT : COL_BG_TOP;
  tft.fillRoundRect(RRS_TOGGLE_X, toggleY, RRS_TOGGLE_W, RRS_TOGGLE_H, 4, toggleColor);
  tft.setTextColor(rfidResetEnabled ? COL_BG_TOP : COL_TEXT_DIM, toggleColor);
  centerTextInBox(rfidResetEnabled ? "ON" : "OFF", toggleY + 4, RRS_TOGGLE_X, RRS_TOGGLE_W);

  // ---- Rows 1-3: day / hour / minute ----
  for (int f = 0; f < 3; f++) {
    drawRRSStepperRow(f + 1, rrsFields[f].label, *rrsFields[f].value);
  }

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  // Explicit \n breaks rather than leaving this to wrapTextInBox's own word
  // wrap: at this box width (292px) its wrap point (~48 chars) is wider than
  // the 31-char scratch buffer it draws each line from, so the two disagree
  // and the tail of a wrapped line gets silently cut off. Breaking by hand
  // into segments well under 31 chars each sidesteps that.
  char info[110];
  if (rfidResetFiredMonth > 0) {
    snprintf(info, sizeof(info),
             "Zeroes every card's usage\non day %d at %02d:%02d, monthly.\nLast: %04d-%02d.",
             rfidResetDay, rfidResetHour, rfidResetMinute,
             (int)(rfidResetFiredMonth / 100), (int)(rfidResetFiredMonth % 100));
  } else {
    snprintf(info, sizeof(info),
             "Zeroes every card's usage\non day %d at %02d:%02d, monthly.",
             rfidResetDay, rfidResetHour, rfidResetMinute);
  }
  wrapTextInBox(info, RRS_ROW_X, RRS_ROW_W, rrsRowY(RRS_ROW_COUNT) + 2, 1, true);

  drawBackButton("< Back");
}

void handleAdminRFIDResetScreen() {
  if (!isRealTouch()) return;
  if (millis() - lastTouchTime <= TOUCH_DEBOUNCE) return;
  lastTouchTime = millis();
  TS_Point raw = ts.getPoint();
  int sx, sy;
  mapTouchToScreen(raw, sx, sy);

  if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
    currentScreen = SCREEN_ADMIN_RFID_CARDS;
    drawAdminRFIDCardsScreen();
    return;
  }

  int y0 = rrsRowY(0);
  int toggleY = y0 + (RRS_ROW_H - RRS_TOGGLE_H) / 2;
  if (pointInRect(sx, sy, RRS_TOGGLE_X, toggleY, RRS_TOGGLE_W, RRS_TOGGLE_H)) {
    rfidResetEnabled = !rfidResetEnabled;
    saveRFIDResetSchedule();
    drawAdminRFIDResetScreen();
    return;
  }

  for (int f = 0; f < 3; f++) {
    int y = rrsRowY(f + 1);
    int ctrlY = y + 3, ctrlH = RRS_ROW_H - 6;

    if (pointInRect(sx, sy, RRS_MINUS_X, ctrlY, RRS_BTN_W, ctrlH)) {
      int v = *rrsFields[f].value - 1;
      if (v < rrsFields[f].lo) v = rrsFields[f].lo;
      *rrsFields[f].value = v;
      saveRFIDResetSchedule();
      drawAdminRFIDResetScreen();
      return;
    }
    if (pointInRect(sx, sy, RRS_PLUS_X, ctrlY, RRS_BTN_W, ctrlH)) {
      int v = *rrsFields[f].value + 1;
      if (v > rrsFields[f].hi) v = rrsFields[f].hi;
      *rrsFields[f].value = v;
      saveRFIDResetSchedule();
      drawAdminRFIDResetScreen();
      return;
    }
  }
}
