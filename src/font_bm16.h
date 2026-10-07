// ----------------------------------------------------------------------
// EXPERIMENT: BMplain 6x8 (TinyOLED-Fonts) stretched vertically to 6x12.
// Uppercase/digit subset, ASCII 32..93 (space..']'), 62 glyphs.
// Vertical-only 1.5x stretch: whole rows duplicated (even pattern), stems
// stay 1px, horizontal bars thicken where doubled. Cell widened to 7px with
// an empty left column (glyph pixels still occupy the right 6 columns).
// Source font from the TinyOLED-Fonts library (data lives in fonts.cpp).
// ----------------------------------------------------------------------

#pragma once

#include <Tiny4kOLED_common.h>
#include <avr/pgmspace.h>

extern const uint8_t ssd1306xled_fontBMplain2x [] PROGMEM;
extern const DCfont TinyOLED4kfontBMplain2x;

#define FONT_BMPLAIN2X (&TinyOLED4kfontBMplain2x)
