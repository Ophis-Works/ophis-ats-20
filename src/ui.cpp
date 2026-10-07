// ----------------------------------------------------------------------
// UI drawing module (Milestone 7 extraction from main.cpp, pure refactor).
// All show*/draw functions: frequency, modulation, band tag, volume,
// charge, step, S-meter, RDS and the settings screens.
// ----------------------------------------------------------------------

#include <SI4735.h>
#include "ui.h"
#include "eeprom_io.h"
#include "Utils.h"
#include "font14x24sevenSeg.h"

extern SI4735 g_si4735;

void showFrequency(bool cleanDisplay)
{
    if (g_settingsActive)
        return;

    char unit[4];
    char freqDisplay[7];
    char ssbSuffix[4];
    static uint8_t prevLen = 0;
    uint16_t khzBFO, tailBFO;
    uint8_t off = (isSSB() ? -5 : 4) + 8;

    unit[0] = 'K';
    unit[1] = 'H';
    unit[2] = 'Z';
    unit[3] = 0x0;

    ssbSuffix[0] = '.';
    ssbSuffix[1] = '0';
    ssbSuffix[2] = '0';
    ssbSuffix[3] = '\0';

    if (g_bandIndex == FM_BAND_TYPE)
    {
        convertToChar(freqDisplay, g_currentFrequency, 5, 3, '.', '/');
        unit[0] = 'M';
    }
    else if (g_bandIndex == AIR_BAND_TYPE)
    {
        //Chip frequency is 1/5 of the real airband frequency (see defs.h).
        //5*chip overflows 16-bit math (AVR int), so emit the low digit
        //separately: 5*chip = 10*(chip/2) + 5*(chip&1) -> digits of chip/2
        //followed by a trailing '0' or '5'
        convertToChar(freqDisplay, g_currentFrequency / 2, 5, 0, 0);
        freqDisplay[5] = '0' + (g_currentFrequency & 1) * 5;
        freqDisplay[6] = '\0';
    }
    else
    {
        if (g_bandIndex == SW_BAND_TYPE)
            showBandTag();

        if (!isSSB())
        {
            bool swMhz = g_Settings[SettingsIndex::SWUnits].param == 1;
            convertToChar(freqDisplay, g_currentFrequency, 5, (g_bandIndex == SW_BAND_TYPE && swMhz) ? 2 : 0, '.', '/');
            if (g_bandIndex == SW_BAND_TYPE && swMhz)
                unit[0] = 'M';
        }
        else
        {
            splitFreq(khzBFO, tailBFO);
            //utoa(freqDisplay, khzBFO);
            convertToChar(freqDisplay, khzBFO, ilen(khzBFO));
        }
    }

    uint8_t len = isSSB() ? ilen(khzBFO) : ilen(g_currentFrequency);
    if (cleanDisplay)
    {
        oled.setCursor(0, 3);
        oledPrint("/////////", 0, 3, FONT14X24SEVENSEG); // This character is an empty space in my seven seg font.
    }
    else if (isSSB() && len > prevLen && len == 5)
        oledPrint("   ", 102, 4, DEFAULT_FONT);

    oledPrint(freqDisplay, off, 3, FONT14X24SEVENSEG);

    if (isSSB())
    {
        //utoa((ilen(tailBFO) == 1) ? &ssbSuffix[2] : &ssbSuffix[1], tailBFO);
        convertToChar((ilen(tailBFO) == 1) ? &ssbSuffix[2] : &ssbSuffix[1], tailBFO, ilen(tailBFO));
        ssbSuffix[3] = 0;
        oledPrint(ssbSuffix);
        if (len != prevLen && len < prevLen)
            oledPrint("/");
    }

    if (g_Settings[SettingsIndex::UnitsSwitch].param == 1 && (!isSSB() || (isSSB() && len < 5)))
        oledPrint(unit, 102, 4, DEFAULT_FONT);
        
    prevLen = len;
}
void showStatus(bool cleanFreq)
{
    showFrequency(cleanFreq);
    showModulation();
    showStep();
    showBandwidth();
    showCharge(true);
    showVolume();
}
void updateLowerDisplayLine()
{
    oledPrint(_literal_EmptyLine, 0, 6, DEFAULT_FONT);
    showModulation();
    showStep();
    showCharge(true);
}

