// ---------- Theme Colors (3 built-in palettes + a Custom one, Config.h's CFG_COLOR_THEME) ----------
// The whole UI is built from this one set of macros — change which theme is
// selected in Config.h and every screen picks it up, since every screen
// draws with these macros instead of raw ILI9341_* constants.
//
// All the themes below share the same underlying design language this app
// was built around: a light, card-based look with a near-white top fading
// into a saturated bottom (the gradient background), pale cards that read
// as sitting a shade lighter than the wash behind them, a deep saturated
// border framing each card, a vivid accent reserved for the one actionable
// thing on a screen, dark primary text, and status colors (danger/success/
// warning) that stay warm and rich rather than reaching for stock primary
// red/green/yellow. Only the hue family changes between themes — the
// contrast relationships that make every screen legible do not, and every
// theme below is built to the same rules for exactly that reason:
//
//   - COL_BG_TOP must stay light and COL_TEXT must stay dark: every screen
//     pairs text=COL_TEXT against bg=COL_BG_TOP (or the reverse, text=
//     COL_BG_TOP against bg=COL_ACCENT for button labels) and depends on
//     that contrast holding regardless of which theme is active.
//   - COL_ACCENT, COL_CARD_BRD, COL_DANGER, COL_SUCCESS and COL_WARNING all
//     also get used directly as small text on COL_CARD/COL_BG_TOP (e.g.
//     Screen_02_Select's price, "Qty Avail" caption, atLimit warnings), so
//     each is pulled a couple of shades deeper/richer than a "fill-only"
//     pastel would need — still unmistakably that theme's hue, but legible
//     as text too.
//   - COL_CARD is kept pale rather than saturated for the same reason:
//     COL_ACCENT text drawn on top of it (the price) is the tightest-margin
//     pairing in every one of these palettes, so the card can't get much
//     more saturated without that text washing out.
//   - COL_SHADOW sits close to COL_TEXT_DIM in hue but lighter, since it's a
//     drop-shadow tone (drawCardShadow()/drawCard(), Core_08_UIHelpers.ino)
//     under a light card on a light background, not text — a neutral grey
//     shadow would read as a smudge against any of these warm/cool tinted
//     palettes, where a same-family tone reads as depth instead.
//   - COL_DANGER/COL_SUCCESS/COL_WARNING stay a consistent red/green/gold
//     family across every theme so their meaning never has to be relearned
//     — except on the handful of themes whose own ACCENT/CARD_BRD already
//     sits in one of those hue families (Ruby Red, Wine Burgundy, Crimson
//     for danger; Emerald Forest, Mint Fresh, Sage Green, Turquoise for
//     success; Golden Amber, Mocha Coffee for warning), where that one
//     status color is shifted just enough in shade to stay visually
//     distinct from the theme's own accent rather than blending into it.
//
// Pick a theme by number in Config.h's CFG_COLOR_THEME — 0 (Blossom) is the
// original theme this machine shipped with and is the default.
// select colors from the theme and use this site to get RGB565 value https://rgbcolorpicker.com/565

#if CFG_COLOR_THEME == 0
// Theme 0: Blossom (pink / rose) - the original default theme this machine shipped with
#define COL_BG_TOP     0xFF7E   // #FFEFF7 — near-white with a whisper of pink (gradient top)
#define COL_BG_BOTTOM  0xFE1B   // #FFC2DE — saturated bubblegum pink (gradient bottom)
#define COL_CARD       0xFF3D   // #FFE7EE — pale pink card fill, lighter than the wash behind it
#define COL_CARD_BRD   0xD1AD   // #D6356B — deep raspberry-rose card border/glow
#define COL_ACCENT     0xE0F1   // #E61C8C — vivid fuchsia accent (buttons, highlights)
#define COL_TEXT       0x4886   // #4A1031 — deep wine-plum, primary text
#define COL_TEXT_DIM   0x9B4F   // #9C697B — dusty rose-mauve, secondary text
#define COL_SHADOW     0xE5B9   // #E6B6CE — soft dusty pink, card/button shadow
#define COL_DANGER     0xE1CA   // #E63952 — rose-crimson, not pure red
#define COL_SUCCESS    0x350B   // #31A25A — fresh green, the one deliberately non-pink accent
#define COL_WARNING    0xB421   // #B58608 — rich gold, not pale yellow

#elif CFG_COLOR_THEME == 1
// Theme 1: Ocean Blue - crisp sky blue, clean and corporate
#define COL_BG_TOP     0xEFBF   // #EAF6FF
#define COL_BG_BOTTOM  0x8E9F   // #8FD0FA
#define COL_CARD       0xE79F   // #E1F0FC
#define COL_CARD_BRD   0x1373   // #146C9E
#define COL_ACCENT     0x13D9   // #1179C9
#define COL_TEXT       0x0968   // #0B2E45
#define COL_TEXT_DIM   0x5BF2   // #5D7C90
#define COL_SHADOW     0xC6FD   // #C7DEEC
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 2
// Theme 2: Mint Fresh - cool teal-green, airy and modern
#define COL_BG_TOP     0xEFDE   // #EAFBF5
#define COL_BG_BOTTOM  0x8718   // #82E3C0
#define COL_CARD       0xDFBD   // #DFF6EC
#define COL_CARD_BRD   0x0C6D   // #0C8C6A
#define COL_ACCENT     0x0D0F   // #0BA37C
#define COL_TEXT       0x09C5   // #0A3B2D
#define COL_TEXT_DIM   0x5C4F   // #5C8879
#define COL_SHADOW     0xC75B   // #C3E8D9
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x4D6A   // #4CAF50 — shifted warmer/yellower than the teal accent so "success" doesn't blend into it
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 3
// Theme 3: Custom - every colour comes from custom_theme.h in this sketch folder.
// docs/theme_designer.html writes that file ("Save config"); copy it over the one
// here, set CFG_COLOR_THEME to 3 in Config.h, then compile and flash.
#include "custom_theme.h"

#else
#error "CFG_COLOR_THEME must be 0-3 - see the list in Config.h"
#endif

// ===========================================================================
// Component colours
// ===========================================================================
// Every part of the UI (buttons, header, cards, switches, price text, ...) has
// its own COL_* macro below, so one component can be recoloured without
// touching the others. Each one defaults to a colour from the 11-colour palette
// a theme sets above, so the 20 built-in themes look exactly as before. To give
// a component its own colour in a theme, #define it inside that theme's block
// (above the #elif of the next theme) and the #ifndef here will leave it alone.
//
// Example, inside a theme block:
//   #define COL_BTN_PRI_FILL   0xE0F1   // primary buttons only
//   #define COL_PRICE_TEXT     0x350B   // price text only
//
// docs/theme_designer.html shows every one of these, grouped, and exports the
// ones you change.


