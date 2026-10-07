#pragma once

#include <Arduino.h>
#include "defs.h"
#include "types.h"

//Settings callbacks (the do* manipulate functions) and RDS config.
//Globals from globals.h (single-include) accessed by this module:

extern SettingsItem g_Settings[];
extern bool g_displayRDS;
extern bool g_sMeterOn;
extern uint8_t g_muteVolume;
extern int8_t g_bandIndex;
extern volatile int8_t g_stepIndex;
extern int8_t g_FMStepIndex;
extern Band g_bandList[];
extern int g_tabStep[];
extern int8_t g_tabStepFM[];
extern const int8_t g_lastStepFM;
extern uint8_t g_amTotalSteps;
extern uint8_t g_amTotalStepsSSB;
extern int8_t g_bwIndexSSB;
extern int8_t g_bwIndexAM;
extern int8_t g_bwIndexFM;
extern Bandwidth g_bandwidthSSB[];
extern Bandwidth g_bandwidthAM[];
extern const char* g_bandwidthFM[];
extern const uint8_t g_maxFilterAM;
extern const uint8_t g_bwSSBMaxIdx;

void doStep(int8_t v);
void doVolume(int8_t v);
void doSwitchLogic(int8_t& param, int8_t low, int8_t high, int8_t step);
void doAttenuation(int8_t v);
void doSoftMute(int8_t v);
void doBrightness(int8_t v);
void doSSBAVC(int8_t v);
void doAvc(int8_t v);
void doSync(int8_t v);
void doDeEmp(int8_t v);
void doSWUnits(int8_t v);
void doSSBSoftMuteMode(int8_t v);
void doCutoffFilter(int8_t v);
void doCPUSpeed(int8_t v);
void doBFOCalibration(int8_t v);
void doCWSwitch(int8_t v);
#if USE_RDS
void doRDSErrorLevel(int8_t v);
void doRDS();
void setRDSConfig(uint8_t bias);
#endif
void doBandwidthLogic(int8_t& bwIndex, uint8_t upperLimit, uint8_t v);
void doBandwidth(uint8_t v);
