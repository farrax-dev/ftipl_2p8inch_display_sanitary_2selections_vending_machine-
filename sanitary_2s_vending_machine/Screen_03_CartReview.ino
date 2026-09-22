// =====================================================
//                  CART REVIEW SCREEN
// =====================================================
// Starts at 36 rather than 48: the header is one row now (the total moved up
// beside the title), and the rows take the line that freed.
const int CART_AREA_X = 15, CART_AREA_Y = 36, CART_AREA_W = 290, CART_AREA_H = 149;
const int CART_ROW_GAP = 4;

// Rows divide the area between them, but only up to a point. Letting one
// item stretch to the full 149px gives a card taller than a button bar with
// a name and a price adrift in the middle of it — the row grew, the content
// it exists to show did not. Past this the stack stays put and centres in
// the area instead.
const int CART_ROW_H_MAX = 64;

int cartRowH(int count) {
  return min((CART_AREA_H - (count - 1) * CART_ROW_GAP) / count, CART_ROW_H_MAX);
}

int cartRowY(int k, int rowH, int count) {
  int stackH = count * rowH + (count - 1) * CART_ROW_GAP;
  return CART_AREA_Y + (CART_AREA_H - stackH) / 2 + k * (rowH + CART_ROW_GAP);
}

void drawCartReviewScreen() {
  drawGradientBackground();

  // The total moves up beside the title rather than sitting on its own line
  // under it, the way the Select screen's cart counter does: one header row
  // on every screen, and the 12px it frees goes to the item rows below. The
  // item count went with it — the rows themselves are the count, and each
  // one already shows its own quantity.
  drawScreenTitle("Your Cart", 110);

  char totalBuf[16];
  snprintf(totalBuf, sizeof(totalBuf), "Rs %d", cartTotal());
  tft.setTextSize(2);
  tft.setTextColor(COL_ACCENT, COL_BG_TOP);
  rightText(totalBuf, headerRightX(), headerTextY(2));

  int idx[MAX_PRODUCTS];
  int count = 0;
  for (int i = 0; i < MAX_PRODUCTS; i++) {
    if (cartQty[i] > 0) idx[count++] = i;
  }

  if (count == 0) {
    tft.setTextSize(2);
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    centerText("Cart is empty", 110);
  } else {
    int rowH = cartRowH(count);
    for (int k = 0; k < count; k++) {
      drawCartRow(idx[k], CART_AREA_X, cartRowY(k, rowH, count), CART_AREA_W, rowH);
    }
  }

  drawBackButton("< Back");
  drawProceedButton("Pay >");
}

// One name size for every row rather than per row, and never a size that
// clips a name that a smaller one would have shown whole. Size 3 fits only
// 8 characters in the text column, so a one-item cart was rendering
// "Regular+Wings" as "Regular+" on a row 149px tall — the largest type on
// the screen doing the worst job of its one duty.
int cartNameSize(int textW, int rowH) {
  if (rowH < 20) return 1;
  if (rowH >= 60) {
    bool fits = true;
    for (int i = 0; i < MAX_PRODUCTS && fits; i++) {
      if (cartQty[i] > 0 && (int)strlen(products[i].name) * 18 > textW) fits = false;
    }
    if (fits) return 3;
  }
  // Floor of 2. A name too long for size 2 is clipped rather than shrunk:
  // dropping every row to size 1 to accommodate one long name costs more
  // than clipping the one name does.
  return 2;
}

