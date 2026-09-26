// ---------- Theme Colors (20 selectable palettes, Config.h's CFG_COLOR_THEME) ----------
// The whole UI is built from this one set of macros — change which theme is
// selected in Config.h and every screen picks it up, since every screen
// draws with these macros instead of raw ILI9341_* constants.
//
// All 20 themes below share the same underlying design language this app
// was built around: a light, card-based look with a near-white top fading
// into a saturated bottom (the gradient background), pale cards that read
// as sitting a shade lighter than the wash behind them, a deep saturated
// border framing each card, a vivid accent reserved for the one actionable
// thing on a screen, dark primary text, and status colors (danger/success/
// warning) that stay warm and rich rather than reaching for stock primary
// red/green/yellow. Only the hue family changes between themes — the
// contrast relationships that make every screen legible do not, and every
// theme below is built to the same rules for exactly that reason:
//
//   - COL_BG_TOP must stay light and COL_TEXT must stay dark: every screen
//     pairs text=COL_TEXT against bg=COL_BG_TOP (or the reverse, text=
//     COL_BG_TOP against bg=COL_ACCENT for button labels) and depends on
//     that contrast holding regardless of which theme is active.
//   - COL_ACCENT, COL_CARD_BRD, COL_DANGER, COL_SUCCESS and COL_WARNING all
//     also get used directly as small text on COL_CARD/COL_BG_TOP (e.g.
//     Screen_02_Select's price, "Qty Avail" caption, atLimit warnings), so
//     each is pulled a couple of shades deeper/richer than a "fill-only"
//     pastel would need — still unmistakably that theme's hue, but legible
//     as text too.
//   - COL_CARD is kept pale rather than saturated for the same reason:
//     COL_ACCENT text drawn on top of it (the price) is the tightest-margin
//     pairing in every one of these palettes, so the card can't get much
//     more saturated without that text washing out.
//   - COL_SHADOW sits close to COL_TEXT_DIM in hue but lighter, since it's a
//     drop-shadow tone (drawCardShadow()/drawCard(), Core_08_UIHelpers.ino)
//     under a light card on a light background, not text — a neutral grey
//     shadow would read as a smudge against any of these warm/cool tinted
//     palettes, where a same-family tone reads as depth instead.
//   - COL_DANGER/COL_SUCCESS/COL_WARNING stay a consistent red/green/gold
//     family across every theme so their meaning never has to be relearned
//     — except on the handful of themes whose own ACCENT/CARD_BRD already
//     sits in one of those hue families (Ruby Red, Wine Burgundy, Crimson
//     for danger; Emerald Forest, Mint Fresh, Sage Green, Turquoise for
//     success; Golden Amber, Mocha Coffee for warning), where that one
//     status color is shifted just enough in shade to stay visually
//     distinct from the theme's own accent rather than blending into it.
//
// Pick a theme by number in Config.h's CFG_COLOR_THEME — 0 (Blossom) is the
// original theme this machine shipped with and is the default.
#if CFG_COLOR_THEME == 0
// Theme 0: Blossom (pink / rose) - the original default theme this machine shipped with
#define COL_BG_TOP     0xFF7E   // #FFEFF7 — near-white with a whisper of pink (gradient top)
#define COL_BG_BOTTOM  0xFE1B   // #FFC2DE — saturated bubblegum pink (gradient bottom)
#define COL_CARD       0xFF3D   // #FFE7EE — pale pink card fill, lighter than the wash behind it
#define COL_CARD_BRD   0xD1AD   // #D6356B — deep raspberry-rose card border/glow
#define COL_ACCENT     0xE0F1   // #E61C8C — vivid fuchsia accent (buttons, highlights)
#define COL_TEXT       0x4886   // #4A1031 — deep wine-plum, primary text
#define COL_TEXT_DIM   0x9B4F   // #9C697B — dusty rose-mauve, secondary text
#define COL_SHADOW     0xE5B9   // #E6B6CE — soft dusty pink, card/button shadow
#define COL_DANGER     0xE1CA   // #E63952 — rose-crimson, not pure red
#define COL_SUCCESS    0x350B   // #31A25A — fresh green, the one deliberately non-pink accent
#define COL_WARNING    0xB421   // #B58608 — rich gold, not pale yellow

