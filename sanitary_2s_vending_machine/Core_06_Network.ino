// ---------- WiFi credentials ----------
// wifiEnabled (Config.h's CFG_WIFI_ENABLED) lives in Core_02_AppState.ino —
// see the comment there for why.
char wifiSSID[33] = "";  // 802.11 SSID max is 32 chars + null
char wifiPass[64] = "";  // WPA2 PSK max is 63 chars + null

bool connectWiFiIfNeeded() {
  if (!wifiEnabled) return false;  // master switch — see Core_02_AppState.ino
  if (WiFi.status() == WL_CONNECTED) return true;
  if (strlen(wifiSSID) == 0) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID, wifiPass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(300);
  }
  return WiFi.status() == WL_CONNECTED;
}

// ---------- NTP time sync ----------
const char* NTP_SERVER_1 = "pool.ntp.org";
const char* NTP_SERVER_2 = "time.google.com";
const long  NTP_GMT_OFFSET_SEC      = 19800;  // IST = UTC+5:30 (5*3600 + 30*60)
const int   NTP_DAYLIGHT_OFFSET_SEC = 0;      // India has no DST

bool ntpConfigured = false;
bool timeSynced    = false;
// True only once WiFi NTP or LTE NITZ/NTP has actually set the clock — the
// onboard DS3231 fallback (Core_18_RTC.ino) sets timeSynced without setting
// this, so a WiFi/LTE sync can still arrive later and take over instead of
// being permanently blocked by "timeSynced is already true".
bool timeSyncedFromNetwork = false;
unsigned long lastNtpCheck = 0;
const unsigned long NTP_CHECK_INTERVAL_MS = 2000;

// WiFi Setup's on/off toggle (Screen_13_AdminWiFi.ino) calls this, not the
// variable directly — same "a plain assignment would forget something"
// reasoning as setLteEnabled() (Core_13_LTEModem.ino). Placed here, after
// ntpConfigured is declared above, rather than next to connectWiFiIfNeeded()
// where it'd more naturally sit — this function uses that variable, and
// Arduino only auto-forward-declares functions, never variables, even
// within the same tab.
//
//   * Turning OFF stops loop() from calling maintainNTP() on its very next
//     tick (that's just the flag), but the radio itself would otherwise stay
//     associated and the status icon would go on reporting a connection
//     nothing is maintaining any more. WiFi.disconnect(true) plus
//     WIFI_OFF drops the association and powers the radio down, same as
//     switching a modem off should stop actually transmitting.
//   * Turning ON attempts a connection immediately rather than waiting for
//     the next time something happens to call connectWiFiIfNeeded() — feels
//     instant, same as LTE zeroing its retry timer on its own switch-on.
//
// requestedOn is ANDed with CFG_WIFI_ENABLED so this can never turn WiFi on
// for a unit with no radio meant to be used at all — the hardware ceiling
// always wins, same as loadPersistedProductData() enforces it on every boot.
void setWifiEnabled(bool requestedOn) {
  wifiEnabled = requestedOn && CFG_WIFI_ENABLED;
  saveWifiEnabled();  // Core_09_Storage.ino

  if (!wifiEnabled) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    ntpConfigured = false;
  } else {
    connectWiFiIfNeeded();
    ntpConfigured = false;   // re-run SNTP against whatever it just connected to
  }
  indicatorDirty = true;  // Core_02_AppState.ino — redraw the status icon now
}

// Display preference only — doesn't affect how a date/time is typed on the
// Admin > Clock screen, which accepts either convention regardless. Default
// false (12-hour + AM/PM) matches this machine's historical display.
// Persisted/toggled from that same screen — see Screen_14_AdminTextEntry.ino.
bool clock24Hour = false;

void maintainNTP() {
  if (WiFi.status() != WL_CONNECTED) {
    ntpConfigured = false;   // re-arm SNTP on next reconnect
    return;                  // timeSynced stays true: the ESP32 RTC keeps ticking
  }

  if (!ntpConfigured) {
    configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2);
    ntpConfigured = true;
    lastNtpCheck = millis();
    return;
  }

  if (timeSyncedFromNetwork) return;
  if (millis() - lastNtpCheck < NTP_CHECK_INTERVAL_MS) return;
  lastNtpCheck = millis();

  struct tm ti;
  if (getLocalTime(&ti, 10) && ti.tm_year > (2020 - 1900)) {
    timeSynced = true;
    timeSyncedFromNetwork = true;
    Serial.println("NTP time synced");
  }
}

