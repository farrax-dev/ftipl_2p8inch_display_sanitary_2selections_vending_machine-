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
  drawScreenTitle(TXT_YOUR_CART, 110);

  // Matches the product cards' own showPrice gate (Screen_02_Select.ino) —
  // a cart total is still a price, so it stays hidden under the same flag
  // rather than being the one place on this screen that leaks it.
  if (!hideProductPrices) {
    char totalBuf[16];
    snprintf(totalBuf, sizeof(totalBuf), "Rs. %d", cartTotal());
    tft.setTextSize(2);
    tft.setTextColor(COL_PRICE_TEXT, COL_HDR_BG);
    rightText(totalBuf, headerRightX(), headerTextY(2));
  }

  int idx[MAX_PRODUCTS];
  int count = 0;
  for (int i = 0; i < MAX_PRODUCTS; i++) {
    if (cartQty[i] > 0) idx[count++] = i;
  }

  if (count == 0) {
    tft.setTextSize(2);
    tft.setTextColor(COL_MSG_BODY, COL_BG_BOTTOM);
    centerText(TXT_CART_IS_EMPTY, 110);
  } else {
    int rowH = cartRowH(count);
    for (int k = 0; k < count; k++) {
      drawCartRow(idx[k], CART_AREA_X, cartRowY(k, rowH, count), CART_AREA_W, rowH);
    }
  }

  drawBackButton(TXT_BACK_ARROW);
  drawProceedButton(freeVendMode ? TXT_GET_FREE : TXT_PAY);
}

// Serif face for item names, the same family as the Welcome screen's
// "SANITARY NAPKIN" title (FreeSerifBold). One face for every row: the
// largest (12pt, else 9pt) at which every name in the cart fits the text
// column on one line and the row is tall enough. nullptr means neither fits
// and the bitmap font is used instead.
// Visible height of a capital in a font (resets the font to the default afterwards).
// (Takes const void*: the Arduino auto-prototype for a function with a GFXfont parameter lands
// before the type is declared and fails to compile.)
int cartFontCapH(const void* font) {
  int16_t x1, y1; uint16_t w, h;
  tft.setTextSize(1);
  tft.setFont((const GFXfont*)font);
  tft.getTextBounds("H", 0, 0, &x1, &y1, &w, &h);
  tft.setFont();
  return (int)h;
}

// Regular-weight (not bold) serif for the name. The largest one that fits the text column AND,
// when prices are shown, leaves room for the price line under it in this row height.
const GFXfont* cartNameFont(int textW, int rowH) {
  const GFXfont* candidates[2] = { &UI_FONT_RM, &UI_FONT_XS };
  for (int c = 0; c < 2; c++) {
    if (candidates[c]->yAdvance > rowH - 4) continue;
    if (!hideProductPrices && cartFontCapH(candidates[c]) + 4 + uiBmpCapH(1) > rowH - 4) continue;
    tft.setTextSize(1);
    tft.setFont(candidates[c]);
    bool fits = true;
    for (int i = 0; i < MAX_PRODUCTS && fits; i++) {
      if (cartQty[i] <= 0) continue;
      int16_t x1, y1; uint16_t w, h;
      tft.getTextBounds(products[i].name, 0, 0, &x1, &y1, &w, &h);
      if ((int)w > textW) fits = false;
    }
    tft.setFont();
    if (fits) return candidates[c];
  }
  return nullptr;
}

