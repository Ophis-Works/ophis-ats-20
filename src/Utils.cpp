// ----------------------------------------------------------------------
// Small OLED/text helpers shared by all modules. Definitions moved here
// from Utils.h during Milestone 7 (utils.h keeps the declarations).
// ----------------------------------------------------------------------

#include "Utils.h"
#include "defs.h"

const DCfont* LastFont = DEFAULT_FONT;

//Globals from globals.h (single-include) accessed here
extern int g_currentBFO;
extern uint16_t g_currentFrequency;

void oledSetFont(const DCfont* font)
{
    if (font && LastFont != font)
    {
        LastFont = font;
        oled.setFont(font);
    }
}

void oledPrint(const char* text, int offX, int offY, const DCfont* font, bool invert)
{
    oledSetFont(font);
    if (invert)
        oled.invertOutput(invert);
    if (offX >= 0 && offY >= 0)
        oled.setCursor(offX, offY);
    oled.print(text);
    if (invert)
        oled.invertOutput(false);
}

void convertToChar(char* strValue, uint16_t value, uint8_t len, uint8_t dot, uint8_t separator, uint8_t space)
{
    char d;
    int8_t i;
    for (i = (len - 1); i >= 0; i--)
    {
        d = value % 10;
        value = value / 10;
        strValue[i] = d + 48;
    }
    strValue[len] = '\0';

    if (dot > 0)
    {
        for (int i = len; i >= dot; i--)
        {
            strValue[i + 1] = strValue[i];
        }
        strValue[dot] = separator;
        len = dot;
    }
    i = 0;
    len--;

    while ((i < len) && ('0' == strValue[i]))
    {
        strValue[i++] = space;
    }
}

int ilen(uint16_t n)
{
    if (n < 10)
        return 1;
    else if (n < 100)
        return 2;
    else if (n < 1000)
        return 3;
    else if (n < 10000)
        return 4;
    else
        return 5;
}

void splitFreq(uint16_t& khz, uint16_t& tail)
{
    int bfo = g_currentBFO / 1000;
    int rem = g_currentBFO % 1000;
    if (g_currentBFO < 0 && rem != 0)
    {
        khz = g_currentFrequency + bfo - 1;
        tail = (1000 + rem) / 10;
    }
    else
    {
        khz = g_currentFrequency + bfo;
        tail = rem / 10;
    }
}

uint8_t strlen8(const char* str)
{
    uint8_t n = 0;
    while (str[n] != '\0')
        n++;
    return n;
}
