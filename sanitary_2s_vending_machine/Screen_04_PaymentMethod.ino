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

// Opens the screen for one payment method (index into PAYMENT_NAMES).
void openPaymentMethod(int p) {
  switch (p) {
    case 0: currentScreen = SCREEN_PAYMENT_UPI; drawPaymentUPIScreen(); break;
    case 1: currentScreen = SCREEN_PAYMENT_CASH; drawPaymentCashScreen(); break;
    case 2: currentScreen = SCREEN_PAYMENT_RFID; drawPaymentRFIDScreen(); break;
  }
}

// Entry point to payment. With exactly one enabled method there is nothing to
// choose, so the method screen is skipped and that method opens directly.
void startPaymentFlow() {
  int idx[PAYMENT_COUNT];
  if (getEnabledPaymentMethods(idx) == 1) {
    openPaymentMethod(idx[0]);
    return;
  }
  currentScreen = SCREEN_PAYMENT_METHOD;
  drawPaymentMethodScreen();
}

// Back out of a payment screen: to the method list when there is a choice to
// make, otherwise to wherever the customer came from.
void backFromPayment() {
  int idx[PAYMENT_COUNT];
  if (getEnabledPaymentMethods(idx) != 1) {
    currentScreen = SCREEN_PAYMENT_METHOD;
    drawPaymentMethodScreen();
  } else if (isQuickVend()) {
    resetCart();
    currentScreen = SCREEN_SELECT;
    drawSelectScreen();
  } else {
    currentScreen = SCREEN_CART_REVIEW;
    drawCartReviewScreen();
  }
}

void drawPaymentMethodScreen() {
  drawGradientBackground();

  drawScreenTitle("Select Payment");

  int idx[PAYMENT_COUNT];
  int count = getEnabledPaymentMethods(idx);

  if (count == 0) {
    drawSerifLine("No payment methods", 0, tft.width(), 100, COL_TEXT, 1);
    drawSerifLine("available", 0, tft.width(), 135, COL_TEXT, 1);
  } else {
    // Largest bold face at which every method name fits one line of a card.
    int cx, cy, cw, ch;
    getSelectCardRect(0, count, cx, cy, cw, ch);
    const GFXfont* nameFont = &FreeSerifBold9pt7b;
    const GFXfont* faces[2] = { &FreeSerifBold18pt7b, &FreeSerifBold12pt7b };
    for (int f = 0; f < 2; f++) {
      tft.setTextSize(1);
      tft.setFont(faces[f]);
      bool fits = true;
      for (int k = 0; k < count && fits; k++) {
        int16_t bx, by; uint16_t bw, bh;
        tft.getTextBounds(PAYMENT_NAMES[idx[k]], 0, 0, &bx, &by, &bw, &bh);
        if ((int)bw > cw - 12) fits = false;
      }
      tft.setFont();
      if (fits) { nameFont = faces[f]; break; }
    }

    for (int k = 0; k < count; k++) {
      int x, y, w, h;
      getSelectCardRect(k, count, x, y, w, h);
      int p = idx[k];

      drawCardShadow(x, y, w, h, 10);
      tft.fillRoundRect(x, y, w, h, 10, COL_CARD);
      tft.drawRoundRect(x, y, w, h, 10, COL_CARD_BRD);

      // Heading in the bold serif face (same family as the screen titles and
      // the Welcome card), measured with the description and drawn as one
      // block centred in the card: bold accent heading, a short rule, then
      // the description in the dark text colour.
      int innerX = x + 6, innerW = w - 12;

      tft.setTextSize(1);
      int16_t bx, by; uint16_t bw, bh;
      tft.setFont(nameFont);
      tft.getTextBounds(PAYMENT_NAMES[p], 0, 0, &bx, &by, &bw, &bh);
      tft.setFont();
      int nameH = nameFont->yAdvance;

      const GFXfont* descFont = &FreeSerifBold9pt7b;
      int descLines = wrapFontInBox(descFont, PM_DESCRIPTIONS[p], innerX, innerW, 0, false);
      int descH = wrapFontHeight(descLines, descFont);

      const int PM_RULE_GAP = 9;
      int blockY = y + (h - (nameH + PM_RULE_GAP * 2 + 1 + descH)) / 2;

      tft.setFont(nameFont);
      tft.setTextColor(COL_ACCENT, COL_CARD);
      tft.setCursor(innerX + (innerW - (int)bw) / 2 - bx, blockY - by);
      tft.print(PAYMENT_NAMES[p]);
      tft.setFont();

      int ruleW = innerW / 2;
      tft.fillRect(innerX + (innerW - ruleW) / 2, blockY + nameH + PM_RULE_GAP, ruleW, 2, COL_CARD_BRD);

      tft.setTextColor(COL_TEXT, COL_CARD);
      wrapFontInBox(descFont, PM_DESCRIPTIONS[p], innerX, innerW,
                    blockY + nameH + PM_RULE_GAP * 2 + 1, true);
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
          openPaymentMethod(idx[k]);
          return;
        }
      }
    }
  }
}
