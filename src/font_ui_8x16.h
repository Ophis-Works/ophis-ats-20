// ----------------------------------------------------------------------
// Custom UI font for Tiny4kOLED: uppercase-only subset of Pixel Operator
// Bold 8x16, ASCII 32..93 (space..']'; the three glyphs past 'Z' back the
// bracketed step display). Generated from TinyOLED-Fonts PixelOperatorBold.h,
// then modified (subsetting + glyph additions) - this is NOT the original
// library font. All firmware text is normalized to uppercase to match.
// Pixel Operator is CC0: (c) 2009-2018 Jayvee Enaguas (HarvettFox96).
// ----------------------------------------------------------------------
// Declarations only since Milestone 7: definitions live in fonts.cpp.

#pragma once

#include <Tiny4kOLED_common.h>
#include <avr/pgmspace.h>

extern const uint8_t ssd1306xled_font8x16pob_upper [] PROGMEM;
extern const DCfont TinyOLED4kfont8x16pob_upper;

//For backwards compatibility
#define FONT8X16POB_UPPER (&TinyOLED4kfont8x16pob_upper)
