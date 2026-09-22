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
const int RFCARD_ROW_X = 20, RFCARD_ROW_W = 280, RFCARD_ROW_H = 20;
const int RFCARD_ROW_Y0 = 44, RFCARD_ROW_GAP = 4;
// 4, not 5 — leaves clearance below the last row for the add/error status
// line (y=150) before the action buttons start.
const int RFCARD_ROWS_PER_PAGE = 4;

const int RFCARD_SCAN_BTN_X = 20,  RFCARD_SCAN_BTN_W = 138;
const int RFCARD_MANUAL_BTN_X = 162, RFCARD_MANUAL_BTN_W = 138;
const int RFCARD_ACTION_Y = 166, RFCARD_ACTION_H = 20;

int rfidCardsPageCount() {
  int pages = (rfidCardCount + RFCARD_ROWS_PER_PAGE - 1) / RFCARD_ROWS_PER_PAGE;
  return (pages < 1) ? 1 : pages;
}

int rfidCardRowY(int row) {
  return RFCARD_ROW_Y0 + row * (RFCARD_ROW_H + RFCARD_ROW_GAP);
}

// ---------- "waiting for a tap" overlay ----------
bool rfidCardsScanning = false;
unsigned long rfidCardsScanStartMs = 0;
const unsigned long RFID_SCAN_TIMEOUT_MS = 30000;

// rfidCardsMsg/rfidCardsMsgColor/rfidCardsMsgUntil live in Core_09_Storage.ino
// — see the comment there for why.

void drawRFIDCardsScanOverlay() {
  drawGradientBackground();
  drawScreenTitle("Scan New Card");

  tft.setTextSize(2);
  tft.setTextColor(COL_ACCENT, COL_BG_BOTTOM);
  centerText("Hold card near", 90);
  centerText("the reader", 113);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  centerText("Cancels automatically after 30s", 145);

  drawBackButton("Cancel");
}

