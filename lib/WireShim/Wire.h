#pragma once

#include <stdint.h>
#include <stddef.h>

// Minimal polling TWI master for AVR that mimics the subset of the Arduino
// TwoWire API used by this project (PU2CLR SI4735 + Tiny4kOLED).
// It replaces the stock Wire library to save ~2KB of flash and ~100B of RAM.
//
// Differences from the stock library:
// - Writes are transmitted eagerly (no 32-byte buffering), errors are latched
//   and reported by endTransmission() exactly like the stock library
// - Every hardware wait is bounded by a timeout, so a wedged bus cannot hang
//   the firmware (the stock driver waits forever)
// - Single master only, no slave mode, no interrupts

class MinimalWire
{
public:
    void begin();
    void setClock(uint32_t frequency);
    void beginTransmission(uint8_t address);
    size_t write(uint8_t value);
    size_t write(const uint8_t *data, size_t length);
    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }
    uint8_t endTransmission(uint8_t sendStop);
    inline uint8_t endTransmission() { return endTransmission(true); }
    uint8_t requestFrom(uint8_t address, uint8_t length);
    inline uint8_t requestFrom(int address, int length) { return requestFrom((uint8_t)address, (uint8_t)length); }
    int read();
    inline uint8_t available() { return _rxLength - _rxIndex; }

private:
    bool _waitTwint();
    bool _sendStart();
    bool _sendSla(uint8_t sla);
    void _sendStop();

    uint8_t _address;
    bool _transmitting;
    bool _started;
    uint8_t _txError;
    uint8_t _rxBuffer[32];
    uint8_t _rxLength;
    uint8_t _rxIndex;
};

extern MinimalWire Wire;
