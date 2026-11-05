// custom_theme.h - the Custom colour theme (CFG_COLOR_THEME 3 in Config.h).
// Written by docs/theme_designer.html ("Save config"). Copy the saved file over
// this one, then compile and flash. You can also edit the values by hand.
// Colours are RGB565. A component colour set to another COL_ name follows it.

#define COL_BG_TOP              0xF63B   // #F6C6DE
#define COL_BG_BOTTOM           0xFE1B   // #FFC2DE
#define COL_CARD                0xFF3D   // #FFE6EE
#define COL_CARD_BRD            0xD1AD   // #D5346A
#define COL_ACCENT              0xE0F1   // #E61C8B
#define COL_TEXT                0x4886   // #4A1031
#define COL_TEXT_DIM            0x9B4F   // #9C697B
#define COL_SHADOW              0xE5B9   // #E6B6CD
#define COL_DANGER              0xE1CA   // #E63852
#define COL_SUCCESS             0x350B   // #31A15A
#define COL_WARNING             0xB421   // #B48508

// ---------- Header and Welcome strip ----------
#define COL_HDR_BG              COL_BG_TOP            // Band behind every screen title
#define COL_HDR_TITLE           COL_TEXT              // Screen title text
#define COL_HDR_RULE            COL_ACCENT            // Line under the title
#define COL_HDR_RULE_FADE       COL_HDR_BG            // Colour the title line fades into
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
#define COL_PILL_HOLLOW         COL_BG_TOP            // Pill fill while it blinks to outline

// ---------- Cards, rows and tiles ----------
#define COL_CARD_FILL           COL_CARD              // Card, row and tile fill
#define COL_CARD_BORDER         COL_CARD_BRD          // Card, row and tile border
#define COL_CARD_SHADOW         COL_SHADOW            // Shadow under cards
#define COL_CARD_RING           COL_ACCENT            // Inner highlight ring on key cards
#define COL_CARD_RULE           COL_CARD_BRD          // Divider line inside a card
#define COL_CARD_TITLE          COL_TEXT              // Main text on cards and rows
#define COL_CARD_CAPTION        COL_TEXT_DIM          // Small caption text on cards and rows
#define COL_VALUE_TEXT          COL_ACCENT            // Editable values in rows (name, ID, clock)
#define COL_HINT_TEXT           COL_TEXT              // Hint lines on the background
#define COL_TILE_STRIPE         COL_ACCENT            // Stripe on Admin tiles
#define COL_TILE_STRIPE_OFF     COL_TEXT_DIM          // Stripe on disabled tiles
#define COL_TILE_STRIPE_OK      COL_SUCCESS           // Stripe on a connected network tile
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
#define COL_FIELD_BORDER        COL_CARD_BRD          // Text preview field border
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

// ---------- Fonts ----------
// One family for the whole UI: Serif, Sans or Mono. Sizes are fixed: XS 9, SM bold 9, MD bold 12, LG bold 18, RM 12.
#define UI_FONT_XS              FreeSerif9pt7b
#define UI_FONT_SM              FreeSerifBold9pt7b
#define UI_FONT_MD              FreeSerifBold12pt7b
#define UI_FONT_LG              FreeSerifBold18pt7b
#define UI_FONT_RM              FreeSerif12pt7b

// ---------- Text ----------
// Reword any label. Keep it short, and do not use % or double quotes.

// -- Welcome --
#define TXT_SYNCING                     "Syncing..."
#define TXT_NO_NETWORK                  "No network"
#define TXT_OUT_OF_STOCK                "Out of Stock"
#define TXT_TOUCH_TO_CONTINUE           "Touch to Continue"

// -- Select products --
#define TXT_SELECT_PRODUCTS             "Select Products"
#define TXT_TAP_TO_SELECT               "Tap to select"
#define TXT_NO_PRODUCTS_AVAILABLE       "No products available"

// -- Shared labels --
#define TXT_BACK_ARROW                  "< Back"
#define TXT_GET_FREE                    "Get Free >"

// -- Select products --
#define TXT_PROCEED                     "Proceed >"
#define TXT_CART                        "Cart >"
#define TXT_OUT_OF_STOCK_CAPS           "OUT OF STOCK"
#define TXT_ALL_IN_CART                 "ALL IN CART"
#define TXT_CART_FULL                   "CART FULL"

// -- Your cart --
#define TXT_YOUR_CART                   "Your Cart"
#define TXT_CART_IS_EMPTY               "Cart is empty"
#define TXT_PAY                         "Pay >"

// -- Select payment --
#define TXT_SCAN_THE_QR_CODE            "Scan the QR code"
#define TXT_INSERT_EXACT_CHANGE         "Insert exact change"
#define TXT_SELECT_PAYMENT              "Select Payment"
#define TXT_NO_PAYMENT_METHODS          "No payment methods"
#define TXT_AVAILABLE                   "available"

