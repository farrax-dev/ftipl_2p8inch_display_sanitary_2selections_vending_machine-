// =====================================================
//   4G/LTE MODEM (SIMCom A7600-series, UART/AT-command)
// =====================================================
// WiFi-fallback data path: when the primary WiFi network isn't connected,
// this brings up a cellular data session over the modem's UART so the machine
// can still reach the internet. Three things ride on it, all live:
//   - the UPI QR payment API, via httpsRequestLTE() below
//   - the clock, when NTP over WiFi isn't available
//   - the sales report emails, which layer TLS on the modem's raw TCP socket
//     (Core_17_Gmail.ino) since the modem has no SMTP engine of its own
//
// Requires the "TinyGSM" library (Library Manager: TinyGSM by Volodymyr
// Shymanskyy) — it speaks the SIMCom AT command set so we don't hand-roll
// AT command parsing here. TINY_GSM_MODEM_SIM7600 covers the SIM7600/A7600/
// A7670 family per TinyGSM's own modem list; check TinyGSM's docs if your
// exact chip print needs a more specific #define.
//
// Wiring: the module's own DC barrel-jack supplies its power — 4G modems
// draw multi-amp current spikes during transmission that the ESP32 can't
// supply, so do NOT power it from the ESP32's 3.3V/5V rail. Only GND and
// two UART lines are shared with the ESP32:
//   ESP32 GPIO17 / TX2 (LTE_TX_PIN) -> module RXD
//   ESP32 GPIO16 / RX2 (LTE_RX_PIN) <- module TXD
//   ESP32 GND                      <-> module GND
// These are the literal silkscreen-labeled TX2/RX2 pins, freed up from
// motors 9-10 (Core_04_Hardware.ino) specifically so LTE wouldn't need a
// boot-strapping pin — an earlier attempt using GPIO12 caused an actual
// upload failure (strapping pins are read at every reset, and something
// external held it in a state that confused the flash-voltage detection).
// Neither GPIO16 nor GPIO17 have that concern. If your specific module needs
// a PWRKEY pulse to power on (check its datasheet/silkscreen), there's no
// spare GPIO left on this build to drive it from the ESP32 — tie PWRKEY to
// GND on the module itself so it auto-boots when DC is applied, which is
// how most of these breakout boards are designed to work already.
#define TINY_GSM_MODEM_SIM7600
// A 1KB receive buffer rather than the 256-byte default: TLS records arrive in
// large chunks, and a small buffer drops bytes mid-handshake.
#define TINY_GSM_RX_BUFFER 1024
#include <TinyGsmClient.h>
// Software TLS over the modem's plain TCP socket. This is what makes Gmail
// reachable on cellular: the modem's own firmware has no SMTP engine and its
// AT+HTTP engine only speaks HTTP, but TinyGsmClient gives a raw socket and
// ESP_SSLClient wraps it in TLS on the ESP32 side — so the ESP32 can speak
// SMTP itself. Library Manager: "ESP_SSLClient" by mobizt.
#include <ESP_SSLClient.h>
// ESP32 core 3.x narrowed HTTPClient::begin() to only accept NetworkClient&
// (what WiFiClientSecure was updated to inherit from). TinyGsmClient still
// inherits from the plain, portable Arduino Client class, so it doesn't fit
// that overload — hence a second, separate HTTP client library here just
// for the cellular path. Library Manager: "ArduinoHttpClient" by Arduino.
#include <ArduinoHttpClient.h>

// lteEnabled lives in Core_02_AppState.ino — see the comment there for the
// split between it and CFG_LTE_ENABLED. setLteEnabled() below, near the end
// of this file, is the only place that should ever change it after boot.
const int LTE_TX_PIN = 17;  // ESP32 TX2 -> module RXD
const int LTE_RX_PIN = 16;  // ESP32 RX2 <- module TXD
const unsigned long LTE_UART_BAUD = 115200;

// Airtel India's documented APN. The generic "internet" that was here before
// still *attaches* on Airtel, but onto a restricted bearer that hands back no
// usable DNS — which is what produced "+HTTPACTION: 1,714" (DNS resolve
// failed) and "+CNTP: 6" (timeout) while gprsConnect() reported success.
// Change this if you move to a different carrier's SIM.
const char* LTE_APN      = CFG_LTE_APN;
const char* LTE_APN_USER = CFG_LTE_APN_USER;
const char* LTE_APN_PASS = CFG_LTE_APN_PASS;

TinyGsm lteModem(Serial2);
TinyGsmClient lteClient(lteModem);

bool lteModemStarted = false;
bool lteRegistered = false;

// Cached modem status, declared here because connectLTEIfNeeded() further
// down primes it. Arduino auto-declares functions across tabs but never
// variables, so these have to precede their first use in the file.
bool lteHaveIP = false;
int  lteLastCsq = 0;
unsigned long lteLastStatusPollMs = 0;

// Long on purpose: signal strength and "is the bearer up" change on the scale
// of tens of seconds, not frames.
const unsigned long LTE_STATUS_POLL_MS = 20000;

unsigned long lteLastAttemptMs = 0;
const unsigned long LTE_RETRY_INTERVAL_MS = 30000;  // don't hammer a dead SIM/no-signal modem

bool ltePingAttempted = false;
bool ltePingSuccess = false;
int  ltePingHttpCode = 0;
unsigned long ltePingMs = 0;

void initLTEModem() {
  Serial2.begin(LTE_UART_BAUD, SERIAL_8N1, LTE_RX_PIN, LTE_TX_PIN);
}

// ---------- TEMPORARY wiring diagnostic ----------
// Set to true, upload, then open the Serial Monitor at 115200 baud with line
// ending set to "Both NL & CR". Type an AT command (e.g. just "AT") and
// press Enter — whatever the modem sends back (or doesn't) is echoed
// straight through. This bypasses the whole app (touchscreen/motors/etc.
// never start) so it's purely about the raw wiring link. Set back to false
// and re-upload once you've confirmed the module responds.
const bool LTE_AT_BRIDGE_TEST = false;

void runLTEATBridgeIfEnabled() {
  if (!LTE_AT_BRIDGE_TEST) return;

  Serial.println();
  Serial.println("=== LTE AT bridge test: type AT commands, Enter to send ===");
  while (true) {
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();
      Serial.print("> ");
      Serial.println(cmd);
      Serial2.print(cmd);
      Serial2.print("\r\n");
    }
    if (Serial2.available()) {
      while (Serial2.available()) Serial.write(Serial2.read());
    }
  }
}

