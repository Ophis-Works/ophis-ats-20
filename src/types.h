#pragma once

#include <Arduino.h>
#include "defs.h"

// Shared type/enum declarations used by globals.h (single-include, holds the
// definitions) and by the module headers. Types only: safe to include many
// times, unlike globals.h.

// Modulations
enum Modulations : uint8_t
{
    AM,
    LSB,
    USB,
    CW,
    FM
};

//int8_t is atomic on AVR, which keeps main/ISR accesses tear-free
extern volatile uint8_t g_currentMode;

inline bool isSSB()
{
    return g_currentMode > AM && g_currentMode < FM;
}

enum SettingType
{
    ZeroAuto,
    Num,
    Switch,
    SwitchAuto
};

struct SettingsItem
{
    char name[5];
    int8_t param;
    uint8_t type;
    void (*manipulateCallback)(int8_t);
};

enum SettingsIndex
{
    ATT,
    SoftMute,
    SVC,
    Sync,
    DeEmp,
    AutoVolControl,
    Brightness,
    SWUnits,
    SSM,
    CutoffFilter,
    CPUSpeed,
#if USE_RDS
    RDSError,
#endif
    BFO,
    UnitsSwitch,
    ScanSwitch,
    CWSwitch,
    SETTINGS_MAX
};

//For managing BW
struct Bandwidth
{
    uint8_t idx;      //Internal SI473X index
    const char* desc;
};

//Band table structures
enum BandType : uint8_t
{
    LW_BAND_TYPE,
    MW_BAND_TYPE,
    SW_BAND_TYPE,
    AIR_BAND_TYPE,
    FM_BAND_TYPE
};

struct Band
{
    uint16_t minimumFreq;
    uint16_t maximumFreq;
    uint16_t currentFreq;
    int8_t currentStepIdx;
    int8_t bandwidthIdx;     // Bandwidth table index (internal table in Si473x controller)
};

#if USE_RDS
enum RDSActiveInfo : uint8_t
{
    StationName,
    StationInfo,
    ProgramInfo
};
#endif

//Extern declarations for const globals so their definitions in globals.h
//(single-include) get external linkage and modules can read them:
extern const uint8_t g_lastBand;
extern const uint8_t g_SWSubBandCount;
extern const int8_t g_lastStepFM;
extern const uint8_t g_maxFilterAM;
extern const uint8_t g_bwSSBMaxIdx;
