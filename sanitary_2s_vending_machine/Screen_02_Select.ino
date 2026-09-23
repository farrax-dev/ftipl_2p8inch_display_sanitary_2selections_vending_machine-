// =====================================================
//                  SELECT PAD SCREEN
// =====================================================
// A short-lived complaint shown in the header strip when a tap is refused.
// The cards fill y=45..185 and the buttons start at 190, so there is nowhere
// to put a banner — but the right half of the title strip is empty, and a
// message there sits right next to the counter it is explaining.
char selectMsg[26] = "";
unsigned long selectMsgUntil = 0;

// How much of the band's right-hand side the counter/message keeps for
// itself. 110px fits the longest of the three things that go there ("Tap to
// select", 78px at text size 1) and still leaves "Select Products" its full
// 12pt width — which is why the refusal strings below are phrased as tightly
// as they are, and why the stock ones don't repeat CFG_STOCK_UNIT (the card
// under the message is already showing it). Reserved for all three states,
// not just the one showing, so the title can't change size as they swap.
const int SELECT_HDR_RESERVED = 110;

// Title on the left, "items in cart / limit" on the right. Showing the limit
// permanently means a customer meets it as a number that was always there,
// rather than as a tap that mysteriously does nothing.
void drawSelectHeader() {
  // Clears and redraws the whole strip (title font included) every call —
  // rejectCardTap() and the message-timeout path in handleSelectScreen()
  // call this directly, not through a full drawSelectScreen() redraw, so
  // the right-aligned counter/message beside it has to be repainted in the
  // same pass or it'd go stale against the freshly-cleared band.
  drawScreenTitle("Select Products", SELECT_HDR_RESERVED);

  if (selectMsg[0] != 0) {
    tft.setTextSize(1);
    tft.setTextColor(COL_DANGER, COL_BG_TOP);
    rightText(selectMsg, headerRightX(), headerTextY(1));
    return;
  }

  if (isQuickVend()) {
    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT_DIM, COL_BG_TOP);
    rightText("Tap to select", headerRightX(), headerTextY(1));
    return;
  }

  int qty = cartTotalQty();
  char counter[12];
  snprintf(counter, sizeof(counter), "%d / %d", qty, maxCartQty);
  tft.setTextSize(2);
  tft.setTextColor(qty >= maxCartQty ? COL_WARNING : COL_TEXT_DIM, COL_BG_TOP);
  rightText(counter, headerRightX(), headerTextY(2));
}

void drawSelectScreen() {
  drawGradientBackground();
  // A full redraw is always a fresh start — an add, or re-entering the screen.
  // Refusal messages are drawn by rejectCardTap() alone, which repaints only
  // the header and the one card.
  selectMsg[0] = 0;
  drawSelectHeader();

  int idx[MAX_PRODUCTS];
  int count = getEnabledProducts(idx);

  if (count == 0) {
    tft.setTextSize(2);
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    centerText("No products available", 110);
  } else {
    for (int k = 0; k < count; k++) {
      int x, y, w, h;
      getSelectCardRect(k, count, x, y, w, h);
      drawProductCard(idx[k], x, y, w, h);
    }
  }

  drawBackButton("< Back");

  // The Proceed button's "just buy the only thing here" shortcut only means
  // something with exactly one product on sale — with 2+, there's no single
  // product for it to mean without a card tap having picked one already
  // (which sends straight to payment on its own; see onSelectTouched()).
  // So quick-vend with multiple products draws no Proceed button at all —
  // every card is already the complete action, nothing left to proceed to.
  if (isQuickVend() && count == 1) {
    drawProceedButton("Proceed >");
  } else if (!isQuickVend()) {
    int items = cartItemCount();
    char cartLabel[16];
    if (items > 0) snprintf(cartLabel, sizeof(cartLabel), "Cart(%d) >", items);
    else snprintf(cartLabel, sizeof(cartLabel), "Cart >");
    drawProceedButton(cartLabel);
  }
}