// ---------- Header and Welcome strip ----------
#ifndef COL_HDR_BG
#define COL_HDR_BG             COL_BG_TOP       // Band behind every screen title
#endif
#ifndef COL_HDR_TITLE
#define COL_HDR_TITLE          COL_TEXT         // Screen title text
#endif
#ifndef COL_HDR_RULE
#define COL_HDR_RULE           COL_ACCENT       // Line under the title
#endif
#ifndef COL_HDR_RULE_FADE
#define COL_HDR_RULE_FADE      COL_HDR_BG       // Colour the title line fades into
#endif
#ifndef COL_HDR_NOTE
#define COL_HDR_NOTE           COL_ACCENT       // Hint in the header (Tap to select)
#endif
#ifndef COL_HDR_COUNT
#define COL_HDR_COUNT          COL_TEXT         // Counter in the header (1 / 5)
#endif
#ifndef COL_HDR_COUNT_DIM
#define COL_HDR_COUNT_DIM      COL_TEXT_DIM     // Dim counter in the header (RFID card count)
#endif
#ifndef COL_STRIP_BG
#define COL_STRIP_BG           COL_BG_TOP       // Welcome top strip
#endif
#ifndef COL_STRIP_LINE
#define COL_STRIP_LINE         COL_CARD_BRD     // Line under the Welcome strip
#endif
#ifndef COL_STRIP_TIME
#define COL_STRIP_TIME         COL_TEXT         // Clock text
#endif
#ifndef COL_STRIP_DATE
#define COL_STRIP_DATE         COL_ACCENT       // Date text
#endif
#ifndef COL_WELCOME_SUB
#define COL_WELCOME_SUB        COL_ACCENT       // Subtitle on the Welcome card
#endif
#ifndef COL_PILL_FILL
#define COL_PILL_FILL          COL_ACCENT       // Touch to Continue pill
#endif
#ifndef COL_PILL_TEXT
#define COL_PILL_TEXT          COL_BG_TOP       // Text on the pill
#endif
#ifndef COL_PILL_HOLLOW
#define COL_PILL_HOLLOW        COL_BG_TOP       // Pill fill while it blinks to outline
#endif

// ---------- Cards, rows and tiles ----------
#ifndef COL_CARD_FILL
#define COL_CARD_FILL          COL_CARD         // Card, row and tile fill
#endif
#ifndef COL_CARD_BORDER
#define COL_CARD_BORDER        COL_CARD_BRD     // Card, row and tile border
#endif
#ifndef COL_CARD_SHADOW
#define COL_CARD_SHADOW        COL_SHADOW       // Shadow under cards
#endif
#ifndef COL_CARD_RING
#define COL_CARD_RING          COL_ACCENT       // Inner highlight ring on key cards
#endif
#ifndef COL_CARD_RULE
#define COL_CARD_RULE          COL_CARD_BRD     // Divider line inside a card
#endif
#ifndef COL_CARD_TITLE
#define COL_CARD_TITLE         COL_TEXT         // Main text on cards and rows
#endif
#ifndef COL_CARD_CAPTION
#define COL_CARD_CAPTION       COL_TEXT_DIM     // Small caption text on cards and rows
#endif
#ifndef COL_VALUE_TEXT
#define COL_VALUE_TEXT         COL_ACCENT       // Editable values in rows (name, ID, clock)
#endif
#ifndef COL_HINT_TEXT
#define COL_HINT_TEXT          COL_TEXT         // Hint lines on the background
#endif
#ifndef COL_TILE_STRIPE
#define COL_TILE_STRIPE        COL_ACCENT       // Stripe on Admin tiles
#endif
#ifndef COL_TILE_STRIPE_OFF
#define COL_TILE_STRIPE_OFF    COL_TEXT_DIM     // Stripe on disabled tiles
#endif
#ifndef COL_TILE_STRIPE_OK
#define COL_TILE_STRIPE_OK     COL_SUCCESS      // Stripe on a connected network tile
#endif
#ifndef COL_TILE_LABEL
#define COL_TILE_LABEL         COL_TEXT_DIM     // Caption on Reports tiles
#endif
#ifndef COL_TILE_VALUE
#define COL_TILE_VALUE         COL_TEXT         // Number on Reports tiles
#endif
#ifndef COL_TILE_VALUE_HI
#define COL_TILE_VALUE_HI      COL_ACCENT       // Highlighted number on Reports tiles
#endif

// ---------- Products, price and cart ----------
#ifndef COL_PRODUCT_NAME
#define COL_PRODUCT_NAME       COL_TEXT         // Product name
#endif
#ifndef COL_PRICE_TEXT
#define COL_PRICE_TEXT         COL_ACCENT       // Price (Rs. 10) on cards, cart and totals
#endif
#ifndef COL_STOCK_TEXT
#define COL_STOCK_TEXT         COL_SUCCESS      // Qty Avail text
#endif
#ifndef COL_PRODUCT_FILL
#define COL_PRODUCT_FILL       COL_CARD         // Product card fill
#endif
#ifndef COL_PRODUCT_OOS_FILL
#define COL_PRODUCT_OOS_FILL   COL_BG_TOP       // Out-of-stock card fill
#endif
#ifndef COL_PRODUCT_BORDER
#define COL_PRODUCT_BORDER     COL_CARD_BRD     // Product card border
#endif
#ifndef COL_PRODUCT_SEL_BORDER
#define COL_PRODUCT_SEL_BORDER COL_ACCENT       // Border of a product in the cart
#endif
#ifndef COL_PRODUCT_LIMIT_BORDER
#define COL_PRODUCT_LIMIT_BORDER COL_WARNING      // Border when the cart is full
#endif
#ifndef COL_PRODUCT_ERR_BORDER
#define COL_PRODUCT_ERR_BORDER COL_DANGER       // Flash border on a refused tap
#endif
#ifndef COL_QTY_BTN_FILL
#define COL_QTY_BTN_FILL       COL_ACCENT       // Cart minus and plus buttons
#endif
#ifndef COL_QTY_BTN_GLYPH
#define COL_QTY_BTN_GLYPH      COL_BG_TOP       // Minus and plus symbols
#endif
#ifndef COL_QTY_BOX_FILL
#define COL_QTY_BOX_FILL       COL_CARD         // Cart quantity box
#endif
#ifndef COL_QTY_TEXT
#define COL_QTY_TEXT           COL_TEXT         // Cart quantity number
#endif
#ifndef COL_METHOD_NAME
#define COL_METHOD_NAME        COL_ACCENT       // UPI and Cash card names
#endif
#ifndef COL_METHOD_RULE
#define COL_METHOD_RULE        COL_CARD_BRD     // Line under the payment name
#endif
#ifndef COL_METHOD_DESC
#define COL_METHOD_DESC        COL_TEXT         // Payment card description
#endif

