/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file rs485_hs_serial.hpp
  @brief Arduino HardwareSerial-backed RS485Component::ISerial (library-internal)
  @note Not part of the public API: this header is included by the library's translation units
        only, so that HardwareSerial never leaks into the framework-agnostic public headers.
        The ESP-IDF counterpart is rs485_idf_serial.hpp.
 */
#ifndef M5_UNIT_RS485_RS485_HS_SERIAL_HPP
#define M5_UNIT_RS485_RS485_HS_SERIAL_HPP

#if defined(ARDUINO)

#include "rs485_component.hpp"
#include <HardwareSerial.h>

namespace m5 {
namespace unit {
namespace rs485 {
namespace detail {

//
// HardwareSerial-backed ISerial. Direct pass-through; derive and override write() to add
// direction-pin control (see UnitSIT3088).
//
struct HSSerial : public RS485Component::ISerial {
    explicit HSSerial(HardwareSerial *hs) : _hs(hs)
    {
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
    size_t readBytes(uint8_t *b, const size_t n, const uint32_t timeout_ms) override
    {
        // HardwareSerial keeps the timeout as object state, so restore it to leave the caller's
        // setTimeout() untouched.
        const auto previous = _hs->getTimeout();
        _hs->setTimeout(timeout_ms);
        const size_t got = _hs->readBytes(b, n);
        _hs->setTimeout(previous);
        return got;
    }
    inline void flush() override
    {
        _hs->flush();
    }
    inline void flush(const bool txOnly) override
    {
        _hs->flush(txOnly);
    }
    inline size_t write(const uint8_t *b, const size_t n) override
    {
        return _hs->write(b, n);
    }
    inline size_t write(const uint8_t b) override
    {
        return _hs->write(b);
    }
    inline uint32_t baudRate() const override
    {
        return _hs->baudRate();
    }

protected:
    HardwareSerial *_hs{};
};

}  // namespace detail
}  // namespace rs485
}  // namespace unit
}  // namespace m5

#endif  // ARDUINO
#endif
