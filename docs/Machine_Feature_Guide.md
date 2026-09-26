# Sanitary Napkin Vending Machine — Complete Feature Guide

**Platform:** ESP32 + 2.8" ILI9341 touchscreen (320×240, resistive touch via XPT2046)
**Build:** 2-selection sanitary napkin dispenser (this machine's flashed configuration)
**Firmware family:** modular, per-machine configuration in `Config.h`, everything else fixed logic

This document catalogues every feature of the machine end to end: what a customer sees and can do, what a technician/admin can configure, how the hardware behaves, and how data, payments, and reports flow. Numbers quoted (timeouts, limits, pin numbers) are the values actually compiled into this specific build unless marked "admin-adjustable."

---

## 1. Hardware Overview

| Subsystem | Component | Interface / Pins |
|---|---|---|
| Display | ILI9341 320×240 TFT | SPI — CS=5, DC=2, RST=4 |
| Touch | XPT2046 resistive | SPI — CS=21 |
| Dispensing motors | Up to 6-pin pool, 2 fitted (M1, M2) | GPIO13 (M1), GPIO25 (M2); pool also reserves GPIO26/27 (M3/M4, idle/reserved) and GPIO32/33 (M5/M6, dual-role with coin acceptor via external DIP switches) |
| Coin acceptor | Pulse-output coin validator | Pulse in = GPIO33 (shared/dip-switched with M6), power relay = GPIO32 (shared/dip-switched with M5) |
| Product drop sensor | IR break-beam, active-low — **optional**, `Config.h`-only (`CFG_IR_SENSOR_PRESENT`) | GPIO35 (input-only, no internal pull-up — external 10k pull-up to 3V3 fitted) |
| RFID reader | Custom binary-protocol UART reader | UART1 — TX=GPIO22, RX=GPIO34, 19200 baud, inverted RS232 levels |
| RTC | DS3231 battery-backed clock | I2C — SDA=GPIO14, SCL=GPIO15, address 0x68 |
| 4G/LTE modem | SIMCom A7600/SIM7600 family | UART2 — TX=GPIO17, RX=GPIO16, 115200 baud, own DC power supply (never from ESP32 rail) |
| WiFi | ESP32 onboard | 2.4 GHz 802.11 |

**Motor/product scaling:** This build fits `CFG_MOTOR_COUNT = 2` and `CFG_PRODUCT_COUNT = 2`, but the firmware supports up to 6 motors and 6 products without a code change — only `Config.h` and the physical DIP switches (for M5/M6) need to change.

**Per-unit dispense time:** 2500 ms of motor run per unit, fixed globally (not per-product).

**Stock ceiling per motor:** 3 units (`CFG_STOCK_MAX`), matching this build's physical hopper capacity.

**Drop sensor fitted:** yes, on this build (`CFG_IR_SENSOR_PRESENT = true`) — every dispense is verified (see §2.8). A build with no sensor wired sets this to `false` in `Config.h` and dispensing then relies on motor timing alone; see §2.8 and §7 for exactly what changes.

**Free vend:** off on this build (`CFG_FREE_VEND_MODE = false`) — customers pay normally. See §3.6 for the admin toggle.

---

## 2. Customer Journey

### 2.1 Welcome / Idle Screen
- Live clock (12h or 24h, admin-selectable) and date, dimmed with "Syncing time..." or "No network" until the clock has synced from any source.
- Configurable 3-line title card (e.g. "SANITARY / NAPKIN / VENDING MACHINE") — set once in `Config.h` per machine.
- Pulsing "Touch to Continue" button (blinks every 600 ms) invites a tap anywhere on the card to start shopping.
- **Out-of-stock override:** if every product is sold out, the button stops pulsing and turns solid red "Out of Stock" instead — the machine never invites a tap into a dead end.
- Machine name and ID shown in small text at the bottom (used to tell one unit apart from another in a fleet, and matched in every report email).
- **Hidden admin entry:** holding any of the four screen corners for 2 full seconds opens the Admin PIN screen. A short or off-target touch is treated as a normal "start shopping" tap instead.
- While idle, the machine quietly runs its background jobs in the same loop tick: checking whether a scheduled report email is due, and whether the monthly RFID usage reset is due — both are deliberately only run when idle, never mid-purchase, since a report send can block for the better part of a minute.

### 2.2 Product Selection
- Grid of product cards (1–3 columns depending on how many products are enabled), each showing name, price, and live stock status:
  - **Out of stock** (red) — card can't be tapped.
  - **All in cart / Cart full** (amber) — distinguishes "you've already added every unit left" from "your cart is at its limit."
  - **In cart** (pink accent) — shows quantity already added and units remaining.
  - **Available** (green) — shows units remaining.
- Price is hidden whenever the admin turns off "Show Prices" (§3.6) — available on a machine-wide Free Vend build or an RFID-only free-vend machine, where nothing is charged and a price line can be more confusing than useful.
- **Quick-vend mode:** if the cart limit is set to 1, tapping a product skips the cart entirely and goes straight to payment — no extra confirmation screens for a one-item purchase.
- Tapping a card that's out of stock, at the per-item stock limit, or would exceed the cart limit is refused with a visible red border-flash and a message ("Out of stock," "Only N left," "Max N item(s)") — a rejected tap is always visibly and audibly (via animation) different from a broken touchscreen.

### 2.3 Cart Review
- Skipped automatically in quick-vend mode.
- One row per item with a running total, and a live "−/qty/+" stepper per row.
- "+" is blocked the same way a product-card tap is — can't exceed remaining stock or the cart-wide quantity cap.
- Empty cart shows "Cart is empty" rather than a blank screen.

### 2.4 Payment Method
- **Skipped entirely on a Free Vend machine** (§3.6) — a completed cart goes straight from Select/Cart Review to dispensing with no payment method shown at all, not even a "no payment methods available" placeholder.
- Otherwise, shows one card per payment method actually enabled for this machine (UPI, Cash, RFID) — a method not fitted/enabled never appears, not even as a greyed-out option.
- Short description per method: "Scan the QR code" (UPI), "Insert exact change" (Cash), "Registered cards only" (RFID).
- "No payment methods available" is shown plainly if every method is switched off while Free Vend itself is off, rather than a blank screen.

### 2.5 UPI Payment (PhonePe Dynamic QR)
- Generates a unique transaction ID and requests a QR code from PhonePe's payment gateway, valid for the configured timeout (default 2 minutes, admin-adjustable 1–10 minutes).
- Displays the QR full-screen; customer scans and pays with any UPI app.
- Polls PhonePe for payment status in the background — check interval adapts to the connection (faster over WiFi, slower over 4G, to avoid queuing requests back-to-back on a slow link).
- On success: shows "Payment Successful!", holds briefly, then dispenses. If the physical dispense itself fails (jam/empty), the success message stays (the money was already taken) but the closing line changes to a dispense-failure notice directing the customer to contact support.
- On timeout: "Payment Timed Out" with Retry (generates a fresh QR) or Back.
- Clear, specific error messages for every failure mode: no internet at all (checks both WiFi and 4G before giving up), connection failure, gateway error code, malformed response, missing QR data — never a generic "something went wrong."
- Automatically falls back from WiFi to the 4G modem if WiFi is down and a modem is fitted and enabled.

### 2.6 Cash Payment (Coin Acceptor)
- Live "Total / Paid / Remaining" display updates with every accepted coin.
- Accepts ₹1, ₹2, ₹5, and ₹10 coins (each denomination individually enable/disable-able by the admin); an unrecognized or admin-disabled coin is not credited.
- **No change is given** — this is stated clearly on screen ("No change given – pay exact amount"), and overpayment is accepted without issuing change.
- Once the first coin is accepted, the transaction can no longer be cancelled — the Back button is replaced with a static "Paying, no refund" notice, since an inserted coin cannot be mechanically returned.
- 90-second inactivity timeout (from the last accepted coin) returns to Payment Method if the customer walks away mid-payment.
- On full payment: dispenses immediately, shows "Product Dispensed!" or a dispense-failure notice, then returns to Welcome.

### 2.7 RFID Tap Payment (Free Vend for Registered Cards)
- For customers issued a registered access card — not a real payment, the tap simply authorizes a free dispense.
- Each card can carry an optional withdrawal limit (0 = unlimited); once used up, further taps are refused with "Limit reached (N left)" until the machine's monthly auto-reset (if configured) or an admin manually resets that card.
- An unregistered card is refused with "Card not registered."
- 60-second inactivity timeout returns to Payment Method.
- On success: "Card Accepted" / dispense outcome, then returns to Welcome. Free vends are still counted in reports as units and transactions but logged at ₹0 revenue, so "N free vends today" is visible without inflating sales figures.

### 2.8 Dispensing (Every Payment Method, and Free Vend)
- A blocking "Dispensing / Please wait..." screen is shown for the whole operation — dispensing is synchronous, never left running invisibly in the background.
- Each unit's motor always runs its full configured time (2500 ms) even if the drop sensor fires early — this deliberately avoids stopping a coil/spiral dispenser mid-rotation, which would jam the next dispense.
- Multi-unit and multi-motor purchases are supported: if a product is wired to more than one motor, dispensing draws from whichever motor(s) still have stock, with a 400 ms gap between consecutive units.
- On a **Free Vend** machine (§3.6), UPI/Cash/RFID are bypassed entirely — the customer's tap or "Get Free" confirmation goes directly into this same dispensing routine, logged at ₹0 revenue but still counted as a real unit sold and a real transaction.

**With the drop sensor fitted (`CFG_IR_SENSOR_PRESENT = true`, this build):**
- After the motor stops, the drop sensor is watched for an additional 5 seconds (total watch window ~7.5 seconds) since a product can take a moment to actually fall clear of the mechanism.
- Every unit is independently verified by the drop sensor; if even one unit in a multi-item purchase fails to register a drop, the whole purchase is reported to the customer as "not confirmed / contact support" — but the sale is still logged (the money was already taken, and stock was already committed to the attempt).
- A jam or empty slot is called out distinctly in the technical log so it's diagnosable after the fact.

**With no drop sensor fitted (`CFG_IR_SENSOR_PRESENT = false`):**
- There is nothing to poll, so the firmware doesn't pretend to check — each unit's motor simply runs its configured time and is reported as delivered, with no extra wait tacked on afterwards and no reads of the sensor pin at all.
- Every dispense is reported to the customer as successful; a physical jam or empty hopper is no longer detectable by the machine itself and would need to be caught by a human (e.g. a low-stock alert, or a customer complaint) rather than the drop-sensor check.
- This is a `Config.h`-only setting — deliberately no admin-screen toggle, since it changes what a "successful dispense" even means rather than a day-to-day setting a technician should flip without opening the unit.

---

## 3. Admin & Configuration

### 3.1 Getting In
- Hold any screen corner for 2 seconds from the Welcome screen → PIN entry (3×4 numeric keypad, PIN 4–8 digits).
- Wrong PIN shows a brief red "Wrong PIN" flash and clears the entry — there is no lockout after repeated wrong attempts.
- "Cancel" on the PIN screen returns to Welcome with no PIN needed.

### 3.2 Admin Panel (Home)
- Lists every product slot with its name, price, assigned motor(s), and enabled/disabled state; tapping a row opens that product's editor.
- Quick-nav chips to Clock (Date & Time), Report (Sales Report), and Setup (Settings).
- "Motor Stock" button for refilling, and "Exit Admin" to return to the customer-facing Welcome screen.

### 3.3 Product Editor
- Edit product name (on-screen keyboard).
- Adjust price in ₹1 steps, floor ₹1, ceiling ₹999 — saved instantly, no separate confirm step.
- Assign or reassign motor(s) to the product.
- Enable/Disable toggle — a disabled product disappears from the customer Select screen entirely; text turns red on this screen so its state is unmistakable at a glance.
- Per-unit dispense time is intentionally a single machine-wide setting (in `Config.h`), not editable per product here.

### 3.4 Motor Assignment
- Grid of motor buttons, each labeled with its current owner ("Mine," "Unused," or another product's name).
- Assigning a motor to one product automatically takes it away from whichever product owned it before — a motor can only ever belong to one product at a time.

### 3.5 Motor Stock (Refilling)
- Per-motor stock counter, 0 up to the commissioned ceiling (3 units on this build).
- "T" test button fires that motor for its normal dispense duration, useful for verifying wiring/mechanism during a refill without needing a real sale.
- Paginated automatically if the machine has more motors than fit legibly on one screen (not needed on this 2-motor build).
- Every stock change re-arms the low-stock email alert so refilling doesn't leave a stale "still low" state hanging, without emailing anything mid-refill.

### 3.6 Settings Hub
- Maximum cart quantity stepper (floor 1, ceiling set by the machine's commissioned limit).
- **Free Vend toggle** — when turned on, the machine stops charging for anything: every completed cart is dispensed for free and the customer never sees a payment method screen at all (not even a "no payment methods" dead end — that screen is skipped entirely). The Select and Cart Review "Proceed"/"Pay" buttons relabel themselves to "Get Free" so the change is obvious to a shopper. This is separate from RFID's own free-vend-for-a-registered-card feature below — this one waives payment for everyone, machine-wide.
- One ON/OFF toggle per payment method actually built into this machine (UPI, Cash, RFID) — hidden methods never show a toggle at all, and this whole section disappears while Free Vend is on, since those toggles would have nothing left to control.
- "Show Prices" toggle — appears whenever a customer never actually pays anything: with Free Vend on, or on an RFID-only free-vend machine. Lets the admin choose whether a reference price still shows on the Select screen even though nothing is charged.
- Machine ID field (used in report emails to identify this specific unit).
- Admin PIN field — the value itself is never displayed, always shown as fixed "****" for security.
- Quick links to WiFi, UPI, Coin, and 4G sub-screens. WiFi/4G links only grey out if that radio hardware isn't physically fitted on this build at all — never just because the admin switched the radio off from its own screen, so there's always a way back in to turn it back on.

### 3.7 UPI / PhonePe Configuration
- Only the Merchant ID and Store ID are editable here — every other PhonePe credential (base URL, provider ID, salt key/index, terminal ID) is fixed at the factory and deliberately never even shown on screen, since this is a device that sits in a public place.
- Payment timeout stepper, 1–10 minutes in whole-minute steps.

### 3.8 WiFi Setup
- Edit SSID and password (password masked while typing).
- Live status: connected/not-connected/turned-off (an intentional "off" always reads differently from a real connection failure), IP address, signal strength with quality label (Excellent/Good/Fair/Weak/Very weak) and a signal-bars icon.
- "Connect" button forces a fresh reconnect and clock resync.
- "Ping" button tests real internet reachability (not just radio association) and reports response time.
- On/off toggle for the WiFi radio itself.
- Screen refreshes its live status every 3 seconds even without a touch.

### 3.9 4G/LTE Setup
- Shows the configured cellular APN (fixed, not editable here — a wrong APN attaches but has no working DNS, so it must match the SIM's carrier exactly).
- Live status priority: turned-off (neutral) → connected → last real error message → "not connected" — so switching the modem off doesn't leave a stale scary error on screen.
- Signal bars when connected.
- "Test Internet" button runs a real internet reachability test over the cellular link (skipped instantly if the modem is off, avoiding a pointless 15–20 second wait).
- On/off toggle for the modem.
- Screen refreshes every 3 seconds.

### 3.10 On-Screen Keyboard (Shared Text Entry)
- One shared, reusable keyboard component (uppercase/lowercase/symbols layers) used by every text field in the admin area — WiFi credentials, machine ID/name, Gmail address and app password, report recipient, nightly send times, RFID card UID/name, admin PIN, product name.
- Field-specific rules are enforced automatically: e.g. the Gmail app password field strips spaces on save (Google displays it in 4 groups but accepts it either way); nightly send times are validated as real "HH:MM" values and rejected (keeping the old value) if malformed; the admin PIN must be 4–8 digits only, because the login keypad can only ever type digits — anything looser would risk locking an admin out of their own machine.
- Masked fields (passwords, PIN) have a "Show" toggle to verify what was typed before committing.

### 3.11 Coin Denomination Control
- One ON/OFF row per accepted coin value — directly controls what the coin acceptor will credit on the Cash payment screen.

### 3.12 Sales Report (4 pages)
1. **Sales overview:** lifetime transaction count, lifetime units sold, lifetime revenue, current total stock on hand, and today's units/revenue/transaction count. Tapping "Today" sends today's report by email on the spot.
2. **By product:** a proper table (not just running text) of units sold / revenue / stock remaining per product. A disabled product with real sales history still shows (that history is real money); a disabled product with zero sales quietly drops off the list.
3. **Schedule & housekeeping:** up to two nightly automatic report send times, low-stock alert on/off and its trigger threshold (cycles 1 through 5), a live list of which product(s) are currently at or below that threshold, how many days of history are stored (out of a 60-day rolling window), and a "Reset all data" control that requires two deliberate taps (arms red "TAP AGAIN," then executes) since it's unrecoverable.
4. **Email setup:** Gmail sender address, app password (shown only as a character count, never the value, until explicitly revealed), report recipient address, and machine name (shown here because it appears in every email subject).
- A global "Send Now" button sends the complete 60-day report with a full transaction spreadsheet attached; it's greyed out entirely if email isn't fully configured, rather than letting an admin wait and then fail.

### 3.13 RFID Card Management
- Paginated list of every registered card, showing its name (or raw ID if unnamed), usage/limit if a limit is set, and a one-tap delete (no confirmation step — deletion is treated as low-stakes since a card can simply be re-registered).
- "Scan New Card" — hold a card near the reader; auto-cancels after 30 seconds if nothing is presented.
- "Enter Manually" — type a card's ID by hand via the on-screen keyboard, for cards that can't be scanned in place.
- A hard cap on the number of registered cards is enforced; once reached, both add-methods show "List full" instead of failing silently.
- Per-card editor: rename, set/adjust a withdrawal limit (0 = unlimited, capped at 999), and manually reset that card's usage counter back to zero at any time.

### 3.14 RFID Monthly Auto-Reset
- Machine-wide schedule that automatically zeroes every registered card's usage counter — e.g. "reset on the 1st of every month at 01:00" for recurring monthly quotas, so an admin doesn't have to reset every card by hand each month.
- On/off toggle, day-of-month (capped at 28 so it's always valid regardless of month length), hour, and minute.
- Every change here saves immediately (unlike the Date & Time screen, this isn't treated as consequential enough to need a separate "Save" step).
- Shows the last month this actually fired, for confirmation it's working.

### 3.15 Date & Time
- Simple stepper-based entry (day/month/year/hour/minute) rather than free typing — removes any chance of a mistyped separator or AM/PM mix-up.
- 12-hour or 24-hour display toggle; switching it only changes how the hour is shown/stepped, never the underlying value.
- Changes are staged and only take effect on "Save" (or discarded on "Cancel").
- Saving sets the running clock immediately and writes the same time to the battery-backed hardware clock, so it survives a power cut even with zero connectivity — but a later real network time sync can still correct it if one comes in.

---

## 4. Connectivity

### 4.1 WiFi
- Primary connection path when available; used for UPI payments, clock sync, and email reports.
- Automatic reconnect attempts; admin can force a reconnect or run a live ping test.

### 4.2 4G/LTE (Cellular Fallback)
- Used automatically for UPI payments, email reports, and clock sync whenever WiFi is unavailable, on machines with a modem fitted and enabled.
- Fully independent hardware toggle at the factory (`Config.h`, "is a modem physically fitted") separate from the admin's on-screen on/off switch (a modem can be fitted but intentionally left off — no SIM, no coverage, or a site that wants to run on the battery clock alone).
- Live signal-strength bars, connection status, and a one-tap "test internet" check.
- All the slow, blocking work (registration, DNS, HTTP/TLS calls) is deliberately kept off the fast per-frame UI path so the touchscreen stays responsive even while the modem is mid-retry.

### 4.3 Clock Sync (Three-Tier Fallback)
The machine's clock always tries the most trustworthy source first and only falls back if that source can't be reached within a fair grace period:
1. **WiFi NTP** (internet time servers) — most accurate, tried first.
2. **Cellular network time** (via the 4G modem, using the carrier's own network time or an NTP/HTTP-date fallback if the carrier doesn't provide it) — used only if WiFi isn't available.
3. **Onboard battery-backed hardware clock** — used only if neither network path has produced a time within about 8 seconds, ensuring the machine never sits with a "no time" screen just because a network was a few seconds slow to associate.

Once any real network sync succeeds, that authoritative time is written back into the hardware clock, so the machine keeps accurate time through future power cuts even with no connectivity at all.

### 4.4 Status Indicator
- A single icon in the top-right corner always shows whichever connection is actually live — the WiFi fan-and-bars glyph when on WiFi, or cellular signal bars when riding on 4G instead — never both, never neither if either is genuinely connected.

---

## 5. Payments — Summary

| Method | Type | Change given? | Cancellable mid-payment? | Free vend? |
|---|---|---|---|---|
| UPI (PhonePe QR) | Digital, exact or scanned amount | N/A | Yes, until success | No |
| Cash (coins) | Physical, ₹1/₹2/₹5/₹10 | No — pay exact, overpay accepted with no refund | No, once first coin accepted | No |
| RFID (registered card) | Access-controlled free vend | N/A | Yes, until tap accepted | Yes |

If the admin turns on **Free Vend** (§3.6), none of the rows above are ever shown to the customer — every checkout skips straight to a free dispense regardless of which methods are enabled underneath.

---

## 6. Reporting & Data

### 6.1 What's Tracked
- **Lifetime totals:** all-time transactions, units sold, and revenue per product and overall — survive indefinitely, never pruned.
- **60-day daily history:** per-day transactions, units and revenue per product, opening stock, and a breakdown by payment method — automatically recycles the oldest day once the 61st day arrives, so the machine always holds a rolling two-month window with no manual cleanup needed.
- **Full transaction log (spreadsheet-ready):** one line per product sold, with machine ID, exact timestamp, product, unit price, quantity, amount paid, payment method, and resulting stock level — trimmed to the same 60-day window automatically.
- Restock actions themselves are not separately logged — the live stock count is treated as the single source of truth for refill decisions.

### 6.2 Email Reports
- **Daily summary** — sent automatically at up to two configured times per day, with that day's transaction spreadsheet attached.
- **Low-stock alert** — sent automatically the moment any product's stock crosses the configured threshold, with no attachment (kept lightweight and fast). Only fires once per drop below threshold, not on every subsequent sale, and re-arms automatically once restocked above the threshold.
- **Full report** — sent on demand from the admin Report screen: complete 60-day figures, a day-by-day breakdown table, and the full transaction spreadsheet.
- Every email subject line identifies the exact machine by ID (and name, if set) and the report type/date — built so a customer or operator managing several machines can tell them apart instantly.
- Reports are sent over WiFi if available, automatically falling back to the 4G modem if not, using the same email-sending logic either way.
- Sending automatically retries a few times on transient network trouble before giving up, and if a report with an attachment keeps failing, it makes one last attempt without the attachment (the summary figures matter more than the raw spreadsheet on a weak link) rather than not reporting at all.
- The admin's "Send Now" button is disabled outright if email isn't fully configured yet, rather than letting someone wait for a send that can't succeed.

---

## 7. Reliability & Safety Design Highlights

- **Every dispensing motor always completes its full timed run**, even if the product is detected falling early — stopping early risks jamming the mechanism mid-cycle for the next customer.
- **Every dispensed unit is independently verified** by an IR break-beam sensor, on a build that has one fitted; a failure on even one unit in a multi-item order is reported to the customer as unconfirmed, prompting them to contact support, while still being logged for follow-up. On a build with no sensor fitted (`CFG_IR_SENSOR_PRESENT = false` in `Config.h`), there is nothing to verify against, so every dispense is reported as successful based on motor timing alone — a deliberate, factory-set trade-off, not something an admin can toggle from the touchscreen.
- **A coin, once accepted, cannot be returned** — the machine states this plainly rather than implying a refund is possible.
- **No payment method appears to a customer unless it is both physically fitted and switched on** — there is no dead-end "unavailable" payment card shown. On a machine-wide Free Vend build, no payment method appears at all, by design.
- **Radios (WiFi/4G) can never be enabled from the admin screen beyond what the machine was physically built with** — an admin toggle is always capped by what hardware is actually fitted, so a stored setting from a different build can never turn on a radio that isn't there.
- **Two-tap confirmation** is required before permanently erasing all sales history; simple, easily-undone actions (like deleting a single registered card) require only one tap.
- **Duplicate-safe scheduling:** every automatic action (nightly report, low-stock alert, monthly RFID reset) records the date/day it last fired, so a power cut or reboot can never cause the same alert or report to be sent twice, and a machine that happened to be off at the exact scheduled minute still catches up on its next active moment.
- **The clock always prefers the most accurate available source** (internet time over cellular time over the onboard battery clock) but never gets stuck waiting indefinitely for one.

---

## 8. Commissioning a New Machine (What's Configured Once, at the Factory)

Set once in `Config.h` and re-flashed per unit — the admin screens take over managing these values day-to-day after that first flash, until the file is edited and reflashed again (which resets them back to the file's values, useful for reassigning a unit to a new location):

- Welcome screen title (3 lines) and machine name/ID
- Number of motors and products physically fitted, and per-motor stock ceiling
- Motor run time per unit, motor drive polarity, drop-sensor polarity
- Maximum items per cart, admin PIN (initial), which payment methods exist on this build at all
- WiFi and cellular radios fitted/not fitted, WiFi credentials, cellular APN
- Gmail sending account and app password, default report recipient, low-stock alert default
- PhonePe/UPI merchant credentials and default payment timeout
- Whether a Free Vend build starts pre-enabled (`CFG_FREE_VEND_MODE`) — a seed value only; Admin > Settings owns the toggle from the first flash onward
- Whether a product-drop IR sensor is physically fitted at all (`CFG_IR_SENSOR_PRESENT`) — **factory-only, with no admin-screen equivalent**, since it changes what a "successful dispense" means rather than a business setting

Everything above except the last one becomes admin-editable from the touchscreen immediately after that flash (subject to the hardware ceilings noted throughout this document), so day-to-day operation — refilling stock, adjusting prices, changing WiFi passwords, updating the report recipient, or toggling Free Vend on and off — never requires reflashing the machine again. Whether a drop sensor is fitted is the one exception: that one is a hardware fact set once at commissioning and left alone.
