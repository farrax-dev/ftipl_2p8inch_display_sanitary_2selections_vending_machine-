// ---------- Theme Colors (citrus: white/orange/green/yellow) ----------
// The whole UI is built from this one palette — change a value here and
// every screen picks it up, since every screen draws with these macros
// instead of raw ILI9341_* constants.
//
// A light "citrus grove" theme: white background fading to a whisper of
// warm cream, cards in a soft peach/apricot orange framed by a leaf-green
// border (the complementary contrast keeps them from blending into the
// warm-toned body), a vivid burnt-orange accent reserved for the one
// actionable thing on a screen, and status colors (red/green/gold) that
// read as part of the same fruit-stand palette instead of clashing
// defaults.
//
// Every screen already pairs these macros contrastively (e.g. text=COL_TEXT
// against bg=COL_BG_TOP, or text=COL_BG_TOP against bg=COL_ACCENT for
// button labels) — COL_BG_TOP doubles as "the light label color for text
// sitting on a saturated fill" throughout the code, so it must stay light,
// and COL_TEXT must stay dark, for every screen to keep reading correctly.
//
// A few macros (COL_ACCENT, COL_CARD_BRD, COL_DANGER, COL_SUCCESS,
// COL_WARNING) also get used directly as small text on COL_CARD/COL_BG_TOP
// (e.g. Screen_02_Select's price, "Qty Avail" caption, atLimit warnings).
// Pure/bright versions of orange, green and gold read as low-contrast wash
// on a light card, so each is pulled one or two shades deeper than a
// "fill-only" vivid version would need — still unmistakably orange/green/
// gold, but legible as text too. COL_CARD itself is kept a *soft* peach
// rather than a saturated one for the same reason: COL_ACCENT text drawn
// on top of it (the price) is the tightest-margin pairing in the whole
// palette, so the card can't get much darker without that text washing out.
#define COL_BG_TOP     0xFFFF   // #FFFFFF — pure white (gradient top)
#define COL_BG_BOTTOM  0xFFBB   // #FFF6DE — soft warm cream (gradient bottom)
#define COL_CARD       0xFF5A   // #FFE9D6 — soft peach/apricot card fill
#define COL_CARD_BRD   0x1407   // #15803D — deep leaf-green card border/glow
#define COL_ACCENT     0xEAC1   // #E8590C — vivid burnt-orange accent (buttons, highlights)
#define COL_TEXT       0x2903   // #2E2118 — warm espresso-brown, primary text
#define COL_TEXT_DIM   0x6AE9   // #6B5F4E — warm taupe-gray, secondary text

// Drop-shadow tone for drawCardShadow()/drawCard() (Core_08_UIHelpers.ino) —
// every card and button on a real countertop reads as sitting slightly above
// the background rather than painted flat onto it. A muted warm taupe, not a
// grey: a neutral shadow would read as a smudge against this palette's warm
// cream/peach, where a same-temperature-family tone reads as depth instead.
// Close to COL_TEXT_DIM in hue but lighter, since it sits under a light card
// on a light background rather than acting as text.
#define COL_SHADOW     0xD613   // #D8C2A0 — soft warm taupe, card/button shadow

// ---------- Status colors ----------
// Use these instead of ILI9341_RED/GREEN/YELLOW so error/success/warning
// states read as part of this theme rather than as a clash of defaults.
#define COL_DANGER     0xD984   // #D93025 — errors, out-of-stock, disabled
#define COL_SUCCESS    0x1407   // #15803D — connected, paid, enabled (same green as COL_CARD_BRD)
#define COL_WARNING    0x9B40   // #9C6B00 — at-limit, low stock, caution (rich gold, not pale yellow)
