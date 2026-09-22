// ---------- Global app state shared by every screen ----------
AppScreen currentScreen = SCREEN_WELCOME;

// Which product slot (0..MAX_PRODUCTS-1) the Admin Edit/Motor-Assign/Text-Entry
// screens are currently operating on. Set by the Admin Panel screen.
int adminEditingSlot = -1;

// Motor Stock screen paginates MAX_MOTORS rows. Declared here (not in
// Screen_10_AdminMotor.ino) because Screen_08_AdminPanel.ino resets it before
// Screen_10's own tab has loaded — Arduino only auto-forward-declares
// functions, never variables, so a variable must be defined before its first
// use across tabs.
const int MOTORS_PER_PAGE = 6;
const int MOTOR_STOCK_PAGES = (MAX_MOTORS + MOTORS_PER_PAGE - 1) / MOTORS_PER_PAGE;
int motorStockPage = 0;

// Which of the Report screen's three pages is showing, and whether the
// destructive "Reset all totals" action has been armed by a first tap. Both
// live here for the same reason motorStockPage does: Screen_08_AdminPanel.ino
// resets the page before Screen_17_AdminReport.ino's tab has been reached in
// the concatenated build.
int reportPage = 0;
bool reportResetArmed = false;

// Admin RFID Cards screen paginates the registered-card list. Declared here
// for the same cross-tab reason as motorStockPage/reportPage above —
// Screen_11_AdminSettings.ino resets it before Screen_18_AdminRFIDCards.ino's
// own tab has been reached in the concatenated build.
int rfidCardsPage = 0;

// Which rfidCards[] slot (0..rfidCardCount-1) the Edit RFID Card / Text-Entry
// screens are currently operating on — the RFID-card equivalent of
// adminEditingSlot above. Set by Screen_18_AdminRFIDCards.ino when a card row
// is tapped.
int rfidEditingCard = -1;

// Master radio switches from Config.h. Declared here (not in
// Core_06_Network.ino / Core_13_LTEModem.ino, where they'd more naturally
// live) because Core_12_Main.ino's setup()/loop() reference both, and
// Core_12 loads before Core_13 in the concatenated build — the same
// define-before-use rule the comment above exists for.
const bool wifiEnabled = CFG_WIFI_ENABLED;

// Whether the firmware currently tries to use the cellular radio — separate
// from CFG_LTE_ENABLED, which says whether a modem is physically fitted at
// all. That macro alone still gates the one-time hardware bring-up in
// setup() and whether Admin > Settings' "4G" button is reachable
// (Screen_11_AdminSettings.ino) — a unit with no modem should never have
// either touched, and nothing should ever be able to lock a technician out
// of the one screen that can turn this back on.
//
// This flag is what Admin > 4G/LTE Setup's on/off toggle actually flips
// (setLteEnabled(), Core_13_LTEModem.ino). It is a *seed* value here, like
// everything else Config.h hands out — loadPersistedProductData()
// (Core_09_Storage.ino) overwrites it from NVS on every boot, and always
// ANDs it with CFG_LTE_ENABLED, so a unit with no modem fitted stays off no
// matter what was last stored.
bool lteEnabled = CFG_LTE_ENABLED;