// -- UPI payment --
#define TXT_SCAN_TO_PAY                 "Scan to Pay"
#define TXT_AMOUNT_TO_PAY               "Amount to pay"
#define TXT_SCAN_WITH_ANY               "Scan with any"
#define TXT_UPI_APP                     "UPI app"
#define TXT_QR_GENERATION_FAILED        "QR generation failed"
#define TXT_CANCEL_ARROW                "< Cancel"

// -- Shared labels --
#define TXT_DONE                        "Done"
#define TXT_PAYMENT                     "Payment"

// -- UPI payment --
#define TXT_SUCCESSFUL                  "Successful!"
#define TXT_PREPARING_YOUR_ITEM         "Preparing your item..."
#define TXT_DISPENSE_FAILED_CONTACT_SUPPOR"Dispense failed - contact support"

// -- Shared labels --
#define TXT_TIMED_OUT                   "Timed Out"
#define TXT_NO_PAYMENT_WAS_RECEIVED_IN_TIM"No payment was received in time."

// -- UPI payment --
#define TXT_RETRY                       "Retry"

// -- Cash and RFID payment --
#define TXT_REMAINING                   "Remaining"
#define TXT_PAYING_NO_REFUND            "Paying, no refund"
#define TXT_INSERT_COINS                "Insert Coins"

// -- Shared labels --
#define TXT_DISPENSE                    "Dispense"
#define TXT_FAILED                      "Failed"
#define TXT_PLEASE_CONTACT_SUPPORT      "Please contact support"

// -- Cash and RFID payment --
#define TXT_COIN_NOT_RECOGNIZED         "Coin not recognized"
#define TXT_TAP_RFID_CARD               "Tap RFID Card"
#define TXT_NO_CHARGE_FOR               "No charge for"
#define TXT_REGISTERED_CARDS            "registered cards"
#define TXT_HOLD_YOUR_CARD_NEAR_THE_READER"Hold your card near the reader"
#define TXT_DISPENSE_FAILED             "Dispense Failed"
#define TXT_CARD_NOT_REGISTERED         "Card not registered"
#define TXT_NO_CARD_WAS_TAPPED_IN_TIME  "No card was tapped in time."

// -- Admin login --
#define TXT_ENTER_PIN                   "Enter PIN"
#define TXT_ADMIN_LOGIN                 "Admin Login"

// -- Shared labels --
#define TXT_CANCEL                      "Cancel"

// -- Admin panel --
#define TXT_FREE_VEND                   "Free vend"
#define TXT_NONE_ENABLED                "None enabled"
#define TXT_WIFI_CONNECTED              "WiFi connected"
#define TXT_4G_CONNECTED                "4G connected"
#define TXT_OFFLINE                     "Offline"
#define TXT_ID_PIN_CLOCK                "ID, PIN, clock"
#define TXT_ADMIN_PANEL                 "Admin Panel"

// -- Shared labels --
#define TXT_PRODUCTS                    "Products"

// -- Admin panel --
#define TXT_STOCK                       "Stock"

// -- Shared labels --
#define TXT_PAYMENTS                    "Payments"
#define TXT_NETWORK                     "Network"
#define TXT_REPORTS                     "Reports"
#define TXT_MACHINE                     "Machine"

// -- Admin panel --
#define TXT_EXIT_ADMIN                  "Exit Admin"

// -- Shared labels --
#define TXT_NAME                        "Name"
#define TXT_EDIT                        "Edit"

// -- Edit product --
#define TXT_PRICE_RS                    "Price (Rs.)"
#define TXT_MOTORS                      "Motors"
#define TXT_ASSIGN                      "Assign"
#define TXT_VISIBLE                     "Visible"

// -- Shared labels --
#define TXT_BACK                        "Back"

// -- Motors and stock --
#define TXT_SELECTED                    "Selected"
#define TXT_FREE                        "Free"
#define TXT_TAP_A_MOTOR_TO_ASSIGN_OR_RELEA"Tap a motor to assign or release it"

// -- Shared labels --
#define TXT_SAVE                        "Save"

// -- Motors and stock --
#define TXT_FILL_MAX                    "Fill Max"
#define TXT_MOTOR_STOCK                 "Motor Stock"
#define TXT_UNASSIGNED                  "Unassigned"
#define TXT_TAP_EDIT_TO_TYPE_AN_EXACT_QUAN"Tap Edit to type an exact quantity"
#define TXT_REFILL_TRAYS                "Refill Trays"
#define TXT_SET_MAX                     "Set Max"
#define TXT_ALL_TRAYS                   "All Trays"