// ---------- Product card ----------
// Name, price, status — three lines centred in the card as one block.
//
// The name is what the customer is actually choosing between, so it leads,
// and the sizing is built around it. Three things were wrong with how it
// used to be set:
//
//   * Each card sized its own name, so one screen could mix size 2 and
//     size 1 names and read as a mistake rather than as a fit.
//   * In practice they all lost anyway. A 3-column card gives 79px of text
//     width, and size 2 needs 12px a character — so a 7-letter name like
//     "Regular" needed 84px, missed by 5px, and dropped to size 1. The
//     product name ended up exactly as big as the "Qty Avail" caption
//     beneath it, which is the whole card's hierarchy gone.
//   * It was hard-truncated to one line's worth of characters, so a real
//     name came out as "XL Overni" — and the truncation width shrank by the
//     qty badge when the item went in the cart, so the name visibly got
//     shorter just for being chosen.
//
// So: one size for every card on the screen (productNameSize()), word
// wrapped to at most two lines rather than cut off, and nothing stealing
// width from it. The quantity in the cart moved to the status line, where
// it reads as words instead of a corner chip — the accent border already
// says the card is chosen, the badge was saying it twice and charging the
// name 24px for the privilege.
const int PCARD_PAD = 6;
// 3, not more: a two-line name at size 2 is 34px, and a 68px card has room
// for exactly that plus the price line, the status line, two gaps of 3 and
// 2px of margin top and bottom. At a gap of 4 the two-line name no longer
// fits and every card on the screen drops to size 1 to compensate.
const int PCARD_GAP = 3;
const int PCARD_MAX_NAME_LINES = 2;

// Largest name size at which EVERY enabled product wraps to at most two
// lines and still leaves room for the price and status lines under it.
// Uniform by construction — the shortest name on the screen doesn't get to
// be the biggest word on it just because it had room to spare.
int productNameSize(int nameBoxW, int cardH, int priceSize) {
  int idx[MAX_PRODUCTS];
  int count = getEnabledProducts(idx);
  int maxSize = (nameBoxW >= 240) ? 3 : 2;
  // What is left for the name once the price line, the status line, the gaps
  // between them and 2px of breathing room top and bottom are taken out.
  int avail = cardH - 4 - (PCARD_GAP + 8 * priceSize) - (PCARD_GAP + 8);

  for (int sz = maxSize; sz > 1; sz--) {
    bool fits = true;
    for (int k = 0; k < count && fits; k++) {
      int lines = wrapTextInBox(products[idx[k]].name, 0, nameBoxW, 0, sz, false);
      if (lines > PCARD_MAX_NAME_LINES) fits = false;
      else if (wrapTextHeight(lines, sz) > avail) fits = false;
    }
    if (fits) return sz;
  }
  return 1;
}

// Same serif family as every screen header (drawHeaderTitle(),
// Core_08_UIHelpers.ino) and the Welcome card's big title
// (drawBigTitleLine(), Screen_01_Welcome.ino) — one typographic voice
// instead of the plain bitmap font this card used to fall back to for
// product names. Tries 12pt first (the same face the header strip uses),
// then 9pt (the header's own fallback face) if 12pt would run past two
// lines or blow the card's height budget, uniform across every enabled
// product for the same "shortest name doesn't win by default" reason
// productNameSize() above exists. Only the tightest layouts — a 5-product,
// 3-column build with ~43px card rows — fall all the way back to the old
// bitmap-font sizing, where even 9pt runs too tall; that path returns
// nullptr and *outBitmapSize instead of a font.
const GFXfont* productNameFont(int nameBoxW, int cardH, int priceSize, int &outLines, int &outBitmapSize) {
  int idx[MAX_PRODUCTS];
  int count = getEnabledProducts(idx);
  int avail = cardH - 4 - (PCARD_GAP + 8 * priceSize) - (PCARD_GAP + 8);

  const GFXfont* candidates[2] = { &FreeSerifBold12pt7b, &FreeSerifBold9pt7b };
  for (int c = 0; c < 2; c++) {
    bool fits = true;
    int maxLines = 0;
    for (int k = 0; k < count && fits; k++) {
      int lines = wrapFontInBox(candidates[c], products[idx[k]].name, 0, nameBoxW, 0, false);
      if (lines > PCARD_MAX_NAME_LINES) fits = false;
      else if (wrapFontHeight(lines, candidates[c]) > avail) fits = false;
      if (lines > maxLines) maxLines = lines;
    }
    if (fits) {
      outLines = maxLines;
      return candidates[c];
    }
  }

  outBitmapSize = productNameSize(nameBoxW, cardH, priceSize);
  return nullptr;
}