// ---------- Buttons ----------
#ifndef COL_BTN_PRI_FILL
#define COL_BTN_PRI_FILL       COL_ACCENT       // Primary button fill (Pay, Save, Done)
#endif
#ifndef COL_BTN_PRI_TEXT
#define COL_BTN_PRI_TEXT       COL_BG_TOP       // Primary button label
#endif
#ifndef COL_BTN_SEC_FILL
#define COL_BTN_SEC_FILL       COL_CARD         // Normal button fill (Back, Cancel, keypad)
#endif
#ifndef COL_BTN_SEC_BORDER
#define COL_BTN_SEC_BORDER     COL_CARD_BRD     // Normal button border
#endif
#ifndef COL_BTN_SEC_TEXT
#define COL_BTN_SEC_TEXT       COL_TEXT         // Normal button label
#endif
#ifndef COL_BTN_DANGER_FILL
#define COL_BTN_DANGER_FILL    COL_DANGER       // Danger button fill
#endif
#ifndef COL_BTN_DANGER_TEXT
#define COL_BTN_DANGER_TEXT    COL_BG_TOP       // Danger button label
#endif
#ifndef COL_BTN_OFF_FILL
#define COL_BTN_OFF_FILL       COL_CARD         // Disabled button fill
#endif
#ifndef COL_BTN_OFF_BORDER
#define COL_BTN_OFF_BORDER     COL_CARD_BRD     // Disabled button border
#endif
#ifndef COL_BTN_OFF_TEXT
#define COL_BTN_OFF_TEXT       COL_TEXT_DIM     // Disabled button label
#endif
#ifndef COL_BTN_SHADOW
#define COL_BTN_SHADOW         COL_SHADOW       // Shadow under buttons
#endif

// ---------- Switch and stepper ----------
#ifndef COL_SW_ON_FILL
#define COL_SW_ON_FILL         COL_ACCENT       // Switch track when on
#endif
#ifndef COL_SW_ON_BORDER
#define COL_SW_ON_BORDER       COL_ACCENT       // Switch outline and knob ring when on
#endif
#ifndef COL_SW_ON_TEXT
#define COL_SW_ON_TEXT         COL_BG_TOP       // ON label
#endif
#ifndef COL_SW_OFF_FILL
#define COL_SW_OFF_FILL        COL_SHADOW       // Switch track when off
#endif
#ifndef COL_SW_OFF_BORDER
#define COL_SW_OFF_BORDER      COL_CARD_BRD     // Switch outline and knob ring when off
#endif
#ifndef COL_SW_OFF_TEXT
#define COL_SW_OFF_TEXT        COL_TEXT_DIM     // OFF label
#endif
#ifndef COL_SW_KNOB
#define COL_SW_KNOB            COL_BG_TOP       // Switch knob
#endif
#ifndef COL_STEP_BTN_FILL
#define COL_STEP_BTN_FILL      COL_ACCENT       // Stepper minus and plus buttons
#endif
#ifndef COL_STEP_BTN_GLYPH
#define COL_STEP_BTN_GLYPH     COL_BG_TOP       // Minus and plus symbols
#endif
#ifndef COL_STEP_VALUE_FILL
#define COL_STEP_VALUE_FILL    COL_BG_TOP       // Stepper value box
#endif
#ifndef COL_STEP_VALUE_BORDER
#define COL_STEP_VALUE_BORDER  COL_CARD_BRD     // Stepper value box border
#endif
#ifndef COL_STEP_VALUE_TEXT
#define COL_STEP_VALUE_TEXT    COL_ACCENT       // Stepper number
#endif
#ifndef COL_STEP_VALUE_OFF
#define COL_STEP_VALUE_OFF     COL_TEXT_DIM     // Stepper number when disabled
#endif
#ifndef COL_STEP_VALUE_LOW
#define COL_STEP_VALUE_LOW     COL_DANGER       // Stock number when low
#endif

// ---------- Tabs, tables and pickers ----------
#ifndef COL_TAB_ON_FILL
#define COL_TAB_ON_FILL        COL_ACCENT       // Selected tab
#endif
#ifndef COL_TAB_ON_TEXT
#define COL_TAB_ON_TEXT        COL_BG_TOP       // Selected tab label
#endif
#ifndef COL_TAB_OFF_FILL
#define COL_TAB_OFF_FILL       COL_CARD         // Other tabs
#endif
#ifndef COL_TAB_OFF_BORDER
#define COL_TAB_OFF_BORDER     COL_CARD_BRD     // Other tabs border
#endif
#ifndef COL_TAB_OFF_TEXT
#define COL_TAB_OFF_TEXT       COL_TEXT         // Other tabs label
#endif
#ifndef COL_TBL_HEAD_FILL
#define COL_TBL_HEAD_FILL      COL_ACCENT       // Items table header
#endif
#ifndef COL_TBL_HEAD_TEXT
#define COL_TBL_HEAD_TEXT      COL_BG_TOP       // Items table header text
#endif
#ifndef COL_TBL_ROW_A
#define COL_TBL_ROW_A          COL_BG_TOP       // Items table odd rows
#endif
#ifndef COL_TBL_ROW_B
#define COL_TBL_ROW_B          COL_CARD         // Items table even rows
#endif
#ifndef COL_TBL_TEXT
#define COL_TBL_TEXT           COL_TEXT         // Items table text
#endif
#ifndef COL_TBL_FRAME
#define COL_TBL_FRAME          COL_CARD_BRD     // Items table frame
#endif
#ifndef COL_ROW_CHEVRON
#define COL_ROW_CHEVRON        COL_TEXT_DIM     // Arrow on tappable rows
#endif
#ifndef COL_PICK_VALUE_FILL
#define COL_PICK_VALUE_FILL    COL_CARD         // Hour, minute and date value boxes
#endif
#ifndef COL_PICK_VALUE_BORDER
#define COL_PICK_VALUE_BORDER  COL_CARD_BRD     // Value box border
#endif
#ifndef COL_PICK_VALUE_TEXT
#define COL_PICK_VALUE_TEXT    COL_ACCENT       // Value box number
#endif
#ifndef COL_PICK_LABEL
#define COL_PICK_LABEL         COL_TEXT         // Field label (Hour, Day)
#endif
#ifndef COL_PICK_LABEL_OFF
#define COL_PICK_LABEL_OFF     COL_TEXT_DIM     // Field label when off
#endif