// -- Admin payments --
#define TXT_FREE_VEND_LABEL             "Free Vend"
#define TXT_SHOW_PRICES                 "Show Prices"

// -- Shared labels --
#define TXT_SETUP                       "Setup"

// -- Admin payments --
#define TXT_CARDS                       "Cards"

// -- Shared labels --
#define TXT_RESET                       "Reset"

// -- Admin payments --
#define TXT_FREE_VEND_CUSTOMERS_ARE_NEVER"Free vend: customers are never charged"

// -- Shared labels --
#define TXT_NOT_SET                     "(not set)"

// -- UPI setup --
#define TXT_UPI_CONFIGURATION           "UPI Configuration"
#define TXT_TIMEOUT_MIN                 "Timeout (min)"

// -- WiFi setup --
#define TXT_IP                          "IP: -"

// -- Shared labels --
#define TXT_SIGNAL                      "Signal: -"

// -- WiFi setup --
#define TXT_PING_WIFI_IS_TURNED_OFF     "Ping: WiFi is turned off"
#define TXT_SET_A_NETWORK_NAME_TO_GET_STAR"Set a network name to get started"
#define TXT_PING_NOT_TESTED_YET         "Ping: not tested yet"
#define TXT_PING_FAILED_NO_INTERNET_REACHA"Ping failed - no internet reachable"
#define TXT_WIFI_SETUP                  "WiFi Setup"

// -- Shared labels --
#define TXT_PASSWORD                    "Password"

// -- WiFi setup --
#define TXT_NONE                        "(none)"
#define TXT_CONNECT                     "Connect"
#define TXT_PING                        "Ping"

// -- Text entry --
#define TXT_ENTER_NAME                  "Enter Name"
#define TXT_WIFI_NAME                   "WiFi Name"
#define TXT_WIFI_PASSWORD               "WiFi Password"
#define TXT_UPI_MERCHANT_ID             "UPI Merchant ID"
#define TXT_UPI_STORE_ID                "UPI Store ID"

// -- Shared labels --
#define TXT_MACHINE_ID                  "Machine ID"
#define TXT_STORE_ID                    "Store ID"

// -- Text entry --
#define TXT_MACHINE_NAME                "Machine Name"
#define TXT_GMAIL_ADDRESS               "Gmail Address"
#define TXT_GMAIL_APP_PASSWORD          "Gmail App Password"
#define TXT_SEND_REPORT_TO              "Send Report To"
#define TXT_EMAIL_1_AT_HH_MM            "Email 1 at HH:MM"
#define TXT_EMAIL_2_AT_HH_MM            "Email 2 at HH:MM"
#define TXT_CARD_NUMBER_HEX             "Card Number (hex)"
#define TXT_CARD_HOLDER_NAME            "Card Holder Name"

// -- Shared labels --
#define TXT_ADMIN_PIN                   "Admin PIN"

// -- Text entry --
#define TXT_CONFIRM_NEW_PIN             "Confirm New PIN"
#define TXT_USE_HH_MM_24H               "Use HH:MM (24h)"

// -- Shared labels --
#define TXT_CARD_ADDED                  "Card added"
#define TXT_ALREADY_REGISTERED          "Already registered"

// -- Text entry --
#define TXT_NEED_4_8_DIGITS             "Need 4-8 digits"
#define TXT_PINS_DIDN_T_MATCH           "PINs didn't match"

// -- 4G / LTE setup --
#define TXT_THIS_CAN_TAKE_20_SECONDS    "This can take 20+ seconds"
#define TXT_TURNED_OFF                  "Turned off"
#define TXT_CONNECTED                   "Connected"
#define TXT_NOT_CONNECTED               "Not connected"
#define TXT_USED_AS_THE_BACKUP_LINK_WHEN_W"Used as the backup link when WiFi is down"
#define TXT_TEST_4G_IS_TURNED_OFF       "Test: 4G is turned off"
#define TXT_TEST_NOT_TESTED_YET         "Test: not tested yet"
#define TXT_TEST_FAILED_COULD_NOT_REACH_TH"Test failed - could not reach the internet"
#define TXT_4G_LTE_SETUP                "4G / LTE Setup"
#define TXT_TEST_4G                     "Test 4G"

