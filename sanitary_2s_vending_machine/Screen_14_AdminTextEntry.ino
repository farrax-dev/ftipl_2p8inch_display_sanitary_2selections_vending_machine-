// =====================================================
//          ADMIN TEXT ENTRY (on-screen keyboard)
// =====================================================
// ---------- Generic on-screen text entry state ----------
TextEntryTarget textEntryTarget = TE_PRODUCT_NAME;
char textEntryBuffer[128] = "";
int  textEntryMaxLen = 12;
bool textEntryMask = false;
bool textEntryShow = false;
const char* textEntryTitle = "Enter Name";
AppScreen textEntryReturnScreen = SCREEN_ADMIN_EDIT;

KbLayer kbLayer = KB_LAYER_UPPER;

const KeyDef KB_UPPER[4][11] = {
  {{"Q",1},{"W",1},{"E",1},{"R",1},{"T",1},{"Y",1},{"U",1},{"I",1},{"O",1},{"P",1},{NULL,0}},
  {{"A",1},{"S",1},{"D",1},{"F",1},{"G",1},{"H",1},{"J",1},{"K",1},{"L",1},{"DEL",1},{NULL,0}},
  {{"aA",1},{"Z",1},{"X",1},{"C",1},{"V",1},{"B",1},{"N",1},{"M",1},{"-",1},{"_",1},{NULL,0}},
  {{"?123",2},{"SPACE",5},{"@",1},{".",1},{"?",1},{NULL,0}}
};

const KeyDef KB_LOWER[4][11] = {
  {{"q",1},{"w",1},{"e",1},{"r",1},{"t",1},{"y",1},{"u",1},{"i",1},{"o",1},{"p",1},{NULL,0}},
  {{"a",1},{"s",1},{"d",1},{"f",1},{"g",1},{"h",1},{"j",1},{"k",1},{"l",1},{"DEL",1},{NULL,0}},
  {{"Aa",1},{"z",1},{"x",1},{"c",1},{"v",1},{"b",1},{"n",1},{"m",1},{"-",1},{"_",1},{NULL,0}},
  {{"?123",2},{"SPACE",5},{"@",1},{".",1},{"?",1},{NULL,0}}
};

const KeyDef KB_SYM[4][11] = {
  {{"1",1},{"2",1},{"3",1},{"4",1},{"5",1},{"6",1},{"7",1},{"8",1},{"9",1},{"0",1},{NULL,0}},
  {{"!",1},{"@",1},{"#",1},{"$",1},{"%",1},{"&",1},{"*",1},{"(",1},{")",1},{"DEL",1},{NULL,0}},
  {{"~",1},{"+",1},{"-",1},{"=",1},{"/",1},{"\\",1},{":",1},{";",1},{"'",1},{"\"",1},{NULL,0}},
  {{"ABC",2},{"SPACE",5},{",",1},{".",1},{"?",1},{NULL,0}}
};

const KeyDef* kbRow(int row) {
  switch (kbLayer) {
    case KB_LAYER_LOWER: return KB_LOWER[row];
    case KB_LAYER_SYM:   return KB_SYM[row];
    default:             return KB_UPPER[row];
  }
}

const int KB_X0 = 6, KB_Y0 = 86, KB_COL_W = 29, KB_ROW_H = 33, KB_GX = 2, KB_GY = 3;

const int TE_CANCEL_X = 10,  TE_BTN_Y = 34, TE_BTN_W = 95, TE_BTN_H = 20;
const int TE_SHOW_X   = 112;
const int TE_DONE_X   = 214;

void getKeyRect(int row, int keyIdx, int &x, int &y, int &w, int &h) {
  const KeyDef* r = kbRow(row);
  int col = 0;
  for (int k = 0; k < keyIdx; k++) col += r[k].span;
  x = KB_X0 + col * (KB_COL_W + KB_GX);
  y = KB_Y0 + row * (KB_ROW_H + KB_GY);
  w = r[keyIdx].span * KB_COL_W + (r[keyIdx].span - 1) * KB_GX;
  h = KB_ROW_H;
}