// ---------- PIN, keyboard and text field ----------
#ifndef COL_PIN_FIELD_FILL
#define COL_PIN_FIELD_FILL     COL_CARD         // PIN box
#endif
#ifndef COL_PIN_FIELD_BORDER
#define COL_PIN_FIELD_BORDER   COL_CARD_BRD     // PIN box border
#endif
#ifndef COL_PIN_DOT
#define COL_PIN_DOT            COL_ACCENT       // Entered PIN dots
#endif
#ifndef COL_PIN_PLACEHOLDER
#define COL_PIN_PLACEHOLDER    COL_TEXT_DIM     // Enter PIN prompt
#endif
#ifndef COL_PIN_TEXT
#define COL_PIN_TEXT           COL_TEXT         // PIN box message
#endif
#ifndef COL_KEY_FILL
#define COL_KEY_FILL           COL_CARD         // Letter key fill
#endif
#ifndef COL_KEY_BORDER
#define COL_KEY_BORDER         COL_CARD_BRD     // Key border
#endif
#ifndef COL_KEY_TEXT
#define COL_KEY_TEXT           COL_TEXT         // Letter key text
#endif
#ifndef COL_KEY_MOD_FILL
#define COL_KEY_MOD_FILL       COL_BG_TOP       // DEL, SPACE and ?123 key fill
#endif
#ifndef COL_KEY_MOD_TEXT
#define COL_KEY_MOD_TEXT       COL_ACCENT       // DEL, SPACE and ?123 key text
#endif
#ifndef COL_FIELD_FILL
#define COL_FIELD_FILL         COL_CARD         // Text preview field
#endif
#ifndef COL_FIELD_BORDER
#define COL_FIELD_BORDER       COL_CARD_BRD     // Text preview field border
#endif
#ifndef COL_FIELD_TEXT
#define COL_FIELD_TEXT         COL_ACCENT       // Typed text
#endif
#ifndef COL_FIELD_EMPTY
#define COL_FIELD_EMPTY        COL_TEXT_DIM     // (empty) placeholder
#endif
#ifndef COL_FIELD_COUNT
#define COL_FIELD_COUNT        COL_TEXT_DIM     // 12/30 counter
#endif
#ifndef COL_FIELD_ERROR
#define COL_FIELD_ERROR        COL_DANGER       // Error shown in the field
#endif

// ---------- Indicators, messages and status ----------
#ifndef COL_DOT_ON
#define COL_DOT_ON             COL_SUCCESS      // Status dot on
#endif
#ifndef COL_DOT_OFF_FILL
#define COL_DOT_OFF_FILL       COL_BG_TOP       // Status dot off
#endif
#ifndef COL_DOT_OFF_RING
#define COL_DOT_OFF_RING       COL_TEXT_DIM     // Status dot off ring
#endif
#ifndef COL_WIFI_GOOD
#define COL_WIFI_GOOD          COL_SUCCESS      // Signal icon, strong
#endif
#ifndef COL_WIFI_FAIR
#define COL_WIFI_FAIR          COL_WARNING      // Signal icon, fair
#endif
#ifndef COL_WIFI_BAD
#define COL_WIFI_BAD           COL_DANGER       // Signal icon, weak or none
#endif
#ifndef COL_WIFI_OFF
#define COL_WIFI_OFF           COL_TEXT_DIM     // Signal icon, unlit bars
#endif
#ifndef COL_WIFI_BG
#define COL_WIFI_BG            COL_HDR_BG       // Behind the signal icon
#endif
#ifndef COL_MSG_TITLE
#define COL_MSG_TITLE          COL_ACCENT       // Big message headline
#endif
#ifndef COL_MSG_BODY
#define COL_MSG_BODY           COL_TEXT         // Message body text
#endif
#ifndef COL_OK_TEXT
#define COL_OK_TEXT            COL_SUCCESS      // Success text (paid, connected)
#endif
#ifndef COL_ERR_TEXT
#define COL_ERR_TEXT           COL_DANGER       // Error text (failed, timed out)
#endif
#ifndef COL_WARN_TEXT
#define COL_WARN_TEXT          COL_WARNING      // Warning text
#endif
#ifndef COL_COUNTDOWN_TEXT
#define COL_COUNTDOWN_TEXT     COL_TEXT         // Idle countdown
#endif
#ifndef COL_COUNTDOWN_URGENT
#define COL_COUNTDOWN_URGENT   COL_DANGER       // Idle countdown in the last 10 seconds
#endif


// ---------- Fonts ----------
// The text roles the UI draws with. Each is one Adafruit GFX font: Free<Family>[Bold]<size>pt7b,
// where family is Serif, Sans or Mono and size is 9, 12, 18 or 24. Keep XS <= SM <= MD <= LG;
// bigger fonts need bigger boxes, and text that no longer fits is shrunk or clipped with '..'.
// A theme (custom_theme.h) can #define any of these before this point to change them.
//   XS regular small   captions, long values       SM bold small   secondary labels, small buttons
//   MD bold medium     main labels, buttons, titles LG bold large   big numbers, Welcome title, messages
//   RM regular medium  the stock figure on product cards
#ifndef UI_FONT_XS
#define UI_FONT_XS FreeSerif9pt7b
#endif
#ifndef UI_FONT_SM
#define UI_FONT_SM FreeSerifBold9pt7b
#endif
#ifndef UI_FONT_MD
#define UI_FONT_MD FreeSerifBold12pt7b
#endif
#ifndef UI_FONT_LG
#define UI_FONT_LG FreeSerifBold18pt7b
#endif
#ifndef UI_FONT_RM
#define UI_FONT_RM FreeSerif12pt7b
#endif