void drawCartRow(int i, int x, int y, int w, int h) {
  drawCardShadow(x, y, w, h, 8);
  tft.fillRoundRect(x, y, w, h, 8, COL_CARD);
  tft.drawRoundRect(x, y, w, h, 8, COL_CARD_BRD);

  int btnSize = constrain(h - 8, 16, 30);
  int plusX = x + w - 10 - btnSize;
  int qtyBoxW = 34;
  int qtyBoxX = plusX - 6 - qtyBoxW;
  int minusX = qtyBoxX - 6 - btnSize;
  int btnY = y + (h - btnSize) / 2;

  int textX = x + 10;
  int textW = minusX - textX - 6;

  // Row height is whatever the item count leaves: 149px with one item down
  // to 26px with five. The old code always stacked a name over a price line
  // and only checked h >= 34 first, which went wrong at both ends — at five
  // items the two lines came to 28px in a 26px row and overflowed it, and at
  // four they fitted with 3px of margin, which is the stuffed look on the
  // bench unit.
  //
  // So the second line is earned, not assumed. Below 44px the row carries
  // the name alone, properly centred with 9px of air, and the money lives in
  // the running total up in the header — the stepper beside it is already
  // showing the quantity, and "Rs15 x1 = Rs15" was restating that in the
  // least legible type on the screen.
  bool twoLine = (h >= 44);

  int nameSize = cartNameSize(textW, h);
  int nameMaxChars = textW / (6 * nameSize);
  if (nameMaxChars < 4) nameMaxChars = 4;
  char nameBuf[24];
  snprintf(nameBuf, sizeof(nameBuf), "%.*s", nameMaxChars, products[i].name);

  const int ROW_LINE_GAP = 6;
  int moneySize = 0;
  char moneyBuf[32];
  if (twoLine) {
    int lineTotal = products[i].price * cartQty[i];
    // The unit price only earns its place once it stops being the same
    // number as the line total.
    if (cartQty[i] > 1) {
      snprintf(moneyBuf, sizeof(moneyBuf), "Rs %d  (Rs %d each)", lineTotal, products[i].price);
    } else {
      snprintf(moneyBuf, sizeof(moneyBuf), "Rs %d", lineTotal);
    }
    moneySize = fitTextSize(moneyBuf, textW, (h >= 60) ? 2 : 1);
  }

  int blockH = 8 * nameSize + (twoLine ? ROW_LINE_GAP + 8 * moneySize : 0);
  int blockY = y + (h - blockH) / 2;

  tft.setTextSize(nameSize);
  tft.setTextColor(COL_TEXT, COL_CARD);
  tft.setCursor(textX, blockY);
  tft.print(nameBuf);

  if (twoLine) {
    tft.setTextSize(moneySize);
    tft.setTextColor(COL_ACCENT, COL_CARD);
    tft.setCursor(textX, blockY + 8 * nameSize + ROW_LINE_GAP);
    tft.print(moneyBuf);
  }

  // 28, not 26: a size-3 glyph is 18x24, which in a 26px button leaves one
  // pixel of margin and reads as a button that is too small for its own mark.
  int glyphSize = (btnSize >= 28) ? 3 : (btnSize >= 22) ? 2 : 1;
  // Two digits at size 3 are 36px wide and the read-out box is 34, so a cart
  // that reaches double figures steps that one figure back down a size.
  int qtySize = (glyphSize >= 3 && cartQty[i] >= 10) ? 2 : glyphSize;

  tft.fillRoundRect(minusX, btnY, btnSize, btnSize, 5, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  tft.setTextSize(glyphSize);
  centerTextInBox("-", btnY + btnSize / 2 - 4 * glyphSize, minusX, btnSize);

  // Rounded to match the -/+ buttons either side; no shadow — nested on the
  // already-shadowed row card, and a read-only figure, not a control.
  tft.fillRoundRect(qtyBoxX, btnY, qtyBoxW, btnSize, 5, COL_CARD);
  tft.setTextColor(COL_TEXT, COL_CARD);
  tft.setTextSize(qtySize);
  char qbuf[4];
  snprintf(qbuf, sizeof(qbuf), "%d", cartQty[i]);
  centerTextInBox(qbuf, btnY + btnSize / 2 - 4 * qtySize, qtyBoxX, qtyBoxW);

  tft.fillRoundRect(plusX, btnY, btnSize, btnSize, 5, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  tft.setTextSize(glyphSize);
  centerTextInBox("+", btnY + btnSize / 2 - 4 * glyphSize, plusX, btnSize);
}

void handleCartReviewScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);
      onCartReviewTouched(sx, sy);
    }
  }
}

void onCartReviewTouched(int sx, int sy) {
  if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
    currentScreen = SCREEN_SELECT;
    drawSelectScreen();
    return;
  }

  if (pointInRect(sx, sy, BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H)) {
    if (cartItemCount() == 0) return;
    orderTotal = cartTotal();
    currentScreen = SCREEN_PAYMENT_METHOD;
    drawPaymentMethodScreen();
    return;
  }

  int idx[MAX_PRODUCTS];
  int count = 0;
  for (int i = 0; i < MAX_PRODUCTS; i++) {
    if (cartQty[i] > 0) idx[count++] = i;
  }
  if (count == 0) return;

  int rowH = cartRowH(count);
  for (int k = 0; k < count; k++) {
    int i = idx[k];
    int y = cartRowY(k, rowH, count);
    int btnSize = constrain(rowH - 8, 16, 30);
    int plusX = CART_AREA_X + CART_AREA_W - 10 - btnSize;
    int qtyBoxW = 34;
    int qtyBoxX = plusX - 6 - qtyBoxW;
    int minusX = qtyBoxX - 6 - btnSize;
    int btnY = y + (rowH - btnSize) / 2;

    if (pointInRect(sx, sy, minusX, btnY, btnSize, btnSize)) {
      cartQty[i]--;
      drawCartReviewScreen();
      return;
    }
    if (pointInRect(sx, sy, plusX, btnY, btnSize, btnSize)) {
      if (cartQty[i] < totalStockForProduct(i) && cartTotalQty() < maxCartQty) cartQty[i]++;
      drawCartReviewScreen();
      return;
    }
  }
}
