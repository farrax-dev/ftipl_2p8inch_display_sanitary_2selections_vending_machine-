// ---------- Cart state ----------
int cartQty[MAX_PRODUCTS];
int orderTotal = 0;

void resetCart() {
  for (int i = 0; i < MAX_PRODUCTS; i++) cartQty[i] = 0;
}

int cartItemCount() {
  int c = 0;
  for (int i = 0; i < MAX_PRODUCTS; i++) if (cartQty[i] > 0) c++;
  return c;
}

int cartTotal() {
  int t = 0;
  for (int i = 0; i < MAX_PRODUCTS; i++) t += products[i].price * cartQty[i];
  return t;
}

// A cart limit of one leaves nothing to review: whichever single card gets
// tapped, cartQty for every product but that one stays 0 and the total is
// just that one item's price — the same outcome the customer would reach by
// tapping the card, then Cart, then Confirm, just without the two screens
// in between that showed them nothing new. So maxCartQty alone decides this
// (not "and exactly one product is enabled" — Screen_02_Select.ino's card
// tap handler already sends whichever product was tapped straight to
// payment, so this generalizes to any product count on its own; only the
// Proceed button's own "vend the only product" shortcut still needs a
// single-product check, since with 2+ products there's no one product for
// Proceed to mean without a card tap having chosen it).
bool isQuickVend() {
  return maxCartQty == 1;
}

int cartTotalQty() {
  int t = 0;
  for (int i = 0; i < MAX_PRODUCTS; i++) t += cartQty[i];
  return t;
}