// Set at whichever stage connectLTEIfNeeded() last failed at, so the admin
// screen can show *why* it's not connected instead of just that it isn't.
char lteLastError[40] = "";

// Forward decls — the time code below sends raw AT commands, whose helper is
// defined further down with the rest of the HTTPS engine.
// NOTE: do not add a forward declaration for a function that other .ino
// files call (isLTEConnected(), etc). arduino-builder suppresses its own
// auto-generated prototype once it sees an explicit declaration, and this
// file is concatenated after Core_07_WiFiIndicator.ino — so declaring it
// here makes it *invisible* there. lteSendAT() below is safe to declare
// only because nothing outside this file uses it.
bool lteSendAT(const String &cmd, String &outRaw, unsigned long timeoutMs);

// Squashes a multi-line AT response onto one log line so the Serial Monitor
// stays readable when dumping diagnostics.
String lteFlatten(const String &raw) {
  String s = raw;
  s.replace("\r\n", " | ");
  s.replace("\r", "");
  s.replace("\n", " | ");
  s.trim();
  return s;
}

// Which PDP context actually carries traffic. Every data-plane AT command
// here (HTTP, NTP) targets a context by id, and they all default to 1 — but
// on LTE the network creates the default EPS bearer at attach and does NOT
// have to put it on cid 1. On this Airtel SIM it lands on cid 2, leaving
// cid 1 defined-but-inactive with address 0.0.0.0. Pointing the HTTP engine
// at cid 1 is what produced "+HTTPACTION: 1,714" and NTP's "network error".
// Detected at connect time rather than hard-coded, since the carrier picks it.
int lteActiveCID = 1;

// Parses the address list from a bare "AT+CGPADDR" (all contexts):
//   +CGPADDR: 1,0.0.0.0
//   +CGPADDR: 2,100.119.66.15
// and latches the first context holding a real address. Leaves lteActiveCID
// alone if nothing qualifies, so a transient read can't make things worse.
bool lteDetectActiveCID() {
  String raw;
  if (!lteSendAT("AT+CGPADDR", raw, 5000)) {
    Serial.println("LTE: AT+CGPADDR failed, keeping previous CID");
    return false;
  }

  int searchFrom = 0;
  while (true) {
    int idx = raw.indexOf("+CGPADDR:", searchFrom);
    if (idx < 0) break;
    int lineEnd = raw.indexOf('\n', idx);
    if (lineEnd < 0) lineEnd = raw.length();
    String line = raw.substring(idx + 9, lineEnd);
    line.trim();
    searchFrom = lineEnd + 1;

    int comma = line.indexOf(',');
    if (comma < 0) continue;                       // "+CGPADDR: 1" with no address
    int cid = line.substring(0, comma).toInt();
    String addr = line.substring(comma + 1);
    addr.replace("\"", "");
    addr.trim();

    if (addr.length() == 0 || addr == "0.0.0.0" || addr.startsWith("0.0.0.0")) continue;

    bool changed = (lteActiveCID != cid);
    lteActiveCID = cid;
    // Only log on a change — this runs from isLTEConnected() every 5s.
    if (changed) Serial.printf("LTE: active PDP context is cid %d (%s)\n", cid, addr.c_str());

    return true;
  }

  Serial.println("LTE: no PDP context has an IP address yet");
  return false;
}

// Activates PDP context 1 explicitly.
//
// The modem's internet-service stack (HTTP, NTP, DNS) uses context 1 and
// will not use a context it didn't bring up itself — so binding HTTP to the
// network's default bearer on cid 2 doesn't help. On this network cid 1 is
// left defined-but-inactive with address 0.0.0.0 after TinyGSM's
// gprsConnect(), and *that* is why every outbound attempt failed: DNS
// (+CDNSGIP: 0,10), HTTP (713), and even NTP against a literal IP
// (network error), which is what proved it wasn't a name-resolution problem.
//
// AT+CGACT can take a while on a cold bearer, hence the long timeout.
bool lteActivateContext1() {
  String raw;
  // Re-assert the APN on context 1 first; harmless if it's already set, and
  // it guarantees activation uses the right one.
  lteSendAT("AT+CGDCONT=1,\"IP\",\"" + String(LTE_APN) + "\"", raw, 5000);

  bool ok = lteSendAT("AT+CGACT=1,1", raw, 60000);
  Serial.printf("LTE: activating PDP cid 1 -> %s [%s]\n",
                ok ? "OK" : "ERROR", lteFlatten(raw).c_str());
  return ok;
}

// Resolves a hostname through the modem to prove DNS actually works,apart
// from any HTTP/TLS concern. "AT+CDNSGIP=<host>" answers OK immediately and
// then reports "+CDNSGIP: 1,"<host>","<ip>"" on success, or "+CDNSGIP: 0,<err>"
// on failure.
bool lteResolveHost(const String &host) {
  while (Serial2.available()) Serial2.read();
  Serial2.print("AT+CDNSGIP=\"" + host + "\"\r\n");

  String resp = "";
  unsigned long t0 = millis();
  while (millis() - t0 < 15000) {
    while (Serial2.available()) resp += (char)Serial2.read();
    int idx = resp.indexOf("+CDNSGIP:");
    if (idx >= 0) {
      int lineEnd = resp.indexOf('\n', idx);
      if (lineEnd < 0) continue;
      String line = resp.substring(idx, lineEnd);
      Serial.printf("LTE: DNS lookup of %s -> [%s]\n", host.c_str(), lteFlatten(line).c_str());
      return line.indexOf("+CDNSGIP: 1") >= 0;
    }
  }
  Serial.printf("LTE: DNS lookup of %s timed out [%s]\n", host.c_str(), lteFlatten(resp).c_str());
  return false;
}

