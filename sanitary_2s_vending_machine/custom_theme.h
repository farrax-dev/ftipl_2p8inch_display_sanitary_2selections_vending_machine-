// custom_theme.h - the Custom colour theme (CFG_COLOR_THEME 3 in Config.h).
// Written by docs/theme_designer.html ("Save config"). Copy the saved file over
// this one, then compile and flash. You can also edit the values by hand.
// Colours are RGB565. A component colour set to another COL_ name follows it.

#define COL_BG_TOP              0xFF7E   // #FFEEF6
#define COL_BG_BOTTOM           0xFE1B   // #FFC2DE
#define COL_CARD                0xFF3D   // #FFE6EE
#define COL_CARD_BRD            0xD1AD   // #D5346A
#define COL_ACCENT              0xE0F1   // #E61C8B
#define COL_TEXT                0xE0B1   // #E6148B
#define COL_TEXT_DIM            0x9B4F   // #9C697B
#define COL_SHADOW              0xE5B9   // #E6B6CD
#define COL_DANGER              0xE1CA   // #E63852
#define COL_SUCCESS             0x350B   // #31A15A
#define COL_WARNING             0xB421   // #B48508

// ---------- Header and Welcome strip ----------
#define COL_HDR_BG              0x1041   // #100808  (default: BG_TOP)
#define COL_HDR_TITLE           0x93F1   // #947D8B  (default: TEXT)
#define COL_HDR_RULE            0x984B   // #9C085A  (default: ACCENT)
#define COL_HDR_RULE_FADE       0xA249   // #A4484A  (default: HDR_BG)
#define COL_HDR_NOTE            COL_ACCENT            // Hint in the header (Tap to select)
#define COL_HDR_COUNT           COL_TEXT              // Counter in the header (1 / 5)
#define COL_HDR_COUNT_DIM       COL_TEXT_DIM          // Dim counter in the header (RFID card count)
#define COL_STRIP_BG            COL_BG_TOP            // Welcome top strip
#define COL_STRIP_LINE          COL_CARD_BRD          // Line under the Welcome strip
#define COL_STRIP_TIME          COL_TEXT              // Clock text
#define COL_STRIP_DATE          COL_ACCENT            // Date text
#define COL_WELCOME_SUB         COL_ACCENT            // Subtitle on the Welcome card
#define COL_PILL_FILL           COL_ACCENT            // Touch to Continue pill
#define COL_PILL_TEXT           COL_BG_TOP            // Text on the pill
#define COL_PILL_HOLLOW         0x5065   // #520C29  (default: BG_TOP)

// ---------- Cards, rows and tiles ----------
#define COL_CARD_FILL           0x6927   // #6A2439  (default: CARD)
#define COL_CARD_BORDER         0x0000   // #000000  (default: CARD_BRD)
#define COL_CARD_SHADOW         COL_SHADOW            // Shadow under cards
#define COL_CARD_RING           COL_ACCENT            // Inner highlight ring on key cards
#define COL_CARD_RULE           COL_CARD_BRD          // Divider line inside a card
#define COL_CARD_TITLE          COL_TEXT              // Main text on cards and rows
#define COL_CARD_CAPTION        COL_TEXT_DIM          // Small caption text on cards and rows
#define COL_VALUE_TEXT          COL_ACCENT            // Editable values in rows (name, ID, clock)
#define COL_HINT_TEXT           0x0000   // #000000  (default: TEXT)
#define COL_TILE_STRIPE         COL_ACCENT            // Stripe on Admin tiles
#define COL_TILE_STRIPE_OFF     COL_TEXT_DIM          // Stripe on disabled tiles
#define COL_TILE_STRIPE_OK      0x54ED   // #529D6A  (default: SUCCESS)
#define COL_TILE_LABEL          COL_TEXT_DIM          // Caption on Reports tiles
#define COL_TILE_VALUE          COL_TEXT              // Number on Reports tiles
#define COL_TILE_VALUE_HI       COL_ACCENT            // Highlighted number on Reports tiles

