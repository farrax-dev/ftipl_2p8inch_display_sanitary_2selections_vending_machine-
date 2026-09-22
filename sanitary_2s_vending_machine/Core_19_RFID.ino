// =====================================================
//        RFID READER (UART1) — CARD-GATED FREE VEND
// =====================================================
// Not a "payment" method in the money sense — it's an access-control check.
// A tap authorises a dispense for free (see dispenseCart()'s freeVend flag
// in Core_11_Dispense.ino); anyone whose card isn't in the registered list
// is refused. Registration itself (scan-to-add or type-the-UID-in) is
// Screen_18_AdminRFIDCards.ino; the registered list is stored and matched in
// Core_09_Storage.ino (isCardRegistered() etc).
//
// Wiring is GPIO22 (ESP32 TX -> module RXD) / GPIO34 (ESP32 RX <- module
// TXD). RX was originally GPIO36 (the "VP" pin) but was swapped with the
// coin acceptor's GPIO34 (Screen_06_PaymentOther.ino) for wiring
// convenience — both are equally suitable input-only, no-internal-pull-
// resistor pins, so the swap needed no other changes on this end. See
// docs/wiring-diagram.html, which reflects this pin plan.
//
// Frame protocol below is transcribed from the customer's own working PIC
// (PIC18F8722) firmware for this exact reader family — not a guess. That
// reference only ever uses two commands, so that's all that's implemented
// here; anything beyond "is a card present" / "read its UID" (writing a
// card, changing reader settings, etc.) would need its own command byte from
// the same source or the module's datasheet.
//
// Frame shape, both directions: AA BB LEN CMD [DATA...] CKSUM
//   LEN   = 2 + number of DATA bytes (i.e. CMD + DATA + CKSUM, counted together)
//   CKSUM = XOR (BCC) of LEN, CMD, and every DATA byte (not including itself)
// Verified against the reference firmware's two literal command frames:
//   check-card: AA BB 02 19 1B   (LEN=2, CMD=0x19, no data, CKSUM=02^19=1B)
//   read-card:  AA BB 02 20 22   (LEN=2, CMD=0x20, no data, CKSUM=02^20=22)
// and its reply frame for a successful read:
//   AA BB 06 20 <uid0> <uid1> <uid2> <uid3> <cksum>
// (Both example CKSUMs above come out the same whether you sum or XOR the
// bytes, because LEN/CMD never share a set bit — that's a coincidence of
// these two specific commands, not evidence the checksum is a sum. A UID
// reply carries four more data bytes that usually do share bits with LEN
// and with each other, so a sum-based check silently rejects real, correct
// replies; XOR is what the reader itself computes and is what must be used
// to validate one.)
//
// Any byte equal to 0xAA that occurs after the header (LEN, CMD, a DATA
// byte, or CKSUM) is followed on the wire by an extra 0x00 that does not
// count toward LEN and must be stripped back out on receive. The AA BB
// header itself is never stuffed this way.
const int RFID_TX_PIN = 22;
const int RFID_RX_PIN = 34;
const unsigned long RFID_BAUD = 19200;  // confirmed by the reference firmware's #use RS232(BAUD=19200, PARITY=N)

const uint8_t RFID_CMD_CHECK_CARD = 0x19;  // "is a card present?" -> LEN 0x04 reply means yes
const uint8_t RFID_CMD_READ_CARD  = 0x20;  // "read its UID" -> AA BB 06 20 <4-byte UID> CKSUM

// The reference firmware paces 5ms between transmitted bytes and gives the
// reader 100-300ms to answer each command; matched here rather than
// tightened, since that pacing is what's actually been proven to work
// against this hardware.
const unsigned long RFID_INTERBYTE_DELAY_MS = 5;
const unsigned long RFID_FRAME_TIMEOUT_MS = 300;

// Only asks the reader once per interval rather than on every loop() tick —
// each ask can block for up to RFID_FRAME_TIMEOUT_MS, and hammering it every
// ~20ms the way a naive poll would is exactly the "burns the whole tick
// waiting for an answer" mistake documented in Core_13_LTEModem.ino's
// isLTEConnected() history. 500ms is imperceptible for a customer holding a
// card to the reader.
unsigned long rfidLastPollMs = 0;
const unsigned long RFID_POLL_INTERVAL_MS = 500;

// Temporary wiring/protocol diagnostics — set to 0 once the reader is
// confirmed working, to stop spamming Serial on every 500ms poll.
#define RFID_DEBUG 1