// ---------- Text ----------
// Every fixed label the UI shows. A theme (custom_theme.h) can #define any of these before
// this point to reword it. Keep them short: text that is too long for its box is shrunk or
// clipped. Do not put % or double quotes inside them (several are used as plain format text).
// docs/theme_designer.html lists them by screen and previews the result.

// -- Welcome --
#ifndef TXT_SYNCING
#define TXT_SYNCING "Syncing..."
#endif
#ifndef TXT_NO_NETWORK
#define TXT_NO_NETWORK "No network"
#endif
#ifndef TXT_OUT_OF_STOCK
#define TXT_OUT_OF_STOCK "Out of Stock"
#endif
#ifndef TXT_TOUCH_TO_CONTINUE
#define TXT_TOUCH_TO_CONTINUE "Touch to Continue"
#endif

// -- Select products --
#ifndef TXT_SELECT_PRODUCTS
#define TXT_SELECT_PRODUCTS "Select Products"
#endif
#ifndef TXT_TAP_TO_SELECT
#define TXT_TAP_TO_SELECT "Tap to select"
#endif
#ifndef TXT_NO_PRODUCTS_AVAILABLE
#define TXT_NO_PRODUCTS_AVAILABLE "No products available"
#endif
#ifndef TXT_PROCEED
#define TXT_PROCEED "Proceed >"
#endif
#ifndef TXT_CART
#define TXT_CART "Cart >"
#endif
#ifndef TXT_OUT_OF_STOCK_CAPS
#define TXT_OUT_OF_STOCK_CAPS "OUT OF STOCK"
#endif
#ifndef TXT_ALL_IN_CART
#define TXT_ALL_IN_CART "ALL IN CART"
#endif
#ifndef TXT_CART_FULL
#define TXT_CART_FULL "CART FULL"
#endif

// -- Shared labels --
#ifndef TXT_BACK_ARROW
#define TXT_BACK_ARROW "< Back"
#endif
#ifndef TXT_GET_FREE
#define TXT_GET_FREE "Get Free >"
#endif
#ifndef TXT_DONE
#define TXT_DONE "Done"
#endif
#ifndef TXT_PAYMENT
#define TXT_PAYMENT "Payment"
#endif
#ifndef TXT_TIMED_OUT
#define TXT_TIMED_OUT "Timed Out"
#endif
#ifndef TXT_NO_PAYMENT_WAS_RECEIVED_IN_TIM
#define TXT_NO_PAYMENT_WAS_RECEIVED_IN_TIM "No payment was received in time."
#endif
#ifndef TXT_DISPENSE
#define TXT_DISPENSE "Dispense"
#endif
#ifndef TXT_FAILED
#define TXT_FAILED "Failed"
#endif
#ifndef TXT_PLEASE_CONTACT_SUPPORT
#define TXT_PLEASE_CONTACT_SUPPORT "Please contact support"
#endif
#ifndef TXT_CANCEL
#define TXT_CANCEL "Cancel"
#endif
#ifndef TXT_PRODUCTS
#define TXT_PRODUCTS "Products"
#endif
#ifndef TXT_PAYMENTS
#define TXT_PAYMENTS "Payments"
#endif
#ifndef TXT_NETWORK
#define TXT_NETWORK "Network"
#endif
#ifndef TXT_REPORTS
#define TXT_REPORTS "Reports"
#endif
#ifndef TXT_MACHINE
#define TXT_MACHINE "Machine"
#endif
#ifndef TXT_NAME
#define TXT_NAME "Name"
#endif
#ifndef TXT_EDIT
#define TXT_EDIT "Edit"
#endif
#ifndef TXT_BACK
#define TXT_BACK "Back"
#endif
#ifndef TXT_SAVE
#define TXT_SAVE "Save"
#endif
#ifndef TXT_SETUP
#define TXT_SETUP "Setup"
#endif
#ifndef TXT_RESET
#define TXT_RESET "Reset"
#endif
#ifndef TXT_NOT_SET
#define TXT_NOT_SET "(not set)"
#endif
#ifndef TXT_SIGNAL
#define TXT_SIGNAL "Signal: -"
#endif
#ifndef TXT_PASSWORD
#define TXT_PASSWORD "Password"
#endif
#ifndef TXT_MACHINE_ID
#define TXT_MACHINE_ID "Machine ID"
#endif
#ifndef TXT_STORE_ID
#define TXT_STORE_ID "Store ID"
#endif
#ifndef TXT_ADMIN_PIN
#define TXT_ADMIN_PIN "Admin PIN"
#endif
#ifndef TXT_CARD_ADDED
#define TXT_CARD_ADDED "Card added"
#endif
#ifndef TXT_ALREADY_REGISTERED
#define TXT_ALREADY_REGISTERED "Already registered"
#endif

// -- Your cart --
#ifndef TXT_YOUR_CART
#define TXT_YOUR_CART "Your Cart"
#endif
#ifndef TXT_CART_IS_EMPTY
#define TXT_CART_IS_EMPTY "Cart is empty"
#endif
#ifndef TXT_PAY
#define TXT_PAY "Pay >"
#endif

// -- Select payment --
#ifndef TXT_SCAN_THE_QR_CODE
#define TXT_SCAN_THE_QR_CODE "Scan the QR code"
#endif
#ifndef TXT_INSERT_EXACT_CHANGE
#define TXT_INSERT_EXACT_CHANGE "Insert exact change"
#endif
#ifndef TXT_SELECT_PAYMENT
#define TXT_SELECT_PAYMENT "Select Payment"
#endif
#ifndef TXT_NO_PAYMENT_METHODS
#define TXT_NO_PAYMENT_METHODS "No payment methods"
#endif
#ifndef TXT_AVAILABLE
#define TXT_AVAILABLE "available"
#endif

// -- UPI payment --
#ifndef TXT_SCAN_TO_PAY
#define TXT_SCAN_TO_PAY "Scan to Pay"
#endif
#ifndef TXT_AMOUNT_TO_PAY
#define TXT_AMOUNT_TO_PAY "Amount to pay"
#endif
#ifndef TXT_SCAN_WITH_ANY
#define TXT_SCAN_WITH_ANY "Scan with any"
#endif
#ifndef TXT_UPI_APP
#define TXT_UPI_APP "UPI app"
#endif
#ifndef TXT_QR_GENERATION_FAILED
#define TXT_QR_GENERATION_FAILED "QR generation failed"
#endif
#ifndef TXT_CANCEL_ARROW
#define TXT_CANCEL_ARROW "< Cancel"
#endif
#ifndef TXT_SUCCESSFUL
#define TXT_SUCCESSFUL "Successful!"
#endif
#ifndef TXT_PREPARING_YOUR_ITEM
#define TXT_PREPARING_YOUR_ITEM "Preparing your item..."
#endif
#ifndef TXT_DISPENSE_FAILED_CONTACT_SUPPOR
#define TXT_DISPENSE_FAILED_CONTACT_SUPPOR "Dispense failed - contact support"
#endif
#ifndef TXT_RETRY
#define TXT_RETRY "Retry"
#endif

