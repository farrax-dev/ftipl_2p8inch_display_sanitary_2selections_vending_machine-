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
// (Screen_06_PaymentOther.ino's handlePaymentRFIDScreen()). There's no
// automatic reset yet — "Reset Usage" here is the only way to zero a card
// back out.
const int RFEDIT_LIMIT_Y = 112;
const int RFEDIT_USED_Y  = 150;

void drawAdminRFIDCardEditScreen() {
  int i = rfidEditingCard;
  drawGradientBackground();

  drawScreenTitle("Edit Card");

  // ---- Name ----
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, 50);
  tft.print("Name:");

  drawCard(70, 44, 150, 26, 6);
  tft.setTextColor(COL_TEXT, COL_CARD);
  centerTextInBox(rfidCards[i].name[0] ? rfidCards[i].name : "(no name)", 52, 70, 150);

  drawCardShadow(230, 44, 70, 26, 6);
  tft.fillRoundRect(230, 44, 70, 26, 6, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  tft.setCursor(248, 52);
  tft.print("Edit");

  // ---- UID (read-only — re-register the card to change it) ----
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, 84);
  tft.print("UID:");

  drawCard(70, 78, 230, 20, 5);
  tft.setTextColor(COL_TEXT, COL_CARD);
  centerTextInBox(rfidCards[i].uid, 84, 70, 230);

  // ---- Withdrawal limit (0 = unlimited) ----
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, 122);
  tft.print("Limit:");

  drawCardShadow(75, RFEDIT_LIMIT_Y, 28, 28, 6);
  tft.fillRoundRect(75, RFEDIT_LIMIT_Y, 28, 28, 6, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  tft.setTextSize(2);
  tft.setCursor(85, RFEDIT_LIMIT_Y + 7);
  tft.print("-");

  drawCardShadow(163, RFEDIT_LIMIT_Y, 28, 28, 6);
  tft.fillRoundRect(163, RFEDIT_LIMIT_Y, 28, 28, 6, COL_ACCENT);
  tft.setCursor(173, RFEDIT_LIMIT_Y + 7);
  tft.print("+");

  // Rounded fill only, no shadow — read-only figure between two buttons
  // that already carry their own shadows.
  tft.fillRoundRect(108, RFEDIT_LIMIT_Y, 50, 28, 6, COL_CARD);
  tft.setTextColor(COL_TEXT, COL_CARD);
  char limBuf[8];
  if (rfidCards[i].withdrawLimit > 0) snprintf(limBuf, sizeof(limBuf), "%d", rfidCards[i].withdrawLimit);
  else strcpy(limBuf, "Off");
  centerTextInBox(limBuf, RFEDIT_LIMIT_Y + 8, 108, 50);

  // ---- Usage so far, and a manual reset (no auto reset yet) ----
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  tft.setCursor(20, RFEDIT_USED_Y + 10);
  tft.print("Used:");

  tft.fillRoundRect(70, RFEDIT_USED_Y, 60, 28, 6, COL_CARD);
  tft.setTextColor(COL_TEXT, COL_CARD);
  char usedBuf[8];
  snprintf(usedBuf, sizeof(usedBuf), "%d", rfidCards[i].withdrawUsed);
  tft.setTextSize(2);
  centerTextInBox(usedBuf, RFEDIT_USED_Y + 8, 70, 60);

  drawCardShadow(140, RFEDIT_USED_Y, 160, 28, 6);
  tft.fillRoundRect(140, RFEDIT_USED_Y, 160, 28, 6, COL_DANGER);
  tft.setTextColor(COL_TEXT, COL_DANGER);
  centerTextInBox("Reset Usage", RFEDIT_USED_Y + 8, 140, 160);

  drawBackButton("< Back");
}

void handleAdminRFIDCardEditScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);
      int i = rfidEditingCard;

      if (pointInRect(sx, sy, 230, 44, 70, 26)) {
        openTextEntry(TE_RFID_CARD_NAME);
        return;
      }

      if (pointInRect(sx, sy, 75, RFEDIT_LIMIT_Y, 28, 28)) {
        if (rfidCards[i].withdrawLimit > 0) setRFIDCardLimit(i, rfidCards[i].withdrawLimit - 1);
        drawAdminRFIDCardEditScreen();
        return;
      }
      if (pointInRect(sx, sy, 163, RFEDIT_LIMIT_Y, 28, 28)) {
        if (rfidCards[i].withdrawLimit < 999) setRFIDCardLimit(i, rfidCards[i].withdrawLimit + 1);
        drawAdminRFIDCardEditScreen();
        return;
      }

      if (pointInRect(sx, sy, 140, RFEDIT_USED_Y, 160, 28)) {
        resetRFIDCardUsage(i);
        drawAdminRFIDCardEditScreen();
        return;
      }

      if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
        currentScreen = SCREEN_ADMIN_RFID_CARDS;
        drawAdminRFIDCardsScreen();
        return;
      }
    }
  }
}