void rfidPrintHex(const char* label, const uint8_t* b, int n) {
#if RFID_DEBUG
  Serial.print(label);
  for (int i = 0; i < n; i++) {
    if (b[i] < 0x10) Serial.print('0');
    Serial.print(b[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
#endif
}

void initRFID() {
  // The reader uses inverted RS232 line levels (idles LOW, not HIGH) rather
  // than plain TTL UART. Without the trailing `true` here every byte decodes
  // as garbage no matter the baud rate — this was the whole reader's silent
  // failure mode before it was added.
  Serial1.begin(RFID_BAUD, SERIAL_8N1, RFID_RX_PIN, RFID_TX_PIN, true);
#if RFID_DEBUG
  Serial.printf("RFID: UART1 up, RX=GPIO%d TX=GPIO%d @ %lu baud, inverted\n",
                RFID_RX_PIN, RFID_TX_PIN, RFID_BAUD);
#endif
}

// Writes one frame byte, then (if it's a byte that can appear after the
// header) stuffs an extra 0x00 after any literal 0xAA so the reader doesn't
// mistake it for the start of a new frame.
void rfidSendByte(uint8_t b, bool stuffable) {
  delay(RFID_INTERBYTE_DELAY_MS);
  Serial1.write(b);
  if (stuffable && b == 0xAA) {
    delay(RFID_INTERBYTE_DELAY_MS);
    Serial1.write((uint8_t)0x00);
  }
}

void rfidSendFrame(uint8_t cmd, const uint8_t* data, uint8_t dataLen) {
  uint8_t len = 2 + dataLen;
  uint8_t checksum = (uint8_t)(len ^ cmd);
  for (uint8_t i = 0; i < dataLen; i++) checksum ^= data[i];

#if RFID_DEBUG
  uint8_t dbg[16];
  int dn = 0;
  dbg[dn++] = 0xAA; dbg[dn++] = 0xBB; dbg[dn++] = len; dbg[dn++] = cmd;
  for (uint8_t i = 0; i < dataLen; i++) dbg[dn++] = data[i];
  dbg[dn++] = checksum;
  rfidPrintHex("  RFID TX: ", dbg, dn);
#endif

  rfidSendByte(0xAA, false);
  rfidSendByte(0xBB, false);
  rfidSendByte(len, true);
  rfidSendByte(cmd, true);
  for (uint8_t i = 0; i < dataLen; i++) rfidSendByte(data[i], true);
  rfidSendByte(checksum, true);
}

// Reads one AA/BB-framed reply into buf (capacity bufLen) within timeoutMs.
// Resyncs on stray bytes ahead of a real header instead of assuming the
// first byte seen is always AA. Returns the number of bytes captured, 0 on
// timeout before a complete frame arrived.
int rfidReadFrame(uint8_t* buf, int bufLen, unsigned long timeoutMs) {
  int n = 0;
  int rawSeen = 0;  // every byte read off the wire, header sync or not — diagnostic only
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    while (Serial1.available()) {
      uint8_t b = (uint8_t)Serial1.read();
      rawSeen++;
      if (n == 0 && b != 0xAA) continue;
      if (n == 1 && b != 0xBB) { n = 0; continue; }
      if (n >= 2 && b == 0xAA) {
        // Stuffed byte: a literal 0xAA past the header is always followed
        // by a filler 0x00 that isn't part of the frame — wait briefly for
        // it and drop it so buf holds only real frame bytes.
        unsigned long stuffDeadline = millis() + 20;
        while (!Serial1.available() && millis() < stuffDeadline) {}
        if (Serial1.available()) { Serial1.read(); rawSeen++; }
      }
      if (n < bufLen) buf[n++] = b;
      if (n >= 3) {
        int expected = buf[2] + 3;  // header(2) + LEN byte(1) + LEN more bytes
        if (n >= expected) {
          rfidPrintHex("  RFID RX: ", buf, n);
          return n;
        }
      }
    }
  }
#if RFID_DEBUG
  Serial.printf("  RFID RX: timed out after %lums, %d raw byte(s) seen on GPIO%d\n",
                timeoutMs, rawSeen, RFID_RX_PIN);
#endif
  return 0;
}

bool rfidChecksumOk(const uint8_t* buf, int n) {
  if (n < 5) return false;  // shortest real frame (0 data bytes) is 5 bytes
  uint8_t len = buf[2];
  if (n < len + 3) return false;
  uint8_t bcc = 0;
  for (int i = 2; i < 2 + len; i++) bcc ^= buf[i];
  return bcc == buf[len + 2];
}

// Two-step handshake, exactly matching the reference firmware's own main
// loop: ask if a card is present, and only if so, ask for its UID. Returns
// true and fills uidOut (8 uppercase hex chars, e.g. "04A1B2C3") on a
// verified read; false if no card answered or the reply's checksum didn't
// match (a corrupted frame is treated the same as "no card" rather than
// risking a wrong UID).
bool rfidRequestCardUID(char* uidOut, size_t uidOutLen) {
  if (uidOutLen > 0) uidOut[0] = '\0';

  while (Serial1.available()) Serial1.read();  // drop anything stale before asking

  rfidSendFrame(RFID_CMD_CHECK_CARD, nullptr, 0);
  uint8_t resp[16];
  int n = rfidReadFrame(resp, sizeof(resp), RFID_FRAME_TIMEOUT_MS);
  if (n < 3 || resp[2] != 0x04) return false;  // no card in range right now

  rfidSendFrame(RFID_CMD_READ_CARD, nullptr, 0);
  n = rfidReadFrame(resp, sizeof(resp), RFID_FRAME_TIMEOUT_MS);
  if (n < 9 || resp[2] != 0x06 || resp[3] != RFID_CMD_READ_CARD || !rfidChecksumOk(resp, n)) {
    return false;
  }

  if (uidOutLen < 9) return false;  // 8 hex chars + null
  snprintf(uidOut, uidOutLen, "%02X%02X%02X%02X", resp[4], resp[5], resp[6], resp[7]);
  return true;
}

// Called from the RFID payment/admin screens' poll loops. Throttles actual
// reader traffic to RFID_POLL_INTERVAL_MS — see the comment on that constant.
bool pollRFIDCard(char* uidOut, size_t uidOutLen) {
  if (millis() - rfidLastPollMs < RFID_POLL_INTERVAL_MS) return false;
  rfidLastPollMs = millis();
  return rfidRequestCardUID(uidOut, uidOutLen);
}