void drawCartRow(int i, int x, int y, int w, int h) {
  drawCardShadow(x, y, w, h, 8);
  tft.fillRoundRect(x, y, w, h, 8, COL_CARD_FILL);
  tft.drawRoundRect(x, y, w, h, 8, COL_CARD_BORDER);

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
  const GFXfont* nameFont = cartNameFont(textW, h);
  int nameSize = (h - 4 >= uiBmpCapH(2) && (hideProductPrices || uiBmpCapH(2) + 4 + uiBmpCapH(1) <= h - 4)) ? 2 : 1;  // fallback only: the larger size needs its font's caps to fit the row

  // Everything is placed by CAP HEIGHT (the visible height of a capital letter), not by the font's
  // line height. yAdvance includes leading and descender room that is not ink, so stacking by it
  // let the price line of a 47px row (3 items) run off the bottom of the card, and centred a lone
  // name (4-5 items) a few pixels above the +/- buttons it sits beside.
  int ROW_LINE_GAP = 8;    // from the bottom of the name's capitals to the top of the price's
  int ROW_MARGIN   = 8;    // total spare room kept top and bottom, descenders included
  int nameCap = nameFont ? cartFontCapH(nameFont) : uiBmpCapH(nameSize);
  // With 4 items a row is only ~34px: the roomy spacing above left no space for the price line, so
  // it was dropped. Tighten the spacing before giving the price up.
  if (!hideProductPrices && nameCap + ROW_LINE_GAP + uiBmpCapH(1) > h - ROW_MARGIN) {
    ROW_LINE_GAP = 4;
    ROW_MARGIN   = 4;
  }
  bool twoLine = !hideProductPrices && (nameCap + ROW_LINE_GAP + uiBmpCapH(1) <= h - ROW_MARGIN);

  int moneySize = 0;
  char moneyBuf[32];
  if (twoLine) {
    int lineTotal = products[i].price * cartQty[i];
    // Multi-unit lines read "Rs. 30 (2x15)" - qty x unit price - so the
    // price setting is visible at a glance; a single unit is just "Rs. 15".
    if (cartQty[i] > 1) {
      snprintf(moneyBuf, sizeof(moneyBuf), "Rs. %d (%dx%d)", lineTotal, cartQty[i], products[i].price);
    } else {
      snprintf(moneyBuf, sizeof(moneyBuf), "Rs. %d", lineTotal);
    }
    // Largest size the row is tall enough for; fitTextSize() steps it down further if the text
    // still would not fit the column.
    int maxMoney = 1;
    if (nameCap + ROW_LINE_GAP + uiBmpCapH(3) <= h - ROW_MARGIN)      maxMoney = 3;
    else if (nameCap + ROW_LINE_GAP + uiBmpCapH(2) <= h - ROW_MARGIN) maxMoney = 2;
    moneySize = fitTextSize(moneyBuf, textW, maxMoney);
  }

  int moneyCap = twoLine ? uiBmpCapH(moneySize) : 0;
  int blockCap = nameCap + (twoLine ? ROW_LINE_GAP + moneyCap : 0);
  int blockTop = y + (h - blockCap) / 2;
  int nameCy   = blockTop + nameCap / 2;                                   // centre of the name's capitals
  int moneyCy  = blockTop + nameCap + ROW_LINE_GAP + moneyCap / 2;         // centre of the price's capitals

  tft.setTextColor(COL_PRODUCT_NAME, COL_CARD_FILL);
  if (nameFont) {
    tft.setTextSize(1);
    tft.setFont(nameFont);
    int16_t x1, y1; uint16_t bw, bh;
    tft.getTextBounds(products[i].name, 0, 0, &x1, &y1, &bw, &bh);
    tft.setCursor(textX - x1, nameCy + nameCap / 2);                       // baseline = capital centre + half the cap height
    tft.print(products[i].name);
    tft.setFont();
  } else {
    int maxChars = max(4, textW / (6 * nameSize));
    char nameBuf[24];
    snprintf(nameBuf, sizeof(nameBuf), "%.*s", maxChars, products[i].name);
    tft.setTextSize(nameSize);
    leftTextInBox(nameBuf, textX, textW, nameCy - 4 * nameSize);
  }

  if (twoLine) {
    tft.setTextSize(moneySize);
    tft.setTextColor(COL_PRICE_TEXT, COL_CARD_FILL);
    leftText(moneyBuf, textX, moneyCy - 4 * moneySize);
  }

  // 28, not 26: a size-3 glyph is 18x24, which in a 26px button leaves one
  // pixel of margin and reads as a button that is too small for its own mark.
  int glyphSize = (btnSize >= 28) ? 3 : (btnSize >= 22) ? 2 : 1;
  // Two digits at size 3 are 36px wide and the read-out box is 34, so a cart
  // that reaches double figures steps that one figure back down a size.
  int qtySize = (glyphSize >= 3 && cartQty[i] >= 10) ? 2 : glyphSize;

  tft.fillRoundRect(minusX, btnY, btnSize, btnSize, 5, COL_QTY_BTN_FILL);
  tft.setTextColor(COL_QTY_BTN_GLYPH, COL_QTY_BTN_FILL);
  tft.setTextSize(glyphSize);
  centerTextInBox("-", btnY + btnSize / 2 - 4 * glyphSize, minusX, btnSize);

  // Rounded to match the -/+ buttons either side; no shadow — nested on the
  // already-shadowed row card, and a read-only figure, not a control.
  tft.fillRoundRect(qtyBoxX, btnY, qtyBoxW, btnSize, 5, COL_QTY_BOX_FILL);
  tft.setTextColor(COL_QTY_TEXT, COL_QTY_BOX_FILL);
  tft.setTextSize(qtySize);
  char qbuf[4];
  snprintf(qbuf, sizeof(qbuf), "%d", cartQty[i]);
  centerTextInBox(qbuf, btnY + btnSize / 2 - 4 * qtySize, qtyBoxX, qtyBoxW);

  tft.fillRoundRect(plusX, btnY, btnSize, btnSize, 5, COL_QTY_BTN_FILL);
  tft.setTextColor(COL_QTY_BTN_GLYPH, COL_QTY_BTN_FILL);
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
    if (freeVendMode) {
      // Free-vend machines never show a payment method at all — skip
      // straight to dispensing (Core_11_Dispense.ino's runFreeVendCheckout()).
      runFreeVendCheckout();
      return;
    }
    startPaymentFlow();
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