void openTextEntry(TextEntryTarget target) {
  textEntryTarget = target;
  textEntryShow = false;
  kbLayer = KB_LAYER_UPPER;

  switch (target) {
    case TE_WIFI_SSID:
      strncpy(textEntryBuffer, wifiSSID, sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(wifiSSID) - 1;
      textEntryMask = false;
      textEntryTitle = "WiFi Name";
      textEntryReturnScreen = SCREEN_ADMIN_WIFI;
      break;
    case TE_WIFI_PASS:
      strncpy(textEntryBuffer, wifiPass, sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(wifiPass) - 1;
      textEntryMask = true;
      textEntryTitle = "WiFi Password";
      textEntryReturnScreen = SCREEN_ADMIN_WIFI;
      kbLayer = KB_LAYER_LOWER;
      break;
    case TE_UPI_MERCHANT_ID:
      strncpy(textEntryBuffer, phonepeMerchantId, sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(phonepeMerchantId) - 1;
      textEntryMask = false;
      textEntryTitle = "UPI Merchant ID";
      textEntryReturnScreen = SCREEN_ADMIN_UPI;
      break;
    case TE_UPI_STORE_ID:
      strncpy(textEntryBuffer, phonepeStoreId, sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(phonepeStoreId) - 1;
      textEntryMask = false;
      textEntryTitle = "UPI Store ID";
      textEntryReturnScreen = SCREEN_ADMIN_UPI;
      break;
    case TE_MACHINE_ID:
      strncpy(textEntryBuffer, machineId, sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(machineId) - 1;
      textEntryMask = false;
      textEntryTitle = "Machine ID";
      textEntryReturnScreen = SCREEN_ADMIN_SETTINGS;
      break;
    case TE_MACHINE_NAME:
      strncpy(textEntryBuffer, machineName, sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(machineName) - 1;
      textEntryMask = false;
      textEntryTitle = "Machine Name";
      textEntryReturnScreen = SCREEN_ADMIN_REPORT;
      break;
    // Email fields all start on the lowercase layer: keys and addresses are
    // lowercase far more often than not.
    case TE_GMAIL_USER:
      strncpy(textEntryBuffer, gmailUser, sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(gmailUser) - 1;
      textEntryMask = false;
      textEntryTitle = "Gmail Address";
      textEntryReturnScreen = SCREEN_ADMIN_REPORT;
      kbLayer = KB_LAYER_LOWER;
      break;
    case TE_GMAIL_PASS:
      strncpy(textEntryBuffer, gmailPass, sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(gmailPass) - 1;
      textEntryMask = true;   // a credential; Show reveals it to check a typo
      textEntryTitle = "Gmail App Password";
      textEntryReturnScreen = SCREEN_ADMIN_REPORT;
      kbLayer = KB_LAYER_LOWER;
      break;
    case TE_REPORT_TO:
      strncpy(textEntryBuffer, reportTo, sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(reportTo) - 1;
      textEntryMask = false;
      textEntryTitle = "Send Report To";
      textEntryReturnScreen = SCREEN_ADMIN_REPORT;
      kbLayer = KB_LAYER_LOWER;
      break;
    // Times are typed as "HH:MM" (24-hour) and cleared to blank to switch the
    // slot off. Opens on the symbol layer, which is where the digits and the
    // colon live.
    case TE_NIGHTLY_1:
      strncpy(textEntryBuffer, nightlyTime1, sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(nightlyTime1) - 1;
      textEntryMask = false;
      textEntryTitle = "Email 1 at HH:MM";
      textEntryReturnScreen = SCREEN_ADMIN_REPORT;
      kbLayer = KB_LAYER_SYM;
      break;
    case TE_NIGHTLY_2:
      strncpy(textEntryBuffer, nightlyTime2, sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(nightlyTime2) - 1;
      textEntryMask = false;
      textEntryTitle = "Email 2 at HH:MM";
      textEntryReturnScreen = SCREEN_ADMIN_REPORT;
      kbLayer = KB_LAYER_SYM;
      break;
    // Hex UID, typed by hand when scanning isn't convenient. Starts on
    // uppercase letters (A-F) since digits are one tap away on "?123" either
    // way, and the registered list stores UIDs uppercase.
    case TE_RFID_CARD_MANUAL:
      textEntryBuffer[0] = '\0';
      textEntryMaxLen = sizeof(rfidCards[0].uid) - 1;
      textEntryMask = false;
      textEntryTitle = "Card Number (hex)";
      textEntryReturnScreen = SCREEN_ADMIN_RFID_CARDS;
      kbLayer = KB_LAYER_UPPER;
      break;
    // Admin-set label for telling cards apart on the list — not the card's
    // own data, just a local name against its UID. rfidEditingCard is set by
    // Screen_19_AdminRFIDCardEdit.ino before opening this.
    case TE_RFID_CARD_NAME:
      strncpy(textEntryBuffer, rfidCards[rfidEditingCard].name, sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(rfidCards[0].name) - 1;
      textEntryMask = false;
      textEntryTitle = "Card Holder Name";
      textEntryReturnScreen = SCREEN_ADMIN_RFID_CARD_EDIT;
      break;
    // Pre-filled with the current PIN, same as WiFi/Gmail passwords above —
    // masked either way, and Show still works to check it before saving.
    // Digits only in practice: the login pad (Screen_07_AdminLogin.ino) can
    // only ever type 0-9, so commitTextEntry() below refuses anything else
    // rather than saving a PIN nobody could type back in.
    case TE_ADMIN_PIN:
      strncpy(textEntryBuffer, adminPin, sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(adminPin) - 1;
      textEntryMask = true;
      textEntryTitle = "Admin PIN";
      textEntryReturnScreen = SCREEN_ADMIN_SETTINGS;
      kbLayer = KB_LAYER_SYM;
      break;
    default:
      strncpy(textEntryBuffer, productNames[adminEditingSlot], sizeof(textEntryBuffer));
      textEntryMaxLen = sizeof(productNames[0]) - 1;
      textEntryMask = false;
      textEntryTitle = "Enter Name";
      textEntryReturnScreen = SCREEN_ADMIN_EDIT;
      break;
  }
  textEntryBuffer[sizeof(textEntryBuffer) - 1] = '\0';

  currentScreen = SCREEN_ADMIN_TEXT_ENTRY;
  drawAdminTextEntryScreen();
}

void returnFromTextEntry() {
  currentScreen = textEntryReturnScreen;
  if (textEntryReturnScreen == SCREEN_ADMIN_WIFI) drawAdminWiFiScreen();
  else if (textEntryReturnScreen == SCREEN_ADMIN_UPI) drawAdminUPIScreen();
  else if (textEntryReturnScreen == SCREEN_ADMIN_SETTINGS) drawAdminSettingsScreen();
  else if (textEntryReturnScreen == SCREEN_ADMIN_REPORT) drawAdminReportScreen();
  else if (textEntryReturnScreen == SCREEN_ADMIN_RFID_CARDS) drawAdminRFIDCardsScreen();
  else if (textEntryReturnScreen == SCREEN_ADMIN_RFID_CARD_EDIT) drawAdminRFIDCardEditScreen();
  else if (textEntryReturnScreen == SCREEN_ADMIN_PANEL) drawAdminPanelScreen();
  else drawAdminEditScreen();
}

void drawKeyboardPreview() {
  tft.fillRect(6, 56, 308, 26, COL_BG_TOP);

  int len = strlen(textEntryBuffer);
  char shown[70];
  if (len == 0) {
    strcpy(shown, "(empty)");
  } else if (textEntryMask && !textEntryShow) {
    int n = min(len, 60);
    for (int k = 0; k < n; k++) shown[k] = '*';
    shown[n] = '\0';
  } else {
    strncpy(shown, textEntryBuffer, sizeof(shown) - 1);
    shown[sizeof(shown) - 1] = '\0';
  }

  int shownLen = strlen(shown);
  int textSize = (shownLen > 24) ? 1 : 2;
  int maxChars = (textSize == 1) ? 50 : 25;
  const char* view = shown;
  if (shownLen > maxChars) view = shown + (shownLen - maxChars);

  tft.setTextSize(textSize);
  tft.setTextColor(COL_ACCENT, COL_BG_TOP);
  centerText(view, (textSize == 1) ? 64 : 60);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_TOP);
  char counter[12];
  snprintf(counter, sizeof(counter), "%d/%d", len, textEntryMaxLen);
  tft.setCursor(268, 72);
  tft.print(counter);
}

void drawKey(const KeyDef &key, int row, int keyIdx) {
  int x, y, w, h;
  getKeyRect(row, keyIdx, x, y, w, h);

  bool modifier = (strlen(key.label) > 1);
  uint16_t fill = modifier ? COL_BG_TOP : COL_CARD;

  // Modifier keys (DEL, SPACE, aA, ?123...) stay flush with the background —
  // same "flat" reasoning as elsewhere: a shadow under a background-matched
  // fill reads as a smudge, not a raised key. Both drawAdminTextEntryScreen()
  // and redrawKeyboardKeys() clear the whole keyboard area before calling
  // this for every key, so there's never a stale shadow from a layer switch.
  if (!modifier) drawCardShadow(x, y, w, h, 5);
  tft.fillRoundRect(x, y, w, h, 5, fill);
  tft.drawRoundRect(x, y, w, h, 5, COL_CARD_BRD);
  tft.setTextSize(modifier ? 1 : 2);
  tft.setTextColor(modifier ? COL_ACCENT : COL_TEXT, fill);
  centerTextInBox(key.label, y + (modifier ? 13 : 9), x, w);
}

void drawAdminTextEntryScreen() {
  drawGradientBackground();

  drawScreenTitle(textEntryTitle);

  drawCard(TE_CANCEL_X, TE_BTN_Y, TE_BTN_W, TE_BTN_H, 5);
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT, COL_CARD);
  centerTextInBox("Cancel", TE_BTN_Y + 7, TE_CANCEL_X, TE_BTN_W);

  if (textEntryMask) {
    uint16_t showFill = textEntryShow ? COL_ACCENT : COL_CARD;
    drawCardShadow(TE_SHOW_X, TE_BTN_Y, TE_BTN_W, TE_BTN_H, 5);
    tft.fillRoundRect(TE_SHOW_X, TE_BTN_Y, TE_BTN_W, TE_BTN_H, 5, showFill);
    tft.drawRoundRect(TE_SHOW_X, TE_BTN_Y, TE_BTN_W, TE_BTN_H, 5, COL_CARD_BRD);
    tft.setTextColor(textEntryShow ? COL_BG_TOP : COL_TEXT, showFill);
    centerTextInBox(textEntryShow ? "Hide" : "Show", TE_BTN_Y + 7, TE_SHOW_X, TE_BTN_W);
  }

  drawCardShadow(TE_DONE_X, TE_BTN_Y, TE_BTN_W, TE_BTN_H, 5);
  tft.fillRoundRect(TE_DONE_X, TE_BTN_Y, TE_BTN_W, TE_BTN_H, 5, COL_ACCENT);
  tft.setTextColor(COL_BG_TOP, COL_ACCENT);
  centerTextInBox("Done", TE_BTN_Y + 7, TE_DONE_X, TE_BTN_W);

  drawKeyboardPreview();

  for (int row = 0; row < 4; row++) {
    const KeyDef* r = kbRow(row);
    for (int k = 0; r[k].label != NULL; k++) drawKey(r[k], row, k);
  }
}

void redrawKeyboardKeys() {
  tft.fillRect(0, KB_Y0 - 2, tft.width(), tft.height() - KB_Y0 + 2, COL_BG_BOTTOM);
  for (int row = 0; row < 4; row++) {
    const KeyDef* r = kbRow(row);
    for (int k = 0; r[k].label != NULL; k++) drawKey(r[k], row, k);
  }
}

void appendChar(char c) {
  int len = strlen(textEntryBuffer);
  if (len < textEntryMaxLen && len < (int)sizeof(textEntryBuffer) - 1) {
    textEntryBuffer[len] = c;
    textEntryBuffer[len + 1] = '\0';
    drawKeyboardPreview();
  }
}

void backspaceChar() {
  int len = strlen(textEntryBuffer);
  if (len > 0) {
    textEntryBuffer[len - 1] = '\0';
    drawKeyboardPreview();
  }
}

void handleKeyPress(const char* label) {
  if (strcmp(label, "DEL") == 0)        { backspaceChar(); return; }
  if (strcmp(label, "SPACE") == 0)      { appendChar(' '); return; }
  if (strcmp(label, "aA") == 0)         { kbLayer = KB_LAYER_LOWER; redrawKeyboardKeys(); return; }
  if (strcmp(label, "Aa") == 0)         { kbLayer = KB_LAYER_UPPER; redrawKeyboardKeys(); return; }
  if (strcmp(label, "?123") == 0)       { kbLayer = KB_LAYER_SYM;   redrawKeyboardKeys(); return; }
  if (strcmp(label, "ABC") == 0)        { kbLayer = KB_LAYER_UPPER; redrawKeyboardKeys(); return; }
  appendChar(label[0]);
}

void commitTextEntry() {
  switch (textEntryTarget) {
    case TE_WIFI_SSID:
      strncpy(wifiSSID, textEntryBuffer, sizeof(wifiSSID));
      wifiSSID[sizeof(wifiSSID) - 1] = '\0';
      saveWiFiCredentials();
      break;
    case TE_WIFI_PASS:
      strncpy(wifiPass, textEntryBuffer, sizeof(wifiPass));
      wifiPass[sizeof(wifiPass) - 1] = '\0';
      saveWiFiCredentials();
      break;
    case TE_UPI_MERCHANT_ID:
      strncpy(phonepeMerchantId, textEntryBuffer, sizeof(phonepeMerchantId));
      phonepeMerchantId[sizeof(phonepeMerchantId) - 1] = '\0';
      saveUPISettings();
      break;
    case TE_UPI_STORE_ID:
      strncpy(phonepeStoreId, textEntryBuffer, sizeof(phonepeStoreId));
      phonepeStoreId[sizeof(phonepeStoreId) - 1] = '\0';
      saveUPISettings();
      break;
    case TE_MACHINE_ID:
      strncpy(machineId, textEntryBuffer, sizeof(machineId));
      machineId[sizeof(machineId) - 1] = '\0';
      saveMachineId();
      break;
    case TE_MACHINE_NAME:
      strncpy(machineName, textEntryBuffer, sizeof(machineName));
      machineName[sizeof(machineName) - 1] = '\0';
      saveMachineName();
      break;
    case TE_GMAIL_USER:
      strncpy(gmailUser, textEntryBuffer, sizeof(gmailUser));
      gmailUser[sizeof(gmailUser) - 1] = '\0';
      saveGmailSettings();
      break;
    case TE_GMAIL_PASS: {
      // Google displays app passwords as four space-separated groups; strip
      // the spaces so it works whether or not they were typed in.
      String cleaned = String(textEntryBuffer);
      cleaned.replace(" ", "");
      cleaned.toCharArray(gmailPass, sizeof(gmailPass));
      saveGmailSettings();
      break;
    }
    case TE_REPORT_TO:
      strncpy(reportTo, textEntryBuffer, sizeof(reportTo));
      reportTo[sizeof(reportTo) - 1] = '\0';
      saveEmailSettings();
      break;
    // Rejected rather than stored if it isn't a valid HH:MM, so a typo can't
    // silently disable the nightly email. parseScheduleTime() returns -1 for
    // anything malformed; an empty string is a deliberate "off".
    case TE_NIGHTLY_1:
    case TE_NIGHTLY_2: {
      char* target = (textEntryTarget == TE_NIGHTLY_1) ? nightlyTime1 : nightlyTime2;
      if (strlen(textEntryBuffer) == 0) {
        target[0] = '\0';
      } else if (parseScheduleTime(textEntryBuffer) >= 0) {
        strncpy(target, textEntryBuffer, sizeof(nightlyTime1));
        target[sizeof(nightlyTime1) - 1] = '\0';
      } else {
        Serial.printf("Schedule: ignoring invalid time [%s]\n", textEntryBuffer);
        return;   // leave the old value in place
      }
      saveScheduleSettings();
      break;
    }
    case TE_RFID_CARD_MANUAL:
      if (strlen(textEntryBuffer) > 0) {
        bool added = addRFIDCard(textEntryBuffer);
        snprintf(rfidCardsMsg, sizeof(rfidCardsMsg), added ? "Card added" : "Already registered");
        rfidCardsMsgColor = added ? COL_SUCCESS : COL_WARNING;
        rfidCardsMsgUntil = millis() + 2000;
      }
      break;
    case TE_RFID_CARD_NAME:
      setRFIDCardName(rfidEditingCard, textEntryBuffer);
      break;
    // Rejected (old PIN kept, same "ignore and log" precedent as the nightly
    // times and the clock above) unless it's 4-8 digits — nothing else can
    // ever be typed back in on the login screen's numeric pad, so accepting
    // anything looser here would let an admin lock themselves out.
    case TE_ADMIN_PIN: {
      int len = strlen(textEntryBuffer);
      bool digitsOnly = (len >= 4);
      for (int k = 0; k < len && digitsOnly; k++) {
        if (textEntryBuffer[k] < '0' || textEntryBuffer[k] > '9') digitsOnly = false;
      }
      if (!digitsOnly) {
        Serial.printf("Admin PIN: ignoring invalid entry [%s] (need 4-8 digits)\n", textEntryBuffer);
        return;   // leave the old PIN in place
      }
      strncpy(adminPin, textEntryBuffer, sizeof(adminPin));
      adminPin[sizeof(adminPin) - 1] = '\0';
      saveAdminPin();
      break;
    }
    default:
      strncpy(productNames[adminEditingSlot], textEntryBuffer, sizeof(productNames[0]));
      productNames[adminEditingSlot][sizeof(productNames[0]) - 1] = '\0';
      saveNameSlot(adminEditingSlot);
      break;
  }
}

void handleAdminTextEntryScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (pointInRect(sx, sy, TE_CANCEL_X, TE_BTN_Y, TE_BTN_W, TE_BTN_H)) {
        returnFromTextEntry();
        return;
      }
      if (textEntryMask && pointInRect(sx, sy, TE_SHOW_X, TE_BTN_Y, TE_BTN_W, TE_BTN_H)) {
        textEntryShow = !textEntryShow;
        drawAdminTextEntryScreen();
        return;
      }
      if (pointInRect(sx, sy, TE_DONE_X, TE_BTN_Y, TE_BTN_W, TE_BTN_H)) {
        commitTextEntry();
        returnFromTextEntry();
        return;
      }

      for (int row = 0; row < 4; row++) {
        const KeyDef* r = kbRow(row);
        for (int k = 0; r[k].label != NULL; k++) {
          int x, y, w, h;
          getKeyRect(row, k, x, y, w, h);
          if (pointInRect(sx, sy, x, y, w, h)) {
            handleKeyPress(r[k].label);
            return;
          }
        }
      }
    }
  }
}