// One-shot dump of everything that determines whether the data session can
// actually carry traffic. Run right after the APN connects, so a failing
// build says *where* it broke rather than just that it broke.
void lteLogDataDiagnostics() {
  String raw;
  lteSendAT("AT+COPS?", raw, 5000);
  Serial.printf("LTE diag: operator      %s\n", lteFlatten(raw).c_str());
  lteSendAT("AT+CGREG?", raw, 3000);
  Serial.printf("LTE diag: registration  %s\n", lteFlatten(raw).c_str());
  lteSendAT("AT+CGDCONT?", raw, 3000);
  Serial.printf("LTE diag: PDP contexts  %s\n", lteFlatten(raw).c_str());
  lteSendAT("AT+CGACT?", raw, 3000);
  Serial.printf("LTE diag: PDP active    %s\n", lteFlatten(raw).c_str());
  // Bare CGPADDR lists every context, so a context that got an address can
  // be told apart from one that only got defined.
  lteSendAT("AT+CGPADDR", raw, 3000);
  Serial.printf("LTE diag: IP addresses  %s\n", lteFlatten(raw).c_str());
  Serial.printf("LTE diag: using cid     %d\n", lteActiveCID);
}

// Reads the network-broadcast date/time (NITZ, via AT+CCLK?) and sets the
// ESP32's system clock from it — the same clock getClockStrings()/
// currentMinuteStamp() (Core_06_Network.ino) already read, so the rest of
// the app doesn't need to know or care which path set it. Sets timeSynced,
// same flag maintainNTP() sets on the WiFi path, so this only overrides it
// once per boot (or again if it somehow got desynced).
// Days since 1970-01-01 for a civil (proleptic Gregorian) date. Howard
// Hinnant's days_from_civil. Used instead of mktime() because mktime()
// interprets its input in the *local* TZ, and what the modem gives us is a
// timestamp in whatever zone the network chose — which is the whole bug this
// replaces. Converting by hand keeps the zone handling explicit.
static long lteDaysFromCivil(int y, unsigned m, unsigned d) {
  y -= (m <= 2);
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);              // [0, 399]
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;  // [0, 365]
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;  // [0, 146096]
  return (long)era * 146097L + (long)doe - 719468L;
}

// Reads AT+CCLK? directly rather than going through TinyGSM, because we need
// the trailing timezone field ("+22" = quarter-hours east of UTC) that
// TinyGSM's SIM7600 parser hands back as a lossy float. Format is:
//   +CCLK: "26/09/05,14:30:00+22"
// tzQuarters is left at 0 if the modem omitted the field.
bool lteReadCCLK(int &year, int &mon, int &day, int &hour, int &mins, int &sec, int &tzQuarters) {
  String raw;
  if (!lteSendAT("AT+CCLK?", raw, 3000)) return false;

  int idx = raw.indexOf("+CCLK:");
  if (idx < 0) return false;
  int q1 = raw.indexOf('"', idx);
  int q2 = raw.indexOf('"', q1 + 1);
  if (q1 < 0 || q2 < 0) return false;
  String ts = raw.substring(q1 + 1, q2);   // 26/09/05,14:30:00+22

  int yy = ts.substring(0, 2).toInt();
  mon  = ts.substring(3, 5).toInt();
  day  = ts.substring(6, 8).toInt();
  hour = ts.substring(9, 11).toInt();
  mins = ts.substring(12, 14).toInt();
  sec  = ts.substring(15, 17).toInt();
  year = 2000 + yy;

  tzQuarters = 0;
  if (ts.length() > 17) {
    String tzs = ts.substring(17);
    tzs.trim();
    if (tzs.length() > 0) tzQuarters = tzs.toInt();  // toInt() honours a leading +/-
  }
  return true;
}

// Asks the modem to sync its own RTC from a public NTP server over the
// cellular data session. This is the reliable path on Airtel: NITZ (the
// network broadcasting the time itself) is patchy on Indian networks and is
// what the original AT+CCLK?-only approach was silently depending on. Needs
// an active PDP context, so only call this after gprsConnect() succeeded.
//   AT+CNTP="<server>",<tz in quarter-hours>  configures it
//   AT+CNTP                                    performs the sync
//   +CNTP: 0                                   unsolicited success result
bool lteSyncModemClockViaServer(const char* server) {
  String raw;
  // 22 quarter-hours = UTC+5:30, so the modem stores IST directly in its RTC
  // and reports "+22" back on AT+CCLK?.
  // This firmware rejects the documented AT+CNTP=<server>,<tz>,<cid> form
  // outright, so configure with the two-argument form and rely on
  // AT+CSOCKSETPN (set in lteDetectActiveCID) to have already moved the
  // internet-service stack onto the context that has an address.
  if (!lteSendAT("AT+CNTP=\"" + String(server) + "\",22", raw, 5000)) {
    Serial.println("LTE: AT+CNTP config rejected");
    return false;
  }

  while (Serial2.available()) Serial2.read();
  Serial2.print("AT+CNTP\r\n");

  // The "OK" is immediate; the actual result comes later as "+CNTP: <code>".
  // 0 means the sync completed; 6 is a timeout, which in practice means the
  // server name never resolved.
  String resp = "";
  unsigned long t0 = millis();
  while (millis() - t0 < 20000) {
    while (Serial2.available()) resp += (char)Serial2.read();
    int idx = resp.indexOf("+CNTP:");
    if (idx >= 0) {
      int lineEnd = resp.indexOf('\n', idx);
      if (lineEnd < 0) continue;
      int code = resp.substring(idx + 6, lineEnd).toInt();
      Serial.printf("LTE: AT+CNTP via %s -> result code %d\n", server, code);
      return code == 0;
    }
  }
  Serial.printf("LTE: AT+CNTP via %s timed out, raw buffer was [%s]\n", server, resp.c_str());
  return false;
}

// Applies a UTC epoch to the ESP32 system clock and marks time as synced.
// TZ is what renders it back as IST for getClockStrings().
void lteApplyUTC(long long utc) {
  setenv("TZ", "IST-5:30", 1);
  tzset();
  struct timeval tv = { (time_t)utc, 0 };
  settimeofday(&tv, nullptr);
  timeSynced = true;
  timeSyncedFromNetwork = true;
}