void drawAdminRFIDCardsScreen() {
  int pages = rfidCardsPageCount();
  if (rfidCardsPage >= pages) rfidCardsPage = pages - 1;
  if (rfidCardsPage < 0) rfidCardsPage = 0;

  drawGradientBackground();

  drawScreenTitle("RFID Cards", 60);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_TOP);
  char countBuf[12];
  snprintf(countBuf, sizeof(countBuf), "%d/%d", rfidCardCount, MAX_RFID_CARDS);
  rightText(countBuf, headerRightX(), headerTextY(1));

  int first = rfidCardsPage * RFCARD_ROWS_PER_PAGE;
  int rowsOnPage = min(RFCARD_ROWS_PER_PAGE, rfidCardCount - first);

  if (rfidCardCount == 0) {
    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    centerText("No cards registered yet", 100);
  } else {
    for (int row = 0; row < rowsOnPage; row++) {
      int i = first + row;
      int y = rfidCardRowY(row);
      drawCard(RFCARD_ROW_X, y, RFCARD_ROW_W, RFCARD_ROW_H, 5);

      int delSize = 18;
      int delX = RFCARD_ROW_X + RFCARD_ROW_W - delSize - 6;
      int delY = y + (RFCARD_ROW_H - delSize) / 2;

      // A limit badge sits right-aligned before the delete button, so a
      // glance at the list shows who's close to their cap without opening
      // each card. Blank for unlimited cards (the common case) rather than
      // printing a distracting "0/0".
      if (rfidCards[i].withdrawLimit > 0) {
        char limBuf[12];
        snprintf(limBuf, sizeof(limBuf), "%d/%d", rfidCards[i].withdrawUsed, rfidCards[i].withdrawLimit);
        tft.setTextSize(1);
        tft.setTextColor(COL_TEXT_DIM, COL_CARD);
        rightText(limBuf, delX - 6, y + 7);
      }

      tft.setTextSize(1);
      tft.setTextColor(COL_TEXT, COL_CARD);
      tft.setCursor(RFCARD_ROW_X + 8, y + 7);
      tft.print(rfidCards[i].name[0] ? rfidCards[i].name : rfidCards[i].uid);

      tft.fillRoundRect(delX, delY, delSize, delSize, 4, COL_DANGER);
      tft.setTextColor(COL_BG_TOP, COL_DANGER);
      tft.setTextSize(1);
      centerTextInBox("X", delY + 5, delX, delSize);
    }
  }

  if (rfidCardsMsg[0] != '\0') {
    tft.setTextSize(1);
    tft.setTextColor(rfidCardsMsgColor, COL_BG_BOTTOM);
    centerText(rfidCardsMsg, 150);
  }

  bool full = (rfidCardCount >= MAX_RFID_CARDS);
  uint16_t scanFill = full ? COL_CARD : COL_ACCENT;
  drawCardShadow(RFCARD_SCAN_BTN_X, RFCARD_ACTION_Y, RFCARD_SCAN_BTN_W, RFCARD_ACTION_H, 6);
  tft.fillRoundRect(RFCARD_SCAN_BTN_X, RFCARD_ACTION_Y, RFCARD_SCAN_BTN_W, RFCARD_ACTION_H, 6, scanFill);
  tft.setTextColor(full ? COL_TEXT_DIM : COL_BG_TOP, scanFill);
  tft.setTextSize(1);
  centerTextInBox(full ? "List full" : "Scan New Card", RFCARD_ACTION_Y + 6, RFCARD_SCAN_BTN_X, RFCARD_SCAN_BTN_W);

  drawCardShadow(RFCARD_MANUAL_BTN_X, RFCARD_ACTION_Y, RFCARD_MANUAL_BTN_W, RFCARD_ACTION_H, 6);
  tft.fillRoundRect(RFCARD_MANUAL_BTN_X, RFCARD_ACTION_Y, RFCARD_MANUAL_BTN_W, RFCARD_ACTION_H, 6, scanFill);
  tft.setTextColor(full ? COL_TEXT_DIM : COL_BG_TOP, scanFill);
  centerTextInBox(full ? "List full" : "Enter Manually", RFCARD_ACTION_Y + 6, RFCARD_MANUAL_BTN_X, RFCARD_MANUAL_BTN_W);

  drawBackButton("< Back");
  if (pages > 1) {
    char pageLabel[16];
    snprintf(pageLabel, sizeof(pageLabel), "Page %d/%d >", rfidCardsPage + 1, pages);
    drawProceedButton(pageLabel);
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
        snprintf(rfidCardsMsg, sizeof(rfidCardsMsg), "Card added");
        rfidCardsMsgColor = COL_SUCCESS;
      } else {
        snprintf(rfidCardsMsg, sizeof(rfidCardsMsg), "Already registered");
        rfidCardsMsgColor = COL_WARNING;
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
      if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
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

      if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_SETTINGS;
        drawAdminSettingsScreen();
        return;
      }

      int pages = rfidCardsPageCount();
      if (pages > 1 && pointInRect(sx, sy, BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H)) {
        rfidCardsPage = (rfidCardsPage + 1) % pages;
        drawAdminRFIDCardsScreen();
        return;
      }

      bool full = (rfidCardCount >= MAX_RFID_CARDS);
      if (!full && pointInRect(sx, sy, RFCARD_SCAN_BTN_X, RFCARD_ACTION_Y, RFCARD_SCAN_BTN_W, RFCARD_ACTION_H)) {
        rfidCardsScanning = true;
        rfidCardsScanStartMs = millis();
        drawRFIDCardsScanOverlay();
        return;
      }
      if (!full && pointInRect(sx, sy, RFCARD_MANUAL_BTN_X, RFCARD_ACTION_Y, RFCARD_MANUAL_BTN_W, RFCARD_ACTION_H)) {
        openTextEntry(TE_RFID_CARD_MANUAL);
        return;
      }

      int first = rfidCardsPage * RFCARD_ROWS_PER_PAGE;
      int rowsOnPage = min(RFCARD_ROWS_PER_PAGE, rfidCardCount - first);
      for (int row = 0; row < rowsOnPage; row++) {
        int i = first + row;
        int y = rfidCardRowY(row);
        int delSize = 18;
        int delX = RFCARD_ROW_X + RFCARD_ROW_W - delSize - 6;
        int delY = y + (RFCARD_ROW_H - delSize) / 2;
        if (pointInRect(sx, sy, delX, delY, delSize, delSize)) {
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