void SettingParamToUI(char* buf, uint8_t idx)
{
    int8_t param = g_Settings[idx].param;
    switch (g_Settings[idx].type)
    {
    case SettingType::ZeroAuto:
        if (param == 0)
        {
            buf[0] = 'A';
            buf[1] = 'U';
            buf[2] = 'T';
            buf[3] = 0x0;
        }
        else
            convertToChar(buf, param, 3);

        break;

    case SettingType::Num:
        convertToChar(buf, abs(param), 3);
        if (param < 0)
            buf[0] = '-';
        break;

    case SettingType::SwitchAuto:
        if (param == 0)
        {
            buf[0] = 'A';
            buf[1] = 'U';
            buf[2] = 'T';       }
        else if (param == 1)
        {
            buf[0] = 'O';
            buf[1] = 'N';
            buf[2] = ' ';
        }
        else
        {
            buf[0] = 'O';
            buf[1] = 'F';
            buf[2] = 'F';
        }
        buf[3] = 0x0;
        break;

    case SettingType::Switch:
        if (idx == SettingsIndex::DeEmp)
        {
            if (param == 0)
            {
                buf[0] = '5';
                buf[1] = '0';
                buf[2] = 'U';
            }
            else
            {
                buf[0] = '7';
                buf[1] = '5';
                buf[2] = 'U';
            }
        }
        else if (idx == SettingsIndex::SWUnits)
        {
            if (param == 0)
                buf[0] = 'K';
            else
                buf[0] = 'M';
            buf[1] = 'H';
            buf[2] = 'Z';
        }
        else if (idx == SettingsIndex::SSM)
        {
            if (param == 0)
            {
                buf[0] = 'R';
                buf[1] = 'S';
                buf[2] = 'S';
            }
            else
            {
                buf[0] = 'S';
                buf[1] = 'N';
                buf[2] = 'R';
            }
        }
        else if (idx == SettingsIndex::CWSwitch)
        {
            if (param == 0)
                buf[0] = 'L';
            else
                buf[0] = 'U';

            buf[1] = 'S';
            buf[2] = 'B';
        }
        else if (idx == SettingsIndex::CPUSpeed)
        {
            if (param == 0)
            {
                buf[0] = '1';
                buf[1] = '0';
                buf[2] = '0';
            }
            else
            {
                buf[0] = '5';
                buf[1] = '0';
                buf[2] = '%';
            }
        }
        else
        {
            if (param == 0)
            {
                buf[0] = 'O';
                buf[1] = 'F';
                buf[2] = 'F';
            }
            else
            {
                buf[0] = 'O';
                buf[1] = 'N';
                buf[2] = ' ';
            }
        }
        buf[3] = 0x0;
        break;
    }
}

void DrawSetting(uint8_t idx, bool full)
{
    if (!g_settingsActive)
        return;

    char buf[5];
    //Single scrolling column: place is the viewport row of item idx
    uint8_t place = idx - g_SettingsTop;
    if (place > 2)
        return;
    uint8_t yOffset = place * 2;
    if (full)
        oledPrint(g_Settings[idx].name, 5, 2 + yOffset, DEFAULT_FONT, idx == g_SettingSelected && !g_SettingEditing);
    SettingParamToUI(buf, idx);
    oledPrint(buf, 70, 2 + yOffset, DEFAULT_FONT, idx == g_SettingSelected && g_SettingEditing);
}

void showSettings()
{
    for (uint8_t i = 0; i < 3 && i + g_SettingsTop < SettingsIndex::SETTINGS_MAX; i++)
        DrawSetting(i + g_SettingsTop, true);
}
void showSettingsTitle()
{
    oledPrint("SETTINGS ", 0, 0, DEFAULT_FONT, true);
    oled.invertOutput(true);
    //Selected index display "n/16"; division by the constants 10 is
    //multiply-shift, no 32-bit division routine pulled in
    oled.print((char)('0' + (g_SettingSelected + 1) / 10));
    oled.print((char)('0' + (g_SettingSelected + 1) % 10));
    oled.print('/');
    oled.print((char)('0' + SettingsIndex::SETTINGS_MAX / 10));
    oled.print((char)('0' + SettingsIndex::SETTINGS_MAX % 10));
    oled.invertOutput(false);
}