// Last-resort clock source: every HTTP response carries a "Date:" header in
// GMT, so one plain-http GET yields the time to the second without needing
// the modem's NTP client to work at all. Uses the same AT+HTTP engine the
// payments go through, so if this succeeds the payment path is healthy too —
// and if it fails, the failure is shared and worth seeing.
//   AT+HTTPHEAD returns the response headers of the last AT+HTTPACTION.
bool lteSyncTimeFromHTTPDate() {
  String raw;
  lteSendAT("AT+HTTPTERM", raw, 3000);
  if (!lteSendAT("AT+HTTPINIT", raw, 5000)) return false;

  lteSendAT("AT+HTTPPARA=\"CID\"," + String(lteActiveCID), raw, 3000);
  lteSendAT("AT+HTTPPARA=\"URL\",\"http://www.google.com\"", raw, 5000);

  while (Serial2.available()) Serial2.read();
  Serial2.print("AT+HTTPACTION=0\r\n");
  String resp = "";
  unsigned long t0 = millis();
  bool got = false;
  while (millis() - t0 < 30000) {
    while (Serial2.available()) resp += (char)Serial2.read();
    if (resp.indexOf("+HTTPACTION:") >= 0 && resp.indexOf('\n', resp.indexOf("+HTTPACTION:")) >= 0) {
      got = true;
      break;
    }
  }
  if (!got) {
    Serial.println("LTE: HTTP date probe got no +HTTPACTION");
    lteSendAT("AT+HTTPTERM", raw, 3000);
    return false;
  }
  Serial.printf("LTE: HTTP date probe -> %s\n", lteFlatten(resp).c_str());

  bool haveHead = lteSendAT("AT+HTTPHEAD", raw, 10000);
  lteSendAT("AT+HTTPTERM", raw, 3000);
  if (!haveHead) return false;

  // Date: Fri, 05 Sep 2025 08:30:00 GMT
  int idx = raw.indexOf("Date:");
  if (idx < 0) {
    Serial.println("LTE: no Date header in response");
    return false;
  }
  int lineEnd = raw.indexOf('\r', idx);
  if (lineEnd < 0) lineEnd = raw.length();
  String d = raw.substring(idx + 5, lineEnd);
  d.trim();

  int comma = d.indexOf(',');
  if (comma >= 0) d = d.substring(comma + 1);
  d.trim();

  int day  = d.substring(0, 2).toInt();
  String monName = d.substring(3, 6);
  int year = d.substring(7, 11).toInt();
  int hour = d.substring(12, 14).toInt();
  int mins = d.substring(15, 17).toInt();
  int sec  = d.substring(18, 20).toInt();

  const char* months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                             "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  int mon = 0;
  for (int i = 0; i < 12; i++) if (monName == months[i]) { mon = i + 1; break; }
  if (mon == 0 || year < 2020 || year > 2035) {
    Serial.printf("LTE: couldn't parse Date header [%s]\n", d.c_str());
    return false;
  }

  long days = lteDaysFromCivil(year, (unsigned)mon, (unsigned)day);
  long long utc = (long long)days * 86400LL + hour * 3600LL + mins * 60LL + sec;
  lteApplyUTC(utc);
  Serial.printf("LTE: time synced from HTTP Date header (%04d-%02d-%02d %02d:%02d:%02d UTC)\n",
                year, mon, day, hour, mins, sec);
  return true;
}

bool lteSyncModemClockViaNTP() {
  if (lteSyncModemClockViaServer("pool.ntp.org")) return true;
  // A hostname failure is a DNS failure, not an NTP one — retry against
  // Google's anycast NTP address directly so the clock can still be set on a
  // session whose resolver is broken. Confirms the diagnosis at the same
  // time: if this one works, DNS is the only thing wrong.
  Serial.println("LTE: NTP by hostname failed, retrying against a literal IP...");
  return lteSyncModemClockViaServer("216.239.35.0");
}

// Reads the modem's clock and sets the ESP32's system clock from it — the
// same clock getClockStrings()/currentMinuteStamp() (Core_06_Network.ino)
// already read, so the rest of the app doesn't need to know or care which
// path set it. Sets timeSynced, the same flag maintainNTP() sets on the WiFi
// path, so this only overrides it once per boot (or again if it desynced).
//
// Tries the RTC as-is first (populated by NITZ when the network does provide
// it, which is instant and free), and falls back to driving the modem's own
// NTP client when that comes back as a placeholder.
bool syncTimeFromLTE() {
  int year, mon, day, hour, mins, sec, tzQuarters;

  bool haveTime = lteReadCCLK(year, mon, day, hour, mins, sec, tzQuarters);
  if (haveTime) {
    Serial.printf("LTE: modem RTC reads %04d-%02d-%02d %02d:%02d:%02d, tz field %+d quarter-hours\n",
                  year, mon, day, hour, mins, sec, tzQuarters);
  }

  // Some modems report a placeholder/sentinel date (1980, 2004, 2070...)
  // until something has actually set the RTC. On Airtel that "something"
  // usually never arrives on its own, so ask the modem to go fetch NTP.
  if (!haveTime || year < 2020 || year > 2035) {
    Serial.println("LTE: modem RTC not set by the network, trying NTP over cellular...");
    if (!lteSyncModemClockViaNTP()) {
      // NTP is blocked or the modem's client is unhappy — fall back to the
      // Date header of an ordinary HTTP response, which needs nothing but
      // the data path the payments already use.
      Serial.println("LTE: NTP unavailable, falling back to HTTP Date header...");
      return lteSyncTimeFromHTTPDate();
    }
    if (!lteReadCCLK(year, mon, day, hour, mins, sec, tzQuarters)) return false;
    Serial.printf("LTE: after NTP, modem RTC reads %04d-%02d-%02d %02d:%02d:%02d, tz field %+d\n",
                  year, mon, day, hour, mins, sec, tzQuarters);
    if (year < 2020 || year > 2035) {
      Serial.println("LTE: still a placeholder date after NTP, giving up for now");
      return false;
    }
  }

  // The timestamp is wall-clock time in the zone the tz field names — NOT
  // necessarily IST. Subtracting the offset gives real UTC, which is what the
  // system clock stores; the TZ env var below is what renders it back as IST.
  // Assuming IST here (the previous behaviour) put the clock 5:30 out
  // whenever the network reported UTC with a "+22" tz field instead of
  // pre-applying the offset, which is the default when AT+CTZU is off.
  long days = lteDaysFromCivil(year, (unsigned)mon, (unsigned)day);
  long long utc = (long long)days * 86400LL + hour * 3600LL + mins * 60LL + sec;
  utc -= (long long)tzQuarters * 900LL;  // 1 quarter-hour = 900 s

  lteApplyUTC(utc);
  Serial.println("LTE: time synced from cellular network");
  return true;
}

