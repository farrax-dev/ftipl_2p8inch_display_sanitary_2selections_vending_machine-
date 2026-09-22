// =====================================================
//   GMAIL SMTP SENDER (WiFi and 4G)
// =====================================================
// Speaks SMTP to smtp.gmail.com:465 directly, over whichever link is up.
//
// The cellular path is the interesting one. This modem's firmware has no SMTP
// engine — "AT+SMTPSRV=?" answers ERROR — and its AT+HTTP engine only speaks
// HTTP, so for a while Gmail looked impossible on 4G. It isn't: TinyGsmClient
// exposes a plain TCP socket through the modem, and ESP_SSLClient does the TLS
// handshake in software on the ESP32. The modem is then only carrying bytes,
// and the ESP32 speaks SMTP itself. Port 465 is implicit TLS, so the
// connection is encrypted from the first byte with no STARTTLS step.
//
// Both transports hand this code a plain Client&, so there is one SMTP
// implementation rather than one per link.
//
// ESP_MAIL_CLIENT would also do this job but adds well over 100KB of flash,
// and this build already shares a 1.2MB app partition with the display driver,
// TinyGSM, ArduinoJson and the QR encoder.

const char* GMAIL_SMTP_HOST = "smtp.gmail.com";
const int   GMAIL_SMTP_PORT = 465;

// Wraps the modem's socket in TLS. A global because ESP_SSLClient carries
// sizeable buffers — allocating one on the stack per send would be asking for
// trouble on a device with this much else resident.
ESP_SSLClient lteSSLClient;

// Reads one SMTP reply. Replies can span several lines — continuation lines
// put '-' in the fourth character ("250-STARTTLS"), the final one a space
// ("250 OK") — so this reads until it sees a space there. Returns the leading
// 3-digit status, or -1 on timeout.
int gmailReadReply(Client &client, String &out, unsigned long timeoutMs) {
  out = "";
  unsigned long lastByte = millis();
  String line = "";

  while (millis() - lastByte < timeoutMs) {
    while (client.available()) {
      char c = (char)client.read();
      lastByte = millis();
      out += c;
      if (c == '\n') {
        if (line.length() >= 4 && line[3] == ' ') return line.substring(0, 3).toInt();
        line = "";
      } else if (c != '\r') {
        line += c;
      }
    }
    delay(10);
  }
  return -1;
}

// Sends one command and checks the reply against what SMTP should answer.
// Logs both on mismatch — a failed send is nearly always one specific step,
// and knowing which one is the difference between a fix and a guess.
bool gmailCmd(Client &client, const String &cmd, int expectCode,
              const char* what, bool secret) {
  if (cmd.length()) {
    client.print(cmd);
    client.print("\r\n");
  }

  String reply;
  int code = gmailReadReply(client, reply, 20000);
  if (code == expectCode) return true;

  reply.trim();
  Serial.printf("Gmail: %s expected %d, got %d [%s]%s\n",
                what, expectCode, code, reply.c_str(),
                secret ? " (credential step)" : "");
  return false;
}

// Streams the RFC 5322 message straight to the socket rather than building it
// in a String first. A 60-day report plus a base64 attachment can run to tens
// of KB, and holding that alongside the HTML and CSV it was built from is more
// heap than this device should be asked for.
void gmailWriteMessage(Client &client, const String &subject, const String &html,
                       const String &csv, const String &csvName) {
  const String boundary = "----vend8f3a2c1b9d";

  client.print("From: " + String(machineId) + " <" + String(gmailUser) + ">\r\n");
  client.print("To: " + String(reportTo) + "\r\n");
  client.print("Subject: " + subject + "\r\n");
  client.print("MIME-Version: 1.0\r\n");

  if (csv.length() == 0) {
    client.print("Content-Type: text/html; charset=UTF-8\r\n\r\n");
    client.print(html);
    client.print("\r\n");
    return;
  }

  client.print("Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n\r\n");
  client.print("--" + boundary + "\r\n");
  client.print("Content-Type: text/html; charset=UTF-8\r\n\r\n");
  client.print(html);
  client.print("\r\n--" + boundary + "\r\n");
  client.print("Content-Type: text/csv; name=\"" + csvName + "\"\r\n");
  client.print("Content-Disposition: attachment; filename=\"" + csvName + "\"\r\n");
  client.print("Content-Transfer-Encoding: base64\r\n\r\n");

  // Base64 wrapped at 76 characters, as MIME requires — some clients choke on
  // one unbroken line. Encoded in slices so the whole encoded copy never
  // exists at once: 57 raw bytes is exactly 76 base64 characters.
  for (size_t i = 0; i < csv.length(); i += 57) {
    String chunk = csv.substring(i, min(i + 57, csv.length()));
    client.print(base64::encode(chunk));
    client.print("\r\n");
  }
  client.print("--" + boundary + "--\r\n");
}