void switchSettingsTop()
{
    g_SettingSelected = 0;
    g_SettingsTop = 0;
    g_SettingEditing = false;
    showSettingsTitle();
    showSettings();
}

void switchSettings()
{
    oled.clear();
    if (g_settingsActive)
    {
        g_SettingSelected = 0;
        g_SettingsTop = 0;
        g_SettingEditing = false;
        showSettingsTitle();
        showSettings();
    }
    else
    {
        saveAllReceiverInformation();
        showStatus();
    }
}

void showModulation()
{
    //Band command highlights the BAND tag (showBandTag), never the mode
    oledPrint(g_bandModeDesc[g_currentMode], 0, 0, DEFAULT_FONT);
    oled.print(" ");
    if (isSSB() && g_Settings[SettingsIndex::Sync].param == 1)
        oledPrint("S", -1, -1, LastFont, true);
    else
        oled.print(" ");

    //Next available mode indicator (band command active, multi-mode bands
    //only). The unconditional blank first wipes the region so no residue
    //remains when g_cmdBand deactivates via any redraw path.
    oledPrint("   ", 80, 0, DEFAULT_FONT);
    if (g_cmdBand)
    {
        uint8_t m = g_currentMode;
        for (;;)
        {
            m = (m == FM) ? AM : m + 1;
            if (m == g_currentMode || modeAvailable(g_bandIndex, m))
                break;
        }
        if (m != g_currentMode)
            oledPrint(g_bandModeDesc[m], 80, 0, DEFAULT_FONT);
    }

    showBandTag();
}

//Erase the S-meter / RDS region (y=6, x=32..128). The band tag at x=0
void eraseRdsSMeterArea()
{
    oledPrint("            ", 32, 6, DEFAULT_FONT);
}

void showBandTag()
{
    //All tags are exactly 4 chars so no tag leaves residue over a longer one
    char swTag[5];
    const char* tag;
    if (g_bandIndex == AIR_BAND_TYPE)
        tag = "AIR ";
    else if (g_bandIndex == SW_BAND_TYPE)
    {
        //Numbered sub-band: the selection while browsing, else the segment
        //the frequency currently sits in
        uint8_t sub = g_bandBrowsing ? g_bandSelSub : swSubBandFromFreq(g_currentFrequency);
        uint8_t n = sub + 1; //displayed 1-based
        swTag[0] = 'S';
        swTag[1] = 'W';
        if (n < 10)
        {
            swTag[2] = '0' + n;
            swTag[3] = ' ';
        }
        else
        {
            swTag[2] = '0' + n / 10;
            swTag[3] = '0' + n % 10;
        }
        swTag[4] = 0;
        tag = swTag;
    }
    else
        tag = bandTags[g_bandIndex];

    oledPrint(tag, 0, 6, DEFAULT_FONT, g_cmdBand);
}

void showVolume()
{
    if (g_settingsActive)
        return;

    char buf[3];
    if (g_muteVolume == 0)
        convertToChar(buf, g_si4735.getCurrentVolume(), 2, 0, 0);
    else
    {
        buf[0] = ' ';
        buf[1] = 'M';
        buf[2] = 0;
    }

    oledPrint(buf, (128 - (8 * 2) + 2 - 6), 0, DEFAULT_FONT, g_cmdVolume);
}

