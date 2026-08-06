/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SIT3088.cpp
  @brief SIT3088 unit for M5UnitUnified
 */
#include "unit_SIT3088.hpp"
#include <M5Utility.hpp>

#if defined(ARDUINO)
#include <Arduino.h>  // pinMode / digitalWrite / OUTPUT / HIGH / LOW / HardwareSerial
#endif

using namespace m5::utility::mmh3;
using namespace m5::unit::types;

namespace m5 {
namespace unit {

#if defined(ARDUINO)
//
// DIR-pin-controlled ISerial decorator for SIT3088.
// Read side passes through to HardwareSerial. Write side raises DIR before the transfer,
// blocks on flush() to guarantee the last stop bit is on the wire, then lowers DIR to
// re-enable the receiver. Timing per SIT3088E datasheet: tPHZ / tPLZ <= 85 ns (driver
// disable) and receiver enable from shutdown <= 500 ns, so the small extra pin toggle is
// well within RS-485 turn-around budgets.
//
struct DirControlledSerial final : public RS485Component::ISerial {
    DirControlledSerial(HardwareSerial *hs, const int8_t dir_pin) : _hs(hs), _dir_pin(dir_pin)
    {
        pinMode(_dir_pin, OUTPUT);
        digitalWrite(_dir_pin, LOW);  // Start in RX mode
    }

    inline int available() override
    {
        return _hs->available();
    }
    inline int availableForWrite() override
    {
        return _hs->availableForWrite();
    }
    inline int peek() override
    {
        return _hs->peek();
    }
    inline int read() override
    {
        return _hs->read();
    }
    inline size_t read(uint8_t *b, const size_t n) override
    {
        return _hs->read(b, n);
    }
    inline size_t readBytes(uint8_t *b, const size_t n) override
    {
        return _hs->readBytes(b, n);
    }
    inline void flush() override
    {
        _hs->flush();
    }
    inline void flush(const bool txOnly) override
    {
        _hs->flush(txOnly);
    }
    size_t write(const uint8_t *b, const size_t n) override
    {
        if (n == 0) {
            return 0;
        }
        digitalWrite(_dir_pin, HIGH);
        const size_t written = _hs->write(b, n);
        _hs->flush();  // block until TX complete before releasing DIR
        digitalWrite(_dir_pin, LOW);
        return written;
    }
    size_t write(uint8_t b) override
    {
        digitalWrite(_dir_pin, HIGH);
        const size_t written = _hs->write(b);
        _hs->flush();
        digitalWrite(_dir_pin, LOW);
        return written;
    }
    inline uint32_t baudRate() const override
    {
        return _hs->baudRate();
    }

    HardwareSerial *_hs{};
    int8_t _dir_pin{-1};
};
#endif  // ARDUINO

// class UnitSIT3088
const char UnitSIT3088::name[] = "UnitSIT3088";
const types::uid_t UnitSIT3088::uid{"UnitSIT3088"_mmh3};
const types::attr_t UnitSIT3088::attr{attribute::AccessUART};

UnitSIT3088::UnitSIT3088() : RS485Component(DEFAULT_ADDRESS)
{
}

std::unique_ptr<UnitSIT3088::ISerial> UnitSIT3088::make_serial(AdapterUART *ad)
{
#if defined(ARDUINO)
    if (_dir_pin < 0) {
        M5_LIB_LOGE("UnitSIT3088::make_serial(): dirPin() must be set before begin()");
        return std::unique_ptr<ISerial>();
    }
    return std::unique_ptr<ISerial>(new DirControlledSerial(ad->impl()->getSerial(), _dir_pin));
#else
    (void)ad;
    M5_LIB_LOGE("UnitSIT3088::make_serial(): non-Arduino frameworks are not yet supported");
    return std::unique_ptr<ISerial>();
#endif
}

}  // namespace unit
}  // namespace m5
