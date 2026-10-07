// ----------------------------------------------------------------------
// ATS_EX (Extended) Firmware for ATS-20 and ATS-20+ receivers.
// Based on PU2CLR sources.
// Inspired by closed-source swling.ru firmware.
// For more information check README file in my github repository:
// http://github.com/goshante/ats20_ats_ex
// ----------------------------------------------------------------------
// By Goshante
// 02.2024
// http://github.com/goshante
// ----------------------------------------------------------------------

#include <Arduino.h>

#include <SI4735.h>
#include <EEPROM.h>
#include <Tiny4kOLED.h>
#include "font_ui_8x16.h"

#include "Rotary.h"
#include "SimpleButton.h"

#include "defs.h"
#include "globals.h"
#include "Utils.h"
#include "ui.h"
#include "eeprom_io.h"
#include "tuner.h"
#include "settings.h"
#include "freq_entry.h"

void rotaryEncoder();



// --------------------------
// ------- Main logic -------
// --------------------------

//Initialize controller
void setup()
{
    //We need to save more space with this
    DDRB |=  (1 << DDB5);   //13 pin
    DDRD &= ~(1 << ENCODER_PIN_A);
    PORTD |= (1 << ENCODER_PIN_A);
    DDRD &= ~(1 << ENCODER_PIN_B);
    PORTD |= (1 << ENCODER_PIN_B);
    g_voltagePinConnnected = analogRead(BATTERY_VOLTAGE_PIN) > 300;

    oled.begin(128, 64, sizeof(tiny4koled_init_128x64br), tiny4koled_init_128x64br);
    oled.clear();
    oled.on();
    oled.setFont(DEFAULT_FONT);

    //Don't use digitalRead()
    //Registers save us more space
    if (!(PINC & (1 << (ENCODER_BUTTON - 14))))
    {
        saveAllReceiverInformation();
        oled.print("  EEPROM RESET");
        oled.setCursor(0, 2);
        for (uint8_t i = 0; i < 16; i++)
        {
            oled.print("-"); //Just fancy animation
            delay(60);
        }
    }
    else
    {
        oledPrint("OPHIS ATS 20", 16, 2, DEFAULT_FONT, true);
        //APP_VERSION 100 -> "V1.0" (minor 0-9 shows one digit, 10-99 two)
        uint8_t minor = APP_VERSION % 100;
        char ver[7] = {'V', (char)('0' + APP_VERSION / 100), '.',
                       (char)('0' + (minor >= 10 ? minor / 10 : minor)), 0};
        if (minor >= 10)
        {
            ver[4] = (char)('0' + minor % 10);
            ver[5] = 0;
        }
        oledPrint(ver, (128 - (3 + (minor >= 10 ? 2 : 1)) * 8) / 2, 4);
        delay(2000);
    }
    oled.clear();

    //Encoder interrupts
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);

    g_si4735.getDeviceI2CAddress(RESET_PIN);
    g_si4735.setup(RESET_PIN, MW_BAND_TYPE);

    delay(500);

    //Load settings from EEPROM
    if (EEPROM.read(EEPROM_VERSION_ADDRESS) == APP_VERSION && EEPROM.read(EEPROM_APP_ID_ADDRESS) == EEPROM_APP_ID)
        readAllReceiverInformation();
    else
        saveAllReceiverInformation();

    //Clock speed configuration
    noInterrupts(); //cli()
    CLKPR = 0x80;   //Allow edit CLKPR register
    CLKPR = g_Settings[SettingsIndex::CPUSpeed].param;
    interrupts();   //sei()

    //Initialize current band settings and read frequency
    applyBandConfiguration();
    g_currentFrequency = g_previousFrequency = g_si4735.getFrequency();
    g_si4735.setVolume(g_volume);

    //Draw main screen
    oled.clear();
    showStatus();
}

uint8_t volumeEvent(uint8_t event, uint8_t pin)
{
    if (g_muteVolume)
    {
        if (!BUTTONEVENT_ISDONE(event))
        {
            if ((BUTTONEVENT_SHORTPRESS != event) || (VOLUME_BUTTON == pin))
                doVolume(1);
        }
    }

    if (!g_muteVolume)
    {
#if (0 != VOLUME_DELAY)
#if (VOLUME_DELAY > 1)
        static uint8_t count;
        if (BUTTONEVENT_FIRSTLONGPRESS == event)
        {
            count = 0;
        }
#endif
        if (BUTTONEVENT_ISLONGPRESS(event))
            if (BUTTONEVENT_LONGPRESSDONE != event)
            {
#if (VOLUME_DELAY > 1)
                if (count++ == 0)
#endif
                    doVolume(VOLUME_BUTTON == pin ? 1 : -1);
#if (VOLUME_DELAY > 1)
                count = count % VOLUME_DELAY;
#endif
            }
#else
        if (BUTTONEVENT_FIRSTLONGPRESS == event)
            event = BUTTONEVENT_SHORTPRESS;
#endif
    }
    return event;
}

