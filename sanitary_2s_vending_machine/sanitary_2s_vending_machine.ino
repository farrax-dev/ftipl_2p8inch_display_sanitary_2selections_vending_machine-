// Sanitary Napkin Vending Machine — ESP32 + ILI9341 touchscreen
//
// This sketch is split across multiple tab files that all live in this same
// folder. The Arduino builder concatenates every .ino file in the sketch
// folder into one translation unit (this file always first), so the split
// is purely organizational — it compiles exactly like one big file.
//
// Shared enums/structs/sizing constants (TextEntryTarget, KeyDef, KbLayer,
// AppScreen, Product, MAX_PRODUCTS/MAX_MOTORS/STOCK_MAX) live right in this file, below the
// includes, along with a few hand-written function prototypes — see the
// comment further down for why those can't just live in a Core_* tab.
//
// Load order that matters after that (variables/objects must be defined
// before use; function calls are fine in any order thanks to Arduino's
// auto-prototyping):
//   2. Core_02_AppState.ino       - currentScreen, adminEditingSlot
//   3. Core_03_Theme.ino          - color palette
//   4. Core_04_Hardware.ino       - pins, tft/ts objects, motor control
//   5. Core_05_TouchInput.ino     - touch calibration, debounce, isRealTouch
//   6. Core_06_Network.ino       - WiFi/NTP/UPI(PhonePe) config + connectivity
//   7. Core_07_WiFiIndicator.ino  - status-bar WiFi signal icon
//   8. Core_08_UIHelpers.ino      - shared buttons/text/layout drawing helpers
//   9. Core_09_Storage.ino        - NVS persistence + product/stock data
//  10. Core_10_Cart.ino           - shopping cart state
//  11. Core_11_Dispense.ino       - motor dispensing logic
//  12. Core_12_Main.ino           - setup() / loop()
//  13. Core_13_LTEModem.ino       - 4G modem, HTTPS over AT, cellular clock
//  14. Core_14_SalesLog.ino       - 60-day sales history + transaction CSV
//  15. Core_15_Report.ino         - report bodies and send orchestration
//  16. Core_16_ReportSchedule.ino - nightly emails, low-stock alerts, pruning
//  17. Core_17_Gmail.ino          - SMTP to Gmail over WiFi or the modem
//  18. Core_18_RTC.ino            - DS3231 battery-backed clock, third fallback
//                                   after WiFi NTP and LTE NITZ/NTP
//  19. Core_19_RFID.ino           - YHY5225 reader (UART1) + registered-card store
// Then one Screen_NN_*.ino file per screen in the UI flow (Welcome, Select,
// Cart Review, Payment Method/UPI/Other, and the Admin sub-screens).
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
// Serif, not sans — one family for every heading in the app so they read as
// one typographic voice. Three sizes, in the order you meet them: 12pt is
// the screen header (Core_08_UIHelpers.ino's drawScreenTitle()), 18pt is the
// Welcome screen's big title (24pt ran "ALL PURPOSE" past the card edge),
// and 9pt is only a fallback the header falls back to for a runtime-built
// title too wide for the space a screen has reserved — see drawHeaderTitle().
#include <Fonts/FreeSerifBold9pt7b.h>   // header fallback — Core_08_UIHelpers.ino
#include <Fonts/FreeSerifBold12pt7b.h>  // screen headers — Core_08_UIHelpers.ino's drawScreenTitle()
#include <Fonts/FreeSerifBold18pt7b.h>  // Welcome screen's big title — Screen_01_Welcome.ino
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>      // Library Manager: "ArduinoJson" by Benoit Blanchon
#include <base64.h>           // bundled with the ESP32 Arduino core
#include <time.h>             // bundled with the ESP32 Arduino core (NTP + strftime)
#include <LittleFS.h>         // bundled with the ESP32 Arduino core - sales/restock
                              // event log lives in the filesystem partition.
                              // Needs the "No OTA (2MB APP/2MB SPIFFS)" partition
                              // scheme — the 1.2MB default is ~14KB too small
                              // for this sketch.
                              // LittleFS (not SPIFFS) because it survives power
                              // loss mid-append, which a vending machine will do.
#include "mbedtls/sha256.h"   // bundled with the ESP32 Arduino core
#include "qrcode.h"           // ESP-IDF qrcode component bundled with the ESP32 core
#include "Config.h"           // per-machine settings — the only file to edit
                              // when commissioning a new unit

// ---------- Shared types (kept here, not in a tab, so they exist before ----------
// ---------- Arduino's auto-generated prototypes below reference them)   ----------
enum TextEntryTarget {
  TE_PRODUCT_NAME,
  TE_WIFI_SSID,
  TE_WIFI_PASS,
  TE_UPI_BASE_URL,
  TE_UPI_PROVIDER_ID,
  TE_UPI_MERCHANT_ID,
  TE_UPI_SALT_KEY,
  TE_UPI_STORE_ID,
  TE_UPI_TERMINAL_ID,
  TE_MACHINE_ID,
  TE_MACHINE_NAME,
  TE_GMAIL_USER,
  TE_GMAIL_PASS,
  TE_REPORT_TO,
  TE_NIGHTLY_1,
  TE_NIGHTLY_2,
  TE_RFID_CARD_MANUAL,
  TE_RFID_CARD_NAME,
  TE_SET_DATETIME,
  TE_ADMIN_PIN
};