// ---------- Products, price and cart ----------
#define COL_PRODUCT_NAME        COL_TEXT              // Product name
#define COL_PRICE_TEXT          COL_ACCENT            // Price (Rs. 10) on cards, cart and totals
#define COL_STOCK_TEXT          COL_SUCCESS           // Qty Avail text
#define COL_PRODUCT_FILL        COL_CARD              // Product card fill
#define COL_PRODUCT_OOS_FILL    COL_BG_TOP            // Out-of-stock card fill
#define COL_PRODUCT_BORDER      COL_CARD_BRD          // Product card border
#define COL_PRODUCT_SEL_BORDER  COL_ACCENT            // Border of a product in the cart
#define COL_PRODUCT_LIMIT_BORDERCOL_WARNING           // Border when the cart is full
#define COL_PRODUCT_ERR_BORDER  COL_DANGER            // Flash border on a refused tap
#define COL_QTY_BTN_FILL        COL_ACCENT            // Cart minus and plus buttons
#define COL_QTY_BTN_GLYPH       COL_BG_TOP            // Minus and plus symbols
#define COL_QTY_BOX_FILL        COL_CARD              // Cart quantity box
#define COL_QTY_TEXT            COL_TEXT              // Cart quantity number
#define COL_METHOD_NAME         COL_ACCENT            // UPI and Cash card names
#define COL_METHOD_RULE         COL_CARD_BRD          // Line under the payment name
#define COL_METHOD_DESC         COL_TEXT              // Payment card description

// ---------- Buttons ----------
#define COL_BTN_PRI_FILL        COL_ACCENT            // Primary button fill (Pay, Save, Done)
#define COL_BTN_PRI_TEXT        COL_BG_TOP            // Primary button label
#define COL_BTN_SEC_FILL        COL_CARD              // Normal button fill (Back, Cancel, keypad)
#define COL_BTN_SEC_BORDER      COL_CARD_BRD          // Normal button border
#define COL_BTN_SEC_TEXT        COL_TEXT              // Normal button label
#define COL_BTN_DANGER_FILL     COL_DANGER            // Danger button fill
#define COL_BTN_DANGER_TEXT     COL_BG_TOP            // Danger button label
#define COL_BTN_OFF_FILL        COL_CARD              // Disabled button fill
#define COL_BTN_OFF_BORDER      COL_CARD_BRD          // Disabled button border
#define COL_BTN_OFF_TEXT        COL_TEXT_DIM          // Disabled button label
#define COL_BTN_SHADOW          COL_SHADOW            // Shadow under buttons

// ---------- Switch and stepper ----------
#define COL_SW_ON_FILL          COL_ACCENT            // Switch track when on
#define COL_SW_ON_BORDER        COL_ACCENT            // Switch outline and knob ring when on
#define COL_SW_ON_TEXT          COL_BG_TOP            // ON label
#define COL_SW_OFF_FILL         COL_SHADOW            // Switch track when off
#define COL_SW_OFF_BORDER       COL_CARD_BRD          // Switch outline and knob ring when off
#define COL_SW_OFF_TEXT         COL_TEXT_DIM          // OFF label
#define COL_SW_KNOB             COL_BG_TOP            // Switch knob
#define COL_STEP_BTN_FILL       COL_ACCENT            // Stepper minus and plus buttons
#define COL_STEP_BTN_GLYPH      COL_BG_TOP            // Minus and plus symbols
#define COL_STEP_VALUE_FILL     COL_BG_TOP            // Stepper value box
#define COL_STEP_VALUE_BORDER   COL_CARD_BRD          // Stepper value box border
#define COL_STEP_VALUE_TEXT     COL_ACCENT            // Stepper number
#define COL_STEP_VALUE_OFF      COL_TEXT_DIM          // Stepper number when disabled
#define COL_STEP_VALUE_LOW      COL_DANGER            // Stock number when low