// -- Cash and RFID payment --
#ifndef TXT_REMAINING
#define TXT_REMAINING "Remaining"
#endif
#ifndef TXT_PAYING_NO_REFUND
#define TXT_PAYING_NO_REFUND "Paying, no refund"
#endif
#ifndef TXT_INSERT_COINS
#define TXT_INSERT_COINS "Insert Coins"
#endif
#ifndef TXT_COIN_NOT_RECOGNIZED
#define TXT_COIN_NOT_RECOGNIZED "Coin not recognized"
#endif
#ifndef TXT_TAP_RFID_CARD
#define TXT_TAP_RFID_CARD "Tap RFID Card"
#endif
#ifndef TXT_NO_CHARGE_FOR
#define TXT_NO_CHARGE_FOR "No charge for"
#endif
#ifndef TXT_REGISTERED_CARDS
#define TXT_REGISTERED_CARDS "registered cards"
#endif
#ifndef TXT_HOLD_YOUR_CARD_NEAR_THE_READER
#define TXT_HOLD_YOUR_CARD_NEAR_THE_READER "Hold your card near the reader"
#endif
#ifndef TXT_DISPENSE_FAILED
#define TXT_DISPENSE_FAILED "Dispense Failed"
#endif
#ifndef TXT_CARD_NOT_REGISTERED
#define TXT_CARD_NOT_REGISTERED "Card not registered"
#endif
#ifndef TXT_NO_CARD_WAS_TAPPED_IN_TIME
#define TXT_NO_CARD_WAS_TAPPED_IN_TIME "No card was tapped in time."
#endif

// -- Admin login --
#ifndef TXT_ENTER_PIN
#define TXT_ENTER_PIN "Enter PIN"
#endif
#ifndef TXT_ADMIN_LOGIN
#define TXT_ADMIN_LOGIN "Admin Login"
#endif

// -- Admin panel --
#ifndef TXT_FREE_VEND
#define TXT_FREE_VEND "Free vend"
#endif
#ifndef TXT_NONE_ENABLED
#define TXT_NONE_ENABLED "None enabled"
#endif
#ifndef TXT_WIFI_CONNECTED
#define TXT_WIFI_CONNECTED "WiFi connected"
#endif
#ifndef TXT_4G_CONNECTED
#define TXT_4G_CONNECTED "4G connected"
#endif
#ifndef TXT_OFFLINE
#define TXT_OFFLINE "Offline"
#endif
#ifndef TXT_ID_PIN_CLOCK
#define TXT_ID_PIN_CLOCK "ID, PIN, clock"
#endif
#ifndef TXT_ADMIN_PANEL
#define TXT_ADMIN_PANEL "Admin Panel"
#endif
#ifndef TXT_STOCK
#define TXT_STOCK "Stock"
#endif
#ifndef TXT_EXIT_ADMIN
#define TXT_EXIT_ADMIN "Exit Admin"
#endif

// -- Edit product --
#ifndef TXT_PRICE_RS
#define TXT_PRICE_RS "Price (Rs.)"
#endif
#ifndef TXT_MOTORS
#define TXT_MOTORS "Motors"
#endif
#ifndef TXT_ASSIGN
#define TXT_ASSIGN "Assign"
#endif
#ifndef TXT_VISIBLE
#define TXT_VISIBLE "Visible"
#endif

// -- Motors and stock --
#ifndef TXT_SELECTED
#define TXT_SELECTED "Selected"
#endif
#ifndef TXT_FREE
#define TXT_FREE "Free"
#endif
#ifndef TXT_TAP_A_MOTOR_TO_ASSIGN_OR_RELEA
#define TXT_TAP_A_MOTOR_TO_ASSIGN_OR_RELEA "Tap a motor to assign or release it"
#endif
#ifndef TXT_FILL_MAX
#define TXT_FILL_MAX "Fill Max"
#endif
#ifndef TXT_MOTOR_STOCK
#define TXT_MOTOR_STOCK "Motor Stock"
#endif
#ifndef TXT_UNASSIGNED
#define TXT_UNASSIGNED "Unassigned"
#endif
#ifndef TXT_TAP_EDIT_TO_TYPE_AN_EXACT_QUAN
#define TXT_TAP_EDIT_TO_TYPE_AN_EXACT_QUAN "Tap Edit to type an exact quantity"
#endif
#ifndef TXT_REFILL_TRAYS
#define TXT_REFILL_TRAYS "Refill Trays"
#endif

// -- Admin payments --
#ifndef TXT_FREE_VEND_LABEL
#define TXT_FREE_VEND_LABEL "Free Vend"
#endif
#ifndef TXT_SHOW_PRICES
#define TXT_SHOW_PRICES "Show Prices"
#endif
#ifndef TXT_CARDS
#define TXT_CARDS "Cards"
#endif
#ifndef TXT_FREE_VEND_CUSTOMERS_ARE_NEVER
#define TXT_FREE_VEND_CUSTOMERS_ARE_NEVER "Free vend: customers are never charged"
#endif

// -- UPI setup --
#ifndef TXT_UPI_CONFIGURATION
#define TXT_UPI_CONFIGURATION "UPI Configuration"
#endif
#ifndef TXT_TIMEOUT_MIN
#define TXT_TIMEOUT_MIN "Timeout (min)"
#endif

