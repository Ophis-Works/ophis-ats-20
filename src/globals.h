#pragma once

#include "defs.h"
#include "types.h"
#include <SI4735.h>
#include "Rotary.h"
#include "SimpleButton.h"

long g_storeTime = millis();

bool g_voltagePinConnnected = false;
bool g_ssbLoaded = false;

bool g_cmdVolume = false;
bool g_cmdStep = false;
bool g_cmdBw = false;
bool g_cmdBand = false;
bool g_settingsActive = false;
bool g_sMeterOn = false;
bool g_displayOn = true;
bool g_displayRDS = false;
bool g_rdsSwitchPressed = false;
bool g_seekStop = false;
uint32_t g_lastAdjustmentTime = 0;

uint8_t g_muteVolume = 0;
int g_currentBFO = 0;

//Direct frequency entry via rotary encoder (Delta 4)
bool g_freqEntryActive = false;
uint8_t g_freqEntryDigits[6];
uint8_t g_freqEntryPos = 0;     //Current digit position
uint8_t g_freqEntryCount = 0;   //Total digit positions for the band
uint32_t g_freqEntryLastInput = 0;

// Encoder buttons
SimpleButton  btn_Bandwidth(BANDWIDTH_BUTTON);
SimpleButton  btn_BandUp(BAND_BUTTON);
SimpleButton  btn_BandDn(SOFTMUTE_BUTTON);
SimpleButton  btn_VolumeUp(VOLUME_BUTTON);
SimpleButton  btn_VolumeDn(AVC_BUTTON);
SimpleButton  btn_Encoder(ENCODER_BUTTON);
SimpleButton  btn_AGC(AGC_BUTTON);
SimpleButton  btn_Step(STEP_BUTTON);
SimpleButton  btn_Mode(MODE_SWITCH);

//int8_t is atomic on AVR, which keeps main/ISR accesses tear-free
volatile int8_t g_encoderCount = 0;

//Frequency tracking
uint16_t g_currentFrequency;
uint16_t g_previousFrequency;

void doAttenuation(int8_t v);
void doSoftMute(int8_t v);
void doBrightness(int8_t v);
void doSSBAVC(int8_t v = 0);
void doAvc(int8_t v);
void doSync(int8_t v = 0);
void doDeEmp(int8_t v = 0);
void doSWUnits(int8_t v = 0);
void doSSBSoftMuteMode(int8_t v = 0);
void doCutoffFilter(int8_t v);
void doCPUSpeed(int8_t v = 0);
#if USE_RDS
void doRDSErrorLevel(int8_t v);
#endif
void doBFOCalibration(int8_t v);
void doCWSwitch(int8_t v = 0);

SettingsItem g_Settings[] =
{
    //Page 1
    { "ATT", 0,  SettingType::ZeroAuto,     doAttenuation     },  //Attenuation
    { "SM ", 0,  SettingType::Num,          doSoftMute        },  //Soft Mute
    { "SVC", 1,  SettingType::Switch,       doSSBAVC          },  //SSB AVC Switch
    { "SYN", 0,  SettingType::Switch,       doSync            },  //SSB Sync
    { "DEE", 1,  SettingType::Switch,       doDeEmp           },  //FM DeEmphasis (0 - 50, 1 - 75)
    { "AVC", 46, SettingType::Num,          doAvc             },  //Automatic Volume Control
    //Page 2
    { "SCR", 80, SettingType::Num,          doBrightness      },  //Screen Brightness
    { "SW ", 0,  SettingType::Switch,       doSWUnits         },  //SW Units
    { "SSM", 1,  SettingType::Switch,       doSSBSoftMuteMode },  //SSB Soft Mute Mode
    { "COF", 0,  SettingType::SwitchAuto,   doCutoffFilter    },  //SSB Cutoff Filter
    { "CPU", 0,  SettingType::Switch,       doCPUSpeed        },  //CPU Frequency
#if USE_RDS
    { "RDS", 1,  SettingType::Num,          doRDSErrorLevel   },  //RDS ErrorLevel
#endif
    //Page 3
    { "BFO", 0,  SettingType::Num,          doBFOCalibration  },  //BFO Offset calibration
    { "UNI", 1,  SettingType::Switch,       doSWUnits         },  //Show/Hide frequency units
    { "SCA", 1,  SettingType::Switch,       doSWUnits         },  //AM Encoder scan switch
    { "CW ", 0,  SettingType::Switch,       doCWSwitch        },  //CW is LSB or USB
};

int8_t g_SettingSelected = 0;
int8_t g_SettingsTop = 0;   //First item of the 3-row scrolling viewport
bool g_SettingEditing = false;

int8_t g_bwIndexSSB = 4;
Bandwidth g_bandwidthSSB[] =
{
    { 4, "0.5K" },
    { 5, "1.0K" },
    { 0, "1.2K" },
    { 1, "2.2K" },
    { 2, "3.0K" },
    { 3, "4.0K" }
};
const uint8_t g_bwSSBMaxIdx = 5;