// ---------- Tabs, tables and pickers ----------
#define COL_TAB_ON_FILL         COL_ACCENT            // Selected tab
#define COL_TAB_ON_TEXT         COL_BG_TOP            // Selected tab label
#define COL_TAB_OFF_FILL        COL_CARD              // Other tabs
#define COL_TAB_OFF_BORDER      COL_CARD_BRD          // Other tabs border
#define COL_TAB_OFF_TEXT        COL_TEXT              // Other tabs label
#define COL_TBL_HEAD_FILL       COL_ACCENT            // Items table header
#define COL_TBL_HEAD_TEXT       COL_BG_TOP            // Items table header text
#define COL_TBL_ROW_A           COL_BG_TOP            // Items table odd rows
#define COL_TBL_ROW_B           COL_CARD              // Items table even rows
#define COL_TBL_TEXT            COL_TEXT              // Items table text
#define COL_TBL_FRAME           COL_CARD_BRD          // Items table frame
#define COL_ROW_CHEVRON         COL_TEXT_DIM          // Arrow on tappable rows
#define COL_PICK_VALUE_FILL     COL_CARD              // Hour, minute and date value boxes
#define COL_PICK_VALUE_BORDER   COL_CARD_BRD          // Value box border
#define COL_PICK_VALUE_TEXT     COL_ACCENT            // Value box number
#define COL_PICK_LABEL          COL_TEXT              // Field label (Hour, Day)
#define COL_PICK_LABEL_OFF      COL_TEXT_DIM          // Field label when off

// ---------- PIN, keyboard and text field ----------
#define COL_PIN_FIELD_FILL      COL_CARD              // PIN box
#define COL_PIN_FIELD_BORDER    COL_CARD_BRD          // PIN box border
#define COL_PIN_DOT             COL_ACCENT            // Entered PIN dots
#define COL_PIN_PLACEHOLDER     COL_TEXT_DIM          // Enter PIN prompt
#define COL_PIN_TEXT            COL_TEXT              // PIN box message
#define COL_KEY_FILL            COL_CARD              // Letter key fill
#define COL_KEY_BORDER          COL_CARD_BRD          // Key border
#define COL_KEY_TEXT            COL_TEXT              // Letter key text
#define COL_KEY_MOD_FILL        COL_BG_TOP            // DEL, SPACE and ?123 key fill
#define COL_KEY_MOD_TEXT        COL_ACCENT            // DEL, SPACE and ?123 key text
#define COL_FIELD_FILL          COL_CARD              // Text preview field
#define COL_FIELD_BORDER        0x82CD   // #83596A  (default: CARD_BRD)
#define COL_FIELD_TEXT          COL_ACCENT            // Typed text
#define COL_FIELD_EMPTY         COL_TEXT_DIM          // (empty) placeholder
#define COL_FIELD_COUNT         COL_TEXT_DIM          // 12/30 counter
#define COL_FIELD_ERROR         COL_DANGER            // Error shown in the field

// ---------- Indicators, messages and status ----------
#define COL_DOT_ON              COL_SUCCESS           // Status dot on
#define COL_DOT_OFF_FILL        COL_BG_TOP            // Status dot off
#define COL_DOT_OFF_RING        COL_TEXT_DIM          // Status dot off ring
#define COL_WIFI_GOOD           COL_SUCCESS           // Signal icon, strong
#define COL_WIFI_FAIR           COL_WARNING           // Signal icon, fair
#define COL_WIFI_BAD            COL_DANGER            // Signal icon, weak or none
#define COL_WIFI_OFF            COL_TEXT_DIM          // Signal icon, unlit bars
#define COL_WIFI_BG             COL_HDR_BG            // Behind the signal icon
#define COL_MSG_TITLE           COL_ACCENT            // Big message headline
#define COL_MSG_BODY            COL_TEXT              // Message body text
#define COL_OK_TEXT             COL_SUCCESS           // Success text (paid, connected)
#define COL_ERR_TEXT            COL_DANGER            // Error text (failed, timed out)
#define COL_WARN_TEXT           COL_WARNING           // Warning text
#define COL_COUNTDOWN_TEXT      COL_TEXT              // Idle countdown
#define COL_COUNTDOWN_URGENT    COL_DANGER            // Idle countdown in the last 10 seconds

// DESIGNER-PROJECT {"version":3,"G":{"BG_TOP":65406,"BG_BOTTOM":65051,"CARD":65341,"CARD_BRD":53677,"ACCENT":57585,"TEXT":57521,"TEXT_DIM":39759,"SHADOW":58809,"DANGER":57802,"SUCCESS":13579,"WARNING":46113},"GC":{"HDR_BG":4161,"HDR_TITLE":37873,"HDR_RULE":38987,"HDR_RULE_FADE":41545,"PILL_HOLLOW":20581,"HINT_TEXT":0,"CARD_FILL":26919,"CARD_BORDER":0,"TILE_STRIPE_OK":21741,"FIELD_BORDER":33485},"O":{}}
