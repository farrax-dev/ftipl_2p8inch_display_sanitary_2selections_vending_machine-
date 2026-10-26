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
// Reached from the RFID row on Admin > Payments (Screen_11_AdminSettings.ino)
// via its "Reset" button, next to that row's "Cards" button.
//
// Row 0 is the switch (with a caption saying what it does and when it last
// ran); rows 1-3 are the day / hour / minute steppers.
const int RRS_ROW_X = 16, RRS_ROW_W = 288;
const int RRS_ROW0_Y = 40, RRS_ROW0_H = 40;
const int RRS_ROW_Y1 = 84, RRS_ROW_H = 30, RRS_ROW_GAP = 4;
const int RRS_RIGHT = RRS_ROW_X + RRS_ROW_W - 8;

// Row index 1..3 (day, hour, minute) -> top edge / centre.
int rrsRowY(int row) { return RRS_ROW_Y1 + (row - 1) * (RRS_ROW_H + RRS_ROW_GAP); }
int rrsRowCY(int row) { return rrsRowY(row) + RRS_ROW_H / 2; }

// day/hour/minute share one row shape and one +/- range, so the draw and
// touch code both drive off this table instead of three near-identical
// blocks each.
struct RRSField { const char* label; int* value; int lo, hi; };
RRSField rrsFields[3] = {
  { "Day of month", &rfidResetDay,    1,  28 },
  { "Hour (24h)",   &rfidResetHour,   0,  23 },
  { "Minute",       &rfidResetMinute, 0,  59 },
};

void drawRRSStepperRow(int row, const char* label, int value) {
  int y = rrsRowY(row), cy = rrsRowCY(row);
  auiRow(RRS_ROW_X, y, RRS_ROW_W, RRS_ROW_H);
  auiRowLabel(RRS_ROW_X + 12, cy, auiStepLeftX(RRS_RIGHT) - RRS_ROW_X - 20, label);
  char vbuf[8];
  snprintf(vbuf, sizeof(vbuf), "%02d", value);
  auiStepper(RRS_RIGHT, cy, vbuf, COL_ACCENT);
}

void drawAdminRFIDResetScreen() {
  drawGradientBackground();
  drawScreenTitle("RFID Auto-Reset");

  // ---- Row 0: on/off ----
  auiRow(RRS_ROW_X, RRS_ROW0_Y, RRS_ROW_W, RRS_ROW0_H);
  auiRowLabel(RRS_ROW_X + 12, RRS_ROW0_Y + 14, 170, "Monthly reset");
  char caption[40];
  if (rfidResetFiredMonth > 0)
    snprintf(caption, sizeof(caption), "Last run %04d-%02d",
             (int)(rfidResetFiredMonth / 100), (int)(rfidResetFiredMonth % 100));
  else
    snprintf(caption, sizeof(caption), "Zeroes every card's usage");
  auiPut(caption, AL_LEFT, RRS_ROW_X + 12, 190, RRS_ROW0_Y + 31, COL_TEXT_DIM, FACE_XS);
  auiOnOff(RRS_RIGHT, RRS_ROW0_Y + RRS_ROW0_H / 2, rfidResetEnabled);

  // ---- Rows 1-3: day / hour / minute ----
  for (int f = 0; f < 3; f++) {
    drawRRSStepperRow(f + 1, rrsFields[f].label, *rrsFields[f].value);
  }

  auiBarButton(0, 1, "Back", ABTN_SECONDARY);
}

void handleAdminRFIDResetScreen() {
  if (!isRealTouch()) return;
  if (millis() - lastTouchTime <= TOUCH_DEBOUNCE) return;
  lastTouchTime = millis();
  TS_Point raw = ts.getPoint();
  int sx, sy;
  mapTouchToScreen(raw, sx, sy);

  if (auiBarHit(sx, sy, 0, 1)) {
    // This screen is only ever reached from the "Reset" button on the RFID row
    // of Admin > Payments (Screen_11_AdminSettings.ino) — there is no path
    // into it from the RFID Cards screen — so Back returns there, not to
    // Cards.
    currentScreen = SCREEN_ADMIN_SETTINGS;
    drawAdminSettingsScreen();
    return;
  }

  if (pointInRect(sx, sy, RRS_ROW_X, RRS_ROW0_Y, RRS_ROW_W, RRS_ROW0_H)) {
    rfidResetEnabled = !rfidResetEnabled;
    saveRFIDResetSchedule();
    drawAdminRFIDResetScreen();
    return;
  }

  for (int f = 0; f < 3; f++) {
    int step = auiStepTouch(sx, sy, RRS_RIGHT, rrsRowCY(f + 1));
    if (step == 0) continue;
    int v = constrain(*rrsFields[f].value + step, rrsFields[f].lo, rrsFields[f].hi);
    *rrsFields[f].value = v;
    saveRFIDResetSchedule();
    drawAdminRFIDResetScreen();
    return;
  }
}
