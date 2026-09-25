// =====================================================
//         ADMIN DATE & TIME SCREEN (stepper-based)
// =====================================================
// Replaces typing "DD/MM/YYYY HH:MM AM/PM" on the on-screen keyboard (the
// old TE_SET_DATETIME text-entry target) with five +/- steppers, one per
// field — the same control already used for the motor stock quantities and
// (until Config.h took it over) the UPI salt index. Nothing to mistype, and
// no keyboard layer to switch to for the "/"/":" separators or "AM"/"PM".
//
// Always edits in 24-hour; the 12h/24h chip here only flips clock24Hour
// (Core_06_Network.ino), which purely controls how the result is *displayed*
// elsewhere (the Welcome screen clock) — same split that existed before.
const int DT_ROW_X = 14, DT_ROW_W = 292, DT_ROW_H = 26;
const int DT_ROW_Y0 = 38, DT_ROW_GAP = 4;
const int DT_FIELD_COUNT = 5;

const int DT_BTN_W = 24, DT_VAL_W = 60;
const int DT_PLUS_X  = DT_ROW_X + DT_ROW_W - 8 - DT_BTN_W;
const int DT_VAL_X   = DT_PLUS_X - 6 - DT_VAL_W;
const int DT_MINUS_X = DT_VAL_X - 6 - DT_BTN_W;

// Custom 3-button bottom bar (Cancel / 12h-24h / Save) rather than the usual
// Back+Proceed pair — this screen needs a third control and the standard bar
// only budgets for two.
const int DT_BAR_BACK_X = 8,   DT_BAR_BACK_W = 94;
const int DT_BAR_12H_X  = 110, DT_BAR_12H_W  = 94;
const int DT_BAR_SAVE_X = 212, DT_BAR_SAVE_W = 100;

const char* const DT_FIELD_LABELS[DT_FIELD_COUNT] = { "Day", "Month", "Year", "Hour", "Minute" };

// In-progress edit — only reaches the running clock/RTC when "Save" is
// tapped, the same "edit a copy, commit on Done" shape the text-entry field
// it replaces used.
int dtEditDay = 1, dtEditMonth = 1, dtEditYear = 2026, dtEditHour = 0, dtEditMinute = 0;

int dtRowY(int row) {
  return DT_ROW_Y0 + row * (DT_ROW_H + DT_ROW_GAP);
}

int dtFieldValue(int field) {
  switch (field) {
    case 0: return dtEditDay;
    case 1: return dtEditMonth;
    case 2: return dtEditYear;
    case 3: return dtEditHour;
    default: return dtEditMinute;
  }
}

void dtFieldRange(int field, int &lo, int &hi) {
  switch (field) {
    case 0: lo = 1;    hi = 31;   break;  // not clamped to days-in-month —
                                           // same latitude a typed date always had
    case 1: lo = 1;    hi = 12;   break;
    case 2: lo = 2024; hi = 2099; break;
    case 3: lo = 0;    hi = 23;   break;
    default: lo = 0;   hi = 59;   break;
  }
}

void dtStepField(int field, int delta) {
  int lo, hi;
  dtFieldRange(field, lo, hi);
  int v = dtFieldValue(field) + delta;
  if (v < lo) v = lo;
  if (v > hi) v = hi;
  switch (field) {
    case 0: dtEditDay = v;    break;
    case 1: dtEditMonth = v;  break;
    case 2: dtEditYear = v;   break;
    case 3: dtEditHour = v;   break;
    default: dtEditMinute = v; break;
  }
}

// Pre-fills from the running clock (whichever source set it — network or
// the onboard RTC), same starting point the old text-entry field opened
// with. Falls back to a fixed default when nothing has synced yet, since a
// stepper needs some numeric value to start from, unlike a text field that
// could just open blank.
void openAdminDateTimeScreen() {
  struct tm ti;
  if (timeSynced && getLocalTime(&ti, 10)) {
    dtEditDay    = ti.tm_mday;
    dtEditMonth  = ti.tm_mon + 1;
    dtEditYear   = ti.tm_year + 1900;
    dtEditHour   = ti.tm_hour;
    dtEditMinute = ti.tm_min;
  } else {
    dtEditDay = 1; dtEditMonth = 1; dtEditYear = 2026; dtEditHour = 0; dtEditMinute = 0;
  }

  currentScreen = SCREEN_ADMIN_DATETIME;
  drawAdminDateTimeScreen();
}

// Manually setting the clock is the same trust tier as the onboard RTC
// fallback (Core_18_RTC.ino) — it marks the clock usable (timeSynced) but
// NOT network-verified (timeSyncedFromNetwork stays as it was), so a real
// WiFi/LTE sync can still arrive later and correct it. Also written straight
// to the RTC so it survives a power cycle with no connectivity at all,
// which is the exact situation this control exists for.
void commitAdminDateTime() {
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_year = dtEditYear - 1900;
  t.tm_mon  = dtEditMonth - 1;
  t.tm_mday = dtEditDay;
  t.tm_hour = dtEditHour;
  t.tm_min  = dtEditMinute;
  t.tm_sec  = 0;

  setenv("TZ", "IST-5:30", 1);
  tzset();
  time_t epoch = mktime(&t);
  struct timeval tv = { epoch, 0 };
  settimeofday(&tv, nullptr);
  timeSynced = true;
  rtcWriteTime(t);
}

