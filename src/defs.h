#pragma once

#include <avr/pgmspace.h>
#include "font_ui_8x16.h"
#include "font_bm16.h"

//If you set this def to 0 project will be compiled without RDS
//and everything related to RDS will be excluded from build
//Override with -DUSE_RDS=0 (see [env:nano_lite] in platformio.ini)
#ifndef USE_RDS
#define USE_RDS 1
#endif

#define EEPROM_APP_ID				235
#define EEPROM_DATA_START_ADDRESS	1
#define EEPROM_VERSION_ADDRESS      1000
#define EEPROM_APP_ID_ADDRESS       0

//EEPROM layout version, also displayed on the splash as "V<major>.<minor>".
//100 = Ophis v1.0. Bumping it invalidates saved settings (one re-init on boot).
#define APP_VERSION 100

//EEPROM Settings
#define STORE_TIME 3000  // Inactive time to save our settings

// OLED Const values
//TEST: BMplain 2x2 experiment (revert to FONT8X16POB_UPPER to go back)
#define DEFAULT_FONT FONT_BMPLAIN2X
#define RST_PIN -1
#define RESET_PIN 12

//Battery charge monitoring analog pin. The ATS-20+ has a battery divider
//wired to A1 at the factory - no mod needed. The original ATS-20 has no
//divider; the solder mod (10-10 KOhm) goes to A2 and requires changing
//this define back.
#define BATTERY_VOLTAGE_PIN A1

// Encoder
#define ENCODER_PIN_A 2
#define ENCODER_PIN_B 3

// Buttons
#define MODE_SWITCH       4 
#define BANDWIDTH_BUTTON  5
#define VOLUME_BUTTON     6
#define AVC_BUTTON        7
#define BAND_BUTTON       8 
#define SOFTMUTE_BUTTON   9
#define AGC_BUTTON       11
#define STEP_BUTTON      10

#define ENCODER_BUTTON   14

// Default values
#define DEFAULT_VOLUME 25
#define ADJUSTMENT_ACTIVE_TIMEOUT 3000

// Band settings
#define SW_LIMIT_LOW		1710
#define SW_LIMIT_HIGH		30000
#define LW_LIMIT_LOW		153
#define CB_LIMIT_LOW		26200
#define CB_LIMIT_HIGH		28000

// Airband (118.000 - 136.975 MHz, AM). Out-of-spec Si4732/35 mode: the airband
// is received through the 5th LO harmonic, so the chip is tuned to 1/5 of the
// real frequency (23600 - 27395 KHz, inside the documented SW range).
// 5 is the only integer divisor of the whole band, which also maps the 25 KHz
// channel step to whole KHz on the chip side. See airband_si4732.md.
// Reception quality depends on the unit, it is not a characterized range.
#define AIR_FREQ_MULT		5
#define AIR_LIMIT_LOW		23600		//118.000 MHz
#define AIR_LIMIT_HIGH		27395		//136.975 MHz
#define AIR_DEFAULT_FREQ	24300		//121.500 MHz
#define AIR_STEP			5			//25 KHz real

//Direct frequency entry (Delta 4): cancel after this many ms without input
#define FREQ_ENTRY_TIMEOUT	5000

#define BAND_DELAY                 2
#define VOLUME_DELAY               1

// Rows: LW, MW, SW, AIR, FM (BandType). Columns: AM, LSB, USB, CW, FM
// (Modulations, types.h; count is 5 = FM + 1, FM is not declared yet here).
// Single source of truth for which modes each band supports.
// Definition lives in globals.h (single-include header, like all globals).
extern const bool g_bandAvailableModes[][5] PROGMEM;

inline bool modeAvailable(uint8_t band, uint8_t mode)
{
    return pgm_read_byte(&g_bandAvailableModes[band][mode]);
}

#define buttonEvent                NULL