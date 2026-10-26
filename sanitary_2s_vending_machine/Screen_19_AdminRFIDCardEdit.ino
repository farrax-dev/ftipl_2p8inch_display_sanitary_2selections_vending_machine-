// =====================================================
//        ADMIN — EDIT RFID CARD (name / limit / usage)
// =====================================================
// Opened by tapping a card row (not its delete "X") on
// Screen_18_AdminRFIDCards.ino. rfidEditingCard (Core_02_AppState.ino) holds
// which rfidCards[] slot this operates on — the RFID-card equivalent of
// adminEditingSlot for products.
//
// withdrawLimit is a running total, not a per-day/per-tap cap: 0 means
// unlimited, otherwise the card is refused once withdrawUsed would exceed it
// (Screen_06_PaymentOther.ino's handlePaymentRFIDScreen()). Usage is zeroed
// either with "Reset" here (one card) or by the monthly schedule on
// Screen_21_AdminRFIDReset.ino (every card).
//
// Four rows in the same arrangement as Edit Product (Screen_09):
//   Name    - value + Edit; the whole row taps
//   Card ID - read-only (re-register the card to change it)
//   Limit   - [-] value [+]   (Off = unlimited)
//   Used    - value + Reset
const int RFEDIT_ROW_X = 16, RFEDIT_ROW_W = 288, RFEDIT_ROW_H = 32;
const int RFEDIT_ROW_Y0 = 40, RFEDIT_ROW_GAP = 6;
const int RFEDIT_RIGHT = RFEDIT_ROW_X + RFEDIT_ROW_W - 8;
const int RFEDIT_BTN_W = 72;
const int RFEDIT_BTN_X = RFEDIT_RIGHT - RFEDIT_BTN_W;

int rfeditRowY(int row) { return RFEDIT_ROW_Y0 + row * (RFEDIT_ROW_H + RFEDIT_ROW_GAP); }
int rfeditRowCY(int row) { return rfeditRowY(row) + RFEDIT_ROW_H / 2; }

void drawAdminRFIDCardEditScreen() {
  int i = rfidEditingCard;
  drawGradientBackground();

  drawScreenTitle("Edit Card");

  for (int r = 0; r < 4; r++) auiRow(RFEDIT_ROW_X, rfeditRowY(r), RFEDIT_ROW_W, RFEDIT_ROW_H);

  // ---- Name ----
  int cy = rfeditRowCY(0);
  auiRowLabel(RFEDIT_ROW_X + 12, cy, 80, "Name");
  int vx = RFEDIT_ROW_X + 12 + auiTextW("Name", FACE_MD) + 12;
  bool named = rfidCards[i].name[0] != '\0';
  auiPut(named ? rfidCards[i].name : "(no name)", AL_LEFT, vx, RFEDIT_BTN_X - 8 - vx, cy,
         named ? COL_ACCENT : COL_TEXT_DIM, FACE_MD);
  auiRowButton(RFEDIT_BTN_X, cy, RFEDIT_BTN_W, "Edit", ABTN_PRIMARY);

  // ---- UID (read-only) ----
  cy = rfeditRowCY(1);
  auiRowLabel(RFEDIT_ROW_X + 12, cy, 100, "Card ID");
  vx = RFEDIT_ROW_X + 12 + auiTextW("Card ID", FACE_MD) + 12;
  auiPut(rfidCards[i].uid, AL_LEFT, vx, RFEDIT_RIGHT - vx, cy, COL_TEXT, FACE_SM);

  // ---- Withdrawal limit (0 = unlimited) ----
  cy = rfeditRowCY(2);
  auiRowLabel(RFEDIT_ROW_X + 12, cy, auiStepLeftX(RFEDIT_RIGHT) - RFEDIT_ROW_X - 20, "Limit");
  char limBuf[8];
  if (rfidCards[i].withdrawLimit > 0) snprintf(limBuf, sizeof(limBuf), "%d", rfidCards[i].withdrawLimit);
  else strcpy(limBuf, "Off");
  auiStepper(RFEDIT_RIGHT, cy, limBuf, COL_ACCENT);

  // ---- Usage so far, and a per-card reset ----
  cy = rfeditRowCY(3);
  auiRowLabel(RFEDIT_ROW_X + 12, cy, 80, "Used");
  char usedBuf[8];
  snprintf(usedBuf, sizeof(usedBuf), "%d", rfidCards[i].withdrawUsed);
  vx = RFEDIT_ROW_X + 12 + auiTextW("Used", FACE_MD) + 12;
  auiPut(usedBuf, AL_LEFT, vx, 60, cy, COL_ACCENT, FACE_MD);
  auiRowButton(RFEDIT_BTN_X, cy, RFEDIT_BTN_W, "Reset", ABTN_DANGER);

  auiBarButton(0, 1, "Back", ABTN_SECONDARY);
}

void handleAdminRFIDCardEditScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);
      int i = rfidEditingCard;

      if (auiBarHit(sx, sy, 0, 1)) {
        currentScreen = SCREEN_ADMIN_RFID_CARDS;
        drawAdminRFIDCardsScreen();
        return;
      }

      if (pointInRect(sx, sy, RFEDIT_ROW_X, rfeditRowY(0), RFEDIT_ROW_W, RFEDIT_ROW_H)) {
        openTextEntry(TE_RFID_CARD_NAME);
        return;
      }

      int step = auiStepTouch(sx, sy, RFEDIT_RIGHT, rfeditRowCY(2));
      if (step < 0) {
        if (rfidCards[i].withdrawLimit > 0) setRFIDCardLimit(i, rfidCards[i].withdrawLimit - 1);
        drawAdminRFIDCardEditScreen();
        return;
      }
      if (step > 0) {
        if (rfidCards[i].withdrawLimit < 999) setRFIDCardLimit(i, rfidCards[i].withdrawLimit + 1);
        drawAdminRFIDCardEditScreen();
        return;
      }

      if (auiRowButtonHit(sx, sy, RFEDIT_BTN_X, rfeditRowCY(3), RFEDIT_BTN_W)) {
        resetRFIDCardUsage(i);
        drawAdminRFIDCardEditScreen();
        return;
      }
    }
  }
}