// Brings the modem up and registers on the cellular network. Blocking (can
// take several to ~15+ seconds on first registration), so only call this
// when actually needed, not on every loop() tick — see maintainLTEFallback().
bool connectLTEIfNeeded() {
  // False here means either no modem is fitted (CFG_LTE_ENABLED false, in
  // which case Serial2 was never even begun) or one is fitted but the admin
  // has switched it off from Admin > 4G/LTE Setup — see Core_02_AppState.ino.
  // Either way nothing below may run: this is also what makes the manual
  // "Test Internet" button (Screen_16_AdminLTE.ino) refuse instantly rather
  // than spending several seconds finding out the modem is deliberately off.
  if (!lteEnabled) return false;
  // Use isLTEConnected(), not lteModem.isGprsConnected() directly: the latter
  // reports on the socket service, which reads false on this setup, so the
  // whole init/register/APN/activate sequence was re-running on every call.
  if (isLTEConnected()) return true;

  if (!lteModemStarted) {
    Serial.println("LTE: initializing modem...");
    if (!lteModem.init()) {
      Serial.println("LTE: modem init failed (no AT response - check wiring/baud)");
      strncpy(lteLastError, "Modem not responding (check wiring)", sizeof(lteLastError) - 1);
      return false;
    }
    lteModemStarted = true;

    // Ask the modem to accept the network's time/timezone broadcast (NITZ)
    // and apply it to its own RTC. Off by default on the A7670C, which is
    // why AT+CCLK? was only ever returning a placeholder date. CTZR=1 also
    // enables the unsolicited +CTZV report so a late-arriving NITZ (it can
    // trail registration by several seconds) still lands in the RTC for
    // syncTimeFromLTE()'s next retry to pick up.
    String raw;
    lteSendAT("AT+CTZU=1", raw, 3000);
    lteSendAT("AT+CTZR=1", raw, 3000);
  }

  Serial.println("LTE: waiting for network...");
  if (!lteModem.waitForNetwork(15000)) {
    Serial.println("LTE: network registration failed (no signal/SIM issue?)");
    strncpy(lteLastError, "No network (check SIM/antenna/signal)", sizeof(lteLastError) - 1);
    return false;
  }

  Serial.println("LTE: connecting to APN...");
  if (!lteModem.gprsConnect(LTE_APN, LTE_APN_USER, LTE_APN_PASS)) {
    Serial.println("LTE: APN connect failed (wrong APN for this SIM?)");
    strncpy(lteLastError, "APN rejected (check LTE_APN)", sizeof(lteLastError) - 1);
    return false;
  }

  lteRegistered = true;
  lteLastError[0] = '\0';
  Serial.println("LTE: connected");

  // Must come before lteDetectActiveCID(), which picks the lowest context
  // holding a real address — so once cid 1 is up it wins, and the HTTP/NTP
  // engines get the context they're actually able to use.
  lteActivateContext1();
  lteHaveIP = lteDetectActiveCID();
  lteLastCsq = lteModem.getSignalQuality();
  lteLastStatusPollMs = millis();
  lteLogDataDiagnostics();

  // Do this here rather than leaving it to maintainLTEFallback()'s ~10s
  // retry: lteSyncModemClockViaNTP() needs the PDP context that
  // gprsConnect() just brought up, and the DNS config just applied.
  if (!timeSyncedFromNetwork) syncTimeFromLTE();
  return true;
}

// TinyGSM's SIM7600 isGprsConnected() reports on the socket service
// (AT+NETOPEN?), which is a different thing from "the PDP bearer is up and
// has an address" — and it reads false here, which is why the log shows
// connectLTEIfNeeded() re-running the whole APN sequence over and over. The
// native AT+HTTP engine doesn't use the socket service at all, so having an
// addressed context is the condition that actually matters for payments.
// Re-checked at most every few seconds, since this is called from loop().
// Cached modem status. NOTHING in this file may query the modem from a
// function the UI calls every frame — isLTEConnected() and lteSignalBars()
// are read from loop() on every screen, and each AT round trip stalls the
// whole app.
//
// The version this replaces called lteModem.isGprsConnected() on every check.
// That sends AT+NETOPEN? and waits for "+NETOPEN: 1" using TinyGSM's default
// one-second timeout — and on this modem the socket service is never what
// carries traffic, so the match never came and it burned the FULL second,
// every 1.5 seconds, on every screen. Roughly two thirds of all wall-clock
// time went into waiting for an answer that does not exist, which is exactly
// why the machine felt fine on WiFi and unusable on 4G.
// Free: reads cached state, never touches the UART.
bool isLTEConnected() {
  return lteRegistered && lteModemStarted && lteHaveIP;
}

// Also free. maintainLTEStatus() below is the only thing that talks to the
// modem to refresh it.
int lteSignalBars() {
  int csq = lteLastCsq;
  if (csq == 99 || csq <= 0) return 0;
  if (csq >= 20) return 4;
  if (csq >= 15) return 3;
  if (csq >= 10) return 2;
  if (csq >= 2)  return 1;
  return 0;
}

// The one place the modem is polled for status, on a slow cadence, called
// from loop(). Two AT round trips every 20 seconds instead of three every
// 1.5 seconds.
void maintainLTEStatus() {
  if (!lteModemStarted) return;
  if (millis() - lteLastStatusPollMs < LTE_STATUS_POLL_MS) return;
  lteLastStatusPollMs = millis();

  lteHaveIP = lteDetectActiveCID();
  if (lteHaveIP) lteLastCsq = lteModem.getSignalQuality();
  else           lteLastCsq = 0;
}

// ---------- HTTPS over the modem: native AT+HTTP engine ----------
// TinyGsmClientSecure does not exist for TINY_GSM_MODEM_SIM7600 in the
// installed TinyGSM version (confirmed at compile time), so there's no
// TLS-capable Client to hand-roll HTTP over the way this was first
// attempted. Instead this drives the modem's own built-in HTTPS engine
// directly via AT commands — the modem's firmware does the actual TLS
// handshake, so no Client/socket abstraction is needed at all. Verified by
// hand against this exact module (SIMCom A7670C, firmware A7670M6_V1.11.1)
// before writing this:
//   - AT+HTTPPARA="USERDATA","<header>: <value>" only accepts one header
//     per call, but calling it once per header ACCUMULATES them rather than
//     overwriting — confirmed by round-tripping to a header-echoing test
//     server and seeing both custom headers arrive together.
//   - AT+HTTPREAD acknowledges with a bare "OK" FIRST, and only then sends
//     "+HTTPREAD: <len>\r\n<data>\r\n+HTTPREAD: 0". An earlier note here had
//     the OK last, which is why lteHttpRead() originally returned on it and
//     produced an empty body from a perfectly good 200 response.
// A POST with a body (AT+HTTPDATA) against the real PhonePe endpoint is now
// confirmed working over LTE. Only handles response bodies that fit in one
// AT+HTTPREAD (no chunked/paginated reads), which matches PhonePe's small
// JSON replies.