#elif CFG_COLOR_THEME == 1
// Theme 1: Ocean Blue - crisp sky blue, clean and corporate
#define COL_BG_TOP     0xEFBF   // #EAF6FF
#define COL_BG_BOTTOM  0x8E9F   // #8FD0FA
#define COL_CARD       0xE79F   // #E1F0FC
#define COL_CARD_BRD   0x1373   // #146C9E
#define COL_ACCENT     0x13D9   // #1179C9
#define COL_TEXT       0x0968   // #0B2E45
#define COL_TEXT_DIM   0x5BF2   // #5D7C90
#define COL_SHADOW     0xC6FD   // #C7DEEC
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 2
// Theme 2: Mint Fresh - cool teal-green, airy and modern
#define COL_BG_TOP     0xEFDE   // #EAFBF5
#define COL_BG_BOTTOM  0x8718   // #82E3C0
#define COL_CARD       0xDFBD   // #DFF6EC
#define COL_CARD_BRD   0x0C6D   // #0C8C6A
#define COL_ACCENT     0x0D0F   // #0BA37C
#define COL_TEXT       0x09C5   // #0A3B2D
#define COL_TEXT_DIM   0x5C4F   // #5C8879
#define COL_SHADOW     0xC75B   // #C3E8D9
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x4D6A   // #4CAF50 — shifted warmer/yellower than the teal accent so "success" doesn't blend into it
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 3
// Theme 3: Lavender Dream - soft purple, calm and elegant
#define COL_BG_TOP     0xF77F   // #F5EFFF
#define COL_BG_BOTTOM  0xC55E   // #C7A9F2
#define COL_CARD       0xEF3F   // #EFE5FC
#define COL_CARD_BRD   0x69F4   // #6B3FA0
#define COL_ACCENT     0x79FA   // #7E3FD1
#define COL_TEXT       0x28A8   // #2A1547
#define COL_TEXT_DIM   0x7B72   // #7C6C96
#define COL_SHADOW     0xDE7E   // #DCCCF0
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 4
// Theme 4: Sunset Orange - warm citrus orange, energetic
#define COL_BG_TOP     0xFF9D   // #FFF3EA
#define COL_BG_BOTTOM  0xFD8F   // #FFB27A
#define COL_CARD       0xFF5B   // #FFE9DA
#define COL_CARD_BRD   0xC2A1   // #C1560F
#define COL_ACCENT     0xEB01   // #E8630F
#define COL_TEXT       0x4901   // #4A2308
#define COL_TEXT_DIM   0x9BAB   // #9C7458
#define COL_SHADOW     0xF655   // #F0C9AE
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 5
// Theme 5: Ruby Red - bold jewel red, confident
#define COL_BG_TOP     0xFF9E   // #FFF0F1
#define COL_BG_BOTTOM  0xFCD4   // #FF9AA4
#define COL_CARD       0xFF1C   // #FFE3E6
#define COL_CARD_BRD   0xA085   // #A3122B
#define COL_ACCENT     0xD0E7   // #D41C39
#define COL_TEXT       0x3842   // #3B0A12
#define COL_TEXT_DIM   0x92EC   // #935C64
#define COL_SHADOW     0xEE19   // #EFC3C9
#define COL_DANGER     0x7884   // #7A1020 — dropped to a near-black maroon so "danger" still reads distinct from the lively ruby accent
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 6
// Theme 6: Emerald Forest - deep saturated green, natural and grounded
#define COL_BG_TOP     0xEFDE   // #EDFBF2
#define COL_BG_BOTTOM  0x7ED4   // #7FD9A0
#define COL_CARD       0xE7BD   // #E2F6E9
#define COL_CARD_BRD   0x1368   // #146C43
#define COL_ACCENT     0x1429   // #10874F
#define COL_TEXT       0x0984   // #0B3320
#define COL_TEXT_DIM   0x5C4E   // #5C8870
#define COL_SHADOW     0xC75A   // #C6E8D3
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x262B   // #22C55E — brighter, cooler spring-green so "success" stands apart from the deeper forest accent
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 7
// Theme 7: Golden Amber - warm honey gold, upscale
#define COL_BG_TOP     0xFFDC   // #FFF8E6
#define COL_BG_BOTTOM  0xFE8F   // #FFD37A
#define COL_CARD       0xFF99   // #FFF0CC
#define COL_CARD_BRD   0xA3A1   // #A6740A
#define COL_ACCENT     0xCC41   // #C98A0A
#define COL_TEXT       0x49A0   // #4A3606
#define COL_TEXT_DIM   0x9C2B   // #9C8558
#define COL_SHADOW     0xF6F5   // #F0DFAE
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0x8A41   // #8A4B0A — dropped to a burnt rust so "warning" doesn't blend into the lighter amber accent

