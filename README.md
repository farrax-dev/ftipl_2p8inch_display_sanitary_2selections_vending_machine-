# FTIPL Vending Machine — Master Skeleton (2.8" Display)

ESP32-based vending machine firmware with a 2.8" ILI9341 touchscreen, designed and maintained by **Future Techniks India Pvt. Ltd. (FTIPL)**.

This repository is the **master skeleton** — a single configurable codebase that can be flashed to any machine in the fleet. Per-unit differences (product names, prices, WiFi credentials, UPI merchant details, etc.) are isolated in `Config.h`.

## Hardware

| Component | Details |
|---|---|
| MCU | ESP32 DevKit (VSPI) |
| Display | 2.8" ILI9341 TFT (SPI) |
| Touch | XPT2046 resistive touchscreen (polled, no IRQ line wired) |
| Motors | Up to 5 DC dispensing motors via MOSFET/relay driver (GPIO13/25/26/27/32 — a fixed pool, not expandable without a pin change) |
| Connectivity | WiFi + optional 4G/LTE modem (SIMCom A7600-series, UART2 on GPIO16/17) |
| Coin Acceptor | Pulse-counting coin acceptor on GPIO33 |
| RFID Reader | YHY5225 13.56MHz reader/writer, UART1 on GPIO22/34, for registered-card free-vend |
| RTC | DS3231 battery-backed clock (I2C, GPIO14/15) — third clock fallback after WiFi NTP and LTE NTP |
| Storage | NVS (settings, stock, RFID cards) + LittleFS (sales log) |

A complete, firmware-sourced pin reference — every GPIO, including spares and the ones that must never be wired — is in [`docs/wiring-diagram.html`](docs/wiring-diagram.html).

## Features

- **Multi-product support** — up to 6 configurable product slots, each assignable to any of the 5 motors
- **Shopping cart** — customers can select multiple items in one transaction (configurable max quantity); a single-product machine skips the cart entirely in **Quick-Vend Mode**
- **Three payment methods** — UPI/PhonePe (dynamic QR), cash (coin acceptor), and registered RFID cards for free vend — each independently enabled per machine
- **RFID free-vend program** — register a contactless card in one tap, with an optional running withdrawal limit; every free dispense is still logged and reported, not just written off
- **Admin panel** — PIN-protected on-device admin menu (hold any corner for 2 seconds) for:
  - Product name, price, and motor assignment editing
  - Stock level management
  - WiFi, UPI, coin denomination, LTE, and reporting configuration
  - RFID card registration and per-card limits
  - Motor testing
