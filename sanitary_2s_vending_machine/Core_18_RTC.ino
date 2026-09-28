// =====================================================
//        DS3231 BATTERY-BACKED RTC (I2C) — CLOCK FALLBACK
// =====================================================
// Third and last stop in the clock chain: Core_06_Network.ino's maintainNTP()
// tries WiFi, Core_13_LTEModem.ino's maintainLTEFallback() tries the modem,
// and this file only ever supplies a time when BOTH of those have had a fair
// chance and neither has produced one — e.g. a machine that boots with no
// WiFi in range and no SIM/signal. It never overrides a real network sync,
// and once WiFi or LTE does succeed, that time is written back to this chip
// so it stays accurate through the next power cycle with no connectivity at
// all.
//
// Wiring is the proposed SDA=GPIO14 / SCL=GPIO15 pair from
// docs/wiring-diagram.html. These were the motor pool's spare M7/M8 slots
// when the pool still went up to 8; since this build only ever needs 5
// motors now (Core_04_Hardware.ino's MOTOR_PIN_POOL), GPIO14/15 are the
// RTC's full time, not a pin shared with a hypothetical 7th/8th motor.
//
// This chip's register map (0x00-0x06, BCD-encoded seconds..year) is a fixed
// public standard shared by every DS3231/DS3231M/DS1307-compatible module,
// so unlike the RFID reader below there is nothing module-specific left to
// verify here — this talks to the silicon directly over Wire.h, no library
// dependency needed.
const int RTC_SDA_PIN = 14;
const int RTC_SCL_PIN = 15;
const uint8_t RTC_I2C_ADDR = 0x68;

// How long to let WiFi/LTE try before trusting the RTC alone. Both of those
// paths can take several seconds just to associate/register, so falling back
// immediately on boot would routinely use stale RTC time when a real sync
// was only moments away.
const unsigned long RTC_FALLBACK_GRACE_MS = 8000;

unsigned long rtcModuleBootMs = 0;
bool rtcFallbackUsed = false;        // true once the clock has been set FROM the RTC this boot
bool rtcWrittenFromNetwork = false;  // true once a network time has been written back to it this boot

uint8_t rtcBcdToDec(uint8_t bcd) { return (uint8_t)((bcd / 16) * 10 + (bcd % 16)); }
uint8_t rtcDecToBcd(uint8_t dec) { return (uint8_t)((dec / 10) * 16 + (dec % 10)); }

void initRTC() {
  // Belt-and-suspenders: most DS3231 breakout boards carry their own 4.7k
  // pull-ups, but enabling the ESP32's internal ones too costs nothing and
  // covers a board that doesn't, or wiring long/noisy enough to need the
  // extra help — either of which would otherwise look identical to "no RTC
  // fitted" (rtcReadTime() NACKs the same way either way).
  pinMode(RTC_SDA_PIN, INPUT_PULLUP);
  pinMode(RTC_SCL_PIN, INPUT_PULLUP);
  Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN);
  rtcModuleBootMs = millis();

  // Probed immediately, not left to maintainRTCFallback()'s 8s grace delay —
  // that delay exists to give WiFi/LTE a fair shot, it has nothing to do
  // with whether the chip is even on the bus. Logging the presence check
  // right here turns "no clock after reboot" from a guessing game between a
  // firmware bug and a wiring/power problem into something visible on the
  // very first line of the serial log.
  Wire.beginTransmission(RTC_I2C_ADDR);
  if (Wire.endTransmission() == 0) {
    Serial.println("RTC: DS3231 found on I2C (SDA=GPIO14 / SCL=GPIO15)");
  } else {
    Serial.println("RTC: no ACK from DS3231 at 0x68 - check SDA=GPIO14/SCL=GPIO15 wiring, "
                    "module power, and backup battery");
  }
}