uint8_t simpleEvent(uint8_t event, uint8_t pin)
{
    if (BUTTONEVENT_FIRSTLONGPRESS == event)
        event = BUTTONEVENT_SHORTPRESS;
    return event;
}

uint8_t bandEvent(uint8_t event, uint8_t pin)
{
#if (0 != BAND_DELAY)
    static uint8_t count;
    if (BUTTONEVENT_ISLONGPRESS(event) && !g_settingsActive)
    {
        if (BUTTONEVENT_LONGPRESSDONE != event)
        {
            if (BUTTONEVENT_FIRSTLONGPRESS == event)
            {
                count = 0;
            }
            if (count++ == 0)
            {
                if (BAND_BUTTON == pin)
                {
                    if (g_bandIndex < g_lastBand)
                        bandSwitch(true);
                }
                else
                {
                    if (g_bandIndex)
                        bandSwitch(false);
                }
            }
            count = count % BAND_DELAY;
        }
    }
#else
    if (BUTTONEVENT_FIRSTLONGPRESS == event)
        event = BUTTONEVENT_SHORTPRESS;
#endif
    return event;
}

// Handle encoder direction
void rotaryEncoder()
{
    uint8_t encoderStatus = g_encoder.process();
    if (encoderStatus)
    {
        //Accumulate detents made between loop passes for faster scrolling
        g_encoderCount += (encoderStatus == DIR_CW) ? 1 : -1;
        g_seekStop = true;
    }
}




//Draw frequency. 
//BFO and main frequency produce actual frequency that is displayed on LCD
//Too sensitive logic, do not change




//Update and draw main screen UI. 
//basicUpdate - update minimum as possible
//cleanFreq   - force clean frequency line

//Converts settings value to UI value
// If full false - update only value
//Update and draw settings UI (the 3-row sliding viewport)

//BAND+ in settings: jump the viewport back to the top of the list
//Switch between main screen and settings mode
//Draw curremt modulation
//stays visible in every state (fixed slot, plan.md Delta 6).
//Draw current band
//Draw volume level

//Draw steps (with units)

//Draw bandwidth (Ignored for CW mode)







void switchCommand(bool* b, void (*showFunction)())
{
    static bool* prev = NULL;
    static void (*prevFunc)() = NULL;

    if (!b)
    {
        if (prev)
        {
            //Timeout / BAND- exit: apply the pending deferred band switch
            if (prev == &g_cmdBand)
                bandSelectFinish();
            *prev = false;
            if (prevFunc)
                prevFunc();
            g_lastAdjustmentTime = 0;
            prev = NULL;
        }
        return;
    }

    bool last = *b;
    prev = b;
    prevFunc = showFunction;

    //Deferred band switching (plan.md Delta 8): record the starting band
    //when the command turns on; apply the pending switch once when it
    //turns off via this toggle
    if (b == &g_cmdBand)
    {
        if (last)
            bandSelectFinish();
        else
            bandSelectBegin();
    }

    if (*b == false)
    {
        g_cmdVolume = false;
        g_cmdStep = false;
        g_cmdBw = false;
        g_cmdBand = false;
        g_lastAdjustmentTime = millis();
        showVolume();
        showStep();
        showBandwidth();
        showModulation();
    }
    else
        g_lastAdjustmentTime = 0;

    *b = !last;

    if (showFunction)
        showFunction();
}



void resetLowerLine()
{
    if (g_sMeterOn || g_displayRDS)
    {
        g_sMeterOn = false;
        g_displayRDS = false;
        updateLowerDisplayLine();
    }
}



