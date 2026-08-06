/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file rs485_component.cpp
  @brief Common base for RS-485 transceiver units in M5UnitUnified
 */
#include "rs485_component.hpp"
#include <cstring>
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;

namespace m5 {
namespace unit {

//
// Null ISerial: used before begin() and on unsupported frameworks so that all Serial-compat
// methods have safe no-op behavior instead of crashing on null-pointer dereference.
//
struct NullSerial final : public RS485Component::ISerial {
    inline int available() override
    {
        return 0;
    }
    inline int availableForWrite() override
    {
        return 0;
    }
    inline int peek() override
    {
        return -1;
    }
    inline int read() override
    {
        return -1;
    }
    inline size_t read(uint8_t *, const size_t) override
    {
        return 0;
    }
    inline size_t readBytes(uint8_t *, const size_t) override
    {
        return 0;
    }
    inline void flush() override
    {
    }
    inline void flush(const bool) override
    {
    }
    inline size_t write(const uint8_t *, const size_t) override
    {
        return 0;
    }
    inline size_t write(uint8_t) override
    {
        return 0;
    }
    inline uint32_t baudRate() const override
    {
        return 0;
    }
};

#if defined(ARDUINO)
//
// Arduino HardwareSerial-backed ISerial. Direct pass-through.
//
struct UUSerial final : public RS485Component::ISerial {
    explicit UUSerial(HardwareSerial *hs) : _hs(hs)
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
    inline size_t write(uint8_t b) override
    {
        return _hs->write(b);
    }
    inline uint32_t baudRate() const override
    {
        return _hs->baudRate();
    }
    HardwareSerial *_hs{};
};
#endif  // ARDUINO

// class RS485Component
RS485Component::RS485Component(const uint8_t addr) : Component(addr)
{
    _serial.reset(new NullSerial);
}

std::unique_ptr<RS485Component::ISerial> RS485Component::make_serial(AdapterUART *ad)
{
#if defined(ARDUINO)
    return std::unique_ptr<ISerial>(new UUSerial(ad->impl()->getSerial()));
#else
    (void)ad;
    // ESP-IDF native support is planned; see #11.
    return std::unique_ptr<ISerial>(new NullSerial);
#endif
}

bool RS485Component::begin()
{
#if defined(ARDUINO)
    auto ad = asAdapter<AdapterUART>(Adapter::Type::UART);
    if (!ad || !ad->impl()->getSerial()) {
        M5_LIB_LOGE("Illegal adapter");
        return false;
    }
    _serial = make_serial(ad);
    if (!_serial) {
        return false;
    }

    if (_cfg.flushRX) {
        const uint32_t start = m5::utility::millis();
        while (m5::utility::millis() - start < 100) {
            ad->flushRX();
            m5::utility::delay(1);
        }
    }
    return true;
#else
    M5_LIB_LOGE("RS485Component::begin(): non-Arduino frameworks are not yet supported");
    return false;
#endif
}

size_t RS485Component::write(const uint8_t d, const bool flush)
{
    size_t n = _serial->write(d);
    if (flush && n > 0) {
        _serial->flush(true);
    }
    return n;
}

size_t RS485Component::write(const uint8_t *buffer, const size_t size, const bool flush)
{
    size_t n = _serial->write(buffer, size);
    if (flush && n > 0) {
        _serial->flush(true);
    }
    return n;
}

size_t RS485Component::write(const char *buffer, const size_t size, const bool flush)
{
    return write(reinterpret_cast<const uint8_t *>(buffer), size, flush);
}

size_t RS485Component::write(const char *s, const bool flush)
{
    if (!s) {
        return 0;
    }
    return write(reinterpret_cast<const uint8_t *>(s), strlen(s), flush);
}

}  // namespace unit
}  // namespace m5