void drawAdminDateTimeScreen() {
  drawGradientBackground();
  drawScreenTitle("Date & Time");

  for (int f = 0; f < DT_FIELD_COUNT; f++) {
    int y = dtRowY(f);
    drawCard(DT_ROW_X, y, DT_ROW_W, DT_ROW_H, 5);

    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT, COL_CARD);
    tft.setCursor(DT_ROW_X + 10, y + (DT_ROW_H - 8) / 2);
    tft.print(DT_FIELD_LABELS[f]);

    int ctrlY = y + 3, ctrlH = DT_ROW_H - 6;
    int textY = ctrlY + (ctrlH - 16) / 2;

    drawCardShadow(DT_MINUS_X, ctrlY, DT_BTN_W, ctrlH, 4);
    tft.fillRoundRect(DT_MINUS_X, ctrlY, DT_BTN_W, ctrlH, 4, COL_ACCENT);
    tft.setTextColor(COL_BG_TOP, COL_ACCENT);
    tft.setTextSize(2);
    centerTextInBox("-", textY, DT_MINUS_X, DT_BTN_W);

    // Rounded to match the flanking -/+ buttons; no shadow, read-only figure.
    tft.fillRoundRect(DT_VAL_X, ctrlY, DT_VAL_W, ctrlH, 4, COL_BG_TOP);
    tft.setTextColor(COL_ACCENT, COL_BG_TOP);
    tft.setTextSize(2);
    char vbuf[8];
    snprintf(vbuf, sizeof(vbuf), (f == 2) ? "%04d" : "%02d", dtFieldValue(f));
    centerTextInBox(vbuf, textY, DT_VAL_X, DT_VAL_W);

    drawCardShadow(DT_PLUS_X, ctrlY, DT_BTN_W, ctrlH, 4);
    tft.fillRoundRect(DT_PLUS_X, ctrlY, DT_BTN_W, ctrlH, 4, COL_ACCENT);
    tft.setTextColor(COL_BG_TOP, COL_ACCENT);
    tft.setTextSize(2);
    centerTextInBox("+", textY, DT_PLUS_X, DT_BTN_W);
  }

  drawCard(DT_BAR_BACK_X, BTN_Y, DT_BAR_BACK_W, BTN_H, 8);
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_CARD);
  centerTextInBox("Cancel", BTN_Y + 9, DT_BAR_BACK_X, DT_BAR_BACK_W);

  // Display-only preference (Core_06_Network.ino) — doesn't affect any of
  // the steppers above, which always edit in 24-hour.
  drawCardShadow(DT_BAR_12H_X, BTN_Y, DT_BAR_12H_W, BTN_H, 8);
  tft.fillRoundRect(DT_BAR_12H_X, BTN_Y, DT_BAR_12H_W, BTN_H, 8, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox(clock24Hour ? "24h" : "12h", BTN_Y + 9, DT_BAR_12H_X, DT_BAR_12H_W);

  drawCardShadow(DT_BAR_SAVE_X, BTN_Y, DT_BAR_SAVE_W, BTN_H, 8);
  tft.fillRoundRect(DT_BAR_SAVE_X, BTN_Y, DT_BAR_SAVE_W, BTN_H, 8, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox("Save", BTN_Y + 9, DT_BAR_SAVE_X, DT_BAR_SAVE_W);
}

void handleAdminDateTimeScreen() {
  if (!isRealTouch()) return;
  if (millis() - lastTouchTime <= TOUCH_DEBOUNCE) return;
  lastTouchTime = millis();
  TS_Point raw = ts.getPoint();
  int sx, sy;
  mapTouchToScreen(raw, sx, sy);

  if (pointInRect(sx, sy, DT_BAR_BACK_X, BTN_Y, DT_BAR_BACK_W, BTN_H)) {
    currentScreen = SCREEN_ADMIN_PANEL;
    drawAdminPanelScreen();
    return;
  }

  if (pointInRect(sx, sy, DT_BAR_SAVE_X, BTN_Y, DT_BAR_SAVE_W, BTN_H)) {
    commitAdminDateTime();
    currentScreen = SCREEN_ADMIN_PANEL;
    drawAdminPanelScreen();
    return;
  }

  if (pointInRect(sx, sy, DT_BAR_12H_X, BTN_Y, DT_BAR_12H_W, BTN_H)) {
    clock24Hour = !clock24Hour;
    saveClock24Hour();
    drawAdminDateTimeScreen();
    return;
  }

  for (int f = 0; f < DT_FIELD_COUNT; f++) {
    int y = dtRowY(f);
    int ctrlY = y + 3, ctrlH = DT_ROW_H - 6;

    if (pointInRect(sx, sy, DT_MINUS_X, ctrlY, DT_BTN_W, ctrlH)) {
      dtStepField(f, -1);
      drawAdminDateTimeScreen();
      return;
    }
    if (pointInRect(sx, sy, DT_PLUS_X, ctrlY, DT_BTN_W, ctrlH)) {
      dtStepField(f, 1);
      drawAdminDateTimeScreen();
      return;
    }
  }
}
