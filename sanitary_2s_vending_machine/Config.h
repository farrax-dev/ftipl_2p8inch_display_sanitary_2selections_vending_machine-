// =====================================================
//   PER-MACHINE CONFIGURATION
// =====================================================
// Everything that differs between one physical machine and the next lives in
// this one file. To commission a new unit: edit the values below and flash.
// No other file should need touching.
// 
// HOW THESE INTERACT WITH THE ADMIN SCREENS
// Most of these are *seed* values, not permanent ones. Edit anything below,
// flash, and it takes effect — there is no version number to remember. The
// firmware fingerprints the values in this file and re-seeds NVS whenever
// that fingerprint changes, so the file itself is the record of what a
// machine was commissioned with.
//
// Between flashes the admin screens own these settings, so a technician can
// correct a WiFi password or a nightly send time on site and it survives
// reboots and power cuts. Reflashing with an edited Config.h overrides them
// again, which is what you want when reassigning a machine.
//
// In short: change this file to change the machine; change it on the screen
// for a one-off local fix.

#ifndef CONFIG_H
#define CONFIG_H

// ---------- Welcome screen title ----------
// The large text on the card that fills the idle screen. Three lines, laid
// out exactly as written, so you choose where the words break rather than
// leaving it to the firmware:
//
//     CFG_TITLE_LINE_1     large, white      up to ~10 characters
//     CFG_TITLE_LINE_2     large, white      up to ~10 characters
//     CFG_TITLE_SUBTITLE   smaller, accent   up to ~20 characters
//
// Leave any line as "" to omit it; the rest re-centre on the card. Lines 1
// and 2 are drawn at text size 4, so anything past ~10 characters runs off
// the card edge.
#define CFG_TITLE_LINE_1       "SANITARY"
#define CFG_TITLE_LINE_2       "NAPKIN"
#define CFG_TITLE_SUBTITLE     "VENDING MACHINE"

// ---------- Identity ----------
// Where this particular unit is. Shown in the small line under the Welcome
// card, and in every report email subject beside the ID, e.g.
// "VM-A4C21F | Ladies Room 1 | Daily Sales Summary - 09-09-2026".
//
// Keep this DIFFERENT on every machine — it is what tells one unit's reports
// from another's. The product title above is the same across a whole fleet,
// so it cannot do that job.
#define CFG_MACHINE_NAME       "Sanitary Napkin Vending Machine"

// Leave empty to derive a unique ID from the ESP32's MAC (recommended — it
// cannot collide across machines). Set it only if the customer needs a
// specific asset tag, and then remember it must differ per unit.
#define CFG_MACHINE_ID         "FTIPL0001"

// ---------- Hardware fitted ----------
// How many dispensing motors are PHYSICALLY wired. Pins are taken in order
// from MOTOR_PIN_POOL in Core_04_Hardware.ino, so 2 here means the first two
// pins of that pool (GPIO13, GPIO25) — M1 and M2. Maximum is the size of
// that pool.
//
// This sanitary-napkin build only fills 2 of the 5 pool slots. The other
// three pool pins (GPIO26, GPIO27, GPIO32) are deliberately left untouched —
// GPIO26/27 sit idle in reserve, and GPIO32 is dual-purposed behind an
// external dip switch as the coin acceptor's ON/OFF power control line (see
// COIN_ENABLE_PIN in Screen_06_PaymentOther.ino). Raising this back past 4
// would collide with that dip-switch wiring on GPIO32 — move the switch back
// to its "M5" position first.
#define CFG_MOTOR_COUNT        

// How many product slots the admin menu offers. Names, prices and motor
// assignments for each are set on the admin screens.
//
// Out of the box products 1-3 are enabled and assigned to motors 1-3. If you
// set CFG_MOTOR_COUNT below 3, those products keep an assignment to a motor
// that is not fitted and will fail to dispense — reassign them under
// Admin > product > Assign Motors after flashing.
#define CFG_PRODUCT_COUNT      2

// Stock level a freshly-initialised motor starts at, and the maximum the
// admin +/- buttons will count up to.
#define CFG_STOCK_MAX          25

// Unit shown after the quantity-available figure on the product select
// screen, e.g. a stock of 20 reads "20" with this left as "", "20ml" with
// this set to "ml", or "20 Units" with this set to " Units" (include the
// leading space yourself if you want one).
#define CFG_STOCK_UNIT         ""

// How long a motor runs to dispense one unit, in milliseconds — one value,
// used for every product/motor. There is no admin-screen control for this,
// so change it here and reflash.
#define CFG_MOTOR_RUN_MS       2500

// true  = a HIGH on the GPIO energises the motor (most MOSFET boards)
// false = a LOW energises it (most opto-isolated relay boards)
// Getting this wrong makes every motor run continuously from power-on, so
// check it against your driver board before the first test.
#define CFG_MOTOR_ACTIVE_HIGH  true