void loop()
{
    bool skipButtonEvents = false;
    bool frequencyRecentlyUpdated = millis() - g_lastFreqChange < 70;

    //Faster frequency tune
    if (g_processFreqChange && !isSSB())
    {
        if (!frequencyRecentlyUpdated && g_encoderCount == 0)
        {
            g_si4735.setFrequency(g_currentFrequency);
            g_processFreqChange = false;
        }
        else if (frequencyRecentlyUpdated && g_encoderCount != 0)
        {
            doFrequencyTune();
            g_encoderCount = 0;
            return;
        }
    }
    
    if (millis() - g_lastFreqChange >= 1000)
    {
#if USE_RDS
        showRDS();
#endif

        if (g_sMeterOn && !g_settingsActive)
            showSMeter();

        showCharge(false);
    }

    if (g_lastAdjustmentTime != 0 && millis() - g_lastAdjustmentTime > ADJUSTMENT_ACTIVE_TIMEOUT)
        switchCommand(NULL, NULL);

    //Encoder rotation check
    if (g_encoderCount != 0)
    {
        //Menus act on a single step per pass, frequency tuning uses the accumulated count
        int8_t dir = (g_encoderCount > 0) ? 1 : -1;

        if (g_lastAdjustmentTime != 0)
            g_lastAdjustmentTime = millis();

        if (g_freqEntryActive)
        {
            doFreqEntryDigit(dir);
        }
        else if (g_settingsActive)
        {
            if (!g_SettingEditing)
            {
                int8_t next = g_SettingSelected + g_encoderCount;

                //No wraparound: clamp at both ends, slide the viewport
                if (next < 0)
                    next = 0;
                else if (next > SettingsIndex::SETTINGS_MAX - 1)
                    next = SettingsIndex::SETTINGS_MAX - 1;
                g_SettingSelected = next;

                if (g_SettingSelected < g_SettingsTop)
                    g_SettingsTop = g_SettingSelected;
                else if (g_SettingSelected > g_SettingsTop + 2)
                    g_SettingsTop = g_SettingSelected - 2;

                showSettingsTitle();
                showSettings();
            }
            else
            {
                (*g_Settings[g_SettingSelected].manipulateCallback)(dir);
                DrawSetting(g_SettingSelected, false);
                delay(25);
            }
        }
        else if (g_cmdVolume)
            doVolume(dir);
        else if (g_cmdStep)
            doStep(dir);
        else if (g_cmdBw)
            doBandwidth(dir);
        else if (g_cmdBand)
        {
            //Deferred band selection (Delta 8): selection only, no chip
            //access, no frequency redraw. Fast spins move several stops
            //at once via the accumulated detent count.
            bandSelectMove(dir == 1, g_encoderCount < 0 ? -g_encoderCount : g_encoderCount);
        }
        else if (isSSB())
        {
            doFrequencyTuneSSB();
            skipButtonEvents = true;
        }
        else
        {
            doFrequencyTune();
            skipButtonEvents = true;
        }
        g_encoderCount = 0;
        resetEepromDelay();
    }

    if (skipButtonEvents)
        goto saveAttempt;

    //Direct frequency entry active: other buttons are ignored (events consumed
    //without action), encoder short-press confirms the digit, timeout cancels
    if (g_freqEntryActive)
    {
        if (millis() - g_freqEntryLastInput > FREQ_ENTRY_TIMEOUT)
            doFreqEntryCancel();
        else
        {
            //Consume without action (simpleEvent has no side effects)
            btn_Bandwidth.checkEvent(simpleEvent);
            btn_BandUp.checkEvent(simpleEvent);
            btn_BandDn.checkEvent(simpleEvent);
            btn_VolumeUp.checkEvent(simpleEvent);
            btn_VolumeDn.checkEvent(simpleEvent);
            btn_AGC.checkEvent();
            btn_Step.checkEvent();
            btn_Mode.checkEvent(simpleEvent);
            if (BUTTONEVENT_SHORTPRESS == btn_Encoder.checkEvent(encoderEvent))
                doFreqEntryConfirm();
        }
        goto saveAttempt;
    }

    //Own scope so the goto above does not cross local initializations
    {
    //Command-checkers
    if (BUTTONEVENT_SHORTPRESS == btn_Bandwidth.checkEvent(simpleEvent))
    {
        if (!g_settingsActive && g_currentMode != CW)
            switchCommand(&g_cmdBw, showBandwidth);
    }
    if (BUTTONEVENT_SHORTPRESS == btn_BandUp.checkEvent(bandEvent))
    {
        if (!g_settingsActive)
        {
            resetLowerLine();
            switchCommand(&g_cmdBand, showModulation);
        }
        else
        {
            switchSettingsTop();
        }
    }
    if (BUTTONEVENT_SHORTPRESS == btn_BandDn.checkEvent(bandEvent))
    {
        if (!g_settingsActive)
            switchCommand(NULL, NULL);
        g_settingsActive = !g_settingsActive;
        switchSettings();
    }
    if (BUTTONEVENT_SHORTPRESS == btn_VolumeUp.checkEvent(volumeEvent))
    {
        if (!g_settingsActive && g_muteVolume == 0)
            switchCommand(&g_cmdVolume, showVolume);
    }
    if (BUTTONEVENT_SHORTPRESS == btn_VolumeDn.checkEvent(volumeEvent))
    {
        if (!g_cmdVolume)
        {
            uint8_t vol = g_si4735.getCurrentVolume();
            if (vol > 0 && g_muteVolume == 0)
            {
                g_muteVolume = vol;
                g_si4735.setVolume(0);
            }
            else if (g_muteVolume > 0)
            {
                g_si4735.setVolume(g_muteVolume);
                g_muteVolume = 0;
            }
            showVolume();
        }
    }
    if (BUTTONEVENT_SHORTPRESS == btn_Encoder.checkEvent(encoderEvent))
    {
        if (g_cmdBand)
        {
            //Encoder-confirm exit: apply the pending deferred band switch
            bandSelectFinish();
            g_cmdBand = false;
            showModulation();
        }
        else if (g_cmdStep)
        {
            g_cmdStep = false;
            showStep();
        }
        else if (g_cmdBw)
        {
            g_cmdBw = false;
            showBandwidth();
        }
        else if (g_cmdVolume)
        {
            g_cmdVolume = false;
            showVolume();
        }
        else if (g_settingsActive)
        {
            g_SettingEditing = !g_SettingEditing;
            DrawSetting(g_SettingSelected, true);
        }
        else if (g_displayRDS)
            g_rdsSwitchPressed = true;
        else if (isSSB() || g_Settings[SettingsIndex::ScanSwitch].param == 0)
        {
            if (!g_settingsActive)
            {
                switchCommand(&g_cmdStep, showStep);
                resetLowerLine();
            }
        }
        //Seek in SSB/CW is not allowed
        else if (g_currentMode == FM || g_currentMode == AM)
            doSeek();
    }

    //Pass no callback: the events are fully handled below and passing the same-named
    //local result variable as an event handler was undefined behavior (garbage function pointer)
    uint8_t agcEvent = btn_AGC.checkEvent();
    if (BUTTONEVENT_SHORTPRESS == agcEvent)
    {
        if (!g_settingsActive || (g_settingsActive && !g_displayOn))
        {
            g_displayOn = !g_displayOn;
            if (g_displayOn)
                oled.on();
            else
                oled.off();
        }
    }
    if (BUTTONEVENT_LONGPRESS == agcEvent)
    {
        if (!g_settingsActive)
        {
            if (isSSB())
                doSync(1);
        }
    }
    uint8_t stepEvent = btn_Step.checkEvent();
    if (BUTTONEVENT_SHORTPRESS == stepEvent)
    {
        if (!g_settingsActive)
        {
            switchCommand(&g_cmdStep, showStep);
            resetLowerLine();
        }
    }
    if (BUTTONEVENT_LONGPRESSDONE == stepEvent)
    {
        if (!g_settingsActive)
        {
            g_sMeterOn = !g_sMeterOn;
            if (g_sMeterOn)
            {
                g_displayRDS = false;
                showSMeter();
            }
            else
                updateLowerDisplayLine();
        }
    }
    if (BUTTONEVENT_SHORTPRESS == btn_Mode.checkEvent(simpleEvent))
    {
        if (!g_settingsActive)
        {
            //Do nothing on FM mode (unfortunately no NBFM patch), otherwise switch AM modulation
            //Airband stays AM too: SSB at VHF is out of scope of the patch and the airband hack
            if (g_currentMode != FM && g_bandIndex != AIR_BAND_TYPE)
            {
                g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
                g_prevMode = g_currentMode;
                switch (g_currentMode)
                {
                case AM:
                    //Patch Si473x memory every time when enabling SSB
                    loadSSBPatch();
                    g_processFreqChange = false;
                    //Allow pass through

                case LSB:
                    g_currentMode++;
                    g_bandList[g_bandIndex].currentFreq += g_currentBFO / 1000;
                    break;

                case USB:
                    g_currentMode++;
                    g_cmdBw = false;
                    g_bandList[g_bandIndex].currentFreq += g_currentBFO / 1000;
                    break;

                case CW:
                    g_currentMode = AM;
                    g_ssbLoaded = false;
                    if (g_stepIndex >= g_amTotalSteps)
                        g_stepIndex = 0;

                    g_currentFrequency += (g_currentBFO / 1000);
                    break;
                }

                g_bandList[g_bandIndex].currentStepIdx = g_stepIndex;
                applyBandConfiguration();
            }
#if USE_RDS
            else if (g_currentMode == FM)
                doRDS();
#endif
        }
    }
    }

saveAttempt:
    //Save EEPROM if anough time passed and frequency changed
    if (g_currentFrequency != g_previousFrequency)
    {
        if ((millis() - g_storeTime) > STORE_TIME)
        {
            saveAllReceiverInformation();
            g_storeTime = millis();
            g_previousFrequency = g_currentFrequency;
        }
    }
}

//Overriding original main to save some space
int main(void)
{
    init();
    setup();
    while(1)
        loop();
    return 0;
}