// Sends a raw AT command and waits up to timeoutMs for it to finish,
// collecting everything received. Returns true on "OK", false on "ERROR" or
// timeout. Talks to Serial2 directly (not through TinyGSM) since TinyGSM
// doesn't wrap this modem's HTTP(S) AT command set at all.
bool lteSendAT(const String &cmd, String &outRaw, unsigned long timeoutMs) {
  while (Serial2.available()) Serial2.read();
  Serial2.print(cmd);
  Serial2.print("\r\n");

  outRaw = "";
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    while (Serial2.available()) {
      outRaw += (char)Serial2.read();
      if (outRaw.endsWith("OK\r\n")) return true;
      if (outRaw.endsWith("ERROR\r\n")) return false;
      // A rejected command usually comes back as "+CME ERROR: <n>", which
      // doesn't end in a bare "ERROR" — without this it burned the entire
      // timeout on every failure instead of returning immediately, making a
      // misconfigured modem look like a dead one.
      if (outRaw.endsWith("\r\n") &&
          (outRaw.indexOf("+CME ERROR:") >= 0 || outRaw.indexOf("+CMS ERROR:") >= 0)) {
        return false;
      }
    }
  }
  return false;
}

// Issues AT+HTTPREAD for a response of known length and collects the body.
//
// This cannot go through lteSendAT(): that returns as soon as it sees
// "OK\r\n", and this firmware acknowledges AT+HTTPREAD with a bare "OK"
// *before* sending anything. The real sequence is:
//   OK
//   +HTTPREAD: <len>
//   <len bytes of body>
//   +HTTPREAD: 0
// so returning on the "OK" handed back an empty string every time — a 200
// response with 342 bytes waiting still parsed as EmptyInput.
//
// Waits for the "+HTTPREAD: <len>" announcement (len > 0; a zero is the
// terminator, not the data), then keeps reading until <len> bytes have
// actually arrived rather than assuming they're all in the buffer at once.
bool lteHttpRead(int expectedLen, String &outBody, unsigned long timeoutMs) {
  outBody = "";
  while (Serial2.available()) Serial2.read();
  Serial2.print("AT+HTTPREAD=0," + String(expectedLen) + "\r\n");

  String raw = "";
  int bodyStart = -1;
  int announced = 0;
  unsigned long t0 = millis();

  while (millis() - t0 < timeoutMs) {
    while (Serial2.available()) raw += (char)Serial2.read();

    if (bodyStart < 0) {
      int searchFrom = 0;
      while (true) {
        int idx = raw.indexOf("+HTTPREAD:", searchFrom);
        if (idx < 0) break;
        int lineEnd = raw.indexOf('\n', idx);
        if (lineEnd < 0) break;                       // announcement still arriving
        String lenStr = raw.substring(idx + 10, lineEnd);
        lenStr.trim();
        int n = lenStr.toInt();
        if (n > 0) { announced = n; bodyStart = lineEnd + 1; break; }
        searchFrom = lineEnd + 1;                     // "+HTTPREAD: 0" terminator
      }
    }

    // Take exactly the announced byte count — the body is JSON and must not
    // be trimmed or truncated at a marker that could legitimately appear
    // inside it.
    if (bodyStart >= 0 && (int)raw.length() - bodyStart >= announced) {
      outBody = raw.substring(bodyStart, bodyStart + announced);
      return true;
    }
  }

  int got = (bodyStart >= 0) ? (int)raw.length() - bodyStart : 0;
  Serial.printf("LTE HTTPS: HTTPREAD incomplete, got %d of %d bytes [%s]\n",
                got, announced, lteFlatten(raw).c_str());
  if (bodyStart >= 0 && got > 0) {
    outBody = raw.substring(bodyStart);
    outBody.trim();
    return outBody.length() > 0;
  }
  return false;
}

