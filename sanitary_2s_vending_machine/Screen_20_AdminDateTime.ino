// =====================================================
//         ADMIN DATE & TIME SCREEN (stepper-based)
// =====================================================
// Replaces typing "DD/MM/YYYY HH:MM AM/PM" on the on-screen keyboard (the
// old TE_SET_DATETIME text-entry target) with five +/- steppers, one per
// field — the same control already used for the motor stock quantities and
// (until Config.h took it over) the UPI salt index. Nothing to mistype, and
// no keyboard layer to switch to for the "/"/":" separators or "AM"/"PM".
//
// dtEditHour always STORES the true 24-hour value (0-23), which is what
// commitAdminDateTime() writes out. The 12h/24h chip (Core_06_Network.ino's
// clock24Hour) only changes how that value is *displayed and stepped* here:
// in 12h mode the Hour field shows 1-12 with an AM/PM chip instead of 0-23,
// so the stepper can never be walked past 12 the way it used to be able to
// when the range check didn't know about clock24Hour at all.
const int DT_FIELD_COUNT = 5;

// Five columns, one per field, each a vertical stepper: [+] over the value over
// [-], with the field name above. Whole-width buttons 34px tall are far easier
// to hit than the 24x20 side-by-side -/+ the old row layout squeezed in, and
// the five fields read left to right as the date and time do.
const int DT_COL_W[DT_FIELD_COUNT] = { 44, 44, 72, 54, 54 };
const int DT_COL_GAP = 5;
const int DT_LABEL_CY = 47;
const int DT_PLUS_Y = 56,  DT_PLUS_H = 34;
const int DT_VAL_Y  = 93,  DT_VAL_H  = 30;
const int DT_MINUS_Y = 126, DT_MINUS_H = 34;
// AM/PM chip, Hour column only, in the space under its [-] button — only
// drawn/hit-tested when clock24Hour is off. Tapping it flips the real 24-hour
// value by 12 hours, same as stepping the Hour field twelve times but in one
// tap.
const int DT_AMPM_Y = 164, DT_AMPM_H = 26;

// Bottom bar: Cancel / 12h-24h / Save. Slots from Core_08b_AdminUI.ino.

int dtColX(int f) {
  int x = 16;
  for (int k = 0; k < f; k++) x += DT_COL_W[k] + DT_COL_GAP;
  return x;
}

const char* const DT_FIELD_LABELS[DT_FIELD_COUNT] = { "Day", "Month", "Year", "Hour", "Minute" };

// In-progress edit — only reaches the running clock/RTC when "Save" is
// tapped, the same "edit a copy, commit on Done" shape the text-entry field
// it replaces used.
int dtEditDay = 1, dtEditMonth = 1, dtEditYear = 2026, dtEditHour = 0, dtEditMinute = 0;

// Hour field only: converts the stored 24-hour value to what a 12-hour clock
// shows (1-12), so the on-screen figure and the +/- range agree with the
// AM/PM chip instead of just counting on past 12.
int dtHour12() {
  int h = dtEditHour % 12;
  return (h == 0) ? 12 : h;
}

int dtFieldValue(int field) {
  switch (field) {
    case 0: return dtEditDay;
    case 1: return dtEditMonth;
    case 2: return dtEditYear;
    case 3: return clock24Hour ? dtEditHour : dtHour12();
    default: return dtEditMinute;
  }
}

void dtFieldRange(int field, int &lo, int &hi) {
  switch (field) {
    case 0: lo = 1;    hi = 31;   break;  // not clamped to days-in-month —
                                           // same latitude a typed date always had
    case 1: lo = 1;    hi = 12;   break;
    case 2: lo = 2024; hi = 2099; break;
    case 3: lo = clock24Hour ? 0 : 1; hi = clock24Hour ? 23 : 12; break;
    default: lo = 0;   hi = 59;   break;
  }
}

