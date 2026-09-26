// =====================================================
//         PAYMENT - UPI (Dynamic QR using Admin variables)
// =====================================================
enum UPIStage { UPI_STAGE_INIT, UPI_STAGE_WAITING, UPI_STAGE_SUCCESS, UPI_STAGE_TIMEOUT, UPI_STAGE_ERROR };
UPIStage upiStage = UPI_STAGE_INIT;

char upiTransactionId[24] = "";
char upiQRText[200] = "";
char upiErrorMsg[48] = "";
unsigned long upiStartTime = 0;
unsigned long upiLastPollTime = 0;
unsigned long upiSuccessEnteredTime = 0;
unsigned long upiLastStatusLineUpdate = 0;

const int UPI_QR_VERSION   = 11;
const int UPI_QR_MODULE_PX = 2;
// 40, not the old 34: the QR paints a 6px white quiet zone above itself, and
// at 34 that would have started on top of the header rule (which ends at
// HDR_BOTTOM=32, Core_08_UIHelpers.ino). A version-11 code is 61 modules, so
// the block still ends at 162 and leaves the status line and Cancel button
// below it untouched.
const int UPI_QR_TOP_Y     = 40;
int upiQRSize = 0;

String computePhonePeChecksum(const String &base64Body, const String &endpointPath) {
  String toHash = base64Body + endpointPath + String(phonepeSaltKey);
  unsigned char hash[32];
  mbedtls_sha256((const unsigned char*)toHash.c_str(), toHash.length(), hash, 0);
  char hex[65];
  for (int i = 0; i < 32; i++) sprintf(hex + i * 2, "%02x", hash[i]);
  hex[64] = '\0';
  return String(hex) + "###" + String(phonepeSaltIndex);
}

void generateUPITransactionId() {
  snprintf(upiTransactionId, sizeof(upiTransactionId), "VEND%08X", (unsigned int)esp_random());
}

// Picks whichever transport is actually up — WiFi (preferred, via the normal
// HTTPClient) or the LTE modem (Core_13_LTEModem.ino's hand-rolled HTTPS,
// since neither HTTPClient nor ArduinoHttpClient can reach an HTTPS API
// through TinyGsmClient) — so initiateUPIPayment()/checkUPIPaymentStatus()
// don't need to know which one is active. Returns false only on a
// transport-level failure (no connectivity, couldn't connect at all).
bool sendPhonePeHTTPS(const String &method, const String &path, const String &checksum,
                      const String &body, int &outCode, String &outResponse) {
  outCode = -1;
  outResponse = "";

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, String(phonepeBaseUrl) + path);
    if (body.length() > 0) http.addHeader("Content-Type", "application/json");
    http.addHeader("x-verify", checksum);
    http.addHeader("X-PROVIDER-ID", phonepeProviderId);

    outCode = (method == "POST") ? http.POST(body) : http.GET();
    outResponse = http.getString();
    http.end();
    return true;
  }

  if (isLTEConnected()) {
    // Content-Type is set separately by httpsRequestLTE() itself (via the
    // dedicated AT+HTTPPARA="CONTENT",... parameter) whenever there's a
    // body — don't also add it here, or it ends up declared twice in the
    // actual request.
    String headers = "x-verify: " + checksum + "\r\n";
    headers += "X-PROVIDER-ID: " + String(phonepeProviderId) + "\r\n";
    return httpsRequestLTE(phonepeBaseUrl, path, method, headers, body, outCode, outResponse);
  }

  return false;  // no connectivity at all
}