// method: "GET" or "POST". extraHeaders: "Name: value\r\n"-per-line block
// (may be empty) — split and sent one AT+HTTPPARA="USERDATA",... call per
// line, since the modem only accepts one header per call.
bool httpsRequestLTE(const String &baseUrl, const String &path, const String &method,
                      const String &extraHeaders, const String &body,
                      int &outStatusCode, String &outBody) {
  outStatusCode = -1;
  outBody = "";
  String raw;

  lteSendAT("AT+HTTPTERM", raw, 3000);  // clear any stale session first, ignore result
  if (!lteSendAT("AT+HTTPINIT", raw, 5000)) {
    Serial.println("LTE HTTPS: HTTPINIT failed");
    return false;
  }

  // Bind the HTTP engine to whichever PDP context actually has an IP. The
  // engine defaults to cid 1, which on this network is the dead one — that
  // default is the whole reason requests came back "+HTTPACTION: 1,714".
  lteSendAT("AT+HTTPPARA=\"CID\"," + String(lteActiveCID), raw, 3000);

  if (!lteSendAT("AT+HTTPPARA=\"URL\",\"" + baseUrl + path + "\"", raw, 5000)) {
    Serial.println("LTE HTTPS: setting URL failed");
    lteSendAT("AT+HTTPTERM", raw, 3000);
    return false;
  }

  // TLS setup for an https:// URL. AT+HTTPSSL=1 was tried here first and is
  // simply not part of this modem's command set — it belongs to the older
  // SIM800 series — so it returned ERROR and the request then went out with
  // no TLS context at all, which is why LTE payments failed while the same
  // call over WiFi worked. The A7670/SIM7600 engine instead takes a numbered
  // SSL context configured via AT+CSSLCFG and bound to the HTTP session with
  // AT+HTTPPARA="SSLCFG",<ctx>.
  if (baseUrl.startsWith("https://")) {
    // sslversion 4 = "all", i.e. let the modem negotiate up to TLS 1.2 —
    // PhonePe's endpoint refuses anything older.
    lteSendAT("AT+CSSLCFG=\"sslversion\",0,4", raw, 3000);
    // authmode 0 = don't verify the server certificate. This deliberately
    // matches what the WiFi path already does (client.setInsecure() in
    // Screen_05_PaymentUPI.ino) — there's no CA bundle loaded into the
    // modem to verify against, and it also means the handshake no longer
    // depends on the modem's RTC being correct.
    lteSendAT("AT+CSSLCFG=\"authmode\",0,0", raw, 3000);

    // Server Name Indication, off by default on this module. A host that
    // shares an IP with many others behind a CDN — api.brevo.com does — can't
    // tell which certificate to present without it, so the handshake dies and
    // AT+HTTPACTION reports a 7xx even though DNS and the bearer are fine.
    // PhonePe's endpoint doesn't need it, which is why the payment path
    // worked from the start and this one didn't.
    bool sniOk = lteSendAT("AT+CSSLCFG=\"enableSNI\",0,1", raw, 3000);
    Serial.printf("LTE HTTPS: SNI -> %s [%s]\n",
                  sniOk ? "enabled" : "ERROR/unsupported", lteFlatten(raw).c_str());

    bool sslOk = lteSendAT("AT+HTTPPARA=\"SSLCFG\",0", raw, 3000);
    Serial.printf("LTE HTTPS: SSL context bound -> %s\n", sslOk ? "OK" : "ERROR");
  }

  int lineStart = 0;
  while (lineStart < (int)extraHeaders.length()) {
    int nl = extraHeaders.indexOf('\n', lineStart);
    String line = (nl >= 0) ? extraHeaders.substring(lineStart, nl) : extraHeaders.substring(lineStart);
    line.trim();
    if (line.length() > 0) lteSendAT("AT+HTTPPARA=\"USERDATA\",\"" + line + "\"", raw, 3000);
    if (nl < 0) break;
    lineStart = nl + 1;
  }

  if (body.length() > 0) {
    lteSendAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"", raw, 3000);

    Serial.printf("LTE HTTPS: uploading %d byte body via HTTPDATA\n", body.length());
    while (Serial2.available()) Serial2.read();
    Serial2.print("AT+HTTPDATA=" + String(body.length()) + ",10000\r\n");

    String prompt = "";
    unsigned long t0 = millis();
    while (millis() - t0 < 5000 && prompt.indexOf("DOWNLOAD") < 0) {
      while (Serial2.available()) prompt += (char)Serial2.read();
    }
    if (prompt.indexOf("DOWNLOAD") < 0) {
      Serial.printf("LTE HTTPS: HTTPDATA DOWNLOAD prompt not seen, got [%s]\n", prompt.c_str());
      lteSendAT("AT+HTTPTERM", raw, 3000);
      return false;
    }

    // Written in 256-byte chunks rather than one blocking print. A multi-KB
    // burst at 115200 baud can outrun the modem's receive buffer, and the
    // symptom is a body that arrives truncated — which the modem then reports
    // as a 7xx failure from AT+HTTPACTION rather than as an upload error.
    const char* p = body.c_str();
    size_t remaining = body.length();
    while (remaining > 0) {
      size_t n = (remaining < 256) ? remaining : 256;
      Serial2.write((const uint8_t*)p, n);
      p += n;
      remaining -= n;
      delay(5);
    }

    // Timeout scales with body size: a fixed 5s was fine for PhonePe's 258
    // bytes but far too short once a report with an attachment is in flight.
    unsigned long dataTimeout = 5000 + (body.length() / 100) * 20;
    String dataResp;
    unsigned long t1 = millis();
    while (millis() - t1 < dataTimeout &&
           dataResp.indexOf("OK") < 0 && dataResp.indexOf("ERROR") < 0) {
      while (Serial2.available()) dataResp += (char)Serial2.read();
    }
    Serial.printf("LTE HTTPS: HTTPDATA upload response was [%s]\n",
                  lteFlatten(dataResp).c_str());

    // Previously this result was logged but never acted on, so a rejected or
    // incomplete upload still went on to fire AT+HTTPACTION with a partial
    // body. Bail out instead — the caller can then retry with less data.
    if (dataResp.indexOf("OK") < 0) {
      Serial.println("LTE HTTPS: body upload failed, abandoning request");
      lteSendAT("AT+HTTPTERM", raw, 3000);
      return false;
    }
  }

  // AT+HTTPACTION's immediate "OK" only means the command was accepted, not
  // that the request finished — the actual result arrives later as an
  // unsolicited "+HTTPACTION: <method>,<code>,<len>" line, so this can't
  // just wait for "OK" the way lteSendAT does.
  int actionCode = (method == "POST") ? 1 : 0;
  while (Serial2.available()) Serial2.read();
  Serial2.print("AT+HTTPACTION=" + String(actionCode) + "\r\n");

  // Scaled with the request size. A flat 30s was fine for PhonePe's 258-byte
  // POST but far too tight once real payloads are in flight: a 4276-byte send
  // to Brevo took 25.7s to answer, and an identical one was cut off at 30s
  // and misreported as a failure. Bigger bodies need proportionally longer.
  String actionResp = "";
  int dataLen = -1;
  unsigned long actionTimeout = 40000 + body.length() * 4;
  unsigned long t2 = millis();
  while (millis() - t2 < actionTimeout) {
    while (Serial2.available()) actionResp += (char)Serial2.read();
    int idx = actionResp.indexOf("+HTTPACTION:");
    if (idx >= 0) {
      int lineEnd = actionResp.indexOf('\n', idx);
      if (lineEnd >= 0) {
        String line = actionResp.substring(idx, lineEnd);
        int c1 = line.indexOf(',');
        int c2 = line.indexOf(',', c1 + 1);
        if (c1 > 0 && c2 > c1) {
          outStatusCode = line.substring(c1 + 1, c2).toInt();
          String lenStr = line.substring(c2 + 1);
          lenStr.trim();
          dataLen = lenStr.toInt();
        }
        Serial.printf("LTE HTTPS: +HTTPACTION line was [%s] -> code=%d len=%d\n",
                      line.c_str(), outStatusCode, dataLen);
        break;
      }
    }
  }

  if (outStatusCode < 0) {
    Serial.printf("LTE HTTPS: no +HTTPACTION response after %lums (timed out)\n",
                  actionTimeout);
    Serial.printf("LTE HTTPS: raw buffer was [%s]\n", actionResp.c_str());
    lteSendAT("AT+HTTPTERM", raw, 3000);
    return false;
  }

  // 7xx codes are the modem's own errors, not the server's — the request
  // never reached PhonePe. 714 in particular is a DNS failure, so prove
  // whether the resolver works at all while we're still connected.
  if (outStatusCode >= 700) {
    Serial.printf("LTE HTTPS: modem-side error %d (not an HTTP status from the server)\n",
                  outStatusCode);
    String host = baseUrl;
    host.replace("https://", "");
    host.replace("http://", "");
    int slash = host.indexOf('/');
    if (slash >= 0) host = host.substring(0, slash);
    lteResolveHost(host);
    lteLogDataDiagnostics();
  }

  if (dataLen > 0) {
    lteHttpRead(dataLen, outBody, 10000);
  } else {
    Serial.println("LTE HTTPS: modem reported 0-length body, skipping HTTPREAD");
  }

  lteSendAT("AT+HTTPTERM", raw, 3000);
  return true;
}

