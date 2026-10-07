#pragma once

#include <Arduino.h>
#include "defs.h"
#include "types.h"

//OLED drawing (show*/draw functions), settings screens included.
//Globals from globals.h (single-include) accessed by this module:

extern bool g_settingsActive;
extern bool g_sMeterOn;
extern bool g_displayRDS;
extern bool g_cmdBand;
extern bool g_cmdVolume;
extern bool g_cmdStep;
extern bool g_cmdBw;
extern bool g_voltagePinConnnected;
extern uint8_t g_muteVolume;
extern uint16_t g_currentFrequency;
extern int8_t g_bandIndex;
extern volatile int8_t g_stepIndex;
extern int8_t g_FMStepIndex;
extern uint8_t g_amTotalSteps;
extern int g_tabStep[];
extern int8_t g_tabStepFM[];
extern const char* bandTags[];
extern const char* g_bandModeDesc[];
extern char _literal_EmptyLine[17];
extern uint8_t g_bandSelSub;
extern bool g_bandBrowsing;
extern const uint8_t g_SWSubBandCount;
uint8_t swSubBandFromFreq(uint16_t freq);
extern int8_t g_bwIndexSSB;
extern int8_t g_bwIndexAM;
extern int8_t g_bwIndexFM;
extern Bandwidth g_bandwidthSSB[];
extern Bandwidth g_bandwidthAM[];
extern const char* g_bandwidthFM[];
extern SettingsItem g_Settings[];
extern int8_t g_SettingSelected;
extern int8_t g_SettingsTop;
extern bool g_SettingEditing;
#if USE_RDS
extern bool g_rdsSwitchPressed;
extern uint8_t g_rdsActiveInfo;
extern char g_rdsPrevLen;
extern char* g_RDSCells[3];
#endif

void showStatus(bool cleanFreq = false);
void showFrequency(bool cleanDisplay = false);
void updateLowerDisplayLine();
void SettingParamToUI(char* buf, uint8_t idx);
void DrawSetting(uint8_t idx, bool full);
void showSettings();
void showSettingsTitle();
void switchSettingsTop();
void switchSettings();
void showModulation();
void eraseRdsSMeterArea();
void showBandTag();
void showVolume();
void showCharge(bool forceShow);
#if USE_RDS
void showRDS();
#endif
void showStep();
void showSMeter();
void showBandwidth();
