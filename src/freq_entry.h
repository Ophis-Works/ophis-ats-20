#pragma once

#include <Arduino.h>
#include "defs.h"
#include "types.h"

//Direct frequency entry via rotary encoder (plan.md Delta 4).
//Globals from globals.h (single-include) accessed by this module:

extern bool g_freqEntryActive;
extern uint8_t g_freqEntryDigits[6];
extern uint8_t g_freqEntryPos;
extern uint8_t g_freqEntryCount;
extern uint32_t g_freqEntryLastInput;
extern bool g_settingsActive;
extern bool g_cmdVolume;
extern bool g_cmdStep;
extern bool g_cmdBw;
extern bool g_cmdBand;
extern uint16_t g_currentFrequency;
extern int g_currentBFO;
extern int8_t g_bandIndex;
extern Band g_bandList[];
extern bool g_processFreqChange;
extern uint32_t g_lastFreqChange;
extern uint32_t g_lastAdjustmentTime;

void drawFreqEntry();
void doFreqEntryCancel();
void doFreqEntryApply();
void doFreqEntryConfirm();
void doFreqEntryDigit(int8_t dir);
void doFreqEntryStart();
uint8_t encoderEvent(uint8_t event, uint8_t pin);
