// ----------------------------------------------------------------------
// EEPROM storage module (Milestone 7 extraction from main.cpp, pure
// refactor). Saves/loads all receiver information, EEPROM delay helper.
// ----------------------------------------------------------------------

#include <Arduino.h>
#include <EEPROM.h>
#include <SI4735.h>
#include "eeprom_io.h"
#include "tuner.h"
#include "Utils.h"
#include "ui.h"

extern SI4735 g_si4735;


//EEPROM Save
void saveAllReceiverInformation()
{
    uint8_t addr = EEPROM_DATA_START_ADDRESS;
    EEPROM.update(EEPROM_VERSION_ADDRESS, APP_VERSION);
    EEPROM.update(EEPROM_APP_ID_ADDRESS, EEPROM_APP_ID);

    EEPROM.update(addr++, g_muteVolume > 0 ? g_muteVolume : g_si4735.getVolume());
    EEPROM.update(addr++, g_bandIndex);
    EEPROM.update(addr++, g_currentMode);
    EEPROM.update(addr++, g_currentBFO >> 8);
    EEPROM.update(addr++, g_currentBFO & 0XFF);
    EEPROM.update(addr++, g_FMStepIndex);
    EEPROM.update(addr++, g_prevMode); 
    EEPROM.update(addr++, g_bwIndexSSB);

    for (uint8_t i = 0; i <= g_lastBand; i++)
    {
        EEPROM.update(addr++, (g_bandList[i].currentFreq >> 8));
        EEPROM.update(addr++, (g_bandList[i].currentFreq & 0xFF));
        EEPROM.update(addr++, ((i != FM_BAND_TYPE && g_bandList[i].currentStepIdx >= g_amTotalSteps) ? 0 : g_bandList[i].currentStepIdx));
        EEPROM.update(addr++, g_bandList[i].bandwidthIdx);
    }

    for (uint8_t i = 0; i < SettingsIndex::SETTINGS_MAX; i++)
        EEPROM.update(addr++, g_Settings[i].param);
}
//EEPROM Load
void readAllReceiverInformation()
{
    uint8_t addr = EEPROM_DATA_START_ADDRESS;
    int8_t bwIdx;
    g_volume = EEPROM.read(addr++);
    g_bandIndex = EEPROM.read(addr++);
    g_currentMode = EEPROM.read(addr++);
    sanitizeMode();
    uint8_t bfoHi = EEPROM.read(addr++);
    uint8_t bfoLo = EEPROM.read(addr++);
    g_currentBFO = (int16_t)((bfoHi << 8) | bfoLo);
    g_FMStepIndex = EEPROM.read(addr++);
    g_prevMode = EEPROM.read(addr++);
    g_bwIndexSSB = EEPROM.read(addr++);

    for (uint8_t i = 0; i <= g_lastBand; i++)
    {
        uint8_t freqHi = EEPROM.read(addr++);
        uint8_t freqLo = EEPROM.read(addr++);
        g_bandList[i].currentFreq = (freqHi << 8) | freqLo;
        g_bandList[i].currentStepIdx = EEPROM.read(addr++);
        g_bandList[i].bandwidthIdx = EEPROM.read(addr++);
    }

    for (uint8_t i = 0; i < SettingsIndex::SETTINGS_MAX; i++)
        g_Settings[i].param = EEPROM.read(addr++);

    oled.setContrast(uint8_t(g_Settings[SettingsIndex::Brightness].param) * 2);

    g_previousFrequency = g_currentFrequency = g_bandList[g_bandIndex].currentFreq;
    if (g_bandIndex == FM_BAND_TYPE)
        g_FMStepIndex = g_bandList[g_bandIndex].currentStepIdx;
    else
        g_stepIndex = g_bandList[g_bandIndex].currentStepIdx;
    bwIdx = g_bandList[g_bandIndex].bandwidthIdx;
    if (g_stepIndex >= g_amTotalSteps)
        g_stepIndex = 0;

    if (isSSB())
    {
        loadSSBPatch();
        g_si4735.setSSBAudioBandwidth(g_bandwidthSSB[g_bwIndexSSB].idx);
        updateSSBCutoffFilter();
    }
    else if (g_currentMode == AM)
    {
        g_bwIndexAM = bwIdx;
        g_si4735.setBandwidth(g_bandwidthAM[g_bwIndexAM].idx, 1);
    }
    else
    {
        g_bwIndexFM = bwIdx;
        g_si4735.setFmBandwidth(g_bwIndexFM);
    }

    applyBandConfiguration();
}
//For saving features
void resetEepromDelay()
{
    g_storeTime = millis();
    g_previousFrequency = 0;
}

