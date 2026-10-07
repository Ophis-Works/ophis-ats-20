// ----------------------------------------------------------------------
// Direct frequency entry module (Milestone 7 extraction from main.cpp,
// pure refactor). Digit entry overlay via rotary encoder (plan.md Delta 4).
// ----------------------------------------------------------------------

#include <Arduino.h>
#include <SI4735.h>
#include "freq_entry.h"
#include "tuner.h"
#include "ui.h"
#include "eeprom_io.h"
#include "Utils.h"
#include "font14x24sevenSeg.h"
#include <SimpleButton.h>

extern SI4735 g_si4735;

//Direct frequency entry via rotary encoder (plan.md Delta 4)

void drawFreqEntry()
{
    char buf[7];
    //Positions before the cursor are confirmed, the cursor position is drawn
    //live (it is the one being edited), the rest stay blank '/'
    for (uint8_t i = 0; i < g_freqEntryCount; i++)
        buf[i] = (i <= g_freqEntryPos) ? ('0' + g_freqEntryDigits[i]) : '/';
    buf[g_freqEntryCount] = 0;
    oledPrint("/////////", 0, 3, FONT14X24SEVENSEG); // This character is an empty space in my seven seg font.
    oledPrint(buf, 12, 3, FONT14X24SEVENSEG);
    oledPrint("   ", 102, 4, DEFAULT_FONT); //Clear the unit area
}

void doFreqEntryCancel()
{
    g_freqEntryActive = false;
    showFrequency(); //Restore the live display
}

void doFreqEntryApply()
{
    uint32_t value = 0;
    for (uint8_t i = 0; i < g_freqEntryCount; i++)
        value = value * 10 + g_freqEntryDigits[i];

    uint16_t freq;
    if (g_bandIndex == AIR_BAND_TYPE)
        freq = value / AIR_FREQ_MULT; //Entry is in real airband kHz
    else
        freq = value; //kHz (LW/MW/SW) or MHz*10 (FM), same unit as g_currentFrequency

    //Clamp into the band range
    uint16_t bMin = g_bandList[g_bandIndex].minimumFreq, bMax = g_bandList[g_bandIndex].maximumFreq;
    if (freq > bMax)
        freq = bMax;
    else if (freq < bMin)
        freq = bMin;

    g_currentFrequency = freq;
    g_bandList[g_bandIndex].currentFreq = freq;

    if (isSSB())
    {
        //Entry sets the carrier frequency: reset BFO like a band switch
        g_currentBFO = 0;
        updateBFO();
        g_si4735.setFrequency(g_currentFrequency);
        agcSetFunc(); //Re-apply to remove noize
        g_currentFrequency = g_si4735.getFrequency();
        g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
    }
    else
    {
        //Deferred tune like doFrequencyTune()
        g_processFreqChange = true;
    }
    g_lastFreqChange = millis();
    resetEepromDelay(); //Also forces EEPROM update (g_previousFrequency = 0)
    showFrequency();
}

void doFreqEntryConfirm()
{
    g_freqEntryLastInput = millis();
    if (g_freqEntryPos + 1 == g_freqEntryCount)
    {
        g_freqEntryActive = false;
        doFreqEntryApply();
    }
    else
        g_freqEntryPos++;
}

void doFreqEntryDigit(int8_t dir)
{
    g_freqEntryDigits[g_freqEntryPos] = (g_freqEntryDigits[g_freqEntryPos] + 10 + dir) % 10;
    g_freqEntryLastInput = millis();
    drawFreqEntry();
}

void doFreqEntryStart()
{
    g_freqEntryActive = true;
    g_freqEntryPos = 0;
    g_freqEntryCount = (g_bandIndex == LW_BAND_TYPE || g_bandIndex == MW_BAND_TYPE) ? 4 :
                       (g_bandIndex == AIR_BAND_TYPE) ? 6 : 5; //SW: 5 kHz digits, FM: 5 MHz*10 digits
    for (uint8_t i = 0; i < g_freqEntryCount; i++)
        g_freqEntryDigits[i] = 0;
    g_freqEntryLastInput = millis();
    g_lastAdjustmentTime = 0; //No pending command timeout redraw while entering
    drawFreqEntry();
}

//Encoder button events: long-press toggles direct frequency entry, short-press
//keeps its existing per-context behavior (folded like simpleEvent did)
uint8_t encoderEvent(uint8_t event, uint8_t pin)
{
    if (BUTTONEVENT_FIRSTLONGPRESS == event)
    {
        if (!g_settingsActive && !g_cmdVolume && !g_cmdStep && !g_cmdBw && !g_cmdBand)
        {
            if (g_freqEntryActive)
                doFreqEntryCancel(); //Second long-press cancels
            else
                doFreqEntryStart();
            return BUTTON_IDLE; //Long-press fully handled here
        }
        event = BUTTONEVENT_SHORTPRESS; //No entry possible: previous folded behavior
    }
    return event;
}
