// =====================================================
//   AUTOMATIC REPORT SCHEDULING
// =====================================================
// Drives the two unattended email paths:
//
//   * Up to two nightly sends per day, at admin-configured times, each
//     carrying that day's sales summary.
//   * A low-stock alert, fired when a product's motor crosses the threshold.
//
// Everything here is edge-triggered and the "already done" markers are kept
// in NVS, so a power cut or a reboot can't cause a duplicate send — which
// matters when the machine may reset several times a day on site power.
//
// Sending blocks for up to ~90 seconds across its retries, so NOTHING here
// sends from the purchase path. maintainReportSchedule() is called only from
// the idle Welcome screen and is the single place a send is started; a sale
// that drops stock below the threshold merely queues the alert.

// YYYYMMDD of the last successful nightly send for each slot, so a given
// slot fires at most once per day. Persisted.
uint32_t nightlySentDate[2] = { 0, 0 };

// One bit per motor: set once an alert has gone out for that motor, cleared
// when it's refilled above the threshold. Stops a machine sitting at 1 unit
// from mailing on every single sale. Persisted.
uint32_t lowAlertedMask = 0;

// Set when a motor crosses the threshold, cleared once the alert has been
// attempted. The send itself is deliberately NOT done at detection time:
// detection happens inside dispenseCart(), and a cellular send can block for
// well over a minute across its retries — which would leave a customer
// staring at a frozen screen immediately after paying. The flag is picked up
// by maintainReportSchedule() instead, which only runs on the idle Welcome
// screen. RAM-only: a power cut before the send loses one alert, which is a
// better trade than holding up a paying customer.
bool lowStockAlertPending = false;

// A scheduled daily send that FAILED (no link, bad credentials, a dropped TLS
// handshake) is retried every NIGHTLY_RETRY_MS, up to NIGHTLY_MAX_TRIES times,
// instead of being written off for the whole day. RAM only: after a reboot the
// slot simply gets a fresh set of tries, which is the safe direction.
const int NIGHTLY_MAX_TRIES = 6;
const unsigned long NIGHTLY_RETRY_MS = 10UL * 60UL * 1000UL;
uint8_t nightlyTries[2] = { 0, 0 };
unsigned long nightlyRetryAt[2] = { 0, 0 };   // millis() gate; 0 = no wait

// True when any sellable motor is at or below the alert level. The daily
// report uses this to add its low-stock section, so one mail carries both.
bool anyLowStock() {
  for (int m = 0; m < MAX_MOTORS; m++) {
    if (motorIsActive(m) && motorStock[m] <= lowStockThreshold) return true;
  }
  return false;
}

// Called when a slot's time is changed on the admin screen, so a new time
// takes effect today even if that slot already sent earlier.
void rearmNightlySlot(int slot) {
  nightlySentDate[slot] = 0;
  saveNightlySent(slot);
  nightlyTries[slot] = 0;
  nightlyRetryAt[slot] = 0;
}

// Last date the CSV was pruned, so the 60-day trim runs once per day.
uint32_t lastPruneDate = 0;

void loadScheduleState() {
  nightlySentDate[0] = (uint32_t)prefs.getULong("nsent0", 0);
  nightlySentDate[1] = (uint32_t)prefs.getULong("nsent1", 0);
  lowAlertedMask     = (uint32_t)prefs.getULong("lowmask", 0);
  lastPruneDate      = (uint32_t)prefs.getULong("prunedt", 0);
}

void saveNightlySent(int slot) {
  prefs.putULong(slot == 0 ? "nsent0" : "nsent1", nightlySentDate[slot]);
}

void saveLowAlertedMask() {
  prefs.putULong("lowmask", lowAlertedMask);
}

// Parses "HH:MM" into minutes-since-midnight. Returns -1 for an empty or
// malformed string, which is how a schedule slot is switched off.
int parseScheduleTime(const char* s) {
  if (s == NULL || strlen(s) < 4) return -1;
  int colon = -1;
  for (int i = 0; s[i]; i++) if (s[i] == ':') { colon = i; break; }
  if (colon < 1) return -1;

  int hh = atoi(String(s).substring(0, colon).c_str());
  int mm = atoi(String(s).substring(colon + 1).c_str());
  if (hh < 0 || hh > 23 || mm < 0 || mm > 59) return -1;
  return hh * 60 + mm;
}

int currentMinutesOfDay() {
  struct tm ti;
  if (!timeSynced || !getLocalTime(&ti, 10)) return -1;
  return ti.tm_hour * 60 + ti.tm_min;
}

// ---------- Low stock ----------
// Called after every dispense, and whenever stock is edited in the admin
// screen. Only mails on the transition into the low state, and only for
// motors that are actually assigned to a product.
void checkLowStockAlert() {
  if (!lowStockAlertOn) return;

  bool newlyLow = false;
  for (int m = 0; m < MAX_MOTORS; m++) {
    // A disabled product can't be sold, so its stock running low isn't
    // something anyone needs waking up for.
    if (!motorIsActive(m)) continue;
    bool low = motorStock[m] <= lowStockThreshold;
    bool alerted = (lowAlertedMask & (1UL << m)) != 0;

    if (low && !alerted) {
      lowAlertedMask |= (1UL << m);
      newlyLow = true;
    } else if (!low && alerted) {
      // Refilled — re-arm so the next time it runs down we hear about it.
      lowAlertedMask &= ~(1UL << m);
      saveLowAlertedMask();
    }
  }

  if (!newlyLow) return;
  saveLowAlertedMask();

  if (!emailConfigured()) {
    Serial.println("Schedule: low stock, but email isn't configured");
    return;
  }
  Serial.println("Schedule: low stock crossed - alert queued");
  lowStockAlertPending = true;
}

