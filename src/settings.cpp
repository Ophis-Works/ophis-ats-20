// ----------------------------------------------------------------------
// Settings callbacks module (Milestone 7 extraction from main.cpp, pure
// refactor). The do* manipulate functions, bandwidth logic and RDS config.
// ----------------------------------------------------------------------

#include <Arduino.h>
#include <SI4735.h>
#include "settings.h"
#include "tuner.h"
#include "ui.h"
#include "Utils.h"

extern SI4735 g_si4735;

#if USE_RDS
void setRDSConfig(uint8_t bias)
{
    g_si4735.setRdsConfig(1, bias, bias, bias, bias);
}
#endif
//Step value regulation
void doStep(int8_t v)
{
    //Airband has a fixed 25 KHz channel step (5 KHz on the chip side)
    if (g_bandIndex == AIR_BAND_TYPE)
        return;

    if (g_currentMode == FM)
    {
        g_FMStepIndex = (v == 1) ? g_FMStepIndex + 1 : g_FMStepIndex - 1;
        if (g_FMStepIndex > g_lastStepFM)
            g_FMStepIndex = 0;
        else if (g_FMStepIndex < 0)
            g_FMStepIndex = g_lastStepFM;

        g_si4735.setFrequencyStep(g_tabStepFM[g_FMStepIndex]);
        g_bandList[g_bandIndex].currentStepIdx = g_FMStepIndex;
        g_si4735.setSeekFmSpacing(1);
        showStep();
    }
    else
    {
        g_stepIndex = (v == 1) ? g_stepIndex + 1 : g_stepIndex - 1;
        if (g_stepIndex > getLastStep())
            g_stepIndex = 0;
        else if (g_stepIndex < 0)
            g_stepIndex = getLastStep();

        //SSB Step limit
        else if (isSSB() && g_stepIndex >= g_amTotalStepsSSB && g_stepIndex < g_amTotalSteps)
            g_stepIndex = v == 1 ? g_amTotalSteps : g_amTotalStepsSSB - 1;
        
        //LW/MW Step limit
        else if ((g_bandIndex == LW_BAND_TYPE || g_bandIndex == MW_BAND_TYPE)
            && v == 1 && g_stepIndex > g_amTotalStepsSSB && g_stepIndex < g_amTotalSteps)
            g_stepIndex = g_amTotalSteps;
        else if ((g_bandIndex == LW_BAND_TYPE || g_bandIndex == MW_BAND_TYPE)
            && v != 1 && g_stepIndex > g_amTotalStepsSSB && g_stepIndex < g_amTotalSteps)
            g_stepIndex = g_amTotalStepsSSB;

        if (!isSSB() || (isSSB() && g_stepIndex < g_amTotalSteps))
        {
            g_si4735.setFrequencyStep(g_tabStep[g_stepIndex]);
            g_bandList[g_bandIndex].currentStepIdx = g_stepIndex;
        }

        if (!isSSB())
            g_si4735.setSeekAmSpacing((g_bandList[g_bandIndex].currentStepIdx >= g_amTotalSteps) ? 1 : g_tabStep[g_bandList[g_bandIndex].currentStepIdx]);
        showStep();
    }
}


//Volume control
void doVolume(int8_t v)
{
    if (g_muteVolume)
    {
        g_si4735.setVolume(g_muteVolume);
        g_muteVolume = 0;
    }
    else
    {
        if (v == 1)
            g_si4735.volumeUp();
        else
            g_si4735.volumeDown();
    }
    showVolume();
}

//Helps to save more flash image size
void doSwitchLogic(int8_t& param, int8_t low, int8_t high, int8_t step)
{
    param += step;
    if (param < low)
        param = high;
    else if (param > high)
        param = low;
}


//Settings: Attenuation
void doAttenuation(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::ATT].param, 0, 37, v);
    agcSetFunc();
}

//Settings: Soft Mute
void doSoftMute(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::SoftMute].param, 0, 32, v);

    if (g_currentMode != FM)
        g_si4735.setAmSoftMuteMaxAttenuation(g_Settings[SettingsIndex::SoftMute].param);
}

//Settings: Brightness
void doBrightness(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::Brightness].param, 5, 125, v);
    oled.setContrast(uint8_t(g_Settings[SettingsIndex::Brightness].param) * 2);
}

