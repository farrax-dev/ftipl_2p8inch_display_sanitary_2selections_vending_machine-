// =====================================================
//           ADMIN MOTOR ASSIGN SCREEN
// =====================================================
// A 3-column grid of motor tiles (M1..M<MAX_MOTORS>). A tile in the accent
// colour belongs to the product being edited; tapping a free motor claims it,
// tapping a motor another product owns takes it from that product, tapping
// one of this product's own motors releases it. Tile height shrinks as rows
// are added so the grid still fits when a build fits more motors.
const int MOTOR_BTN_COLS = 3, MOTOR_BTN_W = 92, MOTOR_BTN_GAP = 6;
const int MOTOR_BTN_X0 = 16, MOTOR_BTN_Y0 = 40;

int motorGridRows() { return (MAX_MOTORS + MOTOR_BTN_COLS - 1) / MOTOR_BTN_COLS; }

int motorBtnH() {
  int rows = motorGridRows();
  return min(62, (146 - (rows - 1) * MOTOR_BTN_GAP) / rows);
}

int motorBtnX(int m) { return MOTOR_BTN_X0 + (m % MOTOR_BTN_COLS) * (MOTOR_BTN_W + MOTOR_BTN_GAP); }
int motorBtnY(int m) { return MOTOR_BTN_Y0 + (m / MOTOR_BTN_COLS) * (motorBtnH() + MOTOR_BTN_GAP); }

void drawAdminMotorAssignScreen() {
  int i = adminEditingSlot;
  drawGradientBackground();

  char titleBuf[32];
  snprintf(titleBuf, sizeof(titleBuf), "Motors: %s", products[i].name);
  drawScreenTitle(titleBuf);

  int h = motorBtnH();
  for (int m = 0; m < MAX_MOTORS; m++) {
    int x = motorBtnX(m), y = motorBtnY(m);

    bool mine = (products[i].motorMask & (1 << m)) != 0;
    int owner = productForMotor(m);

    uint16_t fill = mine ? COL_ACCENT : COL_CARD;
    drawCardShadow(x, y, MOTOR_BTN_W, h, 8);
    tft.fillRoundRect(x, y, MOTOR_BTN_W, h, 8, fill);
    tft.drawRoundRect(x, y, MOTOR_BTN_W, h, 8, COL_CARD_BRD);

    char label[6];
    snprintf(label, sizeof(label), "M%d", m + 1);
    uint16_t txt = mine ? COL_BG_TOP : COL_TEXT;

    if (h >= 44) {
      auiPut(label, AL_CENTER, x + 4, MOTOR_BTN_W - 8, y + h / 2 - 9, txt, FACE_MD);
      char caption[16];
      if (mine) snprintf(caption, sizeof(caption), "Selected");
      else if (owner == -1) snprintf(caption, sizeof(caption), "Free");
      else snprintf(caption, sizeof(caption), "%s", products[owner].name);
      auiPut(caption, AL_CENTER, x + 4, MOTOR_BTN_W - 8, y + h / 2 + 12,
             mine ? COL_BG_TOP : COL_TEXT_DIM, FACE_XS);
    } else {
      auiPut(label, AL_CENTER, x + 4, MOTOR_BTN_W - 8, y + h / 2, txt, FACE_MD);
    }
  }

  int hintY = MOTOR_BTN_Y0 + motorGridRows() * (h + MOTOR_BTN_GAP) + 8;
  if (hintY <= 186) auiHint("Tap a motor to assign or release it", hintY);

  auiBarButton(0, 1, "Done", ABTN_PRIMARY);
}

void handleAdminMotorAssignScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);
      int i = adminEditingSlot;

      if (auiBarHit(sx, sy, 0, 1)) {
        currentScreen = SCREEN_ADMIN_EDIT;
        drawAdminEditScreen();
        return;
      }

      int h = motorBtnH();
      for (int m = 0; m < MAX_MOTORS; m++) {
        if (pointInRect(sx, sy, motorBtnX(m), motorBtnY(m), MOTOR_BTN_W, h)) {
          bool mine = (products[i].motorMask & (1 << m)) != 0;
          if (mine) {
            products[i].motorMask &= ~(1 << m);
          } else {
            for (int p = 0; p < MAX_PRODUCTS; p++) {
              if (p == i) continue;
              if (products[p].motorMask & (1 << m)) {
                products[p].motorMask &= ~(1 << m);
                saveMaskSlot(p);
              }
            }
            products[i].motorMask |= (1 << m);
          }
          saveMaskSlot(i);
          drawAdminMotorAssignScreen();
          return;
        }
      }
    }
  }
}

// =====================================================
//           ADMIN MOTOR STOCK SCREEN
// =====================================================
// One row per motor, MOTORS_PER_PAGE (Core_02_AppState.ino) to a page:
//
//   <product name>     [Test]  [-]  [ qty ]  [+]
//   Motor 1
//
// Tapping the quantity box opens a keypad to type an exact figure; - and +
// step by one. A quantity at or below the low-stock alert level is drawn in
// the danger colour so a nearly empty motor stands out without opening
// anything. Test runs the motor for the product's normal dispense time.
const int MSTOCK_ROW_X = 16, MSTOCK_ROW_W = 288, MSTOCK_ROW_H = 34;
const int MSTOCK_ROW_Y0 = 40, MSTOCK_ROW_GAP = 4;
const int MSTOCK_RIGHT = MSTOCK_ROW_X + MSTOCK_ROW_W - 8;   // right edge of the stepper
const int MSTOCK_TEST_W = 44;

int motorStockRowY(int row) {
  return MSTOCK_ROW_Y0 + row * (MSTOCK_ROW_H + MSTOCK_ROW_GAP);
}
int motorStockRowCY(int row) { return motorStockRowY(row) + MSTOCK_ROW_H / 2; }
int motorStockTestX() { return auiStepLeftX(MSTOCK_RIGHT) - 6 - MSTOCK_TEST_W; }

// ---------- Edit keypad (type an exact stock figure) ----------
// A modal drawn over this screen rather than a screen of its own: motorEditing
// is the motor being edited, or -1 when the normal list is showing. The keys
// take the left of the screen and Save/Cancel the right, so there is no bottom
// bar on this view and the keys can be full height.
const int MKP_X0 = 16, MKP_Y0 = 78, MKP_W = 62, MKP_H = 32, MKP_GX = 6, MKP_GY = 4;
const int MKP_SIDE_X = 226, MKP_SIDE_W = 78, MKP_SIDE_H = 68;
const char* const MKP_LABELS[12] = {"1","2","3","4","5","6","7","8","9","Clear","0","Del"};

void getKeypadKeyRect(int k, int &x, int &y) {
  x = MKP_X0 + (k % 3) * (MKP_W + MKP_GX);
  y = MKP_Y0 + (k / 3) * (MKP_H + MKP_GY);
}

void drawMotorEditValue() {
  tft.fillRoundRect(16, 38, 288, 34, 8, COL_CARD);
  tft.drawRoundRect(16, 38, 288, 34, 8, COL_CARD_BRD);
  auiPut(motorEditBuf[0] ? motorEditBuf : "0", AL_CENTER, 24, 272, 55, COL_ACCENT, FACE_LG);
  char lim[16];
  snprintf(lim, sizeof(lim), "max %d", STOCK_MAX);
  auiPut(lim, AL_RIGHT, 200, 96, 55, COL_TEXT_DIM, FACE_XS);
}

