// ----------------------------------------------------------------------
// Tuner control module (Milestone 7 extraction from main.cpp, pure
// refactor). Band/mode configuration, tuning, seek, SSB patch, BFO.
// ----------------------------------------------------------------------

#include <Arduino.h>
#include <SI4735.h>
#include "tuner.h"
#include "ui.h"
#include "eeprom_io.h"
#include "settings.h"
#include "Utils.h"
#include "patch_ssb_compressed.h"

extern SI4735 g_si4735;

//Keep only modes the current band supports (g_bandAvailableModes, defs.h);
//otherwise select the first available mode (scan AM..FM)
void sanitizeMode()
{
    if (!modeAvailable(g_bandIndex, g_currentMode))
    {
        uint8_t m = AM;
        while (!modeAvailable(g_bandIndex, m))
            m++;
        g_currentMode = m;
    }
}
#if USE_RDS
extern void setRDSConfig(uint8_t bias); //settings module
#endif

int getSteps()
{
    if (!isSSB())
    {
        if (g_stepIndex >= g_amTotalSteps)
            g_stepIndex = 0;

        return g_tabStep[g_stepIndex];
    }

    //Steps 50k and larger are not valid in SSB and overflow the 16-bit result in Hz
    uint8_t idx = g_stepIndex;
    if (idx >= g_amTotalStepsSSB && idx < g_amTotalSteps)
        idx = g_amTotalStepsSSB - 1;

    if (idx >= g_amTotalSteps)
        return g_tabStep[idx];

    return g_tabStep[idx] * 1000;
}
int getLastStep()
{
    if (isSSB())
        return g_amTotalSteps + g_ssbTotalSteps - 1;

    return g_amTotalSteps - 1;
}
//Saves more flash image size
void updateSSBCutoffFilter()
{
    // Auto mode: If SSB bandwidth 2 KHz or lower - it's better to enable cutoff filter
    if (g_Settings[SettingsIndex::CutoffFilter].param == 0 || g_currentMode == CW)
        g_si4735.setSSBSidebandCutoffFilter((g_bandwidthSSB[g_bwIndexSSB].idx == 0 || g_bandwidthSSB[g_bwIndexSSB].idx == 4 || g_bandwidthSSB[g_bwIndexSSB].idx == 5) ? 0 : 1);
    else
        g_si4735.setSSBSidebandCutoffFilter(g_Settings[SettingsIndex::CutoffFilter].param - 1);
}
//This function is called by station seek logic
void showFrequencySeek(uint16_t freq)
{
    g_currentFrequency = freq;
    delay(10);
    if (g_currentMode == FM)
    {
        //Fix random 10th KHz fraction
        freq = (freq / 10) * 10;
        g_currentFrequency = freq;
        g_si4735.setFrequency(g_currentFrequency);
    }
    else
        g_currentFrequency = g_si4735.getFrequency();

    g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
    showFrequency();
}
bool checkStopSeeking()
{
    return g_seekStop || !(PINC & (1 << (ENCODER_BUTTON - 14)));
}
void doSeek()
{
    if (g_seekDirection)
        g_si4735.frequencyUp();
    else
        g_si4735.frequencyDown();

#if USE_RDS
    if (g_displayRDS)
        eraseRdsSMeterArea();
#endif
    g_seekStop = false;
    g_si4735.seekStationProgress(showFrequencySeek, checkStopSeeking, g_seekDirection);
}
uint16_t getNextSWSuBband(bool up)
{
    uint16_t freq = g_currentFrequency;
    if (isSSB())
        freq += g_currentBFO / 1000;

    for (uint8_t i = 0; i < g_SWSubBandCount; i++)
    {
        uint8_t n = g_SWSubBandCount - 1 - i;
        if (!up && SWSubBands[n] < freq)
            return SWSubBands[n];
        else if (up && SWSubBands[i] > freq)
            return SWSubBands[i];
    }

    return 0;
}
//SW sub-band index the frequency belongs to (largest start <= freq)
uint8_t swSubBandFromFreq(uint16_t freq)
{
    uint8_t sub = 0;
    for (uint8_t i = 0; i < g_SWSubBandCount; i++)
        if (SWSubBands[i] <= freq)
            sub = i;
    return sub;
}
//Deferred band selection (plan.md Delta 8 + numbered SW sub-bands): while
//the band command (g_cmdBand) is active, encoder rotation only moves the
//selection; the real switch (step save, cleanup, chip reconfiguration)
//happens once on command exit. bandSwitch() keeps its immediate behavior
//(incl. SW sub-band hops) for the long-press fast scroll outside the
//command mode.
//
//Browsing operates on a flattened stop list: 0 = LW, 1 = MW, 2..2+SWN-1 =
//SW1..SWN sub-bands (SWN = g_SWSubBandCount), then AIR, then BCST (FM).
//Fast encoder spins move several stops per pass via the accumulated
//g_encoderCount.
static uint8_t g_bandSelFromBand;   //band active when the command started
static uint8_t g_bandSelFromStop;   //flattened stop active when it started
static bool g_bandSelectMoved;      //selection differs from the start
//Flattened stop index of the current band (uses the selected SW sub-band)
static uint8_t stopIndex()
{
    if (g_bandIndex == LW_BAND_TYPE) return 0;
    if (g_bandIndex == MW_BAND_TYPE) return 1;
    if (g_bandIndex == SW_BAND_TYPE) return 2 + g_bandSelSub;
    if (g_bandIndex == AIR_BAND_TYPE) return 2 + g_SWSubBandCount;
    return 3 + g_SWSubBandCount;
}
//Record the starting band when the band command turns on (called from
//switchCommand() in main.cpp)
void bandSelectBegin()
{
    g_bandBrowsing = true;
    g_bandSelFromBand = g_bandIndex;
    g_bandSelFromStop = stopIndex();
    if (g_bandIndex == SW_BAND_TYPE)
        g_bandSelSub = swSubBandFromFreq(g_currentFrequency);
    g_bandSelectMoved = false;
}
//Selection half: move through the flattened stop list, never touch the
//Si4735 and never redraw the frequency - only the band tag follows.
void bandSelectMove(bool up, uint8_t count)
{
    int16_t stop = stopIndex();
    const int16_t lastStop = 3 + g_SWSubBandCount;
    for (uint8_t i = 0; i < count; i++)
    {
        stop += up ? 1 : -1;
        if (stop > lastStop)
            stop = 0;
        else if (stop < 0)
            stop = lastStop;
    }

    if (stop <= 1)
        g_bandIndex = stop; //LW, MW
    else if (stop < 2 + g_SWSubBandCount)
    {
        g_bandIndex = SW_BAND_TYPE;
        g_bandSelSub = stop - 2;
    }
    else if (stop == 2 + g_SWSubBandCount)
        g_bandIndex = AIR_BAND_TYPE;
    else
        g_bandIndex = FM_BAND_TYPE;

    g_bandSelectMoved = (stop != g_bandSelFromStop);
    showBandTag();
}
//Exit hook for the band command: applies the deferred switch once if the
//selection differs from the start. Safe to call on every exit path.
bool bandSelectFinish()
{
    g_bandBrowsing = false;
    if (!g_bandSelectMoved)
        return false;
    g_bandSelectMoved = false;

    //Save the band we are leaving (its step globals are still current)
    if (g_currentMode == FM)
        g_bandList[g_bandSelFromBand].currentStepIdx = g_FMStepIndex;
    else
        g_bandList[g_bandSelFromBand].currentStepIdx = g_stepIndex;

    //Landing on SW parks the band on the selected sub-band start
    if (g_bandIndex == SW_BAND_TYPE)
        g_bandList[SW_BAND_TYPE].currentFreq = SWSubBands[g_bandSelSub];

    if (g_sMeterOn)
    {
        g_sMeterOn = false;
        eraseRdsSMeterArea();
    }

#if USE_RDS
    //Check the TARGET band: g_currentMode is still the starting band's
    //mode here, the reconfiguration happens in applyBandConfiguration() below
    if (g_displayRDS && g_bandIndex != FM_BAND_TYPE)
    {
        g_displayRDS = false;
        eraseRdsSMeterArea();
    }
#endif

    g_currentBFO = 0;
    if (isSSB())
        updateBFO();
    applyBandConfiguration();
    return true;
}
void bandSwitch(bool up)
{
    uint16_t nextSW = getNextSWSuBband(up);
    
    if (g_bandIndex == SW_BAND_TYPE && nextSW != 0)
    {
        g_currentFrequency = nextSW;

        g_currentBFO = 0;
        if (isSSB())
            updateBFO();
        g_si4735.setFrequency(nextSW);
        agcSetFunc(); //Re-apply to remove noize
        showFrequency();
        showBandTag();
    }
    else
    {
        if (g_currentMode == FM)
            g_bandList[g_bandIndex].currentStepIdx = g_FMStepIndex;
        else
            g_bandList[g_bandIndex].currentStepIdx = g_stepIndex;

        if (up)
        {
            if (g_bandIndex < g_lastBand)
                g_bandIndex++;
            else
                g_bandIndex = 0;
        }
        else
        {
            if (g_bandIndex > 0)
                g_bandIndex--;
            else
                g_bandIndex = g_lastBand;
        }

        if (g_sMeterOn)
        {
            g_sMeterOn = false;
            eraseRdsSMeterArea();
        }

#if USE_RDS
        //Check the TARGET band: g_currentMode is still FM here, the band
        //reconfiguration happens in applyBandConfiguration() below
        if (g_displayRDS && g_bandIndex != FM_BAND_TYPE)
        {
            g_displayRDS = false;
            eraseRdsSMeterArea();
        }
#endif

        g_currentBFO = 0;
        if (isSSB())
            updateBFO();
        applyBandConfiguration();
    }
}
// This function is required for using SSB. Si473x controllers do not support SSB by-default.
// But we can patch internal RAM of Si473x with special patch to make it work in SSB mode.
// Patch must be applied every time we enable SSB after AM or FM.
void loadSSBPatch()
{
    // This works, but i am not sure it's safe
    //g_si4735.setI2CFastModeCustom(700000);
    g_si4735.setI2CFastModeCustom(500000);
    g_si4735.queryLibraryId(); //Do we really need this? Research it.
    g_si4735.patchPowerUp();
    delay(50);
    g_si4735.downloadCompressedPatch(ssb_patch_content, sizeof(ssb_patch_content), cmd_0x15, sizeof(cmd_0x15));
    g_si4735.setSSBConfig(g_bandwidthSSB[g_bwIndexSSB].idx, 1, 0, 1, 0, 1);
    g_si4735.setI2CStandardMode();
    g_ssbLoaded = true;
    g_stepIndex = 0;
}
//Update receiver settings after changing band and modulation
void applyBandConfiguration(bool extraSSBReset)
{
    //Airband is tuned on a SW-range harmonic, the chip wants the SW antenna capacitor
    g_si4735.setTuneFrequencyAntennaCapacitor(uint16_t(g_bandIndex == SW_BAND_TYPE || g_bandIndex == AIR_BAND_TYPE));
    //Drop modes the band does not support (FM band -> FM, AIR band -> AM)
    sanitizeMode();
    if (g_bandIndex == FM_BAND_TYPE)
    {
        g_si4735.setFM(g_bandList[g_bandIndex].minimumFreq,
            g_bandList[g_bandIndex].maximumFreq,
            g_bandList[g_bandIndex].currentFreq,
            g_tabStepFM[g_bandList[g_bandIndex].currentStepIdx]);
        g_si4735.setSeekFmLimits(g_bandList[g_bandIndex].minimumFreq, g_bandList[g_bandIndex].maximumFreq);
        g_si4735.setSeekFmSpacing(1);
        g_ssbLoaded = false;
#if USE_RDS
        setRDSConfig(g_Settings[SettingsIndex::RDSError].param);
#endif
        g_si4735.setFifoCount(1);
        g_bwIndexFM = g_bandList[g_bandIndex].bandwidthIdx;
        g_si4735.setFmBandwidth(g_bwIndexFM);
        g_si4735.setFMDeEmphasis(g_Settings[SettingsIndex::DeEmp].param == 0 ? 1 : 2);
    }
    else
    {
        uint16_t minFreq = g_bandList[g_bandIndex].minimumFreq;
        uint16_t maxFreq = g_bandList[g_bandIndex].maximumFreq;
        if (g_bandIndex == SW_BAND_TYPE)
        {
            minFreq = SW_LIMIT_LOW;
            maxFreq = SW_LIMIT_HIGH;
        }

        //Airband is AM-only (sanitizeMode above selected AM), drop any SSB state coming from another band
        if (g_bandIndex == AIR_BAND_TYPE)
        {
            g_ssbLoaded = false;
        }

        if (g_ssbLoaded)
        {
            g_currentBFO = 0;
            if (extraSSBReset)
                loadSSBPatch();

            //Call this before to call crazy volume after AM when SVC is off
            g_si4735.setSSBAutomaticVolumeControl(g_Settings[SettingsIndex::SVC].param);
            g_si4735.setSSB(minFreq,
                maxFreq,
                g_bandList[g_bandIndex].currentFreq,
                g_bandList[g_bandIndex].currentStepIdx >= g_amTotalSteps ? 0 : g_tabStep[g_bandList[g_bandIndex].currentStepIdx],
                g_currentMode == CW ? g_Settings[SettingsIndex::CWSwitch].param + 1 : g_currentMode);
            updateSSBCutoffFilter();
            g_si4735.setSSBAutomaticVolumeControl(g_Settings[SettingsIndex::SVC].param);
            g_si4735.setSSBDspAfc(g_Settings[SettingsIndex::Sync].param == 1 ? 0 : 1);
            g_si4735.setSSBAvcDivider(g_Settings[SettingsIndex::Sync].param == 0 ? 0 : 3); //Set Sync mode
            g_si4735.setAmSoftMuteMaxAttenuation(g_Settings[SettingsIndex::SoftMute].param);
            g_si4735.setSSBAudioBandwidth(g_currentMode == CW ? g_bandwidthSSB[0].idx : g_bandwidthSSB[g_bwIndexSSB].idx);
            updateBFO();
            g_si4735.setSSBSoftMute(g_Settings[SettingsIndex::SSM].param);
        }
        else
        {
            g_currentMode = AM;
            //Airband step is fixed: 25 KHz real = 5 KHz on the chip side
            uint16_t amStep = (g_bandIndex == AIR_BAND_TYPE) ? AIR_STEP :
                (g_bandList[g_bandIndex].currentStepIdx >= g_amTotalSteps ? 0 : g_tabStep[g_bandList[g_bandIndex].currentStepIdx]);

            g_si4735.setAM(minFreq,
                maxFreq,
                g_bandList[g_bandIndex].currentFreq,
                amStep);
            g_si4735.setAmSoftMuteMaxAttenuation(g_Settings[SettingsIndex::SoftMute].param);
            g_bwIndexAM = g_bandList[g_bandIndex].bandwidthIdx;
            g_si4735.setBandwidth(g_bandwidthAM[g_bwIndexAM].idx, 1);
        }

        agcSetFunc();
        g_si4735.setAvcAmMaxGain(g_Settings[SettingsIndex::AutoVolControl].param);
        g_si4735.setSeekAmLimits(minFreq, maxFreq);
        g_si4735.setSeekAmSpacing((g_bandIndex == AIR_BAND_TYPE) ? AIR_STEP :
            (g_bandList[g_bandIndex].currentStepIdx >= g_amTotalSteps) ? 1 : g_tabStep[g_bandList[g_bandIndex].currentStepIdx]);
    }

    g_currentFrequency = g_bandList[g_bandIndex].currentFreq;
    if (g_currentMode == FM)
        g_FMStepIndex = g_bandList[g_bandIndex].currentStepIdx;
    else
        g_stepIndex = g_bandList[g_bandIndex].currentStepIdx;

    if ((g_bandIndex == LW_BAND_TYPE || g_bandIndex == MW_BAND_TYPE)
        && g_stepIndex > g_amTotalStepsSSB)
        g_stepIndex = g_amTotalStepsSSB;

    if (!g_settingsActive)
        showStatus(true);
    resetEepromDelay();
}
void updateBFO()
{
    //Actually to move frequency forward you need to move BFO backwards, so just * -1
    g_si4735.setSSBBfo((g_currentBFO + (g_Settings[SettingsIndex::BFO].param * 10)) * -1);
}
void agcSetFunc()
{
    uint8_t att = g_Settings[SettingsIndex::ATT].param;
    uint8_t disableAgc = att > 0;
    uint8_t agcNdx;
    if (att > 1) 
        agcNdx = att - 1;
    else
        agcNdx = 0;
    g_si4735.setAutomaticGainControl(disableAgc, agcNdx);
}
bool clampSSBBand()
{
    uint16_t freq = g_currentFrequency + (g_currentBFO / 1000);
    auto bfoReset = [&]()
    {
        g_currentBFO = 0;
        updateBFO();
        showFrequency(true);
        showModulation();
    };

    bool upd = false;
    if (freq > g_bandList[g_bandIndex].maximumFreq)
    {
        g_currentFrequency = g_bandList[g_bandIndex].minimumFreq;
        upd = true;
    }
    else if (freq < g_bandList[g_bandIndex].minimumFreq)
    {
        g_currentFrequency = g_bandList[g_bandIndex].maximumFreq;
        upd = true;
    }

    if (upd)
    {
        g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
        g_si4735.setFrequency(g_currentFrequency);
        bfoReset();
        return true;
    }

    return false;
}
void doFrequencyTune()
{
    g_seekDirection = g_encoderCount > 0 ? 1 : 0;

    //Update frequency
    g_previousFrequency = g_currentFrequency; //Force EEPROM update
    if (g_currentMode == FM)
    {
        g_currentFrequency += g_tabStepFM[g_FMStepIndex] * g_encoderCount; //g_si4735.getFrequency() is too slow
#if USE_RDS
        if (g_displayRDS)
            eraseRdsSMeterArea();
#endif
    }
    else
        g_currentFrequency += (g_bandIndex == AIR_BAND_TYPE ? (uint16_t)AIR_STEP : g_tabStep[g_stepIndex]) * g_encoderCount;
    uint16_t bMin = g_bandList[g_bandIndex].minimumFreq, bMax = g_bandList[g_bandIndex].maximumFreq;

    //Special logic for fast and responsive frequency surfing
    if (g_currentFrequency > bMax)
        g_currentFrequency = bMin;
    else if (g_currentFrequency < bMin)
        g_currentFrequency = bMax;

    g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
    g_processFreqChange = true;
    g_lastFreqChange = millis();

    showFrequency();
}
//Special feature to make SSB feel like on expensive TECSUN receivers
//BFO is now part of main frequency in SSB mode
void doFrequencyTuneSSB()
{
    const int BFOMax = 16000;
    int step = getSteps() * g_encoderCount;
    int newBFO = g_currentBFO + step;
    int redundant = 0;

    if (newBFO > BFOMax)
    {
        redundant = (newBFO / BFOMax) * BFOMax;
        g_currentFrequency += redundant / 1000;
        newBFO -= redundant;
    }
    else if (newBFO < -BFOMax)
    {
        redundant = ((abs(newBFO) / BFOMax) * BFOMax);
        g_currentFrequency -= redundant / 1000;
        newBFO += redundant;
    }

    g_currentBFO = newBFO;
    updateBFO();

    if (redundant != 0)
    {
        g_si4735.setFrequency(g_currentFrequency);
        agcSetFunc(); //Re-apply to remove noize
        g_currentFrequency = g_si4735.getFrequency();
        g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
    }

    g_bandList[g_bandIndex].currentFreq = g_currentFrequency + (g_currentBFO / 1000);
    g_lastFreqChange = millis();
    g_previousFrequency = 0; //Force EEPROM update
    if (!clampSSBBand()) //If we move outside of current band - switch it
        showFrequency();
}