//Draw battery charge
//ATS-20+: battery divider is wired to A1 at the factory (BATTERY_VOLTAGE_PIN).
//Original ATS-20: requires the 10-10 KOhm divider solder mod (to A2, and
//BATTERY_VOLTAGE_PIN changed accordingly).
void showCharge(bool forceShow)
{
    if (!g_voltagePinConnnected)
        return;

    // mV, Percent
    //This values represent voltage values in ATMega328p analog units with reference voltage 3.30v
    //Voltage pin reads voltage from voltage divider, so it have to be 1/2 of Li-Ion battery voltage
    constexpr const uint8_t rows = 10;
    const uint16_t dischargeTable[rows][2] =
    {
        { 643, 100 },  //4.15v
        { 620, 95  },  //4.05v
        { 604, 90  },  //3.90v
        { 581, 80  },  //3.75v
        { 573, 60  },  //3.70v
        { 558, 40  },  //3.60v
        { 542, 20  },  //3.50v
        { 503, 15  },  //3.25v
        { 496, 5  },   //3.20v
        { 488, 0  },   //3.15v
    };

    auto getBatteryPercentage = [&](uint16_t currentSamples) -> uint8_t
    {
        if (currentSamples >= dischargeTable[0][0]) 
            return 100;

        if (currentSamples <= dischargeTable[rows - 1][0]) 
            return 0;

        for (uint8_t i = 0; i < rows - 1; ++i) 
        {
            if (currentSamples >= dischargeTable[i + 1][0] && currentSamples <= dischargeTable[i][0]) 
            {
                uint16_t voltageDiff = dischargeTable[i][0] - dischargeTable[i + 1][0];
                uint16_t percentageDiff = dischargeTable[i][1] - dischargeTable[i + 1][1];
                uint16_t voltageOffset = currentSamples - dischargeTable[i + 1][0];
                return dischargeTable[i + 1][1] + (percentageDiff * voltageOffset + voltageDiff / 2) / voltageDiff;
            }
        }

        return 0;
    };

    static uint32_t lastChargeShow = 0;
    static int16_t averageSamples = 0;

    int sample = analogRead(BATTERY_VOLTAGE_PIN);

    if ((millis() - lastChargeShow) > 10000 || forceShow)
    {
        char buf[4];
        buf[3] = 0;
        int16_t percents = getBatteryPercentage(averageSamples);

        uint8_t il = ilen(percents) < 3 ? 2 : 3;
        convertToChar(buf, percents, il);

        if (il < 3)
            buf[2] = '%';

        if (!g_settingsActive && !g_sMeterOn && !g_displayRDS)
            oledPrint(buf, 102, 6, DEFAULT_FONT);
        lastChargeShow = millis();
        averageSamples = sample;
    }

    averageSamples = (averageSamples + sample) / 2;
}

#if USE_RDS
void showRDS()
{
    static uint16_t lastUpdatedFreq = 0;
    static uint32_t lastUpdatedTime = millis();
    static bool succeed = false;

    if (g_currentMode != FM || !g_displayRDS || g_settingsActive)
    {
        //Left FM with RDS text still on screen (e.g. forced band change):
        //erase the stale station name once, the band tag redraws over it
        if (g_displayRDS && !g_settingsActive && lastUpdatedFreq != 0)
            eraseRdsSMeterArea();
        lastUpdatedFreq = 0;
        g_rdsPrevLen = 0;
        succeed = false;
        g_rdsActiveInfo = 0;
        return;
    }

    if (millis() - lastUpdatedTime > 300)
        succeed = false;

    if (lastUpdatedFreq != g_currentFrequency || g_rdsSwitchPressed)
    {
        if (g_rdsSwitchPressed)
        {
            g_rdsActiveInfo++;
            if (g_rdsActiveInfo > RDSActiveInfo::ProgramInfo)
                g_rdsActiveInfo = RDSActiveInfo::StationName;
        }
        else
        {
            g_rdsActiveInfo = RDSActiveInfo::StationName;
            succeed = false;
        }
        g_rdsPrevLen = 0;
        eraseRdsSMeterArea();
    }
    lastUpdatedFreq = g_currentFrequency;

    if (!succeed)
        g_si4735.getRdsStatus();

    if (!succeed && g_si4735.getRdsReceived() && g_si4735.getRdsSync() && g_si4735.getNumRdsFifoUsed() > 1)
    {
        g_RDSCells[RDSActiveInfo::StationName] = g_si4735.getRdsStationName();
        g_RDSCells[RDSActiveInfo::StationInfo] = g_si4735.getRdsStationInformation();
        g_RDSCells[RDSActiveInfo::ProgramInfo] = g_si4735.getRdsProgramInformation();
        g_RDSCells[RDSActiveInfo::StationInfo][17] = '\0';
        g_RDSCells[RDSActiveInfo::ProgramInfo][17] = '\0';
        succeed = true;
        lastUpdatedTime = millis();
    }
    else if (!g_rdsSwitchPressed && succeed)
        return;

    //Cells stay NULL until the first successful sync
    //Uppercase-only font (ASCII 32..90): uppercase letters, blank everything else
    static char rdsBuf[13];
    const char* rdsText = g_RDSCells[g_rdsActiveInfo];
    if (rdsText)
    {
        uint8_t i = 0;
        for (; i < 12 && rdsText[i]; i++)
        {
            char c = rdsText[i];
            if (c >= 'a' && c <= 'z')
                c -= 32;
            if (c < 32 || c > 90)
                c = ' ';
            rdsBuf[i] = c;
        }
        rdsBuf[i] = 0;
        rdsText = rdsBuf;
    }
    uint8_t len = rdsText ? strlen8(rdsText) : 0;

    if (len == 0 && !g_rdsSwitchPressed)
        return;

    oledPrint(rdsText, 32, 6, DEFAULT_FONT);

    uint8_t toPrint = len == 0 ? 3 : (len < g_rdsPrevLen ? min(g_rdsPrevLen - len, 12 - len) : 0);
    char printChar = len == 0 ? '.' : ' ';
    for (uint8_t i = 0; i < toPrint; i++) 
        oled.print(printChar);

    g_rdsPrevLen = len;
    g_rdsSwitchPressed = false;
}
#endif