// -- Reports --
#define TXT_TRANSACTIONS                "Transactions"
#define TXT_PRODUCTS_SOLD               "Products sold"
#define TXT_TOTAL_SALES_RS              "Total sales (Rs.)"
#define TXT_UNITS_IN_STOCK              "Units in stock"
#define TXT_TODAY_RS                    "Today (Rs.)"
#define TXT_TODAY_UNITS_TXNS            "Today units/txns"
#define TXT_OFF                         "Off"
#define TXT_SEND_THIS_REPORT            "Send this report"
#define TXT_REPORT_TIME_1               "Report time 1"
#define TXT_REPORT_TIME_2               "Report time 2"
#define TXT_LOW_STOCK_ALERT             "Low-stock alert"
#define TXT_ALERT_BELOW                 "Alert below"
#define TXT_GMAIL                       "Gmail"
#define TXT_SEND_TO                     "Send to"
#define TXT_HISTORY_STORED              "History stored"
#define TXT_SENDING_VIA                 "Sending via"
#define TXT_TAP_AGAIN_TO_ERASE          "Tap again to erase"
#define TXT_CLEAR_SALES_DATA            "Clear sales data"
#define TXT_CONFIRM                     "Confirm"
#define TXT_CLEAR                       "Clear"
#define TXT_SALES                       "Sales"
#define TXT_ITEMS                       "Items"
#define TXT_AUTO                        "Auto"
#define TXT_EMAIL                       "Email"
#define TXT_DATA                        "Data"
#define TXT_SEND_TODAY                  "Send Today"
#define TXT_SEND_60_DAYS                "Send 60 Days"
#define TXT_SENDING_REPORT              "Sending report..."
#define TXT_THIS_CAN_TAKE_A_MINUTE_OVER_4G"This can take a minute over 4G. Please wait."

// -- RFID cards --
#define TXT_SCAN_NEW_CARD               "Scan New Card"
#define TXT_HOLD_THE_CARD_NEAR          "Hold the card near"
#define TXT_THE_READER                  "the reader"
#define TXT_CANCELS_AUTOMATICALLY_AFTER_30"Cancels automatically after 30 seconds"
#define TXT_RFID_CARDS                  "RFID Cards"
#define TXT_NO_CARDS_REGISTERED_YET     "No cards registered yet"
#define TXT_EDIT_CARD                   "Edit Card"
#define TXT_NO_NAME                     "(no name)"
#define TXT_CARD_ID                     "Card ID"
#define TXT_LIMIT                       "Limit"
#define TXT_USED                        "Used"

// -- Date and time --
#define TXT_DATE_TIME                   "Date & Time"
#define TXT_24_HOUR                     "24 hour"
#define TXT_12_HOUR                     "12 hour"

// -- RFID cards --
#define TXT_RFID_AUTO_RESET             "RFID Auto-Reset"
#define TXT_MONTHLY_RESET               "Monthly reset"
#define TXT_ZEROES_EVERY_CARD_S_USAGE   "Zeroes every card's usage"

// -- Network --
#define TXT_WIFI                        "WiFi"
#define TXT_4G_LTE                      "4G / LTE"
#define TXT_BACKUP_LINK_WHEN_WIFI_IS_DOWN"Backup link when WiFi is down"

// -- Machine --
#define TXT_CHANGE                      "Change"
#define TXT_NOT_SET_LOWER               "not set"
#define TXT_CLOCK                       "Clock"
#define TXT_SET                         "Set"
#define TXT_TIMEOUT_OFF                 "Timeout off"
#define TXT_CUSTOMER                    "Customer"
#define TXT_OPEN                        "Open"

// -- Customer screen settings --
#define TXT_CUSTOMER_SCREENS            "Customer Screens"
#define TXT_MAX_ITEMS                   "Max items"
#define TXT_AUTO_TIMEOUT                "Auto timeout"
#define TXT_SECONDS                     "Seconds"
#define TXT_SHOW_COUNTDOWN              "Show countdown"

// -- Dispensing --
#define TXT_PRODUCT                     "Product"
#define TXT_DISPENSING                  "Dispensing..."
#define TXT_PRODUCT_DISPENSED           "Product Dispensed!"
#define TXT_COLLECT                     "Collect!"
#define TXT_THANK_YOU                   "Thank You!"

// -- Idle time out --
#define TXT_TIME_OUT                    "Time Out!"
#define TXT_NO_ACTIVITY_FOR_A_WHILE     "No activity for a while"
#define TXT_RETURNING_TO_THE_HOME_SCREEN"Returning to the home screen"

// DESIGNER-PROJECT {"version":5,"G":{"BG_TOP":63035,"BG_BOTTOM":65051,"CARD":65341,"CARD_BRD":53677,"ACCENT":57585,"TEXT":18566,"TEXT_DIM":39759,"SHADOW":58809,"DANGER":57802,"SUCCESS":13579,"WARNING":46113},"GC":{},"O":{},"F":{"family":"Serif","XS":9,"SM":9,"MD":12,"LG":18,"RM":12,"bold":{"XS":false,"SM":true,"MD":true,"LG":true,"RM":false},"italic":{"XS":false,"SM":false,"MD":false,"LG":false,"RM":false}},"T":{}}
