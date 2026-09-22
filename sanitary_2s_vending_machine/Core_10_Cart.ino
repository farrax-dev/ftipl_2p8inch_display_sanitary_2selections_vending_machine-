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

// One product on sale and a cart limit of one leaves nothing to review: the
// basket can only ever be that single item at that single price. In that case
// the Select screen sends the customer straight to payment, turning a
// four-tap purchase into two.
bool isQuickVend() {
  int idx[MAX_PRODUCTS];
  return getEnabledProducts(idx) == 1 && maxCartQty == 1;
}

int cartTotalQty() {
  int t = 0;
  for (int i = 0; i < MAX_PRODUCTS; i++) t += cartQty[i];
  return t;
}
