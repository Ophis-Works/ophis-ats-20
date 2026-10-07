#pragma once

#include "OledRef.h"

//Declarations only since the restructuring (Milestone 7): the definitions
//live in Utils.cpp so every module TU can call these helpers.

extern const DCfont* LastFont;

void oledSetFont(const DCfont* font);
void oledPrint(const char* text, int offX = -1, int offY = -1, const DCfont* font = LastFont, bool invert = false);

//Better than sprintf which has overwhelmingly large overhead, it helps to reduce binary size
void convertToChar(char* strValue, uint16_t value, uint8_t len, uint8_t dot = 0, uint8_t separator = 0, uint8_t space = ' ');

//Measure integer digit length
int ilen(uint16_t n);

//Split KHz frequency + BFO to KHz and .00 tail
//Kept in 16-bit math: a 32-bit division here would pull ~300 bytes of div routines into the image
void splitFreq(uint16_t& khz, uint16_t& tail);

uint8_t strlen8(const char* str);