//Settings: SSB AVC Switch
void doSSBAVC(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::SVC].param, 0, 1, v);

    if (isSSB())
    {
        g_si4735.setSSBAutomaticVolumeControl(g_Settings[SettingsIndex::SVC].param);
        applyBandConfiguration(true);
    }
}

//Settings: Automatic Volume Control
void doAvc(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::AutoVolControl].param, 12, 90, v);

    if (g_currentMode != FM)
        g_si4735.setAvcAmMaxGain(g_Settings[SettingsIndex::AutoVolControl].param);
}

//Settings: Sync switch
void doSync(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::Sync].param, 0, 1, v);

    if (isSSB())
    {
        g_si4735.setSSBDspAfc(g_Settings[SettingsIndex::Sync].param == 1 ? 0 : 1);
        g_si4735.setSSBAvcDivider(g_Settings[SettingsIndex::Sync].param == 0 ? 0 : 3); //Set Sync mode
        applyBandConfiguration(true);
    }
}

//Settings: FM DeEmp switch (50 or 75)
void doDeEmp(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::DeEmp].param, 0, 1, v);

    if (g_currentMode == FM)
        g_si4735.setFMDeEmphasis(g_Settings[SettingsIndex::DeEmp].param == 0 ? 1 : 2);
}

//Settings: SW Units / Units switch / Scan switch (identical 0..1 toggles)
void doSWUnits(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::SWUnits].param, 0, 1, v);
}

//Settings: SW Units
void doSSBSoftMuteMode(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::SSM].param, 0, 1, v);

    if (isSSB())
        g_si4735.setSSBSoftMute(g_Settings[SettingsIndex::SSM].param);
}

//Settings: SSB Cutoff filter
void doCutoffFilter(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::CutoffFilter].param, 0, 2, v);

    if (isSSB())
        updateSSBCutoffFilter();
}

//Settings: CPU Frequency divider
void doCPUSpeed(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::CPUSpeed].param, 0, 1, v);

    noInterrupts();
    CLKPR = 0x80;
    CLKPR = g_Settings[SettingsIndex::CPUSpeed].param;
    interrupts();
}

//Settings: BFO Offset calibration
void doBFOCalibration(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::BFO].param, -60, 60, v);

    if (isSSB())
    {
#if USE_RDS
        setRDSConfig(g_Settings[SettingsIndex::BFO].param);
#endif
        updateBFO();
    }
}

//Settings: CW mode switch
void doCWSwitch(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::CWSwitch].param, 0, 1, v);

    if (g_currentMode == CW)
        applyBandConfiguration(true);
}

#if USE_RDS
//Settings: RDS Error Level
void doRDSErrorLevel(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::RDSError].param, 0, 3, v);

    if (g_currentMode == FM)
        setRDSConfig(g_Settings[SettingsIndex::RDSError].param);
}


void doRDS()
{
    g_displayRDS = !g_displayRDS;

    if (g_displayRDS)
    {
        g_sMeterOn = false;
        eraseRdsSMeterArea();
        g_si4735.getRdsStatus();
        showRDS();
    }
    else
        updateLowerDisplayLine();
}
#endif

//Prevents repeatable code for flash image size saving
void doBandwidthLogic(int8_t& bwIndex, uint8_t upperLimit, uint8_t v)
{
    doSwitchLogic(bwIndex, 0, upperLimit, v);
    g_bandList[g_bandIndex].bandwidthIdx = bwIndex;
}

//Bandwidth regulation logic
void doBandwidth(uint8_t v)
{
    if (isSSB())
    {
        doSwitchLogic(g_bwIndexSSB, 0, g_bwSSBMaxIdx, v);
        g_si4735.setSSBAudioBandwidth(g_bandwidthSSB[g_bwIndexSSB].idx);
        updateSSBCutoffFilter();
    }
    else if (g_currentMode == AM)
    {
        doBandwidthLogic(g_bwIndexAM, g_maxFilterAM, v);
        g_si4735.setBandwidth(g_bandwidthAM[g_bwIndexAM].idx, 1);
    }
    else
    {
        doBandwidthLogic(g_bwIndexFM, 4, v);
        g_si4735.setFmBandwidth(g_bwIndexFM);
    }
    showBandwidth();
}
