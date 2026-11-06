// =====================================================
//           ADMIN RFID CARD REGISTRATION SCREEN
// =====================================================
// The registered list (Core_09_Storage.ino: rfidCards[]/rfidCardCount) is the
// RFID payment check — a tap that matches vends for free (subject to that
// card's withdrawal limit, if any), anything else is refused
// (Screen_06_PaymentOther.ino). This screen is how that list is built: scan
// a physical card, or type its number in by hand when scanning isn't
// convenient. Tapping a registered row (not its "X") opens Name/Limit/Usage
// editing — Screen_19_AdminRFIDCardEdit.ino.
//
// Three 34px rows to a page: the card's name over its UID (or just the UID,
// when it has no name), the usage badge for limited cards, and a delete
// button. Scan and Enter buttons sit under the list; Back and the pager are
// in the bottom bar.
const int RFCARD_ROW_X = 16, RFCARD_ROW_W = 288, RFCARD_ROW_H = 34;
const int RFCARD_ROW_Y0 = 38, RFCARD_ROW_GAP = 4;
const int RFCARD_ROWS_PER_PAGE = 3;
const int RFCARD_DEL_W = 32, RFCARD_DEL_H = 26;

const int RFCARD_ACTION_Y = 166, RFCARD_ACTION_H = 26;
const int RFCARD_MSG_CY = 157;

int rfidCardsPageCount() {
  int pages = (rfidCardCount + RFCARD_ROWS_PER_PAGE - 1) / RFCARD_ROWS_PER_PAGE;
  return (pages < 1) ? 1 : pages;
}

int rfidCardRowY(int row) {
  return RFCARD_ROW_Y0 + row * (RFCARD_ROW_H + RFCARD_ROW_GAP);
}

int rfidDelX() { return RFCARD_ROW_X + RFCARD_ROW_W - 8 - RFCARD_DEL_W; }
int rfidDelY(int row) { return rfidCardRowY(row) + (RFCARD_ROW_H - RFCARD_DEL_H) / 2; }

// Two equal buttons under the list.
int rfidActionX(int slot) { return RFCARD_ROW_X + slot * (RFCARD_ROW_W / 2 + 2); }
int rfidActionW() { return RFCARD_ROW_W / 2 - 2; }

// ---------- "waiting for a tap" overlay ----------
bool rfidCardsScanning = false;
unsigned long rfidCardsScanStartMs = 0;
const unsigned long RFID_SCAN_TIMEOUT_MS = 30000;

// rfidCardsMsg/rfidCardsMsgColor/rfidCardsMsgUntil live in Core_09_Storage.ino
// — see the comment there for why.

void drawRFIDCardsScanOverlay() {
  drawGradientBackground();
  drawScreenTitle(TXT_SCAN_NEW_CARD);

  auiPut(TXT_HOLD_THE_CARD_NEAR, AL_CENTER, 10, 300, 90, COL_MSG_TITLE, FACE_MD);
  auiPut(TXT_THE_READER, AL_CENTER, 10, 300, 116, COL_MSG_TITLE, FACE_MD);
  auiHint(TXT_CANCELS_AUTOMATICALLY_AFTER_30, 150);

  auiBarButton(0, 1, TXT_CANCEL, ABTN_SECONDARY);
}

void drawAdminRFIDCardsScreen() {
  int pages = rfidCardsPageCount();
  if (rfidCardsPage >= pages) rfidCardsPage = pages - 1;
  if (rfidCardsPage < 0) rfidCardsPage = 0;

  drawGradientBackground();

  drawScreenTitle(TXT_RFID_CARDS, 70);

  char countBuf[16];
  snprintf(countBuf, sizeof(countBuf), "%d / %d", rfidCardCount, rfidCardCap);
  auiPut(countBuf, AL_RIGHT, 200, headerRightX() - 200, HDR_Y + HDR_H / 2, COL_HDR_COUNT_DIM, FACE_SM);

  int first = rfidCardsPage * RFCARD_ROWS_PER_PAGE;
  int rowsOnPage = min(RFCARD_ROWS_PER_PAGE, rfidCardCount - first);

  if (rfidCardCount == 0) {
    auiPut(TXT_NO_CARDS_REGISTERED_YET, AL_CENTER, 10, 300, 100, COL_MSG_BODY, FACE_MD);
  } else {
    for (int row = 0; row < rowsOnPage; row++) {
      int i = first + row;
      int y = rfidCardRowY(row);
      auiRow(RFCARD_ROW_X, y, RFCARD_ROW_W, RFCARD_ROW_H);

      int rightEdge = rfidDelX() - 8;

      // A limit badge sits right-aligned before the delete button, so a
      // glance at the list shows who's close to their cap without opening
      // each card. Blank for unlimited cards (the common case) rather than
      // printing a distracting "0/0".
      int textW = rightEdge - (RFCARD_ROW_X + 12);
      if (rfidCards[i].withdrawLimit > 0) {
        char limBuf[12];
        snprintf(limBuf, sizeof(limBuf), "%d/%d", rfidCards[i].withdrawUsed, rfidCards[i].withdrawLimit);
        bool full = rfidCards[i].withdrawUsed >= rfidCards[i].withdrawLimit;
        auiPut(limBuf, AL_RIGHT, rightEdge - 56, 56, y + RFCARD_ROW_H / 2,
               full ? COL_ERR_TEXT : COL_VALUE_TEXT, FACE_SM);
        textW -= 62;
      }

      if (rfidCards[i].name[0]) {
        auiPut(rfidCards[i].name, AL_LEFT, RFCARD_ROW_X + 12, textW, y + 11, COL_CARD_TITLE, FACE_MD);
        auiPut(rfidCards[i].uid,  AL_LEFT, RFCARD_ROW_X + 12, textW, y + 26, COL_CARD_CAPTION, FACE_XS);
      } else {
        auiPut(rfidCards[i].uid, AL_LEFT, RFCARD_ROW_X + 12, textW, y + RFCARD_ROW_H / 2, COL_CARD_TITLE, FACE_MD);
      }

      auiButton(rfidDelX(), rfidDelY(row), RFCARD_DEL_W, RFCARD_DEL_H, "X", ABTN_DANGER, FACE_SM);
    }
  }

  if (rfidCardsMsg[0] != '\0') {
    auiPut(rfidCardsMsg, AL_CENTER, 12, 296, RFCARD_MSG_CY, rfidCardsMsgColor, FACE_SM);
  }

  bool full = (rfidCardCount >= rfidCardCap);
  int style = full ? ABTN_OFF : ABTN_PRIMARY;
  auiButton(rfidActionX(0), RFCARD_ACTION_Y, rfidActionW(), RFCARD_ACTION_H,
            full ? "List full" : "Scan Card", style, FACE_SM);
  auiButton(rfidActionX(1), RFCARD_ACTION_Y, rfidActionW(), RFCARD_ACTION_H,
            full ? "List full" : "Enter Number", style, FACE_SM);

  if (pages > 1) {
    char pageLabel[20];
    snprintf(pageLabel, sizeof(pageLabel), "%d/%d >", rfidCardsPage + 1, pages);
    auiBarButton(0, 3, TXT_BACK, ABTN_SECONDARY);
    auiBarButton(1, 3, "<", ABTN_SECONDARY);
    auiBarButton(2, 3, pageLabel, ABTN_PRIMARY);
  } else {
    auiBarButton(0, 1, TXT_BACK, ABTN_SECONDARY);
  }
}