// -- WiFi setup --
#ifndef TXT_IP
#define TXT_IP "IP: -"
#endif
#ifndef TXT_PING_WIFI_IS_TURNED_OFF
#define TXT_PING_WIFI_IS_TURNED_OFF "Ping: WiFi is turned off"
#endif
#ifndef TXT_SET_A_NETWORK_NAME_TO_GET_STAR
#define TXT_SET_A_NETWORK_NAME_TO_GET_STAR "Set a network name to get started"
#endif
#ifndef TXT_PING_NOT_TESTED_YET
#define TXT_PING_NOT_TESTED_YET "Ping: not tested yet"
#endif
#ifndef TXT_PING_FAILED_NO_INTERNET_REACHA
#define TXT_PING_FAILED_NO_INTERNET_REACHA "Ping failed - no internet reachable"
#endif
#ifndef TXT_WIFI_SETUP
#define TXT_WIFI_SETUP "WiFi Setup"
#endif
#ifndef TXT_NONE
#define TXT_NONE "(none)"
#endif
#ifndef TXT_CONNECT
#define TXT_CONNECT "Connect"
#endif
#ifndef TXT_PING
#define TXT_PING "Ping"
#endif

// -- Text entry --
#ifndef TXT_ENTER_NAME
#define TXT_ENTER_NAME "Enter Name"
#endif
#ifndef TXT_WIFI_NAME
#define TXT_WIFI_NAME "WiFi Name"
#endif
#ifndef TXT_WIFI_PASSWORD
#define TXT_WIFI_PASSWORD "WiFi Password"
#endif
#ifndef TXT_UPI_MERCHANT_ID
#define TXT_UPI_MERCHANT_ID "UPI Merchant ID"
#endif
#ifndef TXT_UPI_STORE_ID
#define TXT_UPI_STORE_ID "UPI Store ID"
#endif
#ifndef TXT_MACHINE_NAME
#define TXT_MACHINE_NAME "Machine Name"
#endif
#ifndef TXT_GMAIL_ADDRESS
#define TXT_GMAIL_ADDRESS "Gmail Address"
#endif
#ifndef TXT_GMAIL_APP_PASSWORD
#define TXT_GMAIL_APP_PASSWORD "Gmail App Password"
#endif
#ifndef TXT_SEND_REPORT_TO
#define TXT_SEND_REPORT_TO "Send Report To"
#endif
#ifndef TXT_EMAIL_1_AT_HH_MM
#define TXT_EMAIL_1_AT_HH_MM "Email 1 at HH:MM"
#endif
#ifndef TXT_EMAIL_2_AT_HH_MM
#define TXT_EMAIL_2_AT_HH_MM "Email 2 at HH:MM"
#endif
#ifndef TXT_CARD_NUMBER_HEX
#define TXT_CARD_NUMBER_HEX "Card Number (hex)"
#endif
#ifndef TXT_CARD_HOLDER_NAME
#define TXT_CARD_HOLDER_NAME "Card Holder Name"
#endif
#ifndef TXT_CONFIRM_NEW_PIN
#define TXT_CONFIRM_NEW_PIN "Confirm New PIN"
#endif
#ifndef TXT_USE_HH_MM_24H
#define TXT_USE_HH_MM_24H "Use HH:MM (24h)"
#endif
#ifndef TXT_NEED_4_8_DIGITS
#define TXT_NEED_4_8_DIGITS "Need 4-8 digits"
#endif
#ifndef TXT_PINS_DIDN_T_MATCH
#define TXT_PINS_DIDN_T_MATCH "PINs didn't match"
#endif

// -- 4G / LTE setup --
#ifndef TXT_THIS_CAN_TAKE_20_SECONDS
#define TXT_THIS_CAN_TAKE_20_SECONDS "This can take 20+ seconds"
#endif
#ifndef TXT_TURNED_OFF
#define TXT_TURNED_OFF "Turned off"
#endif
#ifndef TXT_CONNECTED
#define TXT_CONNECTED "Connected"
#endif
#ifndef TXT_NOT_CONNECTED
#define TXT_NOT_CONNECTED "Not connected"
#endif
#ifndef TXT_USED_AS_THE_BACKUP_LINK_WHEN_W
#define TXT_USED_AS_THE_BACKUP_LINK_WHEN_W "Used as the backup link when WiFi is down"
#endif
#ifndef TXT_TEST_4G_IS_TURNED_OFF
#define TXT_TEST_4G_IS_TURNED_OFF "Test: 4G is turned off"
#endif
#ifndef TXT_TEST_NOT_TESTED_YET
#define TXT_TEST_NOT_TESTED_YET "Test: not tested yet"
#endif
#ifndef TXT_TEST_FAILED_COULD_NOT_REACH_TH
#define TXT_TEST_FAILED_COULD_NOT_REACH_TH "Test failed - could not reach the internet"
#endif
#ifndef TXT_4G_LTE_SETUP
#define TXT_4G_LTE_SETUP "4G / LTE Setup"
#endif
#ifndef TXT_TEST_4G
#define TXT_TEST_4G "Test 4G"
#endif

// -- Reports --
#ifndef TXT_TRANSACTIONS
#define TXT_TRANSACTIONS "Transactions"
#endif
#ifndef TXT_PRODUCTS_SOLD
#define TXT_PRODUCTS_SOLD "Products sold"
#endif
#ifndef TXT_TOTAL_SALES_RS
#define TXT_TOTAL_SALES_RS "Total sales (Rs.)"
#endif
#ifndef TXT_UNITS_IN_STOCK
#define TXT_UNITS_IN_STOCK "Units in stock"
#endif
#ifndef TXT_TODAY_RS
#define TXT_TODAY_RS "Today (Rs.)"
#endif
#ifndef TXT_TODAY_UNITS_TXNS
#define TXT_TODAY_UNITS_TXNS "Today units/txns"
#endif
#ifndef TXT_OFF
#define TXT_OFF "Off"
#endif
#ifndef TXT_SEND_THIS_REPORT
#define TXT_SEND_THIS_REPORT "Send this report"
#endif
#ifndef TXT_REPORT_TIME_1
#define TXT_REPORT_TIME_1 "Report time 1"
#endif
#ifndef TXT_REPORT_TIME_2
#define TXT_REPORT_TIME_2 "Report time 2"
#endif
#ifndef TXT_LOW_STOCK_ALERT
#define TXT_LOW_STOCK_ALERT "Low-stock alert"
#endif
#ifndef TXT_ALERT_BELOW
#define TXT_ALERT_BELOW "Alert below"
#endif
#ifndef TXT_GMAIL
#define TXT_GMAIL "Gmail"
#endif
#ifndef TXT_SEND_TO
#define TXT_SEND_TO "Send to"
#endif
#ifndef TXT_HISTORY_STORED
#define TXT_HISTORY_STORED "History stored"
#endif
#ifndef TXT_SENDING_VIA
#define TXT_SENDING_VIA "Sending via"
#endif
#ifndef TXT_TAP_AGAIN_TO_ERASE
#define TXT_TAP_AGAIN_TO_ERASE "Tap again to erase"
#endif
#ifndef TXT_CLEAR_SALES_DATA
#define TXT_CLEAR_SALES_DATA "Clear sales data"
#endif
#ifndef TXT_CONFIRM
#define TXT_CONFIRM "Confirm"
#endif
#ifndef TXT_CLEAR
#define TXT_CLEAR "Clear"
#endif
#ifndef TXT_SALES
#define TXT_SALES "Sales"
#endif
#ifndef TXT_ITEMS
#define TXT_ITEMS "Items"
#endif
#ifndef TXT_AUTO
#define TXT_AUTO "Auto"
#endif
#ifndef TXT_EMAIL
#define TXT_EMAIL "Email"
#endif
#ifndef TXT_DATA
#define TXT_DATA "Data"
#endif
#ifndef TXT_SEND_TODAY
#define TXT_SEND_TODAY "Send Today"
#endif
#ifndef TXT_SEND_60_DAYS
#define TXT_SEND_60_DAYS "Send 60 Days"
#endif
#ifndef TXT_SENDING_REPORT
#define TXT_SENDING_REPORT "Sending report..."
#endif
#ifndef TXT_THIS_CAN_TAKE_A_MINUTE_OVER_4G
#define TXT_THIS_CAN_TAKE_A_MINUTE_OVER_4G "This can take a minute over 4G. Please wait."
#endif