// Fills out with the RTC's current wall-clock time (whatever IST time was
// last written into it — see rtcWriteTime()). Returns false with no change
// to out if nothing ACKs at the DS3231's address, which is exactly what
// happens on a build that has no RTC module fitted — this is a soft
// dependency, not a hard requirement to boot.
bool rtcReadTime(struct tm &out) {
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write((uint8_t)0x00);
  if (Wire.endTransmission(false) != 0) return false;  // NACK -> nothing fitted

  if (Wire.requestFrom((int)RTC_I2C_ADDR, 7) != 7) return false;

  uint8_t sec     = rtcBcdToDec(Wire.read() & 0x7F);
  uint8_t minute  = rtcBcdToDec(Wire.read());
  uint8_t hourRaw = Wire.read();
  uint8_t hour    = rtcBcdToDec(hourRaw & 0x3F);  // bit6=0 -> 24-hour mode, which is how we always write it
  Wire.read();                                    // day-of-week register, unused
  uint8_t date    = rtcBcdToDec(Wire.read());
  uint8_t month   = rtcBcdToDec(Wire.read() & 0x1F);
  uint8_t year    = rtcBcdToDec(Wire.read());

  memset(&out, 0, sizeof(out));
  out.tm_sec  = sec;
  out.tm_min  = minute;
  out.tm_hour = hour;
  out.tm_mday = date;
  out.tm_mon  = month - 1;
  out.tm_year = year + 100;  // DS3231 only stores a 2-digit year; assume 2000s
  return true;
}

// Writes t (wall-clock IST, same as getLocalTime() returns) into the RTC's
// seconds..year registers, always in 24-hour mode. Logs rather than silently
// dropping a failed write — a NACK here (no RTC fitted, or a wiring fault)
// used to look identical to a successful write that just never gets read
// back, which is exactly the failure mode reported: set the clock, power
// cycle, nothing comes back.
void rtcWriteTime(const struct tm &t) {
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write((uint8_t)0x00);
  Wire.write(rtcDecToBcd((uint8_t)t.tm_sec));
  Wire.write(rtcDecToBcd((uint8_t)t.tm_min));
  Wire.write(rtcDecToBcd((uint8_t)t.tm_hour));  // bit6=0 -> 24-hour
  Wire.write(rtcDecToBcd(1));                    // day-of-week — unused, placeholder
  Wire.write(rtcDecToBcd((uint8_t)t.tm_mday));
  Wire.write(rtcDecToBcd((uint8_t)(t.tm_mon + 1)));
  Wire.write(rtcDecToBcd((uint8_t)(t.tm_year % 100)));
  if (Wire.endTransmission() != 0) {
    Serial.println("RTC: write failed (no ACK) - time will NOT survive a power cycle");
    return;
  }
  Serial.println("RTC: time written OK");
}

// Call every loop() tick, same as maintainNTP()/maintainLTEFallback(). Two
// independent jobs, order doesn't matter between them:
//   1. Nothing has synced the clock yet and WiFi/LTE have had their grace
//      period — read the RTC so the machine at least has a plausible time
//      instead of starting at 1970, but WITHOUT marking timeSyncedFromNetwork,
//      so WiFi/LTE keep trying in the background and can still take over.
//   2. The network has since produced an authoritative time — push it back
//      into the RTC once, so the chip stays correct through the next
//      power-cycle-with-no-connectivity.
void maintainRTCFallback() {
  if (!timeSynced && millis() - rtcModuleBootMs > RTC_FALLBACK_GRACE_MS && !rtcFallbackUsed) {
    struct tm rtcTm;
    if (rtcReadTime(rtcTm)) {
      setenv("TZ", "IST-5:30", 1);
      tzset();
      time_t epoch = mktime(&rtcTm);
      struct timeval tv = { epoch, 0 };
      settimeofday(&tv, nullptr);
      timeSynced = true;
      rtcFallbackUsed = true;
      Serial.println("Clock: no WiFi/LTE yet, time set from onboard RTC");
    }
  }

  if (timeSyncedFromNetwork && !rtcWrittenFromNetwork) {
    struct tm ti;
    if (getLocalTime(&ti, 10)) {
      rtcWriteTime(ti);
      rtcWrittenFromNetwork = true;
      Serial.println("Clock: network time written back to onboard RTC");
    }
  }
}