void handleAdminRFIDCardsScreen() {
  if (rfidCardsMsg[0] != '\0' && millis() >= rfidCardsMsgUntil) {
    rfidCardsMsg[0] = '\0';
    drawAdminRFIDCardsScreen();
  }

  if (rfidCardsScanning) {
    char uid[21];
    if (pollRFIDCard(uid, sizeof(uid))) {
      rfidCardsScanning = false;
      if (addRFIDCard(uid)) {
        snprintf(rfidCardsMsg, sizeof(rfidCardsMsg), TXT_CARD_ADDED);
        rfidCardsMsgColor = COL_OK_TEXT;
      } else {
        snprintf(rfidCardsMsg, sizeof(rfidCardsMsg), TXT_ALREADY_REGISTERED);
        rfidCardsMsgColor = COL_WARN_TEXT;
      }
      rfidCardsMsgUntil = millis() + 2000;
      drawAdminRFIDCardsScreen();
      return;
    }

    if (millis() - rfidCardsScanStartMs > RFID_SCAN_TIMEOUT_MS) {
      rfidCardsScanning = false;
      drawAdminRFIDCardsScreen();
      return;
    }

    if (isRealTouch() && millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);
      if (auiBarHit(sx, sy, 0, 1)) {
        rfidCardsScanning = false;
        drawAdminRFIDCardsScreen();
      }
    }
    return;
  }

  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      int pages = rfidCardsPageCount();
      int slots = (pages > 1) ? 3 : 1;

      if (auiBarHit(sx, sy, 0, slots)) {
        currentScreen = SCREEN_ADMIN_SETTINGS;
        drawAdminSettingsScreen();
        return;
      }

      if (pages > 1 && auiBarHit(sx, sy, 1, slots)) {      // previous page
        rfidCardsPage = (rfidCardsPage + pages - 1) % pages;
        drawAdminRFIDCardsScreen();
        return;
      }
      if (pages > 1 && auiBarHit(sx, sy, 2, slots)) {      // next page
        rfidCardsPage = (rfidCardsPage + 1) % pages;
        drawAdminRFIDCardsScreen();
        return;
      }

      bool full = (rfidCardCount >= rfidCardCap);
      if (!full && pointInRect(sx, sy, rfidActionX(0), RFCARD_ACTION_Y, rfidActionW(), RFCARD_ACTION_H)) {
        rfidCardsScanning = true;
        rfidCardsScanStartMs = millis();
        drawRFIDCardsScanOverlay();
        return;
      }
      if (!full && pointInRect(sx, sy, rfidActionX(1), RFCARD_ACTION_Y, rfidActionW(), RFCARD_ACTION_H)) {
        openTextEntry(TE_RFID_CARD_MANUAL);
        return;
      }

      int first = rfidCardsPage * RFCARD_ROWS_PER_PAGE;
      int rowsOnPage = min(RFCARD_ROWS_PER_PAGE, rfidCardCount - first);
      for (int row = 0; row < rowsOnPage; row++) {
        int i = first + row;
        int y = rfidCardRowY(row);
        if (pointInRect(sx, sy, rfidDelX() - 3, rfidDelY(row) - 3, RFCARD_DEL_W + 6, RFCARD_DEL_H + 6)) {
          removeRFIDCard(i);
          drawAdminRFIDCardsScreen();
          return;
        }
        // Anywhere else on the row (checked after the delete button, so a
        // tap on "X" doesn't also open the card) opens Name/Limit/Usage.
        if (pointInRect(sx, sy, RFCARD_ROW_X, y, RFCARD_ROW_W, RFCARD_ROW_H)) {
          rfidEditingCard = i;
          currentScreen = SCREEN_ADMIN_RFID_CARD_EDIT;
          drawAdminRFIDCardEditScreen();
          return;
        }
      }
    }
  }
}