- **Dual connectivity** — WiFi with automatic NTP sync; 4G/LTE fallback for cellular-only sites or a dropped WiFi connection
- **Triple-redundant clock** — WiFi NTP, then LTE NTP, then a battery-backed onboard RTC — timestamps stay correct through a network outage or a power cut
- **Automated reporting** — daily sales summary emails via Gmail SMTP (up to two scheduled times), an on-demand "Send Now," and automatic low-stock alerts
- **Sales logging** — 60-day transaction history stored on LittleFS with CSV export, survives reboots and power cuts
- **Themeable UI** — colour palette defined in `Core_03_Theme.ino`; screen headers use a shared, consistently-sized serif title bar (`Core_08_UIHelpers.ino`'s `drawScreenTitle()`)

## Project Structure

The sketch lives in `SanitaryVendingMachine/` and is split across multiple `.ino` tab files. Arduino concatenates them into a single translation unit — the split is purely organisational.

```
SanitaryVendingMachine/
├── SanitaryVendingMachine.ino   Main file: includes, shared types, prototypes
├── Config.h                     Per-machine configuration (edit this to commission a unit)
│
├── Core_02_AppState.ino         Screen state, admin editing state
├── Core_03_Theme.ino            Colour palette
├── Core_04_Hardware.ino         Pin definitions, TFT/touch objects, motor control
├── Core_05_TouchInput.ino       Touch calibration, debounce
├── Core_06_Network.ino          WiFi, NTP, UPI/PhonePe connectivity
├── Core_07_WiFiIndicator.ino    Status-bar WiFi/4G signal icon
├── Core_08_UIHelpers.ino        Shared UI drawing helpers (headers, buttons, cards, text wrap)
├── Core_09_Storage.ino          NVS persistence, product/stock/RFID-card data
├── Core_10_Cart.ino             Shopping cart state
├── Core_11_Dispense.ino         Motor dispensing logic
├── Core_12_Main.ino             setup() and loop()
├── Core_13_LTEModem.ino         4G modem, HTTPS over AT commands
├── Core_14_SalesLog.ino         60-day sales history, transaction CSV
├── Core_15_Report.ino           Report body generation and send orchestration
├── Core_16_ReportSchedule.ino   Nightly emails, low-stock alerts, log pruning
├── Core_17_Gmail.ino            SMTP to Gmail (WiFi or LTE)
├── Core_18_RTC.ino              DS3231 battery-backed clock, third clock fallback
├── Core_19_RFID.ino             YHY5225 RFID reader + registered-card store
│
├── Screen_01_Welcome.ino             Idle / welcome screen
├── Screen_02_Select.ino              Product selection
├── Screen_03_CartReview.ino          Cart review before payment
├── Screen_04_PaymentMethod.ino       Payment method chooser
├── Screen_05_PaymentUPI.ino          UPI/PhonePe QR payment flow
├── Screen_06_PaymentOther.ino        Cash/coin and RFID payment flows
├── Screen_07_AdminLogin.ino          Admin PIN entry
├── Screen_08_AdminPanel.ino          Admin main menu
├── Screen_09_AdminEdit.ino           Product editing
├── Screen_10_AdminMotor.ino          Motor assignment & stock management
├── Screen_11_AdminSettings.ino       General settings
├── Screen_12_AdminUPIConfig.ino      UPI merchant configuration
├── Screen_13_AdminWiFi.ino           WiFi SSID/password configuration
├── Screen_14_AdminTextEntry.ino      On-screen keyboard for text input
├── Screen_15_AdminCoin.ino           Coin acceptor configuration
├── Screen_16_AdminLTE.ino            LTE modem configuration
├── Screen_17_AdminReport.ino         Report settings & manual send
├── Screen_18_AdminRFIDCards.ino      RFID card list & registration
└── Screen_19_AdminRFIDCardEdit.ino   Edit a registered card's name/limit/usage
```

21 distinct screen states in total across these 19 files — see `AppScreen` in `SanitaryVendingMachine.ino` for the full list.

## Getting Started

### Prerequisites

- **Arduino IDE** (or Arduino CLI)
- **ESP32 board package** installed via Boards Manager
- **Libraries** (install via Library Manager):
  - `Adafruit GFX Library`
  - `Adafruit ILI9341`
  - `XPT2046_Touchscreen`
  - `ArduinoJson` by Benoit Blanchon
  - `TinyGSM` by Volodymyr Shymanskyy — 4G/LTE modem AT-command support
  - `ESP_SSLClient` by mobizt — TLS over the modem's raw TCP socket (needed for SMTP over cellular)
  - `ArduinoHttpClient` by Arduino — HTTPS client for the cellular UPI path

  Everything else (`WiFi`, `HTTPClient`, `WiFiClientSecure`, `Preferences`, `LittleFS`, `Wire`, `SPI`, `time.h`, `base64.h`, `mbedtls/sha256.h`, the bundled `qrcode.h`) ships with the ESP32 Arduino core.

### Board Settings

- Partition scheme: **"No OTA (2MB APP / 2MB SPIFFS)"** — the 1.2MB default is too small for this sketch
- Everything else can stay at its default (240MHz, QIO, 80MHz, 4MB flash)

### Commissioning a New Machine

1. Open `Config.h`
2. Set the machine identity (`CFG_MACHINE_NAME`, `CFG_MACHINE_ID`)
3. Configure product count (`CFG_PRODUCT_COUNT`, 1–6) and motor count (`CFG_MOTOR_COUNT`, 1–5) to match the physical hardware
4. Set WiFi credentials, LTE APN, UPI merchant details, and Gmail credentials as needed
5. Select the **"No OTA (2MB APP/2MB SPIFFS)"** partition scheme
6. Flash to the ESP32

The firmware fingerprints `Config.h` values and re-seeds NVS on change, so editing the file and reflashing is all that's needed to reconfigure a unit. Between flashes, settings can be adjusted on-device via the admin panel — see `Config.h`'s own comments for exactly which fields are seed-only vs. always in effect.

### Admin Access

Hold any screen corner for 2 seconds to open the admin login. Default PIN: `1234` (configurable via `CFG_ADMIN_PIN` in `Config.h` — change this before deploying a machine).

## Documentation

- [`docs/wiring-diagram.html`](docs/wiring-diagram.html) — complete pin-by-pin wiring reference, generated from the firmware's own pin constants, including every spare and excluded GPIO

## License

Proprietary — Future Techniks India Pvt. Ltd.