bool getClockStrings(char* timeBuf, size_t tSize, char* dateBuf, size_t dSize) {
  if (!timeSynced) return false;
  struct tm ti;
  if (!getLocalTime(&ti, 10)) return false;

  if (clock24Hour) {
    strftime(timeBuf, tSize, "%H:%M", &ti);                                // 21:41
  } else {
    strftime(timeBuf, tSize, "%I:%M %p", &ti);                             // 09:41 PM
    if (timeBuf[0] == '0') memmove(timeBuf, timeBuf + 1, strlen(timeBuf)); // drop leading zero
  }
  strftime(dateBuf, dSize, "%a %d %b %Y", &ti);                          // Fri 04 Sep 2026
  return true;
}

// Minute-resolution stamp, used to avoid redrawing the clock every second.
int currentMinuteStamp() {
  struct tm ti;
  if (!timeSynced || !getLocalTime(&ti, 5)) return -1;
  return ti.tm_hour * 60 + ti.tm_min;
}

// ---------- PhonePe PG "QR/Collect" v3 config ----------
// Fallback defaults:
const char* DEFAULT_PHONEPE_BASE_URL    = CFG_UPI_BASE_URL;
const char* DEFAULT_PHONEPE_PROVIDER_ID = CFG_UPI_PROVIDER_ID;
const char* DEFAULT_PHONEPE_MERCHANT_ID = CFG_UPI_MERCHANT_ID;
const char* DEFAULT_PHONEPE_SALT_KEY    = CFG_UPI_SALT_KEY;
const int   DEFAULT_PHONEPE_SALT_INDEX  = CFG_UPI_SALT_INDEX;
const char* DEFAULT_PHONEPE_STORE_ID    = CFG_UPI_STORE_ID;
const char* DEFAULT_PHONEPE_TERMINAL_ID = CFG_UPI_TERMINAL_ID;

// Active configurable variables (Persisted in NVS):
char phonepeBaseUrl[128]   = "";
char phonepeProviderId[64] = "";
char phonepeMerchantId[64] = "";
char phonepeSaltKey[64]    = "";
int  phonepeSaltIndex      = 1;
char phonepeStoreId[32]    = "";
char phonepeTerminalId[32] = "";

// Admin-configurable (Screen_12_AdminUPIConfig.ino), seed-then-owned same as
// maxCartQty (Core_09_Storage.ino) — CFG_UPI_TIMEOUT_MIN seeds it on a board
// that's never had it set, and the admin screen owns it from there. Kept in
// ms since that's what Screen_05_PaymentUPI.ino's countdown consumes; the QR
// itself is asked to stay valid for exactly this long too (initiateUPIPayment()
// sends it as "expiresIn"), so the two can never drift apart the way a fixed
// UPI_QR_EXPIRES_IN_SEC would once this became adjustable.
unsigned long upiTimeoutMs = (unsigned long)CFG_UPI_TIMEOUT_MIN * 60000UL;
const unsigned long UPI_TIMEOUT_MIN_MS = 60000;    // 1 minute floor
const unsigned long UPI_TIMEOUT_MAX_MS = 600000;   // 10 minute ceiling
const unsigned long UPI_POLL_INTERVAL_MS = 4000;
// A status check over the modem takes 10-25s by itself, so a 4-second gap on
// that path just queues requests back to back with no idle time for the UI.
const unsigned long UPI_POLL_INTERVAL_LTE_MS = 12000;
const unsigned long UPI_SUCCESS_HOLD_MS = 2000;

// ---------- WiFi connectivity self-test (used by the Admin WiFi screen) ----------
bool wifiPingAttempted = false;
bool wifiPingSuccess = false;
int wifiPingHttpCode = 0;
unsigned long wifiPingMs = 0;

bool pingGoogleTest() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiPingAttempted = true;
    wifiPingSuccess = false;
    wifiPingHttpCode = -1;
    wifiPingMs = 0;
    return false;
  }
  HTTPClient http;
  unsigned long t0 = millis();
  http.begin("http://www.google.com");
  int code = http.GET();
  wifiPingMs = millis() - t0;
  http.end();

  wifiPingAttempted = true;
  wifiPingHttpCode = code;
  wifiPingSuccess = (code > 0);
  return wifiPingSuccess;
}