void drawProductCard(int i, int x, int y, int w, int h) {
  int totalStock = totalStockForProduct(i);
  bool outOfStock = (totalStock <= 0);
  bool inCart = (cartQty[i] > 0);

  // Two different reasons a card can be at its limit, and they need different
  // words: the whole cart is full, or this product has no more stock behind
  // what is already in the cart.
  bool allInCart = (!outOfStock && cartQty[i] >= totalStock);
  bool cartFull  = (cartTotalQty() >= maxCartQty);
  bool atLimit   = (!outOfStock && (allInCart || cartFull));

  uint16_t bg = outOfStock ? COL_BG_TOP : COL_CARD;
  uint16_t border = atLimit ? COL_WARNING : (inCart ? COL_ACCENT : COL_CARD_BRD);

  // rejectCardTap() redraws this same card in place (no full-screen repaint
  // first), always back to this same shadow+fill+border state, so — unlike a
  // toggling blink — there's never a stale shadow sliver left over.
  drawCardShadow(x, y, w, h, 10);
  tft.fillRoundRect(x, y, w, h, 10, bg);
  tft.drawRoundRect(x, y, w, h, 10, border);
  if (inCart || atLimit) tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 9, border);

  int innerX = x + PCARD_PAD;
  int innerW = w - 2 * PCARD_PAD;

  // Off for a registered-card-only machine (Admin > Settings) — nothing is
  // ever charged there, so a price figure is more confusing than useful.
  // Out of stock hides it too: there is nothing to buy at any price.
  char priceBuf[12];
  snprintf(priceBuf, sizeof(priceBuf), "Rs %d", products[i].price);
  bool showPrice = (!hideProductPrices && !outOfStock);
  int priceSize = fitTextSize(priceBuf, innerW, (h >= 60) ? 2 : 1);

  // One line saying whatever matters most about this card right now. It
  // replaces the old "Qty Avail" caption + figure pair, which spent two
  // lines and a word on something "19ml left" says in one.
  char statusBuf[28];
  uint16_t statusColor;
  if (outOfStock) {
    snprintf(statusBuf, sizeof(statusBuf), "OUT OF STOCK");
    statusColor = COL_DANGER;
  } else if (atLimit) {
    snprintf(statusBuf, sizeof(statusBuf), "%s", allInCart ? "ALL IN CART" : "CART FULL");
    statusColor = COL_WARNING;
  } else if (inCart) {
    snprintf(statusBuf, sizeof(statusBuf), "x%d - %d%s left", cartQty[i], totalStock, CFG_STOCK_UNIT);
    statusColor = COL_ACCENT;
  } else {
    snprintf(statusBuf, sizeof(statusBuf), "%d%s left", totalStock, CFG_STOCK_UNIT);
    statusColor = COL_SUCCESS;
  }

  int nameLines = 0, bitmapSize = 1;
  const GFXfont* nameFont = productNameFont(innerW, h, priceSize, nameLines, bitmapSize);
  int nameH = nameFont ? wrapFontHeight(nameLines, nameFont) : wrapTextHeight(nameLines, bitmapSize);

  // The price may never out-size the name. Only matters on the bitmap-font
  // fallback path — the name there can be forced down to size 1 while
  // "Rs 10" still fits at size 2, and an accent-colored price twice the
  // height of the product it belongs to inverts the whole card. A serif
  // name is already sized to the card's own height budget above, so it
  // never needs this clamp.
  if (!nameFont && priceSize > bitmapSize) priceSize = bitmapSize;

  int blockH = nameH + (showPrice ? PCARD_GAP + 8 * priceSize : 0) + PCARD_GAP + 8;
  int cursorY = y + (h - blockH) / 2;

  tft.setTextColor(outOfStock ? COL_TEXT_DIM : COL_TEXT, bg);
  if (nameFont) {
    wrapFontInBox(nameFont, products[i].name, innerX, innerW, cursorY, true);
  } else {
    tft.setTextSize(bitmapSize);
    wrapTextInBox(products[i].name, innerX, innerW, cursorY, bitmapSize, true);
  }
  cursorY += nameH;

  if (showPrice) {
    cursorY += PCARD_GAP;
    tft.setTextSize(priceSize);
    tft.setTextColor(COL_ACCENT, bg);
    centerTextInBox(priceBuf, cursorY, innerX, innerW);
    cursorY += 8 * priceSize;
  }

  cursorY += PCARD_GAP;
  tft.setTextSize(1);
  tft.setTextColor(statusColor, bg);
  centerTextInBox(statusBuf, cursorY, innerX, innerW);
}

