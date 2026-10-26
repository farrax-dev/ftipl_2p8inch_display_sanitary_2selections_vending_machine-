// =====================================================
//                  ADMIN EDIT SCREEN
// =====================================================
// Motor run time is one value for every product, set in Config.h
// (CFG_MOTOR_RUN_MS), and is deliberately not editable here — see
// runTimeForProduct() in Core_09_Storage.ino.
//
// Four rows, one setting each, every control sized for a fingertip:
//   Name     - value + Edit button (opens the keyboard); the whole row taps
//   Price    - [-] value [+]
//   Motors   - value + Assign button (opens the motor grid); the whole row taps
//   Visible  - switch; the whole row taps
const int EDIT_ROW_X = 16, EDIT_ROW_W = 288, EDIT_ROW_H = 32;
const int EDIT_ROW_Y0 = 40, EDIT_ROW_GAP = 6;
const int EDIT_BTN_W = 72;
const int EDIT_CTRL_RIGHT = EDIT_ROW_X + EDIT_ROW_W - 8;   // right edge of every control
const int EDIT_BTN_X = EDIT_CTRL_RIGHT - EDIT_BTN_W;

int editRowY(int row) { return EDIT_ROW_Y0 + row * (EDIT_ROW_H + EDIT_ROW_GAP); }
int editRowCY(int row) { return editRowY(row) + EDIT_ROW_H / 2; }

// Left edge of the value text on rows that show one, just past the label.
int editValueX(const char* label) {
  return EDIT_ROW_X + 12 + auiTextW(label, FACE_MD) + 12;
}

void drawAdminEditScreen() {
  int i = adminEditingSlot;
  drawGradientBackground();

  char titleBuf[20];
  snprintf(titleBuf, sizeof(titleBuf), "Edit Product %d", i + 1);
  drawScreenTitle(titleBuf);

  for (int r = 0; r < 4; r++) auiRow(EDIT_ROW_X, editRowY(r), EDIT_ROW_W, EDIT_ROW_H);

  // ---- Name ----
  int cy = editRowCY(0);
  auiRowLabel(EDIT_ROW_X + 12, cy, 80, "Name");
  int vx = editValueX("Name");
  auiPut(products[i].name, AL_LEFT, vx, EDIT_BTN_X - 8 - vx, cy, COL_ACCENT, FACE_MD);
  auiRowButton(EDIT_BTN_X, cy, EDIT_BTN_W, "Edit", ABTN_PRIMARY);

  // ---- Price ----
  cy = editRowCY(1);
  auiRowLabel(EDIT_ROW_X + 12, cy, auiStepLeftX(EDIT_CTRL_RIGHT) - EDIT_ROW_X - 20, "Price (Rs.)");
  char pbuf[8];
  snprintf(pbuf, sizeof(pbuf), "%d", products[i].price);
  auiStepper(EDIT_CTRL_RIGHT, cy, pbuf, COL_ACCENT);

  // ---- Motors ----
  cy = editRowCY(2);
  auiRowLabel(EDIT_ROW_X + 12, cy, 90, "Motors");
  char motorBuf[16];  // worst case "1,2,3,4,5" across all MAX_MOTORS
  formatMotorList(products[i].motorMask, motorBuf, sizeof(motorBuf));
  vx = editValueX("Motors");
  auiPut(motorBuf, AL_LEFT, vx, EDIT_BTN_X - 8 - vx, cy, COL_ACCENT, FACE_MD);
  auiRowButton(EDIT_BTN_X, cy, EDIT_BTN_W, "Assign", ABTN_PRIMARY);

  // ---- Visible ----
  cy = editRowCY(3);
  auiRowLabel(EDIT_ROW_X + 12, cy, 140, "Visible");
  auiOnOff(EDIT_CTRL_RIGHT, cy, productEnabled[i]);

  auiBarButton(0, 1, "Back", ABTN_SECONDARY);
}

void handleAdminEditScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);
      int i = adminEditingSlot;

      if (auiBarHit(sx, sy, 0, 1)) {
        currentScreen = SCREEN_ADMIN_PRODUCTS;
        drawAdminProductsScreen();
        return;
      }

      if (pointInRect(sx, sy, EDIT_ROW_X, editRowY(0), EDIT_ROW_W, EDIT_ROW_H)) {
        openTextEntry(TE_PRODUCT_NAME);
        return;
      }

      int step = auiStepTouch(sx, sy, EDIT_CTRL_RIGHT, editRowCY(1));
      if (step < 0) {
        if (products[i].price > 1) products[i].price--;
        savePriceSlot(i);
        drawAdminEditScreen();
        return;
      }
      if (step > 0) {
        if (products[i].price < 999) products[i].price++;
        savePriceSlot(i);
        drawAdminEditScreen();
        return;
      }

      if (pointInRect(sx, sy, EDIT_ROW_X, editRowY(2), EDIT_ROW_W, EDIT_ROW_H)) {
        currentScreen = SCREEN_ADMIN_MOTOR_ASSIGN;
        drawAdminMotorAssignScreen();
        return;
      }

      if (pointInRect(sx, sy, EDIT_ROW_X, editRowY(3), EDIT_ROW_W, EDIT_ROW_H)) {
        productEnabled[i] = !productEnabled[i];
        saveEnabledSlot(i);
        drawAdminEditScreen();
        return;
      }
    }
  }
}
