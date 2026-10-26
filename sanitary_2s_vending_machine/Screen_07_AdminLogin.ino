// =====================================================
//                  ADMIN LOGIN SCREEN
// =====================================================
// The PIN compared against here is adminPin (Core_09_Storage.ino) — a
// persisted, admin-changeable value seeded from Config.h's CFG_ADMIN_PIN,
// not that macro directly. See Core_09_Storage.ino's own comment for the
// change-it-on-screen vs. reset-it-by-reflashing distinction.
//
// Layout: a PIN field across the top (one dot per digit — never the digits
// themselves), a 3x4 keypad of 34px keys below it, Cancel in the header band.
char pinBuffer[9] = "";

void enterAdminLogin() {
  pinBuffer[0] = '\0';
  currentScreen = SCREEN_ADMIN_LOGIN;
  drawAdminLoginScreen();
}

const int LOGIN_CANCEL_W = 68,  LOGIN_CANCEL_H = 22;
const int LOGIN_CANCEL_X = 218, LOGIN_CANCEL_Y = HDR_Y + (HDR_H - LOGIN_CANCEL_H) / 2;

const int LOGIN_PIN_X = 60, LOGIN_PIN_Y = 38, LOGIN_PIN_W = 200, LOGIN_PIN_H = 30;

const int LOGIN_KEY_W = 84, LOGIN_KEY_H = 34, LOGIN_KEY_GX = 6, LOGIN_KEY_GY = 4;
const int LOGIN_KEY_X0 = 28, LOGIN_KEY_Y0 = 74;

const char* const LOGIN_LABELS[12] = {"1","2","3","4","5","6","7","8","9","Clear","0","OK"};

int loginKeyX(int k) { return LOGIN_KEY_X0 + (k % 3) * (LOGIN_KEY_W + LOGIN_KEY_GX); }
int loginKeyY(int k) { return LOGIN_KEY_Y0 + (k / 3) * (LOGIN_KEY_H + LOGIN_KEY_GY); }

// message != NULL replaces the dots (used for "Wrong PIN").
void drawPinField(const char* message, uint16_t messageColor) {
  tft.fillRoundRect(LOGIN_PIN_X, LOGIN_PIN_Y, LOGIN_PIN_W, LOGIN_PIN_H, 8, COL_CARD);
  tft.drawRoundRect(LOGIN_PIN_X, LOGIN_PIN_Y, LOGIN_PIN_W, LOGIN_PIN_H, 8, COL_CARD_BRD);

  int cy = LOGIN_PIN_Y + LOGIN_PIN_H / 2;
  int len = strlen(pinBuffer);
  if (message != NULL) {
    auiPut(message, AL_CENTER, LOGIN_PIN_X + 6, LOGIN_PIN_W - 12, cy, messageColor, FACE_MD);
  } else if (len == 0) {
    auiPut("Enter PIN", AL_CENTER, LOGIN_PIN_X + 6, LOGIN_PIN_W - 12, cy, COL_TEXT_DIM, FACE_MD);
  } else {
    const int pitch = 20;
    int x0 = LOGIN_PIN_X + (LOGIN_PIN_W - (len - 1) * pitch) / 2;
    for (int k = 0; k < len; k++) tft.fillCircle(x0 + k * pitch, cy, 5, COL_ACCENT);
  }
}

void drawPinLine() {
  drawPinField(NULL, COL_TEXT);
}

void drawAdminLoginScreen() {
  drawGradientBackground();

  drawScreenTitle("Admin Login", tft.width() - LOGIN_CANCEL_X);
  auiButton(LOGIN_CANCEL_X, LOGIN_CANCEL_Y, LOGIN_CANCEL_W, LOGIN_CANCEL_H, "Cancel", ABTN_SECONDARY, FACE_SM);

  drawPinLine();

  for (int k = 0; k < 12; k++) {
    const char* lbl = LOGIN_LABELS[k];
    // OK is the one key that submits, so it gets the accent treatment; Clear
    // wipes the whole entry and sits quieter than the digits.
    int style = (k == 11) ? ABTN_PRIMARY : ABTN_SECONDARY;
    int face = (k == 9) ? FACE_SM : FACE_LG;
    if (k == 11) face = FACE_MD;
    auiButton(loginKeyX(k), loginKeyY(k), LOGIN_KEY_W, LOGIN_KEY_H, lbl, style, face);
  }
}

void handleAdminLoginScreen() {
  if (isRealTouch()) {
    if (millis() - lastTouchTime > TOUCH_DEBOUNCE) {
      lastTouchTime = millis();
      TS_Point raw = ts.getPoint();
      int sx, sy;
      mapTouchToScreen(raw, sx, sy);

      if (pointInRect(sx, sy, LOGIN_CANCEL_X - 4, 0, LOGIN_CANCEL_W + 8, HDR_BOTTOM)) {
        currentScreen = SCREEN_WELCOME;
        drawWelcomeScreen();
        return;
      }

      for (int k = 0; k < 12; k++) {
        if (!pointInRect(sx, sy, loginKeyX(k), loginKeyY(k), LOGIN_KEY_W, LOGIN_KEY_H)) continue;

        if (k == 9) {                       // Clear
          pinBuffer[0] = '\0';
          drawPinLine();
        } else if (k == 11) {               // OK
          if (strcmp(pinBuffer, adminPin) == 0) {
            pinBuffer[0] = '\0';
            currentScreen = SCREEN_ADMIN_PANEL;
            drawAdminPanelScreen();
          } else {
            drawPinField("Wrong PIN", COL_DANGER);
            delay(900);
            pinBuffer[0] = '\0';
            drawPinLine();
          }
        } else {
          int len = strlen(pinBuffer);
          if (len < 8) {
            pinBuffer[len] = LOGIN_LABELS[k][0];
            pinBuffer[len + 1] = '\0';
          }
          drawPinLine();
        }
        return;
      }
    }
  }
}
