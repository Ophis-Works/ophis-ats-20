#pragma once

//Access to the shared OLED device for module TUs. Tiny4kOLED.h itself
//defines the `oled` object and must stay included exactly once (main.cpp);
//this header exposes only the class declaration plus an extern reference.

#include <Tiny4kOLED_common.h>

extern SSD1306PrintDevice oled;