void initiateUPIPayment() {
  generateUPITransactionId();
  upiErrorMsg[0] = '\0';

  bool haveConnectivity = (WiFi.status() == WL_CONNECTED) || isLTEConnected();
  if (!haveConnectivity && strlen(wifiSSID) > 0) haveConnectivity = connectWiFiIfNeeded();
  if (!haveConnectivity) haveConnectivity = connectLTEIfNeeded();

  if (!haveConnectivity) {
    upiStage = UPI_STAGE_ERROR;
    strncpy(upiErrorMsg, "No internet (WiFi and 4G both down)", sizeof(upiErrorMsg) - 1);
    return;
  }

  StaticJsonDocument<256> innerDoc;
  innerDoc["merchantId"] = phonepeMerchantId;
  innerDoc["transactionId"] = upiTransactionId;
  innerDoc["merchantOrderId"] = upiTransactionId;
  innerDoc["amount"] = orderTotal * 100;
  innerDoc["expiresIn"] = upiTimeoutMs / 1000;
  innerDoc["storeId"] = phonepeStoreId;
  innerDoc["terminalId"] = phonepeTerminalId;
  String innerJson;
  serializeJson(innerDoc, innerJson);

  String base64Body = base64::encode(innerJson);
  String checksum = computePhonePeChecksum(base64Body, "/v3/qr/init");

  StaticJsonDocument<256> outerDoc;
  outerDoc["request"] = base64Body;
  String outerJson;
  serializeJson(outerDoc, outerJson);

  int httpCode;
  String response;
  if (!sendPhonePeHTTPS("POST", "/v3/qr/init", checksum, outerJson, httpCode, response)) {
    upiStage = UPI_STAGE_ERROR;
    strncpy(upiErrorMsg, "Connection failed (WiFi/4G)", sizeof(upiErrorMsg) - 1);
    return;
  }
  if (httpCode != 200) {
    upiStage = UPI_STAGE_ERROR;
    snprintf(upiErrorMsg, sizeof(upiErrorMsg), "API error (code %d)", httpCode);
    Serial.printf("UPI init: HTTP %d, body: %s\n", httpCode, response.c_str());
    return;
  }

  StaticJsonDocument<512> respDoc;
  DeserializationError parseErr = deserializeJson(respDoc, response);
  if (parseErr || !respDoc["success"].as<bool>()) {
    upiStage = UPI_STAGE_ERROR;
    strncpy(upiErrorMsg, "Invalid response from PhonePe", sizeof(upiErrorMsg) - 1);
    Serial.printf("UPI init: HTTP 200 but parse failed (%s), raw body (%d bytes): [%s]\n",
                  parseErr.c_str(), response.length(), response.c_str());
    return;
  }

  const char* qrStr = respDoc["data"]["qrString"];
  if (!qrStr) {
    upiStage = UPI_STAGE_ERROR;
    strncpy(upiErrorMsg, "No QR data in response", sizeof(upiErrorMsg) - 1);
    return;
  }

  strncpy(upiQRText, qrStr, sizeof(upiQRText) - 1);
  upiQRText[sizeof(upiQRText) - 1] = '\0';

  upiStage = UPI_STAGE_WAITING;
  upiStartTime = millis();
  upiLastPollTime = millis();
}

bool checkUPIPaymentStatus() {
  if (WiFi.status() != WL_CONNECTED && !isLTEConnected()) return false;

  String path = "/v3/transaction/" + String(phonepeMerchantId) + "/" + String(upiTransactionId) + "/status";
  String checksum = computePhonePeChecksum("", path);

  int httpCode;
  String response;
  if (!sendPhonePeHTTPS("GET", path, checksum, "", httpCode, response)) return false;
  if (httpCode != 200) {
    Serial.printf("UPI status check: HTTP %d, body: %s\n", httpCode, response.c_str());
    return false;
  }

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, response)) return false;

  const char* state = doc["data"]["paymentState"];
  return state && strcmp(state, "COMPLETED") == 0;
}

void upiQRDisplay(esp_qrcode_handle_t qr) {
  int size = esp_qrcode_get_size(qr);
  upiQRSize = size;

  int qrPixelSize = size * UPI_QR_MODULE_PX;
  int qx = (tft.width() - qrPixelSize) / 2;
  int qy = UPI_QR_TOP_Y;

  tft.fillRect(qx - 6, qy - 6, qrPixelSize + 12, qrPixelSize + 12, ILI9341_WHITE);
  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      if (esp_qrcode_get_module(qr, x, y)) {
        tft.fillRect(qx + x * UPI_QR_MODULE_PX, qy + y * UPI_QR_MODULE_PX,
                     UPI_QR_MODULE_PX, UPI_QR_MODULE_PX, ILI9341_BLACK);
      }
    }
  }
}

void drawUPIWaitingScreen() {
  drawGradientBackground();

  drawScreenTitle("Scan to Pay");

  esp_qrcode_config_t qrCfg = ESP_QRCODE_CONFIG_DEFAULT();
  qrCfg.display_func = upiQRDisplay;
  qrCfg.max_qrcode_version = UPI_QR_VERSION;

  upiQRSize = 0;
  esp_qrcode_generate(&qrCfg, upiQRText);

  if (upiQRSize == 0) {
    tft.setTextSize(2);
    tft.setTextColor(COL_DANGER, COL_BG_BOTTOM);
    centerText("QR generation failed", 90);
  }

  drawUPIStatusLine(UPI_QR_TOP_Y + upiQRSize * UPI_QR_MODULE_PX + 12);
  drawBackButton("< Cancel");
}

void drawUPIStatusLine(int y) {
  tft.fillRect(0, y - 2, tft.width(), 16, COL_BG_BOTTOM);
  int remainingSec = (int)((upiTimeoutMs - (millis() - upiStartTime)) / 1000);
  if (remainingSec < 0) remainingSec = 0;
  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  char buf[40];
  snprintf(buf, sizeof(buf), "Rs %d - waiting for payment (%ds)", orderTotal, remainingSec);
  centerText(buf, y);
}

bool upiVendPending = false;
// Set once dispenseCart() returns, from its drop-sensor verification —
// separate from upiVendPending because the payment itself already succeeded
// (money moved) by the time this is known, so the screen keeps saying
// "Payment Successful!" either way and only the closing line changes.
bool upiDispenseFailed = false;

