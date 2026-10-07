#pragma once

#include <Arduino.h>
#include "defs.h"
#include "types.h"

//Tuner control: band/mode configuration, tuning, seek, SSB patch and BFO.
//Globals from globals.h (single-include) accessed by this module:

extern bool g_seekStop;
extern bool g_ssbLoaded;
extern bool g_settingsActive;
extern bool g_sMeterOn;
extern bool g_displayRDS;
extern bool g_processFreqChange;
extern uint8_t g_seekDirection;
extern int g_currentBFO;
extern uint16_t g_currentFrequency;
extern uint16_t g_previousFrequency;
extern volatile int8_t g_encoderCount;
extern volatile int8_t g_stepIndex;
extern int8_t g_bandIndex;
extern int8_t g_FMStepIndex;
extern uint8_t g_amTotalSteps;
extern uint8_t g_amTotalStepsSSB;
extern uint8_t g_ssbTotalSteps;
extern int g_tabStep[];
extern int8_t g_tabStepFM[];
extern Band g_bandList[];
extern uint16_t SWSubBands[];
extern int8_t g_bwIndexSSB;
extern int8_t g_bwIndexAM;
extern int8_t g_bwIndexFM;
extern Bandwidth g_bandwidthSSB[];
extern SettingsItem g_Settings[];
extern uint32_t g_lastFreqChange;
extern long g_storeTime;
extern uint8_t g_bandSelSub;
extern bool g_bandBrowsing;
extern const uint8_t g_SWSubBandCount;

void sanitizeMode();
int getSteps();
int getLastStep();
void updateSSBCutoffFilter();
void agcSetFunc();
uint16_t getNextSWSuBband(bool up);
uint8_t swSubBandFromFreq(uint16_t freq);
void bandSwitch(bool up);
void bandSelectBegin();
void bandSelectMove(bool up, uint8_t count);
bool bandSelectFinish();
void loadSSBPatch();
void applyBandConfiguration(bool extraSSBReset = false);
void showFrequencySeek(uint16_t freq);
bool checkStopSeeking();
void doSeek();
bool clampSSBBand();
void doFrequencyTune();
void doFrequencyTuneSSB();
void updateBFO();
