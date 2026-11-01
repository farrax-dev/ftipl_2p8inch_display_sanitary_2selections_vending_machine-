// =====================================================
//               ADMIN MACHINE SCREEN
// =====================================================
// The machine-level settings that aren't about products, payments or the
// network:
//
//   Machine ID - identifies the unit in reports        value  [Edit]
//   Store ID   - printed under the machine ID in e-mails value [Edit]
//   Admin PIN  - never shown, only changed             ****   [Change]
//   Clock      - current time; opens Screen_20          value  [Set]
//   Customer   - max items + idle timeout; Screen_25     value  [Open]
//
// On the old Settings screen these were squeezed in beside the payment
// toggles, with Machine ID and PIN sharing a single 20px strip.
const int MACH_ROW_X = 16, MACH_ROW_W = 288, MACH_ROW_H = 28;
const int MACH_ROW_Y0 = 40, MACH_ROW_GAP = 3;
const int MACH_RIGHT = MACH_ROW_X + MACH_ROW_W - 8;
const int MACH_BTN_W = 72;
const int MACH_BTN_X = MACH_RIGHT - MACH_BTN_W;

int machRowY(int row) { return MACH_ROW_Y0 + row * (MACH_ROW_H + MACH_ROW_GAP); }
int machRowCY(int row) { return machRowY(row) + MACH_ROW_H / 2; }

void drawMachineValueRow(int row, const char* label, const char* value, uint16_t valCol,
                         const char* button, int valFace) {
  int cy = machRowCY(row);
  auiRow(MACH_ROW_X, machRowY(row), MACH_ROW_W, MACH_ROW_H);
  auiRowLabel(MACH_ROW_X + 12, cy, 110, label);
  int vx = MACH_ROW_X + 12 + auiTextW(label, FACE_MD) + 12;
  auiPut(value, AL_LEFT, vx, MACH_BTN_X - 8 - vx, cy, valCol, valFace);
  auiRowButton(MACH_BTN_X, cy, MACH_BTN_W, button, ABTN_PRIMARY);
}

void drawAdminMachineScreen() {
  drawGradientBackground();
  drawScreenTitle(TXT_MACHINE);

  // ---- Machine ID ----
  drawMachineValueRow(0, TXT_MACHINE_ID, strlen(machineId) > 0 ? machineId : TXT_NOT_SET,
                      COL_VALUE_TEXT, TXT_EDIT, FACE_MD);

  // ---- Store ID (printed under the machine ID in report e-mails) ----
  drawMachineValueRow(1, TXT_STORE_ID, strlen(storeId) > 0 ? storeId : TXT_NOT_SET,
                      strlen(storeId) > 0 ? COL_VALUE_TEXT : COL_CARD_CAPTION, TXT_EDIT, FACE_MD);

  // ---- Admin PIN: a fixed placeholder, never the value — it's a credential,
  // not something to glance at ----
  drawMachineValueRow(2, TXT_ADMIN_PIN, "****", COL_VALUE_TEXT, TXT_CHANGE, FACE_MD);

  // ---- Clock ----
  char timeBuf[16], dateBuf[24], clockBuf[44];
  if (getClockStrings(timeBuf, sizeof(timeBuf), dateBuf, sizeof(dateBuf)))
    snprintf(clockBuf, sizeof(clockBuf), "%s  %s", timeBuf, dateBuf);
  else
    snprintf(clockBuf, sizeof(clockBuf), TXT_NOT_SET_LOWER);
  drawMachineValueRow(3, TXT_CLOCK, clockBuf, COL_VALUE_TEXT, TXT_SET, FACE_SM);

  // ---- Customer screens: max items per order + idle timeout ----
  char cust[24];
  if (idleTimeoutOn) snprintf(cust, sizeof(cust), "Timeout %ds", idleTimeoutSec);
  else               snprintf(cust, sizeof(cust), TXT_TIMEOUT_OFF);
  drawMachineValueRow(4, TXT_CUSTOMER, cust, COL_VALUE_TEXT, TXT_OPEN, FACE_SM);

  auiBarButton(0, 1, TXT_BACK, ABTN_SECONDARY);
}

void handleAdminMachineScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (auiBarHit(sx, sy, 0, 1)) {
        currentScreen = SCREEN_ADMIN_PANEL;
        drawAdminPanelScreen();
        return;
      }

      if (pointInRect(sx, sy, MACH_ROW_X, machRowY(0), MACH_ROW_W, MACH_ROW_H)) {
        openTextEntry(TE_MACHINE_ID);
        return;
      }
      if (pointInRect(sx, sy, MACH_ROW_X, machRowY(1), MACH_ROW_W, MACH_ROW_H)) {
        openTextEntry(TE_STORE_ID);
        return;
      }
      if (pointInRect(sx, sy, MACH_ROW_X, machRowY(2), MACH_ROW_W, MACH_ROW_H)) {
        openTextEntry(TE_ADMIN_PIN);
        return;
      }
      if (pointInRect(sx, sy, MACH_ROW_X, machRowY(3), MACH_ROW_W, MACH_ROW_H)) {
        openAdminDateTimeScreen();
        return;
      }
      if (pointInRect(sx, sy, MACH_ROW_X, machRowY(4), MACH_ROW_W, MACH_ROW_H)) {
        currentScreen = SCREEN_ADMIN_CUSTOMER;
        drawAdminCustomerScreen();
        return;
      }
    }
  }
}
