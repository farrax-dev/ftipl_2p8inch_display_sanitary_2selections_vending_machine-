// =====================================================
//           ADMIN MOTOR ASSIGN SCREEN
// =====================================================
// 4 columns x 3 rows fits all MAX_MOTORS buttons (up to 12) on a 320x240
// screen; currently 11 are populated, leaving the last grid slot empty.
const int MOTOR_BTN_W = 68, MOTOR_BTN_H = 42, MOTOR_BTN_GX = 6, MOTOR_BTN_GY = 6;
const int MOTOR_BTN_X0 = 16, MOTOR_BTN_Y0 = 46;
const int MOTOR_BTN_COLS = 4;

void getMotorBtnRect(int m, int &x, int &y, int &w, int &h) {
  int col = m % MOTOR_BTN_COLS;
  int row = m / MOTOR_BTN_COLS;
  x = MOTOR_BTN_X0 + col * (MOTOR_BTN_W + MOTOR_BTN_GX);
  y = MOTOR_BTN_Y0 + row * (MOTOR_BTN_H + MOTOR_BTN_GY);
  w = MOTOR_BTN_W;
  h = MOTOR_BTN_H;
}

void drawAdminMotorAssignScreen() {
  int i = adminEditingSlot;
  drawGradientBackground();

  char titleBuf[22];
  snprintf(titleBuf, sizeof(titleBuf), "Assign Motors: P%d", i + 1);
  drawScreenTitle(titleBuf);

  for (int m = 0; m < MAX_MOTORS; m++) {
    int x, y, w, h;
    getMotorBtnRect(m, x, y, w, h);

    bool mine = (products[i].motorMask & (1 << m)) != 0;
    int owner = productForMotor(m);

    uint16_t fill = mine ? COL_ACCENT : COL_CARD;
    drawCardShadow(x, y, w, h, 8);
    tft.fillRoundRect(x, y, w, h, 8, fill);
    tft.drawRoundRect(x, y, w, h, 8, COL_CARD_BRD);

    tft.setTextSize(2);
    tft.setTextColor(mine ? COL_BG_TOP : COL_TEXT, fill);
    char label[4];
    snprintf(label, sizeof(label), "M%d", m + 1);
    centerTextInBox(label, y + 6, x, w);

    tft.setTextSize(1);
    char caption[10];
    if (mine) snprintf(caption, sizeof(caption), "Mine");
    else if (owner == -1) snprintf(caption, sizeof(caption), "Unused");
    else snprintf(caption, sizeof(caption), "%.9s", products[owner].name);
    tft.setTextColor(mine ? COL_BG_TOP : COL_TEXT_DIM, fill);
    centerTextInBox(caption, y + 27, x, w);
  }

  drawBackButton("< Back");
}

void handleAdminMotorAssignScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);
      int i = adminEditingSlot;

      if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_EDIT;
        drawAdminEditScreen();
        return;
      }

      for (int m = 0; m < MAX_MOTORS; m++) {
        int x, y, w, h;
        getMotorBtnRect(m, x, y, w, h);
        if (pointInRect(sx, sy, x, y, w, h)) {
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
// All MAX_MOTORS rows don't fit at once with usable touch targets on a
// 320x240 screen, so this paginates MOTORS_PER_PAGE at a time (the original
// row layout/sizing, proven at 6-per-page, is unchanged).
const int MSTOCK_ROW_X = 20, MSTOCK_ROW_W = 280, MSTOCK_ROW_H = 21;
const int MSTOCK_ROW_Y0 = 44, MSTOCK_ROW_GAP = 3;

// MOTORS_PER_PAGE, MOTOR_STOCK_PAGES, and motorStockPage live in
// Core_02_AppState.ino — see the comment there for why.

int motorStockRowY(int row) {
  return MSTOCK_ROW_Y0 + row * (MSTOCK_ROW_H + MSTOCK_ROW_GAP);
}

void drawAdminMotorStockScreen() {
  drawGradientBackground();

  drawScreenTitle("Motor Stock");

  int firstMotor = motorStockPage * MOTORS_PER_PAGE;
  int rowsOnPage = min(MOTORS_PER_PAGE, MAX_MOTORS - firstMotor);

  for (int row = 0; row < rowsOnPage; row++) {
    int m = firstMotor + row;
    int y = motorStockRowY(row);
    int h = MSTOCK_ROW_H;
    drawCard(MSTOCK_ROW_X, y, MSTOCK_ROW_W, h, 5);

    int owner = productForMotor(m);
    char label[20];
    if (owner == -1) snprintf(label, sizeof(label), "M%d: (unassigned)", m + 1);
    else snprintf(label, sizeof(label), "M%d: %.10s", m + 1, products[owner].name);

    int btnSize = constrain(h - 6, 14, 26);
    int plusX = MSTOCK_ROW_X + MSTOCK_ROW_W - 8 - btnSize;
    int qtyBoxW = 30;
    int qtyBoxX = plusX - 4 - qtyBoxW;
    int minusX = qtyBoxX - 4 - btnSize;
    int testX  = minusX - 6 - btnSize;
    int btnY = y + (h - btnSize) / 2;

    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT, COL_CARD);
    tft.setCursor(MSTOCK_ROW_X + 6, y + (h - 8) / 2);
    tft.print(label);

    tft.fillRoundRect(testX, btnY, btnSize, btnSize, 4, COL_BG_TOP);
    tft.drawRoundRect(testX, btnY, btnSize, btnSize, 4, COL_CARD_BRD);
    tft.setTextColor(COL_ACCENT, COL_BG_TOP);
    centerTextInBox("T", btnY + btnSize / 2 - 4, testX, btnSize);

    tft.fillRoundRect(minusX, btnY, btnSize, btnSize, 4, COL_ACCENT);
    tft.setTextColor(COL_BG_TOP, COL_ACCENT);
    centerTextInBox("-", btnY + btnSize / 2 - 4, minusX, btnSize);

    // Rounded to match the "T"/"-"/"+" buttons either side of it — was a
    // sharp-cornered fillRect, the one shape on the row that didn't match.
    // No shadow: it's nested on an already-shadowed row card, and a
    // read-only figure, not a control.
    tft.fillRoundRect(qtyBoxX, btnY, qtyBoxW, btnSize, 4, COL_CARD);
    tft.setTextColor(COL_TEXT, COL_CARD);
    char qbuf[4];
    snprintf(qbuf, sizeof(qbuf), "%d", motorStock[m]);
    centerTextInBox(qbuf, btnY + btnSize / 2 - 4, qtyBoxX, qtyBoxW);

    tft.fillRoundRect(plusX, btnY, btnSize, btnSize, 4, COL_ACCENT);
    tft.setTextColor(COL_BG_TOP, COL_ACCENT);
    centerTextInBox("+", btnY + btnSize / 2 - 4, plusX, btnSize);
  }

  drawBackButton("< Back");

  // With MAX_MOTORS at 5 every motor fits on one page, so the pager would be
  // a button that visibly does nothing. Drawn only when it has somewhere to
  // go — the paging code stays for a build with more motors.
  if (MOTOR_STOCK_PAGES > 1) {
    char pageLabel[16];
    snprintf(pageLabel, sizeof(pageLabel), "Page %d/%d >", motorStockPage + 1, MOTOR_STOCK_PAGES);
    drawProceedButton(pageLabel);
  }
}

void handleAdminMotorStockScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_PANEL;
        drawAdminPanelScreen();
        return;
      }

      if (MOTOR_STOCK_PAGES > 1 &&
          pointInRect(sx, sy, BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H)) {
        motorStockPage = (motorStockPage + 1) % MOTOR_STOCK_PAGES;
        drawAdminMotorStockScreen();
        return;
      }

      int firstMotor = motorStockPage * MOTORS_PER_PAGE;
      int rowsOnPage = min(MOTORS_PER_PAGE, MAX_MOTORS - firstMotor);

      for (int row = 0; row < rowsOnPage; row++) {
        int m = firstMotor + row;
        int y = motorStockRowY(row);
        int h = MSTOCK_ROW_H;
        int btnSize = constrain(h - 6, 14, 26);
        int plusX = MSTOCK_ROW_X + MSTOCK_ROW_W - 8 - btnSize;
        int qtyBoxW = 30;
        int qtyBoxX = plusX - 4 - qtyBoxW;
        int minusX = qtyBoxX - 4 - btnSize;
        int testX  = minusX - 6 - btnSize;
        int btnY = y + (h - btnSize) / 2;

        if (pointInRect(sx, sy, testX, btnY, btnSize, btnSize)) {
          int owner = productForMotor(m);
          runMotorPulse(m, (owner >= 0) ? runTimeForProduct(owner) : MOTOR_TEST_MS);
          lastTouchTime = millis();
          return;
        }
        if (pointInRect(sx, sy, minusX, btnY, btnSize, btnSize)) {
          if (motorStock[m] > 0) motorStock[m]--;
          saveMotorStockSlot(m);
          refreshLowStockArm();
          drawAdminMotorStockScreen();
          return;
        }
        if (pointInRect(sx, sy, plusX, btnY, btnSize, btnSize)) {
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
      }
    }
  }
}
