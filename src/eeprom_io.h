#pragma once

#include <Arduino.h>
#include "defs.h"
#include "types.h"

//EEPROM save/load of all receiver information.
//Globals from globals.h (single-include) accessed by this module:

extern bool g_ssbLoaded;
extern uint8_t g_muteVolume;
extern uint8_t g_volume;
extern uint16_t g_currentFrequency;
extern uint16_t g_previousFrequency;
extern int g_currentBFO;
extern int8_t g_bandIndex;
extern volatile uint8_t g_currentMode; //types.h (inline isSSB)
extern volatile uint8_t g_prevMode;
extern volatile int8_t g_stepIndex;
extern int8_t g_FMStepIndex;
extern int8_t g_bwIndexSSB;
extern int8_t g_bwIndexAM;
extern int8_t g_bwIndexFM;
extern Bandwidth g_bandwidthSSB[];
extern Bandwidth g_bandwidthAM[];
extern uint8_t g_amTotalSteps;
extern Band g_bandList[];
extern const uint8_t g_lastBand;
extern SettingsItem g_Settings[];
extern long g_storeTime;

void saveAllReceiverInformation();
void readAllReceiverInformation();
void resetEepromDelay();