void drawMotorEditKeypad() {
  drawGradientBackground();
  char title[24];
  snprintf(title, sizeof(title), "Motor %d Stock", motorEditing + 1);
  drawScreenTitle(title);
  drawMotorEditValue();

  for (int k = 0; k < 12; k++) {
    int x, y;
    getKeypadKeyRect(k, x, y);
    auiButton(x, y, MKP_W, MKP_H, MKP_LABELS[k], ABTN_SECONDARY, (k == 9 || k == 11) ? FACE_SM : FACE_MD);
  }

  auiButton(MKP_SIDE_X, MKP_Y0, MKP_SIDE_W, MKP_SIDE_H, "Save", ABTN_PRIMARY, FACE_MD);
  auiButton(MKP_SIDE_X, MKP_Y0 + MKP_SIDE_H + MKP_GY, MKP_SIDE_W, MKP_SIDE_H, "Cancel", ABTN_SECONDARY, FACE_MD);
}

void handleMotorEditKeypad() {
  if (!isRealTouch() || millis() - lastTouchTime <= TOUCH_DEBOUNCE) return;
  lastTouchTime = millis();
  TS_Point raw = ts.getPoint();
  int sx, sy;
  mapTouchToScreen(raw, sx, sy);

  if (pointInRect(sx, sy, MKP_SIDE_X, MKP_Y0 + MKP_SIDE_H + MKP_GY, MKP_SIDE_W, MKP_SIDE_H)) {
    motorEditing = -1;
    drawAdminMotorStockScreen();
    return;
  }
  if (pointInRect(sx, sy, MKP_SIDE_X, MKP_Y0, MKP_SIDE_W, MKP_SIDE_H)) {
    int m = motorEditing;
    motorStock[m] = constrain(atoi(motorEditBuf), 0, STOCK_MAX);
    saveMotorStockSlot(m);
    refreshLowStockArm();
    motorEditing = -1;
    drawAdminMotorStockScreen();
    return;
  }

  for (int k = 0; k < 12; k++) {
    int x, y;
    getKeypadKeyRect(k, x, y);
    if (!pointInRect(sx, sy, x, y, MKP_W, MKP_H)) continue;
    size_t len = strlen(motorEditBuf);
    if (k == 9) {
      motorEditBuf[0] = '\0';
    } else if (k == 11) {
      if (len > 0) motorEditBuf[len - 1] = '\0';
    } else if (len < sizeof(motorEditBuf) - 1) {
      char digit = MKP_LABELS[k][0];
      char next[8];
      snprintf(next, sizeof(next), "%s%c", motorEditBuf, digit);
      // A value above the configured maximum is simply not accepted.
      if (atoi(next) <= STOCK_MAX) {
        strncpy(motorEditBuf, next, sizeof(motorEditBuf) - 1);
        motorEditBuf[sizeof(motorEditBuf) - 1] = '\0';
        // No leading zeros: "0" then "5" reads as 5.
        if (motorEditBuf[0] == '0' && motorEditBuf[1] != '\0') memmove(motorEditBuf, motorEditBuf + 1, strlen(motorEditBuf));
      }
    }
    drawMotorEditValue();
    return;
  }
}

