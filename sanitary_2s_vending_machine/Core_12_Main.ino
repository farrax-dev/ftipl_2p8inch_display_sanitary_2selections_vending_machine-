void setup() {
  Serial.begin(115200);
  initMotorPins();
  initCoinAcceptor();
  initRTC();
  initRFID();
  initDropSensor();

  // CFG_LTE_ENABLED specifically, not lteEnabled: this runs before
  // loadPersistedProductData() below has loaded lteEnabled's real value from
  // NVS, and hardware bring-up belongs to "is a modem fitted" (the compile-
  // time macro), not "is the admin currently choosing to use it" (the
  // runtime flag Admin > 4G/LTE Setup's toggle controls). Bringing the UART
  // up whenever a modem is fitted, regardless of that toggle, is what lets
  // switching it back on take effect immediately with no reboot needed.
  if (CFG_LTE_ENABLED) {
    initLTEModem();
    runLTEATBridgeIfEnabled();  // TEMPORARY diagnostic - see Core_13_LTEModem.ino
  }

  SPI.begin();
  tft.begin(8000000);
  tft.setRotation(1);

  ts.begin();
  loadPersistedProductData();

  // Identifies the build on a bench full of machines. The seeding line printed
  // just above says whether an edited Config.h was picked up on this boot.
  Serial.printf("Machine: %s (%s) | %d motors, %d products\n",
                machineId, strlen(machineName) ? machineName : "unnamed",
                MAX_MOTORS, MAX_PRODUCTS);
  initSalesLog();   // mounts LittleFS; must follow loadPersistedProductData()
                    // since the lifetime totals come from NVS

  if (wifiEnabled && strlen(wifiSSID) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID, wifiPass);
  }

  drawWelcomeScreen();
}

void loop() {
  switch (currentScreen) {
    case SCREEN_WELCOME:
      handleWelcomeScreen();
      break;
    case SCREEN_SELECT:
      handleSelectScreen();
      break;
    case SCREEN_CART_REVIEW:
      handleCartReviewScreen();
      break;
    case SCREEN_PAYMENT_METHOD:
      handlePaymentMethodScreen();
      break;
    case SCREEN_PAYMENT_UPI:
      handlePaymentUPIScreen();
      break;
    case SCREEN_PAYMENT_CASH:
      handlePaymentCashScreen();
      break;
    case SCREEN_PAYMENT_RFID:
      handlePaymentRFIDScreen();
      break;
    case SCREEN_ADMIN_LOGIN:
      handleAdminLoginScreen();
      break;
    case SCREEN_ADMIN_PANEL:
      handleAdminPanelScreen();
      break;
    case SCREEN_ADMIN_EDIT:
      handleAdminEditScreen();
      break;
    case SCREEN_ADMIN_TEXT_ENTRY:
      handleAdminTextEntryScreen();
      break;
    case SCREEN_ADMIN_MOTOR_ASSIGN:
      handleAdminMotorAssignScreen();
      break;
    case SCREEN_ADMIN_MOTOR_STOCK:
      handleAdminMotorStockScreen();
      break;
    case SCREEN_ADMIN_SETTINGS:
      handleAdminSettingsScreen();
      break;
    case SCREEN_ADMIN_WIFI:
      handleAdminWiFiScreen();
      break;
    case SCREEN_ADMIN_UPI:
      handleAdminUPIScreen();
      break;
    case SCREEN_ADMIN_COIN:
      handleAdminCoinScreen();
      break;
    case SCREEN_ADMIN_LTE:
      handleAdminLTEScreen();
      break;
    case SCREEN_ADMIN_REPORT:
      handleAdminReportScreen();
      break;
    case SCREEN_ADMIN_RFID_CARDS:
      handleAdminRFIDCardsScreen();
      break;
    case SCREEN_ADMIN_RFID_CARD_EDIT:
      handleAdminRFIDCardEditScreen();
      break;
    case SCREEN_ADMIN_DATETIME:
      handleAdminDateTimeScreen();
      break;
    case SCREEN_ADMIN_RFID_RESET:
      handleAdminRFIDResetScreen();
      break;
  }

  maintainWiFiIndicator();
  if (wifiEnabled) maintainNTP();
  if (lteEnabled) {
    maintainLTEStatus();     // the only routine polling of the modem
    maintainLTEFallback();
  }
  maintainRTCFallback();     // third clock source, only engages once WiFi/LTE have had a chance
  delay(20);
}
