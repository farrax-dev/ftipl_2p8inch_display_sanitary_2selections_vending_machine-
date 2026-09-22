// =====================================================
//               PAYMENT METHOD SCREEN
// =====================================================
// Plain sentences with no line breaks in them: wrapTextInBox()
// (Core_08_UIHelpers.ino) breaks these to whatever the card is wide, so
// the wording no longer has to be pre-broken for one particular card
// size. The embedded "\n" these carried before was worse than just
// unnecessary — the default font handles a newline by returning the
// cursor to x=0, the SCREEN's left edge rather than the card's, so the
// second line of the Cash and RFID descriptions was landing outside its
// own card and on top of the one to its left.
const char* PM_DESCRIPTIONS[PAYMENT_COUNT] = {
  "Scan the QR code",
  "Insert exact change",
  "Registered cards only"
};

void drawPaymentMethodScreen() {
  drawGradientBackground();

  drawScreenTitle("Select Payment");

  int idx[PAYMENT_COUNT];
  int count = getEnabledPaymentMethods(idx);

  if (count == 0) {
    tft.setTextSize(2);
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    centerText("No payment methods", 100);
    centerText("available", 130);
  } else {
    for (int k = 0; k < count; k++) {
      int x, y, w, h;
      getSelectCardRect(k, count, x, y, w, h);
      int p = idx[k];

      drawCardShadow(x, y, w, h, 10);
      tft.fillRoundRect(x, y, w, h, 10, COL_CARD);
      tft.drawRoundRect(x, y, w, h, 10, COL_CARD_BRD);

      // Name against the card's top edge and description against its bottom
      // is what this did before, and on a card 145px tall that left the whole
      // middle empty with the two lines stranded at either end. Both are
      // measured first now and drawn as one block centred in the card.
      int innerX = x + 6, innerW = w - 12;
      int nameSize = (w >= 250) ? 3 : 2;

      int nameLines = wrapTextInBox(PAYMENT_NAMES[p], innerX, innerW, 0, nameSize, false);
      int descLines = wrapTextInBox(PM_DESCRIPTIONS[p], innerX, innerW, 0, 1, false);
      int nameH = wrapTextHeight(nameLines, nameSize);
      int descH = wrapTextHeight(descLines, 1);

      const int PM_RULE_GAP = 9;
      int blockY = y + (h - (nameH + PM_RULE_GAP * 2 + 1 + descH)) / 2;

      tft.setTextColor(COL_TEXT, COL_CARD);
      tft.setTextSize(nameSize);
      wrapTextInBox(PAYMENT_NAMES[p], innerX, innerW, blockY, nameSize, true);

      // Short accent hairline between the two halves: the name says what the
      // method is, the line under it says how it works, and at this size a gap
      // on its own doesn't read as a division between the two.
      int ruleW = innerW / 2;
      tft.fillRect(innerX + (innerW - ruleW) / 2, blockY + nameH + PM_RULE_GAP, ruleW, 1, COL_ACCENT);

      tft.setTextColor(COL_TEXT_DIM, COL_CARD);
      tft.setTextSize(1);
      wrapTextInBox(PM_DESCRIPTIONS[p], innerX, innerW,
                    blockY + nameH + PM_RULE_GAP * 2 + 1, 1, true);
    }
  }

  drawBackButton("< Back");
}

void handlePaymentMethodScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
        if (isQuickVend()) {
          // Cart Review was never shown, so going "back" to it would strand
          // the customer on a screen they have not seen. Clear the single
          // item and return to the product, ready to start again.
          resetCart();
          currentScreen = SCREEN_SELECT;
          drawSelectScreen();
        } else {
          currentScreen = SCREEN_CART_REVIEW;
          drawCartReviewScreen();
        }
        return;
      }

      int idx[PAYMENT_COUNT];
      int count = getEnabledPaymentMethods(idx);

      for (int k = 0; k < count; k++) {
        int x, y, w, h;
        getSelectCardRect(k, count, x, y, w, h);
        if (pointInRect(sx, sy, x, y, w, h)) {
          switch (idx[k]) {
            case 0: currentScreen = SCREEN_PAYMENT_UPI; drawPaymentUPIScreen(); break;
            case 1: currentScreen = SCREEN_PAYMENT_CASH; drawPaymentCashScreen(); break;
            case 2: currentScreen = SCREEN_PAYMENT_RFID; drawPaymentRFIDScreen(); break;
          }
          return;
        }
      }
    }
  }
}