void drawAdminMotorStockScreen() {
  if (motorEditing >= 0) { drawMotorEditKeypad(); return; }
  drawGradientBackground();

  drawScreenTitle("Motor Stock");

  int firstMotor = motorStockPage * MOTORS_PER_PAGE;
  int rowsOnPage = min(MOTORS_PER_PAGE, MAX_MOTORS - firstMotor);

  for (int row = 0; row < rowsOnPage; row++) {
    int m = firstMotor + row;
    int y = motorStockRowY(row);
    int cy = motorStockRowCY(row);
    auiRow(MSTOCK_ROW_X, y, MSTOCK_ROW_W, MSTOCK_ROW_H);

    int owner = productForMotor(m);
    int labelW = motorStockTestX() - 8 - (MSTOCK_ROW_X + 10);
    if (owner == -1) auiPut("Unassigned", AL_LEFT, MSTOCK_ROW_X + 10, labelW, y + 11, COL_TEXT_DIM, FACE_MD);
    else             auiPut(products[owner].name, AL_LEFT, MSTOCK_ROW_X + 10, labelW, y + 11, COL_TEXT, FACE_MD);
    char sub[12];
    snprintf(sub, sizeof(sub), "Motor %d", m + 1);
    auiPut(sub, AL_LEFT, MSTOCK_ROW_X + 10, labelW, y + 26, COL_TEXT_DIM, FACE_XS);

    auiRowButton(motorStockTestX(), cy, MSTOCK_TEST_W, "Test", ABTN_SECONDARY);

    char qbuf[6];
    snprintf(qbuf, sizeof(qbuf), "%d", motorStock[m]);
    bool low = motorIsActive(m) && motorStock[m] <= lowStockThreshold;
    auiStepper(MSTOCK_RIGHT, cy, qbuf, low ? COL_DANGER : COL_ACCENT);
  }

  if (rowsOnPage < MOTORS_PER_PAGE) {
    int hintY = motorStockRowY(rowsOnPage) + 8;
    if (hintY <= 186) auiHint("Tap a quantity to type an exact figure", hintY);
  }

  if (MOTOR_STOCK_PAGES > 1) {
    char pageLabel[16];
    snprintf(pageLabel, sizeof(pageLabel), "Page %d/%d", motorStockPage + 1, MOTOR_STOCK_PAGES);
    auiBarButton(0, 2, "Back", ABTN_SECONDARY);
    auiBarButton(1, 2, pageLabel, ABTN_PRIMARY);
  } else {
    auiBarButton(0, 1, "Back", ABTN_SECONDARY);
  }
}

void handleAdminMotorStockScreen() {
  if (motorEditing >= 0) { handleMotorEditKeypad(); return; }
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      int slots = (MOTOR_STOCK_PAGES > 1) ? 2 : 1;
      if (auiBarHit(sx, sy, 0, slots)) {
        currentScreen = SCREEN_ADMIN_PANEL;
        drawAdminPanelScreen();
        return;
      }

      if (MOTOR_STOCK_PAGES > 1 && auiBarHit(sx, sy, 1, slots)) {
        motorStockPage = (motorStockPage + 1) % MOTOR_STOCK_PAGES;
        drawAdminMotorStockScreen();
        return;
      }

      int firstMotor = motorStockPage * MOTORS_PER_PAGE;
      int rowsOnPage = min(MOTORS_PER_PAGE, MAX_MOTORS - firstMotor);

      for (int row = 0; row < rowsOnPage; row++) {
        int m = firstMotor + row;
        int cy = motorStockRowCY(row);

        if (auiRowButtonHit(sx, sy, motorStockTestX(), cy, MSTOCK_TEST_W)) {
          int owner = productForMotor(m);
          runMotorPulse(m, (owner >= 0) ? runTimeForProduct(owner) : MOTOR_TEST_MS);
          lastTouchTime = millis();
          return;
        }

        int step = auiStepTouch(sx, sy, MSTOCK_RIGHT, cy);
        if (step < 0) {
          if (motorStock[m] > 0) motorStock[m]--;
          saveMotorStockSlot(m);
          refreshLowStockArm();
          drawAdminMotorStockScreen();
          return;
        }
        if (step > 0) {
          // Capped at STOCK_MAX (CFG_STOCK_MAX from Config.h), not an
          // arbitrary 99 — that's the ceiling a technician flashed for this
          // machine's chute/hopper capacity.
          if (motorStock[m] < STOCK_MAX) motorStock[m]++;
          saveMotorStockSlot(m);
          // Re-arms the low-stock alert for a motor that's just been
          // refilled, without mailing anything mid-refill.
          refreshLowStockArm();
          drawAdminMotorStockScreen();
          return;
        }

        if (pointInRect(sx, sy, auiStepValX(MSTOCK_RIGHT), cy - ASTEP_H / 2, ASTEP_VAL_W, ASTEP_H)) {
          motorEditing = m;
          snprintf(motorEditBuf, sizeof(motorEditBuf), "%d", motorStock[m]);
          drawMotorEditKeypad();
          return;
        }
      }
    }
  }
}