// The SMTP conversation itself, against whichever Client it's handed.
// Returns 250 on success, 0 if the server refused a step, -1 if the
// connection never got established.
int gmailSendVia(Client &client, const char* linkName, const String &subject,
                 const String &html, const String &csv, const String &csvName) {
  Serial.printf("Gmail: connecting to %s:%d over %s (heap %u)\n",
                GMAIL_SMTP_HOST, GMAIL_SMTP_PORT, linkName, ESP.getFreeHeap());

  if (!client.connect(GMAIL_SMTP_HOST, GMAIL_SMTP_PORT)) {
    Serial.println("Gmail: TCP/TLS connect failed");
    return -1;
  }

  String greeting;
  if (gmailReadReply(client, greeting, 20000) != 220) {
    greeting.trim();
    Serial.printf("Gmail: bad greeting [%s]\n", greeting.c_str());
    client.stop();
    return -1;
  }

  bool ok =
    gmailCmd(client, "EHLO vendingmachine", 250, "EHLO", false) &&
    gmailCmd(client, "AUTH LOGIN", 334, "AUTH LOGIN", false) &&
    // Username and password go base64-encoded, each answered separately.
    gmailCmd(client, base64::encode(String(gmailUser)), 334, "username", true) &&
    gmailCmd(client, base64::encode(String(gmailPass)), 235, "app password", true) &&
    gmailCmd(client, "MAIL FROM:<" + String(gmailUser) + ">", 250, "MAIL FROM", false) &&
    gmailCmd(client, "RCPT TO:<" + String(reportTo) + ">", 250, "RCPT TO", false) &&
    gmailCmd(client, "DATA", 354, "DATA", false);

  if (!ok) {
    client.print("QUIT\r\n");
    client.stop();
    return 0;
  }

  gmailWriteMessage(client, subject, html, csv, csvName);

  // A lone "." on its own line ends the body. Gmail can take a while to
  // accept a large message, hence the generous wait inside gmailCmd().
  bool sent = gmailCmd(client, "\r\n.", 250, "end of DATA", false);
  client.print("QUIT\r\n");
  client.stop();

  Serial.printf("Gmail: %s\n", sent ? "message accepted" : "message rejected");
  return sent ? 250 : 0;
}

// Picks the transport: WiFi's native TLS when it's up, otherwise software TLS
// over the modem socket.
int gmailSend(const String &subject, const String &html,
              const String &csv, const String &csvName) {
  if (strlen(gmailUser) == 0 || strlen(gmailPass) == 0) return -1;

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();     // no CA bundle on the device, as with the UPI path
    client.setTimeout(20000);
    return gmailSendVia(client, "WiFi", subject, html, csv, csvName);
  }

  if (!connectLTEIfNeeded()) return -1;

  lteSSLClient.setClient(&lteClient);
  lteSSLClient.setInsecure();
  // Modest buffers: enough for TLS records without eating the heap the
  // report builders need.
  lteSSLClient.setBufferSizes(2048, 1024);
  lteSSLClient.setDebugLevel(0);
  return gmailSendVia(lteSSLClient, "4G", subject, html, csv, csvName);
}
