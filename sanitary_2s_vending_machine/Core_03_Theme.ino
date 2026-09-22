// ---------- Theme Colors (blossom: white/blush pink/rose/fuchsia) ----------
// The whole UI is built from this one palette — change a value here and
// every screen picks it up, since every screen draws with these macros
// instead of raw ILI9341_* constants.
//
// A light, unapologetically girly "blossom" theme: a near-white blush top
// fading into a saturated bubblegum-pink bottom, cards in a soft pale pink
// so they read as sitting a shade lighter than the wash behind them, framed
// by a deep raspberry-rose border, a vivid fuchsia accent reserved for the
// one actionable thing on a screen, and status colors (crimson/green/gold)
// that stay warm and rosy rather than reaching for stock primary red/green.
//
// Every screen already pairs these macros contrastively (e.g. text=COL_TEXT
// against bg=COL_BG_TOP, or text=COL_BG_TOP against bg=COL_ACCENT for
// button labels) — COL_BG_TOP doubles as "the light label color for text
// sitting on a saturated fill" throughout the code, so it must stay light,
// and COL_TEXT must stay dark, for every screen to keep reading correctly.
//
// A few macros (COL_ACCENT, COL_CARD_BRD, COL_DANGER, COL_SUCCESS,
// COL_WARNING) also get used directly as small text on COL_CARD/COL_BG_TOP
// (e.g. Screen_02_Select's price, "Qty Avail" caption, atLimit warnings). A
// bright, light-value pink reads as a low-contrast wash on a light card, so
// COL_ACCENT is pulled a couple of shades deeper into fuchsia than a
// "fill-only" bubblegum pink would need — still unmistakably pink, but
// legible as text too. COL_CARD itself is kept pale rather than saturated
// for the same reason: COL_ACCENT text drawn on top of it (the price) is
// the tightest-margin pairing in the whole palette, so the card can't get
// much pinker without that text washing out.
#define COL_BG_TOP     0xFF7E   // #FFEFF7 — near-white with a whisper of pink (gradient top)
#define COL_BG_BOTTOM  0xFE1B   // #FFC2DE — saturated bubblegum pink (gradient bottom)
#define COL_CARD       0xFF3D   // #FFE7EE — pale pink card fill, lighter than the wash behind it
#define COL_CARD_BRD   0xD1AD   // #D6356B — deep raspberry-rose card border/glow
#define COL_ACCENT     0xE0F1   // #E61C8C — vivid fuchsia accent (buttons, highlights)
#define COL_TEXT       0x4886   // #4A1031 — deep wine-plum, primary text
#define COL_TEXT_DIM   0x9B4F   // #9C697B — dusty rose-mauve, secondary text

// Drop-shadow tone for drawCardShadow()/drawCard() (Core_08_UIHelpers.ino) —
// every card and button on a real countertop reads as sitting slightly above
// the background rather than painted flat onto it. A muted dusty pink, not a
// grey: a neutral shadow would read as a smudge against this palette's warm
// blush/bubblegum, where a same-family rosy tone reads as depth instead.
// Close to COL_TEXT_DIM in hue but lighter, since it sits under a light card
// on a light background rather than acting as text.
#define COL_SHADOW     0xE5B9   // #E6B6CE — soft dusty pink, card/button shadow

// ---------- Status colors ----------
// Use these instead of ILI9341_RED/GREEN/YELLOW so error/success/warning
// states read as part of this theme rather than as a clash of defaults.
// COL_DANGER is pulled toward red rather than another shade of pink
// specifically so it never gets mistaken for COL_ACCENT at a glance — a
// refused tap needs to read as "stop", not as "here's the highlight color
// again".
#define COL_DANGER     0xE1CA   // #E63952 — errors, out-of-stock, disabled (rose-crimson, not pure red)
#define COL_SUCCESS    0x350B   // #31A25A — connected, paid, enabled (fresh green — the one deliberately non-pink accent, so "yes" still reads as "yes")
#define COL_WARNING    0xB421   // #B58608 — at-limit, low stock, caution (rich gold, not pale yellow)