// -- RFID cards --
#ifndef TXT_SCAN_NEW_CARD
#define TXT_SCAN_NEW_CARD "Scan New Card"
#endif
#ifndef TXT_HOLD_THE_CARD_NEAR
#define TXT_HOLD_THE_CARD_NEAR "Hold the card near"
#endif
#ifndef TXT_THE_READER
#define TXT_THE_READER "the reader"
#endif
#ifndef TXT_CANCELS_AUTOMATICALLY_AFTER_30
#define TXT_CANCELS_AUTOMATICALLY_AFTER_30 "Cancels automatically after 30 seconds"
#endif
#ifndef TXT_RFID_CARDS
#define TXT_RFID_CARDS "RFID Cards"
#endif
#ifndef TXT_NO_CARDS_REGISTERED_YET
#define TXT_NO_CARDS_REGISTERED_YET "No cards registered yet"
#endif
#ifndef TXT_EDIT_CARD
#define TXT_EDIT_CARD "Edit Card"
#endif
#ifndef TXT_NO_NAME
#define TXT_NO_NAME "(no name)"
#endif
#ifndef TXT_CARD_ID
#define TXT_CARD_ID "Card ID"
#endif
#ifndef TXT_LIMIT
#define TXT_LIMIT "Limit"
#endif
#ifndef TXT_USED
#define TXT_USED "Used"
#endif
#ifndef TXT_RFID_AUTO_RESET
#define TXT_RFID_AUTO_RESET "RFID Auto-Reset"
#endif
#ifndef TXT_MONTHLY_RESET
#define TXT_MONTHLY_RESET "Monthly reset"
#endif
#ifndef TXT_ZEROES_EVERY_CARD_S_USAGE
#define TXT_ZEROES_EVERY_CARD_S_USAGE "Zeroes every card's usage"
#endif

// -- Date and time --
#ifndef TXT_DATE_TIME
#define TXT_DATE_TIME "Date & Time"
#endif
#ifndef TXT_24_HOUR
#define TXT_24_HOUR "24 hour"
#endif
#ifndef TXT_12_HOUR
#define TXT_12_HOUR "12 hour"
#endif

// -- Network --
#ifndef TXT_WIFI
#define TXT_WIFI "WiFi"
#endif
#ifndef TXT_4G_LTE
#define TXT_4G_LTE "4G / LTE"
#endif
#ifndef TXT_BACKUP_LINK_WHEN_WIFI_IS_DOWN
#define TXT_BACKUP_LINK_WHEN_WIFI_IS_DOWN "Backup link when WiFi is down"
#endif

// -- Machine --
#ifndef TXT_CHANGE
#define TXT_CHANGE "Change"
#endif
#ifndef TXT_NOT_SET_LOWER
#define TXT_NOT_SET_LOWER "not set"
#endif
#ifndef TXT_CLOCK
#define TXT_CLOCK "Clock"
#endif
#ifndef TXT_SET
#define TXT_SET "Set"
#endif
#ifndef TXT_TIMEOUT_OFF
#define TXT_TIMEOUT_OFF "Timeout off"
#endif
#ifndef TXT_CUSTOMER
#define TXT_CUSTOMER "Customer"
#endif
#ifndef TXT_OPEN
#define TXT_OPEN "Open"
#endif

// -- Customer screen settings --
#ifndef TXT_CUSTOMER_SCREENS
#define TXT_CUSTOMER_SCREENS "Customer Screens"
#endif
#ifndef TXT_MAX_ITEMS
#define TXT_MAX_ITEMS "Max items"
#endif
#ifndef TXT_AUTO_TIMEOUT
#define TXT_AUTO_TIMEOUT "Auto timeout"
#endif
#ifndef TXT_SECONDS
#define TXT_SECONDS "Seconds"
#endif
#ifndef TXT_SHOW_COUNTDOWN
#define TXT_SHOW_COUNTDOWN "Show countdown"
#endif

// -- Dispensing --
#ifndef TXT_PRODUCT
#define TXT_PRODUCT "Product"
#endif
#ifndef TXT_DISPENSING
#define TXT_DISPENSING "Dispensing..."
#endif
#ifndef TXT_PRODUCT_DISPENSED
#define TXT_PRODUCT_DISPENSED "Product Dispensed!"
#endif
#ifndef TXT_COLLECT
#define TXT_COLLECT "Collect!"
#endif
#ifndef TXT_THANK_YOU
#define TXT_THANK_YOU "Thank You!"
#endif

// -- Idle time out --
#ifndef TXT_TIME_OUT
#define TXT_TIME_OUT "Time Out!"
#endif
#ifndef TXT_NO_ACTIVITY_FOR_A_WHILE
#define TXT_NO_ACTIVITY_FOR_A_WHILE "No activity for a while"
#endif
#ifndef TXT_RETURNING_TO_THE_HOME_SCREEN
#define TXT_RETURNING_TO_THE_HOME_SCREEN "Returning to the home screen"
#endif