void drawUPISuccessScreen() {
  drawGradientBackground();

  tft.setTextSize(3);
  tft.setTextColor(COL_SUCCESS, COL_BG_BOTTOM);
  centerText("Payment", 70);
  centerText("Successful!", 100);

  tft.setTextSize(2);
  tft.setTextColor(COL_ACCENT, COL_BG_BOTTOM);
  char buf[24];
  snprintf(buf, sizeof(buf), "Rs %d", orderTotal);
  centerText(buf, 140);

  tft.setTextSize(1);
  if (upiVendPending) {
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    centerText("Preparing your item...", 165);
  } else if (upiDispenseFailed) {
    tft.setTextColor(COL_DANGER, COL_BG_BOTTOM);
    centerText("Dispense failed - contact support", 165);
    drawProceedButton("Done");
  } else {
    tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
    centerText("Please collect your item", 165);
    drawProceedButton("Done");
  }
}

void drawUPITimeoutScreen() {
  drawGradientBackground();

  tft.setTextSize(2);
  tft.setTextColor(COL_DANGER, COL_BG_BOTTOM);
  centerText("Payment Timed Out", 70);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  centerText("No payment was received in time.", 105);
  centerText("You can try again or go back.", 122);

  drawBackButton("< Back");
  drawProceedButton("Retry");
}

void drawUPIErrorScreen() {
  drawGradientBackground();

  tft.setTextSize(2);
  tft.setTextColor(COL_DANGER, COL_BG_BOTTOM);
  centerText("Payment Error", 70);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM, COL_BG_BOTTOM);
  centerText(upiErrorMsg, 105);

  drawBackButton("< Back");
  drawProceedButton("Retry");
}

void drawUPICurrentStage() {
  switch (upiStage) {
    case UPI_STAGE_WAITING: drawUPIWaitingScreen(); break;
    case UPI_STAGE_SUCCESS: drawUPISuccessScreen(); break;
    case UPI_STAGE_TIMEOUT: drawUPITimeoutScreen(); break;
    case UPI_STAGE_ERROR:   drawUPIErrorScreen(); break;
    default: break;
  }
}

void drawPaymentUPIScreen() {
  upiStage = UPI_STAGE_INIT;
  upiDispenseFailed = false;
  initiateUPIPayment();
  drawUPICurrentStage();
}

void handlePaymentUPIScreen() {
  if (upiStage == UPI_STAGE_WAITING) {
    if (millis() - upiStartTime > upiTimeoutMs) {
      upiStage = UPI_STAGE_TIMEOUT;
      drawUPICurrentStage();
    } else {
      if (millis() - upiLastStatusLineUpdate > 1000) {
        upiLastStatusLineUpdate = millis();
        drawUPIStatusLine(UPI_QR_TOP_Y + upiQRSize * UPI_QR_MODULE_PX + 12);
      }
      // Interval measured from when the last request FINISHED. Stamping it
      // before the call meant a 15-second cellular request had already
      // exceeded the 4-second interval by the time it returned, so the next
      // poll fired immediately and the screen never got a frame to redraw or
      // a chance to read the Cancel button.
      unsigned long pollGap = (WiFi.status() == WL_CONNECTED)
                                ? UPI_POLL_INTERVAL_MS : UPI_POLL_INTERVAL_LTE_MS;
      if (millis() - upiLastPollTime > pollGap) {
        if (checkUPIPaymentStatus()) {
          upiStage = UPI_STAGE_SUCCESS;
          upiVendPending = true;
          drawUPISuccessScreen();
          delay(UPI_SUCCESS_HOLD_MS);

          upiDispenseFailed = !dispenseCart("UPI", false);

          upiVendPending = false;
          drawUPICurrentStage();
          upiSuccessEnteredTime = millis();
        }
        upiLastPollTime = millis();
      }
    }
  } else if (upiStage == UPI_STAGE_SUCCESS) {
    if (millis() - upiSuccessEnteredTime > 8000) {
      resetCart();
      currentScreen = SCREEN_WELCOME;
      drawWelcomeScreen();
      return;
    }
  }

  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (upiStage == UPI_STAGE_WAITING) {
        if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
          currentScreen = SCREEN_PAYMENT_METHOD;
          drawPaymentMethodScreen();
        }
      } else if (upiStage == UPI_STAGE_SUCCESS) {
        if (pointInRect(sx, sy, BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H)) {
          resetCart();
          currentScreen = SCREEN_WELCOME;
          drawWelcomeScreen();
        }
      } else if (upiStage == UPI_STAGE_TIMEOUT || upiStage == UPI_STAGE_ERROR) {
        if (pointInRect(sx, sy, BTN_BACK_X, BTN_Y, BTN_BACK_W, BTN_H)) {
          currentScreen = SCREEN_PAYMENT_METHOD;
          drawPaymentMethodScreen();
        }
        if (pointInRect(sx, sy, BTN_PROCEED_X, BTN_Y, BTN_PROCEED_W, BTN_H)) {
          drawPaymentUPIScreen();
        }
      }
    }
  }
}
