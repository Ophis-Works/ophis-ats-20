#include <Arduino.h>
#include <Wire.h>

// TWI status codes (TWSR prescaler bits masked off), ATmega328P datasheet ch.24
#define TWI_START          0x08
#define TWI_REP_START      0x10
#define TWI_MT_SLA_ACK     0x18
#define TWI_MT_SLA_NACK    0x20
#define TWI_MT_DATA_ACK    0x28
#define TWI_MT_DATA_NACK   0x30
#define TWI_MR_SLA_ACK     0x40
#define TWI_MR_SLA_NACK    0x48
#define TWI_MR_DATA_ACK    0x50
#define TWI_MR_DATA_NACK   0x58

#define TWI_READ           0x01
#define TWI_WRITE          0x00

MinimalWire Wire;

// Bounded wait, only ever hit on a wedged bus or missing pull-ups
#define TWI_TIMEOUT        0x8FFF

// endTransmission() return codes, matching the stock TwoWire
#define TWI_OK             0
#define TWI_ERR_SLA_NACK   2
#define TWI_ERR_DATA_NACK  3
#define TWI_ERR_OTHER      4

void MinimalWire::begin()
{
    // SDA = PC4 (A4), SCL = PC5 (A5), prescaler = 1
    PORTC |= _BV(PC4) | _BV(PC5);
    TWSR = 0;
    setClock(100000);
    TWCR = _BV(TWEN);
    _started = false;
}

void MinimalWire::setClock(uint32_t frequency)
{
    TWBR = (uint8_t)((F_CPU / frequency - 16) / 2);
}

bool MinimalWire::_waitTwint()
{
    uint16_t timeout = TWI_TIMEOUT;
    while (!(TWCR & _BV(TWINT)))
    {
        if (--timeout == 0)
        {
            // Reset the peripheral and release the bus
            TWCR = 0;
            TWCR = _BV(TWEN);
            _started = false;
            return false;
        }
    }
    return true;
}

bool MinimalWire::_sendStart()
{
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
    if (!_waitTwint())
        return false;

    uint8_t status = TWSR & 0xF8;
    if (status != TWI_START && status != TWI_REP_START)
    {
        _sendStop();
        return false;
    }
    _started = true;
    return true;
}

bool MinimalWire::_sendSla(uint8_t sla)
{
    TWDR = sla;
    TWCR = _BV(TWINT) | _BV(TWEN);
    if (!_waitTwint())
        return false;

    uint8_t status = TWSR & 0xF8;
    if (status != TWI_MT_SLA_ACK && status != TWI_MR_SLA_ACK)
    {
        _sendStop();
        return false;
    }
    return true;
}

void MinimalWire::_sendStop()
{
    TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN);
    uint16_t timeout = TWI_TIMEOUT;
    while (TWCR & _BV(TWSTO))
    {
        if (--timeout == 0)
        {
            TWCR = 0;
            TWCR = _BV(TWEN);
            break;
        }
    }
    _started = false;
}

void MinimalWire::beginTransmission(uint8_t address)
{
    _address = address;
    _transmitting = true;
    _txError = TWI_OK;
}

size_t MinimalWire::write(uint8_t value)
{
    if (!_transmitting || _txError != TWI_OK)
        return 0;

    if (!_started)
    {
        if (!_sendStart() || !_sendSla((_address << 1) | TWI_WRITE))
        {
            _txError = TWI_ERR_SLA_NACK;
            return 0;
        }
    }

    TWDR = value;
    TWCR = _BV(TWINT) | _BV(TWEN);
    if (!_waitTwint())
    {
        _txError = TWI_ERR_OTHER;
        return 0;
    }

    if ((TWSR & 0xF8) != TWI_MT_DATA_ACK)
    {
        _sendStop();
        _txError = TWI_ERR_DATA_NACK;
        return 0;
    }
    return 1;
}

size_t MinimalWire::write(const uint8_t *data, size_t length)
{
    size_t sent = 0;
    while (length--)
    {
        if (write(*data++) == 0)
            break;
        sent++;
    }
    return sent;
}

uint8_t MinimalWire::endTransmission(uint8_t sendStop)
{
    uint8_t result = _txError;

    // beginTransmission() without any write() still addresses the device
    if (result == TWI_OK && !_started)
    {
        if (!_sendStart() || !_sendSla((_address << 1) | TWI_WRITE))
            result = TWI_ERR_SLA_NACK;
    }

    if (sendStop && _started)
        _sendStop();

    _transmitting = false;
    return result;
}

uint8_t MinimalWire::requestFrom(uint8_t address, uint8_t length)
{
    if (length > sizeof(_rxBuffer))
        length = sizeof(_rxBuffer);

    _rxIndex = 0;
    _rxLength = 0;

    if (!_sendStart() || !_sendSla((address << 1) | TWI_READ))
    {
        _sendStop();
        return 0;
    }

    for (uint8_t i = 0; i < length; i++)
    {
        // ACK every byte except the last one
        TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN);
        if (!_waitTwint())
            return i;

        uint8_t status = TWSR & 0xF8;
        if (status != TWI_MR_DATA_ACK && status != TWI_MR_DATA_NACK)
        {
            _sendStop();
            return i;
        }
        _rxBuffer[i] = TWDR;
    }

    _sendStop();
    _rxLength = length;
    return length;
}

int MinimalWire::read()
{
    if (_rxIndex >= _rxLength)
        return -1;
    return _rxBuffer[_rxIndex++];
}