#elif CFG_COLOR_THEME == 8
// Theme 8: Mocha Coffee - warm brown/tan, cozy and premium
#define COL_BG_TOP     0xFF9D   // #FBF3EC
#define COL_BG_BOTTOM  0xD54E   // #D4A876
#define COL_CARD       0xF73B   // #F3E6D8
#define COL_CARD_BRD   0x6A24   // #6B4423
#define COL_ACCENT     0x8AC5   // #8B5A2B
#define COL_TEXT       0x28C1   // #2E1B0E
#define COL_TEXT_DIM   0x8B8C   // #8A7260
#define COL_SHADOW     0xE697   // #E4D2BE
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x2CEB   // #2E9E5B
#define COL_WARNING    0xCCC3   // #C99A1A — pushed more yellow/vivid so "warning" separates from the warm brown accent

#elif CFG_COLOR_THEME == 9
// Theme 9: Slate Charcoal - cool neutral gray-blue, minimal and professional
#define COL_BG_TOP     0xF7BE   // #F3F5F7
#define COL_BG_BOTTOM  0xADB7   // #A9B4BE
#define COL_CARD       0xEF7D   // #EAEDEF
#define COL_CARD_BRD   0x3A2A   // #3C4650
#define COL_ACCENT     0x42CD   // #45596B
#define COL_TEXT       0x1904   // #1B2126
#define COL_TEXT_DIM   0x6BD0   // #6E7880
#define COL_SHADOW     0xD6FC   // #D7DCE0
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 10
// Theme 10: Aqua Cyan - bright cyan, fresh and clinical-clean
#define COL_BG_TOP     0xEFDF   // #E8FBFC
#define COL_BG_BOTTOM  0x7F1D   // #7FE0EA
#define COL_CARD       0xDFBF   // #DFF7F9
#define COL_CARD_BRD   0x0BF1   // #0E7C88
#define COL_ACCENT     0x0CB4   // #0E97A6
#define COL_TEXT       0x09A7   // #0A3438
#define COL_TEXT_DIM   0x5C52   // #5C8990
#define COL_SHADOW     0xC75D   // #C3E9EC
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 11
// Theme 11: Royal Indigo - deep blue-violet, premium and trustworthy
#define COL_BG_TOP     0xEF9F   // #EFF0FF
#define COL_BG_BOTTOM  0xAD7E   // #A9AEF2
#define COL_CARD       0xE73F   // #E6E7FC
#define COL_CARD_BRD   0x39F4   // #3F3FA0
#define COL_ACCENT     0x4A7A   // #4D4DD1
#define COL_TEXT       0x10A7   // #17173F
#define COL_TEXT_DIM   0x6B72   // #6C6C96
#define COL_SHADOW     0xD69E   // #D2D3F0
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 12
// Theme 12: Peach Coral - soft warm coral, friendly and modern
#define COL_BG_TOP     0xFF9D   // #FFF1EC
#define COL_BG_BOTTOM  0xFD73   // #FFAE99
#define COL_CARD       0xFF3B   // #FFE4DC
#define COL_CARD_BRD   0xC223   // #C1441F
#define COL_ACCENT     0xEAC7   // #E85A3A
#define COL_TEXT       0x48E2   // #4A1C10
#define COL_TEXT_DIM   0x9B6B   // #9C6C5C
#define COL_SHADOW     0xF678   // #F0CCC0
#define COL_DANGER     0xC0E7   // #C21F3D — cooled toward magenta-red so "danger" separates from the warm coral accent
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 13
// Theme 13: Wine Burgundy - deep muted wine, refined and moody
#define COL_BG_TOP     0xFF7E   // #FBEEF0
#define COL_BG_BOTTOM  0xC3F1   // #C77E8C
#define COL_CARD       0xF71C   // #F3E0E3
#define COL_CARD_BRD   0x68A4   // #6E1626
#define COL_ACCENT     0x8907   // #8E2138
#define COL_TEXT       0x2842   // #2A0A10
#define COL_TEXT_DIM   0x8AEC   // #8A5C64
#define COL_SHADOW     0xE619   // #E4C3C9
#define COL_DANGER     0xFA69   // #FF4D4D — brightened well above the muted wine accent so an alert still pops against this deliberately subdued palette
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 14
// Theme 14: Midnight Navy - dark saturated navy, formal and sleek
#define COL_BG_TOP     0xEF9F   // #EAF0FA
#define COL_BG_BOTTOM  0x7C98   // #7C93C2
#define COL_CARD       0xE75E   // #E1E9F6
#define COL_CARD_BRD   0x112B   // #14275A
#define COL_ACCENT     0x19CF   // #1E3A78
#define COL_TEXT       0x0885   // #0A132E
#define COL_TEXT_DIM   0x5B71   // #5C6C8A
#define COL_SHADOW     0xC69D   // #C7D2E8
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 15
// Theme 15: Turquoise - vivid blue-green, tropical and lively
#define COL_BG_TOP     0xEFDE   // #E8FBF7
#define COL_BG_BOTTOM  0x7719   // #74E0C8
#define COL_CARD       0xDFBE   // #DEF7F0
#define COL_CARD_BRD   0x0BED   // #0C7C68
#define COL_ACCENT     0x0D31   // #0DA68A
#define COL_TEXT       0x09A6   // #0A3630
#define COL_TEXT_DIM   0x5C50   // #5C8A81
#define COL_SHADOW     0xC75C   // #C3E9E1
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x6DE7   // #6FBF3E — pushed yellow-green, away from the cooler teal accent
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 16
// Theme 16: Crimson - vivid true red, striking and bold
#define COL_BG_TOP     0xFF9D   // #FFF0EF
#define COL_BG_BOTTOM  0xFC71   // #FF8F8A
#define COL_CARD       0xFF1B   // #FFE1DF
#define COL_CARD_BRD   0xA083   // #A3121C
#define COL_ACCENT     0xD0E5   // #D41C2E
#define COL_TEXT       0x3841   // #3B0A0C
#define COL_TEXT_DIM   0x92EB   // #935C5E
#define COL_SHADOW     0xEE18   // #EFC3C2
#define COL_DANGER     0x6862   // #6B0F14 — dropped to a near-black maroon, same fix as Ruby Red, so "danger" stays distinct from this theme's own red accent
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 17
// Theme 17: Steel Blue - muted industrial blue, calm and technical
#define COL_BG_TOP     0xEF9E   // #EEF3F7
#define COL_BG_BOTTOM  0x9DD9   // #9DB8CC
#define COL_CARD       0xE75E   // #E4EBF1
#define COL_CARD_BRD   0x330F   // #35617D
#define COL_ACCENT     0x3BF4   // #3F7DA0
#define COL_TEXT       0x1126   // #142430
#define COL_TEXT_DIM   0x63D1   // #63798A
#define COL_SHADOW     0xD6FC   // #D0DEE7
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 18
// Theme 18: Sage Green - muted soft green, natural and understated
#define COL_BG_TOP     0xF7BD   // #F1F7EE
#define COL_BG_BOTTOM  0xB693   // #B7D19A
#define COL_CARD       0xEF9C   // #E9F2E1
#define COL_CARD_BRD   0x4BC5   // #4E7A2E
#define COL_ACCENT     0x5C87   // #5E9138
#define COL_TEXT       0x1962   // #1F2E12
#define COL_TEXT_DIM   0x744D   // #74886A
#define COL_SHADOW     0xDF39   // #DAE6CC
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x1D0B   // #1FA35C — more saturated true green than the muted sage accent, so "success" doesn't blend in
#define COL_WARNING    0xB421   // #B58608

#elif CFG_COLOR_THEME == 19
// Theme 19: Plum Purple - rich magenta-purple, playful and premium
#define COL_BG_TOP     0xF77E   // #F7EEF7
#define COL_BG_BOTTOM  0xD4DB   // #D69AD8
#define COL_CARD       0xF71E   // #F1E3F1
#define COL_CARD_BRD   0x78EF   // #7A1F7C
#define COL_ACCENT     0x9973   // #9C2E9E
#define COL_TEXT       0x3046   // #340B36
#define COL_TEXT_DIM   0x8B52   // #8C6890
#define COL_SHADOW     0xEE7D   // #E9CCE9
#define COL_DANGER     0xE1CA   // #E63952
#define COL_SUCCESS    0x350B   // #31A25A
#define COL_WARNING    0xB421   // #B58608

#else
#error "CFG_COLOR_THEME must be 0-19 - see the list in Config.h"
#endif