int8_t g_bwIndexAM = 4;
const uint8_t g_maxFilterAM = 6;
Bandwidth g_bandwidthAM[] =
{
    { 4, "1.0K" }, // 0
    { 5, "1.8K" }, // 1
    { 3, "2.0K" }, // 2
    { 6, "2.5K" }, // 3
    { 2, "3.0K" }, // 4 - Default
    { 1, "4.0K" }, // 5
    { 0, "6.0K" }  // 6
};

int8_t g_bwIndexFM = 0;
const char* g_bandwidthFM[] =
{
    "AUTO",
    "110K",
    " 84K",
    " 60K",
    " 40K"
};

int g_tabStep[] =
{
    // AM steps in KHz
    1,
    5,
    9,
    10,
    // Large AM steps in KHz
    50,
    100,
    1000,
    // SSB steps in Hz
    10,
    25,
    50,
    100,
    500
};
uint8_t g_amTotalSteps = 7;
uint8_t g_amTotalStepsSSB = 4; //Prevent large AM steps appear in SSB mode
uint8_t g_ssbTotalSteps = 5;
volatile int8_t g_stepIndex = 3;

int8_t g_tabStepFM[] =
{
    5,  // 50 KHz
    10, // 100 KHz
    100 // 1 MHz
};
int8_t g_FMStepIndex = 1;
const int8_t g_lastStepFM = (sizeof(g_tabStepFM) / sizeof(int8_t)) - 1;

#if USE_RDS
uint8_t g_rdsActiveInfo = RDSActiveInfo::StationName;
char g_rdsPrevLen = 0;
char* g_RDSCells[3];
#endif

char _literal_EmptyLine[17] = "                ";

//All tags padded to 3 chars: shorter tags leave residue when a longer
//one was drawn before (e.g. the stale "R" when browsing AIR -> FM)
const char* bandTags[] =
{
    "LW  ",
    "MW  ",
    "SW  ",
    "AIR ",
    "BCST",
};

Band g_bandList[] =
{
    /* LW */ { LW_LIMIT_LOW, 520, 300, 0, 4 },
    /* MW */ { 520, 1710, 1476, 3, 4 },
    /* SW */ { SW_LIMIT_LOW, SW_LIMIT_HIGH, SW_LIMIT_LOW, 0, 4 },
    /* AIR */ { AIR_LIMIT_LOW, AIR_LIMIT_HIGH, AIR_DEFAULT_FREQ, 0, 6 },
    /* FM */ { 6400, 10800, 8400, 1, 0 },
};

uint16_t SWSubBands[] =
{
    SW_LIMIT_LOW,  // 160 Meter
    3500, // 80 Meter
    4500,
    5600,
    6800, // 40 Meter
    7200, // 41 Meter
    8500,
    10000, // 30 Meter
    11200,
    13400,
    14000, // 20 Meter
    15000,
    17200,
    18000, // 17 Meter
    21000, // 15 Meter
    21400, // 13 Meter
    24890, // 12 Meter
    CB_LIMIT_LOW, // CB Band (11 Meter)
    CB_LIMIT_HIGH  // 10 Meter
};
const uint8_t g_SWSubBandCount = sizeof(SWSubBands) / sizeof(uint16_t);
const uint8_t g_lastBand = (sizeof(g_bandList) / sizeof(Band)) - 1;
int8_t g_bandIndex = 1;

//Deferred band selection (plan.md Delta 8): numbered SW sub-band browsing
uint8_t g_bandSelSub = 0;     //Selected SW sub-band (0-based into SWSubBands)
bool g_bandBrowsing = false;  //True while the band command is active

volatile uint8_t g_currentMode = FM;
const char* g_bandModeDesc[] =
{
    "AM ",
    "LSB",
    "USB",
    "CW ",
    "FM "
};
volatile uint8_t g_prevMode = FM;
uint8_t g_seekDirection = 1;

//Special logic for fast and responsive frequency surfing
uint32_t g_lastFreqChange = 0;
bool g_processFreqChange = 0;
uint8_t g_volume = DEFAULT_VOLUME;

Rotary g_encoder = Rotary(ENCODER_PIN_A, ENCODER_PIN_B);
SI4735 g_si4735;

//Single definition of the mode availability matrix (defs.h declares it extern
//so module TUs can use modeAvailable()).
const bool g_bandAvailableModes[][5] PROGMEM =
{
    { true,  true,  true,  true,  false },  // LW  - AM, LSB, USB, CW
    { true,  true,  true,  true,  false },  // MW  - AM, LSB, USB, CW
    { true,  true,  true,  true,  false },  // SW  - AM, LSB, USB, CW
    { true,  false, false, false, false },  // AIR - AM only (harmonic tuning)
    { false, false, false, false, true  },  // FM  - FM only
};
