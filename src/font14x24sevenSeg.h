// ----------------------------------------------------------------------
// Seven Segment font for Tiny4kOLED library.
// 14x24 resolution.
// Charset: '.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
// Empty space character as a '/'
// v1.1
// Changes:
// - Removed minus character (-)
// - Font is more aligned to top
// ----------------------------------------------------------------------
// By Goshante
// 02.2024
// http://github.com/goshante
// ----------------------------------------------------------------------
// Declarations only since Milestone 7: definitions live in fonts.cpp.

#pragma once

#include <Tiny4kOLED_common.h>
#include <avr/pgmspace.h>

extern const uint8_t ssd1306xled_font14x24sevenSeg [] PROGMEM;
extern const DCfont TinyOLED4kfont14x24sevenSeg;

//For backwards compatibility
#define FONT14X24SEVENSEG (&TinyOLED4kfont14x24sevenSeg)