void dtStepField(int field, int delta) {
  // In 12h mode the field being stepped (1-12) isn't what's stored (0-23), so
  // clamping the displayed value would either stick at 12 or wrap straight to
  // 1 with no way to cross into the next half of the day. Stepping the real
  // 24-hour value by the same delta instead lets AM/PM flip naturally at the
  // 12-hour boundary, same as a real clock's hour hand.
  if (field == 3 && !clock24Hour) {
    dtEditHour = ((dtEditHour + delta) % 24 + 24) % 24;
    return;
  }

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

void dtDrawGlyph(int x, int w, int y, int h, bool plus) {
  int cx = x + w / 2, cy = y + h / 2;
  tft.fillRect(cx - 7, cy - 1, 15, 3, COL_BG_TOP);
  if (plus) tft.fillRect(cx - 1, cy - 7, 3, 15, COL_BG_TOP);
}

void drawAdminDateTimeScreen() {
  drawGradientBackground();
  drawScreenTitle("Date & Time");

  for (int f = 0; f < DT_FIELD_COUNT; f++) {
    int x = dtColX(f), w = DT_COL_W[f];

    auiPut(DT_FIELD_LABELS[f], AL_CENTER, x - 2, w + 4, DT_LABEL_CY, COL_TEXT, FACE_SM);

    drawCardShadow(x, DT_PLUS_Y, w, DT_PLUS_H, 8);
    tft.fillRoundRect(x, DT_PLUS_Y, w, DT_PLUS_H, 8, COL_ACCENT);
    dtDrawGlyph(x, w, DT_PLUS_Y, DT_PLUS_H, true);

    tft.fillRoundRect(x, DT_VAL_Y, w, DT_VAL_H, 6, COL_CARD);
    tft.drawRoundRect(x, DT_VAL_Y, w, DT_VAL_H, 6, COL_CARD_BRD);
    char vbuf[8];
    snprintf(vbuf, sizeof(vbuf), (f == 2) ? "%04d" : "%02d", dtFieldValue(f));
    auiPut(vbuf, AL_CENTER, x + 2, w - 4, DT_VAL_Y + DT_VAL_H / 2, COL_ACCENT, FACE_MD);

    drawCardShadow(x, DT_MINUS_Y, w, DT_MINUS_H, 8);
    tft.fillRoundRect(x, DT_MINUS_Y, w, DT_MINUS_H, 8, COL_ACCENT);
    dtDrawGlyph(x, w, DT_MINUS_Y, DT_MINUS_H, false);

    // AM/PM chip, Hour column only, 12h mode only — tells the two halves of
    // the day apart now that the value above tops out at 12 instead of 23.
    if (f == 3 && !clock24Hour) {
      auiButton(x, DT_AMPM_Y, w, DT_AMPM_H, dtEditHour >= 12 ? "PM" : "AM", ABTN_SECONDARY, FACE_SM);
    }
  }

  auiBarButton(0, 3, "Cancel", ABTN_SECONDARY);
  // Core_06_Network.ino's clock24Hour, toggled here — also reshapes the Hour
  // stepper above (1-12 + AM/PM chip vs. 0-23) rather than being purely
  // cosmetic for the Welcome screen clock the way it used to be.
  auiBarButton(1, 3, clock24Hour ? "24 hour" : "12 hour", ABTN_SECONDARY);
  auiBarButton(2, 3, "Save", ABTN_PRIMARY);
}

void handleAdminDateTimeScreen() {
  if (!isRealTouch()) return;
  if (millis() - lastTouchTime <= TOUCH_DEBOUNCE) return;
  lastTouchTime = millis();
  TS_Point raw = ts.getPoint();
  int sx, sy;
  mapTouchToScreen(raw, sx, sy);

  // Cancel and Save both come back to the Machine screen, where Clock lives.
  if (auiBarHit(sx, sy, 0, 3)) {
    currentScreen = SCREEN_ADMIN_MACHINE;
    drawAdminMachineScreen();
    return;
  }

  if (auiBarHit(sx, sy, 2, 3)) {
    commitAdminDateTime();
    currentScreen = SCREEN_ADMIN_MACHINE;
    drawAdminMachineScreen();
    return;
  }

  if (auiBarHit(sx, sy, 1, 3)) {
    clock24Hour = !clock24Hour;
    saveClock24Hour();
    drawAdminDateTimeScreen();
    return;
  }

  for (int f = 0; f < DT_FIELD_COUNT; f++) {
    int x = dtColX(f), w = DT_COL_W[f];

    if (f == 3 && !clock24Hour && pointInRect(sx, sy, x, DT_AMPM_Y, w, DT_AMPM_H)) {
      dtEditHour = (dtEditHour + 12) % 24;
      drawAdminDateTimeScreen();
      return;
    }

    if (pointInRect(sx, sy, x, DT_MINUS_Y - 2, w, DT_MINUS_H + 4)) {
      dtStepField(f, -1);
      drawAdminDateTimeScreen();
      return;
    }
    if (pointInRect(sx, sy, x, DT_PLUS_Y - 4, w, DT_PLUS_H + 4)) {
      dtStepField(f, 1);
      drawAdminDateTimeScreen();
      return;
    }
  }
}
