// ---------- Touch calibration + orientation ----------
const int TS_MINX = 369;
const int TS_MAXX = 3922;
const int TS_MINY = 270;
const int TS_MAXY = 3777;

const bool TOUCH_SWAP_XY   = false;
const bool TOUCH_INVERT_X  = true;
const bool TOUCH_INVERT_Y  = true;

void mapTouchToScreen(TS_Point raw, int &sx, int &sy) {
  int rx = raw.x;
  int ry = raw.y;

  if (TOUCH_SWAP_XY) {
    int tmp = rx;
    rx = ry;
    ry = tmp;
  }

  sx = map(rx, TS_MINX, TS_MAXX, 0, tft.width());
  sy = map(ry, TS_MINY, TS_MAXY, 0, tft.height());

  if (TOUCH_INVERT_X) sx = tft.width() - sx;
  if (TOUCH_INVERT_Y) sy = tft.height() - sy;

  sx = constrain(sx, 0, tft.width() - 1);
  sy = constrain(sy, 0, tft.height() - 1);

  Serial.printf("touch raw(%d,%d) z=%d -> screen(%d,%d)\n", raw.x, raw.y, raw.z, sx, sy);
}

// ---------- Touch debounce ----------
unsigned long lastTouchTime = 0;
const unsigned long TOUCH_DEBOUNCE = 350;
const int MIN_PRESSURE = 300;

bool isRealTouch() {
  if (!ts.touched()) return false;
  TS_Point p = ts.getPoint();
  return p.z > MIN_PRESSURE;
}