// Clears the "already alerted" bit for any motor that's been refilled above
// the threshold, WITHOUT sending anything. Called while an admin is editing
// stock, where firing an alert mid-refill would be noise — the point is only
// to re-arm so the next genuine run-down is reported.
void refreshLowStockArm() {
  uint32_t before = lowAlertedMask;
  for (int m = 0; m < MAX_MOTORS; m++) {
    if (!motorIsActive(m) || motorStock[m] > lowStockThreshold) {
      lowAlertedMask &= ~(1UL << m);
    }
  }
  if (lowAlertedMask != before) saveLowAlertedMask();
}

// ---------- Daily maintenance ----------
// Drops CSV rows older than the 60-day window. Runs once per day, on the
// first schedule check after the date changes.
void pruneIfNewDay(uint32_t today) {
  if (today == 0 || today == lastPruneDate) return;
  lastPruneDate = today;
  prefs.putULong("prunedt", today);

  // The daily table self-limits by recycling its oldest slot, so the cutoff
  // is taken from the table rather than by doing calendar arithmetic on
  // dates — which would need real month-length handling to be correct.
  int order[DAILY_HISTORY_DAYS];
  int n = dailySortedIndices(order);
  if (n < DAILY_HISTORY_DAYS) return;   // fewer than 60 days on record, nothing to drop

  pruneEventLog(dailyHistory[order[0]].date);
}

// ---------- Daily send ----------
// Fires when the clock has passed a configured time and that slot hasn't
// already fired today. Uses ">=" rather than an exact match so a send still
// happens if the machine was powered off or busy at the exact minute.
//
// The scheduled mail is always the DAILY report, for both slots. When any
// motor is low, that same mail carries a low-stock section (buildDailyHTML()),
// so the operator gets one message, not a daily and an alert. A due daily
// report is also handled BEFORE any queued stand-alone alert and, once sent,
// cancels it — the alert used to go first and hide the report.
//
// A stand-alone low-stock alert is only sent when no daily report is due, so a
// motor running out mid-afternoon is still reported promptly.
void maintainReportSchedule() {
  if (!timeSynced) return;

  uint32_t today = todayDateNum();
  if (today == 0) return;

  // Opens the new day's record the moment the date turns over, capturing the
  // morning stock while the machine is idle rather than waiting for the first
  // sale to do it.
  ensureTodaySlot();
  pruneIfNewDay(today);

  int nowMin = currentMinutesOfDay();
  const char* slotTimes[2] = { nightlyTime1, nightlyTime2 };

  if (nowMin >= 0) {
    for (int slot = 0; slot < 2; slot++) {
      int target = parseScheduleTime(slotTimes[slot]);
      if (target < 0) continue;                      // slot disabled
      if (nightlySentDate[slot] == today) continue;  // already sent today
      if (nowMin < target) continue;                 // not time yet
      if (nightlyRetryAt[slot] != 0 && (long)(millis() - nightlyRetryAt[slot]) < 0) continue;  // waiting to retry

      if (!emailConfigured()) {
        // Mark it done anyway — without a recipient it can never succeed, and
        // retrying every loop would stall the UI all evening.
        nightlySentDate[slot] = today;
        saveNightlySent(slot);
        continue;
      }

      Serial.printf("Schedule: daily slot %d firing (%s), try %d\n", slot + 1, slotTimes[slot], nightlyTries[slot] + 1);
      // With the attachment: that day's transactions in full, which is small
      // enough to carry even over cellular.
      bool ok = sendReportEmail(REPORT_DAILY, today, true);

      if (ok) {
        nightlySentDate[slot] = today;
        saveNightlySent(slot);
        nightlyTries[slot] = 0;
        nightlyRetryAt[slot] = 0;
        // The daily mail already listed anything low, so a queued stand-alone
        // alert would only repeat it.
        lowStockAlertPending = false;
      } else {
        Serial.printf("Schedule: daily slot %d failed: %s\n", slot + 1, reportLastStatus);
        nightlyTries[slot]++;
        if (nightlyTries[slot] >= NIGHTLY_MAX_TRIES) {
          // Out of tries for today; the status line on the Reports screen keeps
          // the reason. Tomorrow starts clean.
          nightlySentDate[slot] = today;
          saveNightlySent(slot);
          nightlyTries[slot] = 0;
          nightlyRetryAt[slot] = 0;
        } else {
          nightlyRetryAt[slot] = millis() + NIGHTLY_RETRY_MS;
          if (nightlyRetryAt[slot] == 0) nightlyRetryAt[slot] = 1;
        }
      }
      return;   // one blocking send per pass
    }
  }

  // Low-stock alert queued by a sale, sent here rather than at detection so
  // the blocking send happens on an idle machine. Only reached when no daily
  // report was due this pass. Cleared either way: a failure is logged and
  // reported on the admin screen rather than retried every second.
  if (lowStockAlertPending) {
    lowStockAlertPending = false;
    Serial.println("Schedule: sending queued low-stock alert");
    if (!sendReportEmail(REPORT_LOWSTOCK, today, false)) {
      Serial.printf("Schedule: low-stock alert failed: %s", reportLastStatus);
      Serial.println();
    }
  }
}