void showStep()
{
    if (g_sMeterOn || g_displayRDS)
        return;

    char buf[5];
    if (g_bandIndex == AIR_BAND_TYPE)
    {
        buf[0] = ' ';
        buf[1] = ' ';
        buf[2] = '2';
        buf[3] = '5';
        buf[4] = 0x0;
    }
    else
    {
        //SSB shows plain Hz, large steps show as MHz, the rest as kHz
        if (isSSB() && g_stepIndex >= g_amTotalSteps)
            convertToChar(buf, g_tabStep[g_stepIndex], 4);
        else
        {
            uint16_t v = (g_currentMode == FM) ? g_tabStepFM[g_FMStepIndex] * 10 : g_tabStep[g_stepIndex];
            if (v == 1000)
            {
                buf[0] = ' ';
                buf[1] = ' ';
                buf[2] = '1';
                buf[3] = 'M';
                buf[4] = 0x0;
            }
            else
            {
                convertToChar(buf, v, 3);
                buf[3] = 'K';
                buf[4] = '\0';
            }
        }
    }

    //"[9K]" bracket style, leading spaces trimmed; centered in the gap
    //between the band tag (ends x=32) and the battery (x=102).
    //Erase the gap first: shorter step text (e.g. "[25]" after "[100K]"
    //on a band switch) would otherwise leave residue behind
    uint8_t first = 0;
    while (first < 3 && buf[first] == ' ')
        first++;
    char disp[7];
    uint8_t n = 4 - first;
    disp[0] = '[';
    for (uint8_t i = 0; i < n; i++)
        disp[1 + i] = buf[first + i];
    disp[1 + n] = ']';
    disp[2 + n] = 0;
    oledPrint("        ", 32, 6, DEFAULT_FONT); //8 chars = x=32..96, clear of the battery
    oledPrint(disp, 32 + (70 - (n + 2) * 8) / 2, 6, DEFAULT_FONT, g_cmdStep);
}
void showSMeter()
{
    static uint32_t sMeterUpdated = 0;
    if (millis() - sMeterUpdated < 100)
        return;

    g_si4735.getCurrentReceivedSignalQuality();
    uint8_t rssi = g_si4735.getCurrentRSSI();
    rssi = rssi > 64 ? 64 : rssi;

    //Fixed band tag owns x=0; the bar graph lives at x=32..128 (12 bars)
    int sMeterValue = rssi / (64 / 12);
    char buf[13];
    for (uint8_t i = 0; i < sizeof(buf) - 1; i++)
        buf[i] = i < sMeterValue ? '#' : ' ';
    buf[sizeof(buf) - 1] = 0x0;

    oledPrint(buf, 32, 6, DEFAULT_FONT);
    sMeterUpdated = millis();
}

void showBandwidth()
{
    const char* bw;
    if (isSSB())
    {
        bw = g_bandwidthSSB[g_bwIndexSSB].desc;
        if (g_currentMode == CW)
            bw = "    ";
    }
    else if (g_currentMode == AM)
    {
        bw = g_bandwidthAM[g_bwIndexAM].desc;
    }
    else
    {
        bw = g_bandwidthFM[g_bwIndexFM];
    }

    oledPrint(bw, 45, 0, DEFAULT_FONT, g_cmdBw);
}