// Cellular signal quality -> 0-4 bars, same scale as the WiFi bars so the
// status icon and quality label read consistently either way.


unsigned long lteLastTimeSyncAttemptMs = 0;
const unsigned long LTE_TIME_SYNC_RETRY_MS = 10000;  // NITZ can arrive a bit after registration
int lteTimeSyncFailures = 0;
const unsigned long LTE_TIME_SYNC_BACKOFF_MS = 300000;  // 5 min once it's clearly not coming

// Only tries LTE when WiFi genuinely isn't connected, and no more often
// than LTE_RETRY_INTERVAL_MS. Call this every loop() tick like maintainNTP().
void maintainLTEFallback() {
  if (WiFi.status() == WL_CONNECTED) return;

  if (!isLTEConnected()) {
    if (millis() - lteLastAttemptMs < LTE_RETRY_INTERVAL_MS) return;
    lteLastAttemptMs = millis();
    connectLTEIfNeeded();
    return;
  }

  // Already connected — if the clock still isn't set (e.g. the modem's
  // first NITZ read came back as a placeholder value), keep retrying
  // independently of the connection state, since connectLTEIfNeeded() only
  // ever runs its registration/time-read step once per boot.
  // Each failed attempt costs several blocking seconds of AT-command waits,
  // so back off hard once it's clear the network isn't going to provide a
  // time — otherwise a machine with a broken data session spends most of
  // every 10s window unresponsive to touch.
  unsigned long retryInterval = (lteTimeSyncFailures < 3) ? LTE_TIME_SYNC_RETRY_MS
                                                          : LTE_TIME_SYNC_BACKOFF_MS;
  if (!timeSyncedFromNetwork && millis() - lteLastTimeSyncAttemptMs >= retryInterval) {
    lteLastTimeSyncAttemptMs = millis();
    if (!syncTimeFromLTE()) lteTimeSyncFailures++;
  }
}

// The only place lteEnabled should change after boot — Admin > 4G/LTE
// Setup's on/off toggle (Screen_16_AdminLTE.ino) calls this, not the
// variable directly, so the two things a plain assignment would forget to
// do can't be forgotten:
//
//   * Turning OFF stops loop() from calling maintainLTEFallback() on its
//     very next tick (that's just the flag), but isLTEConnected() would
//     otherwise keep reading true off stale lteRegistered/lteHaveIP state
//     from before the switch — the status icon and this screen would go on
//     claiming "Connected" for a radio that's no longer being maintained at
//     all. Clearing both here makes the switch honest immediately.
//   * Turning ON should feel instant, not "eventually, next time the 30s
//     retry timer happens to elapse" — zeroing lteLastAttemptMs makes
//     maintainLTEFallback() try on its very next loop() tick instead.
//
// requestedOn is ANDed with CFG_LTE_ENABLED so this can never turn LTE on
// for a unit with no modem fitted — the hardware ceiling always wins, same
// as loadPersistedProductData() enforces it on every boot.
void setLteEnabled(bool requestedOn) {
  lteEnabled = requestedOn && CFG_LTE_ENABLED;
  saveLteEnabled();  // Core_09_Storage.ino

  if (!lteEnabled) {
    lteRegistered = false;
    lteHaveIP = false;
  } else {
    lteLastAttemptMs = 0;
  }
  indicatorDirty = true;  // Core_08_UIHelpers.ino — redraw the status icon now
}

// Mirrors pingGoogleTest() (Core_06_Network.ino) but over the cellular data
// path, so it can be tested the same way from an admin screen.
bool pingGoogleTestLTE() {
  if (!connectLTEIfNeeded()) {
    ltePingAttempted = true;
    ltePingSuccess = false;
    ltePingHttpCode = -1;
    ltePingMs = 0;
    return false;
  }

  HttpClient lteHttp(lteClient, "www.google.com", 80);
  unsigned long t0 = millis();
  int err = lteHttp.get("/");
  int code = -1;
  if (err == 0) {
    code = lteHttp.responseStatusCode();
  }
  ltePingMs = millis() - t0;
  lteHttp.stop();

  ltePingAttempted = true;
  ltePingHttpCode = code;
  ltePingSuccess = (err == 0 && code > 0);
  return ltePingSuccess;
}

// ---------- Status-bar icon: cellular bars, drawn in the same top-right
// slot as the WiFi fan glyph (Core_07_WiFiIndicator.ino) whenever WiFi is
// down but LTE is actively carrying the connection.
void drawCellularBars(int x, int yBase, int scale, int bars, uint16_t onColor) {
  int barHeights[4] = { 4 * scale, 7 * scale, 10 * scale, 13 * scale };
  int barWidth = 3 * scale;
  int gap = 2 * scale;
  for (int i = 0; i < 4; i++) {
    int bx = x + i * (barWidth + gap);
    int by = yBase - barHeights[i];
    uint16_t color = (bars >= i + 1) ? onColor : COL_TEXT_DIM;
    tft.fillRect(bx, by, barWidth, barHeights[i], color);
  }
}

void drawLTEIndicator() {
  int bars = lteSignalBars();
  uint16_t onColor = (bars >= 3) ? COL_SUCCESS : (bars == 2) ? COL_WARNING : COL_DANGER;

  tft.fillRect(WIFI_ICON_X, WIFI_ICON_Y, WIFI_ICON_W, WIFI_ICON_H, COL_BG_TOP);
  drawCellularBars(WIFI_ICON_X + 2, WIFI_ICON_Y + WIFI_ICON_H - 1, 1, bars, onColor);
}