// Refused tap: pulse the card red, and say why up in the header. Silence was
// the old behaviour and it reads as a broken touchscreen rather than a rule.
void rejectCardTap(int i, int x, int y, int w, int h, const char* msg) {
  strncpy(selectMsg, msg, sizeof(selectMsg) - 1);
  selectMsg[sizeof(selectMsg) - 1] = 0;
  selectMsgUntil = millis() + 1800;
  drawSelectHeader();

  tft.drawRoundRect(x, y, w, h, 10, COL_DANGER);
  tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 9, COL_DANGER);
  delay(160);
  drawProductCard(i, x, y, w, h);
}

void handleSelectScreen() {
  // Let the refusal message time out on its own, so the header goes back to
  // showing the running count without needing another tap.
  if (selectMsg[0] != 0 && millis() >= selectMsgUntil) {
    selectMsg[0] = 0;
    drawSelectHeader();
  }

  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);
      onSelectTouched(sx, sy);
    }
  }
}

// Quick vend has exactly one enabled product, so both tapping its card and
// tapping Proceed mean the same thing: buy that product now. Shared here so
// the two touch targets can't drift out of sync with each other.
void quickVendSelect(int i, int x, int y, int w, int h) {
  if (totalStockForProduct(i) <= 0) {
    rejectCardTap(i, x, y, w, h, "Out of stock");
    return;
  }
  cartQty[i] = 1;
  // orderTotal is normally set when Cart Review is confirmed, so it has to
  // be set here instead, since that screen is skipped entirely for a
  // single item.
  orderTotal = cartTotal();
  currentScreen = SCREEN_PAYMENT_METHOD;
  drawPaymentMethodScreen();
}

void onSelectTouched(int sx, int sy) {
  if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
    resetCart();
    currentScreen = SCREEN_WELCOME;
    drawWelcomeScreen();
    return;
  }

  int idx[MAX_PRODUCTS];
  int count = getEnabledProducts(idx);

  if (pointInRect(sx, sy, BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H)) {
    // Matches drawSelectScreen()'s guard on which button is actually drawn
    // here — quick-vend with 2+ products draws no Proceed button, so there's
    // nothing this tap should do in that case.
    if (isQuickVend() && count == 1) {
      int x, y, w, h;
      getSelectCardRect(0, count, x, y, w, h);
      quickVendSelect(idx[0], x, y, w, h);
    } else if (!isQuickVend() && cartItemCount() > 0) {
      currentScreen = SCREEN_CART_REVIEW;
      drawCartReviewScreen();
    }
    return;
  }

  for (int k = 0; k < count; k++) {
    int x, y, w, h;
    getSelectCardRect(k, count, x, y, w, h);
    if (pointInRect(sx, sy, x, y, w, h)) {
      int i = idx[k];
      int totalStock = totalStockForProduct(i);
      char why[26];

      if (totalStock <= 0) {
        rejectCardTap(i, x, y, w, h, "Out of stock");
      } else if (cartQty[i] >= totalStock) {
        snprintf(why, sizeof(why), "Only %d left", totalStock);
        rejectCardTap(i, x, y, w, h, why);
      } else if (cartTotalQty() >= maxCartQty) {
        snprintf(why, sizeof(why), "Max %d item%s", maxCartQty,
                 maxCartQty == 1 ? "" : "s");
        rejectCardTap(i, x, y, w, h, why);
      } else if (isQuickVend()) {
        quickVendSelect(i, x, y, w, h);
      } else {
        cartQty[i]++;
        drawSelectScreen();
      }
      return;
    }
  }
}