// ---------- Selling ----------
// Maximum items in one purchase. Setting this to 1 with a single enabled
// product turns on quick-vend: the cart is hidden and tapping the product
// goes straight to payment.
#define CFG_MAX_CART_QTY       3

// PIN for the admin menu, reached by holding any screen corner for 2 seconds.
#define CFG_ADMIN_PIN          "1234"

// Which payment methods this machine can use at all. Admin > Settings only
// shows a toggle for methods listed here — leaving one out removes it from
// the machine entirely (it never appears to a customer either), it isn't
// just switched off by default the way the admin ON/OFF toggle is.
//
// "Cash" is the coin acceptor (Screen_06_PaymentOther.ino) — there was never
// a separate paper note acceptor fitted, so that placeholder payment method
// was removed rather than left to show a screen with no hardware behind it.
//
// Set here for a fully offline machine: no UPI (needs internet), just cash
// and registered-card RFID.
//
// RFID is off for this build (registered-card free-vend isn't wanted right
// now) but left available here, not deleted — flip back to true and reflash
// to bring the Screen_18/19 admin card screens and the RFID payment option
// back without touching any other code.
#define CFG_PAYMENT_UPI_AVAILABLE     true
#define CFG_PAYMENT_CASH_AVAILABLE    true
#define CFG_PAYMENT_RFID_AVAILABLE    false

// ---------- Connectivity ----------
// Master switches for the two radios. false fully disables the hardware —
// WiFi is never started and the 4G modem is never initialised, not just
// hidden from the customer — and greys out the WiFi/4G buttons under
// Admin > Settings so there is nothing to accidentally turn back on from the
// touchscreen. Flip both false for a machine that is meant to run
// completely offline (coin/cash/RFID only); flip one back on to add UPI or
// email reports later.
//
// This is only "is a modem fitted at all" — a separate on/off toggle lives
// on the touchscreen itself (Admin > Settings > 4G > "LTE: On/Off") for a
// unit that HAS a modem fitted but shouldn't be trying to use it right now
// (no SIM, no coverage, or simply a machine that's meant to run on its
// battery-backed clock alone without the modem retrying a connection every
// 30 seconds). That on-screen setting starts from CFG_LTE_ENABLED's value
// the first time this file is flashed and is then the admin's to change —
// same seed-then-owned relationship as everything else in this file — but
// it can never turn LTE on when CFG_LTE_ENABLED is false here.
// LTE is off for this build — no modem should be initialised or retried —
// but nothing about the modem code is removed. Flip this back to true and
// reflash any time a SIM/modem is fitted and connectivity is wanted again.
#define CFG_WIFI_ENABLED       true
#define CFG_LTE_ENABLED        false

// Site WiFi. Leave blank for a cellular-only machine. Ignored entirely when
// CFG_WIFI_ENABLED is false.
#define CFG_WIFI_SSID          ""
#define CFG_WIFI_PASSWORD      ""

// Cellular APN for the fitted SIM. Airtel India is "airtelgprs.com";
// Jio is "jionet", Vi is "portalnmms". A wrong APN still attaches but hands
// back a bearer with no working DNS, which looks like a dead connection.
// Ignored entirely when CFG_LTE_ENABLED is false.
#define CFG_LTE_APN            "airtelgprs.com"
#define CFG_LTE_APN_USER       ""
#define CFG_LTE_APN_PASS       ""

// ---------- Reporting ----------
// Sender account. The password is a Google App Password (16 characters, no
// spaces) — a normal account password is rejected by Gmail's SMTP.
#define CFG_GMAIL_USER         "rakesh7adm@gmail.com"
#define CFG_GMAIL_APP_PASSWORD "jfyroayuozubkiej"

// Where reports are delivered. Usually the customer, not you.
#define CFG_REPORT_TO          "rakhiirocky.cr7@gmail.com"

// Automatic daily summaries, 24-hour "HH:MM". Empty disables that slot.
// Two slots allow a mid-day and an end-of-day send.
#define CFG_NIGHTLY_TIME_1     ""
#define CFG_NIGHTLY_TIME_2     ""

// Email an alert when any motor falls to this level or below.
#define CFG_LOW_STOCK_ALERT    true
#define CFG_LOW_STOCK_LEVEL    3

// ---------- UPI / PhonePe merchant account ----------
// Per customer, not per machine — but store and terminal IDs usually differ
// per machine within one merchant account.
#define CFG_UPI_BASE_URL       "https://mercury-t2.phonepe.com"
#define CFG_UPI_PROVIDER_ID    "FUTURETECHNIKSOFFLINE"
#define CFG_UPI_MERCHANT_ID    "FUTURETECHNIKSINDIA"
#define CFG_UPI_SALT_KEY       "b2124854-391c-4d8b-862f-34ab0523df78"
#define CFG_UPI_SALT_INDEX     1
#define CFG_UPI_STORE_ID       "teststore1"
#define CFG_UPI_TERMINAL_ID    "testterminal1"

#endif  // CONFIG_H