// Which email Core_15_Report.ino is building. Has to be here rather than in
// that tab because sendReportEmail() takes one as a parameter, and Arduino's
// auto-generated prototypes are emitted above every tab file — a type used in
// a signature must already be declared by then.
enum ReportKind { REPORT_DAILY, REPORT_LOWSTOCK, REPORT_FULL };

struct KeyDef {
  const char* label;
  uint8_t span;
};

enum KbLayer { KB_LAYER_UPPER, KB_LAYER_LOWER, KB_LAYER_SYM };

enum AppScreen {
  SCREEN_WELCOME,
  SCREEN_SELECT,
  SCREEN_CART_REVIEW,
  SCREEN_PAYMENT_METHOD,
  SCREEN_PAYMENT_UPI,
  SCREEN_PAYMENT_CASH,
  SCREEN_ADMIN_LOGIN,
  SCREEN_ADMIN_PANEL,
  SCREEN_ADMIN_EDIT,
  SCREEN_ADMIN_TEXT_ENTRY,
  SCREEN_ADMIN_MOTOR_ASSIGN,
  SCREEN_ADMIN_MOTOR_STOCK,
  SCREEN_ADMIN_SETTINGS,
  SCREEN_ADMIN_WIFI,
  SCREEN_ADMIN_UPI,
  SCREEN_PAYMENT_RFID,
  SCREEN_ADMIN_COIN,
  SCREEN_ADMIN_LTE,
  SCREEN_ADMIN_REPORT,
  SCREEN_ADMIN_RFID_CARDS,
  SCREEN_ADMIN_RFID_CARD_EDIT
};

// Sizing comes from Config.h so a new machine needs no edits here. Both are
// compile-time constants because they size arrays, including the on-flash
// DailyRecord — changing either resets the stored sales history, which
// loadDailyHistory() detects and reports rather than misreading.
const int MAX_PRODUCTS = CFG_PRODUCT_COUNT;
const int MAX_MOTORS   = CFG_MOTOR_COUNT;
const int STOCK_MAX    = CFG_STOCK_MAX;

struct Product {
  const char* name;
  int price;
  uint16_t motorMask;  // one bit per motor (0..MAX_MOTORS-1); uint8_t only covers 8
};

// ---------- Explicit prototypes ----------
// Arduino's automatic prototype generator inserts its generated forward
// declarations very early in the concatenated sketch, and it can't reliably
// parse functions that take reference parameters (int&, uint8_t&, ...) or
// types it hasn't fully seen yet. Declaring these by hand here avoids
// "was not declared in this scope" / "declared void" build errors — Arduino
// skips auto-generating a prototype for any function that already has one.
void mapTouchToScreen(TS_Point raw, int &sx, int &sy);
void getSelectCardRect(int k, int count, int &x, int &y, int &w, int &h);
void color565toRGB(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b);
bool rtcReadTime(struct tm &out);
void rtcWriteTime(const struct tm &t);
bool parseDateTimeEntry(const char* buf, int &d, int &mo, int &y, int &h, int &mi);
String computePhonePeChecksum(const String &base64Body, const String &endpointPath);
void upiQRDisplay(esp_qrcode_handle_t qr);
void getMotorBtnRect(int m, int &x, int &y, int &w, int &h);
const KeyDef* kbRow(int row);
void getKeyRect(int row, int keyIdx, int &x, int &y, int &w, int &h);
void openTextEntry(TextEntryTarget target);
void drawKey(const KeyDef &key, int row, int keyIdx);
void IRAM_ATTR coinPulseISR();
bool httpsRequestLTE(const String &baseUrl, const String &path, const String &method,
                      const String &extraHeaders, const String &body,
                      int &outStatusCode, String &outBody);
bool lteSendAT(const String &cmd, String &outRaw, unsigned long timeoutMs);
bool lteHttpRead(int expectedLen, String &outBody, unsigned long timeoutMs);
bool sendPhonePeHTTPS(const String &method, const String &path, const String &checksum,
                      const String &body, int &outCode, String &outResponse);
// Report/email functions taking ReportKind — same reason as openTextEntry()
// above: the generated prototype lands before the enum is declared.
String reportSubject(ReportKind kind, uint32_t dateNum);
bool sendReportEmail(ReportKind kind, uint32_t dateNum, bool withAttachment);
int sendViaGmail(ReportKind kind, uint32_t dateNum, const String &html,
                 const String &csv, const String &csvName);
String readCSVTail(size_t maxBytes, bool &truncated);
String readCSVForDate(uint32_t date, size_t maxBytes, bool &truncated);
uint32_t csvRowDateNum(const String &line);
int gmailReadReply(Client &client, String &out, unsigned long timeoutMs);
bool gmailCmd(Client &client, const String &cmd, int expectCode,
              const char* what, bool secret);
void gmailWriteMessage(Client &client, const String &subject, const String &html,
                       const String &csv, const String &csvName);
int gmailSendVia(Client &client, const char* linkName, const String &subject,
                 const String &html, const String &csv, const String &csvName);
int gmailSend(const String &subject, const String &html, const String &csv,
              const String &csvName);
