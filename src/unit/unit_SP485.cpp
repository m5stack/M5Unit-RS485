/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SP485.cpp
  @brief SP485 unit for M5UnitUnified
 */
#include "unit_SP485.hpp"
#include <cstring>
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;

namespace {

}  // namespace

namespace m5 {
namespace unit {

struct UUSerial final : public UnitSP485::ISerial {
    explicit UUSerial(HardwareSerial* hs) : _hs(hs)
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
    inline size_t read(uint8_t* b, const size_t n) override
    {
        return _hs->read(b, n);
    }
    inline size_t readBytes(uint8_t* b, const size_t n) override
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
    inline size_t write(const uint8_t* b, const size_t n) override
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
    HardwareSerial* _hs{};
};

struct NullSerial final : public UnitSP485::ISerial {
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
    inline size_t read(uint8_t*, const size_t) override
    {
        return 0;
    }
    inline size_t readBytes(uint8_t*, const size_t) override
    {
        return 0;
    }
    inline void flush() override
    {
    }
    inline void flush(const bool) override
    {
    }
    inline size_t write(const uint8_t*, const size_t) override
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

// class UnitSP485
const char UnitSP485::name[] = "UnitSP485";
const types::uid_t UnitSP485::uid{"UnitSP485"_mmh3};
const types::attr_t UnitSP485::attr{attribute::AccessUART};

UnitSP485::UnitSP485() : Component(DEFAULT_ADDRESS)
{
    _serial.reset(new NullSerial);
}

bool UnitSP485::begin()
{
    auto ad = asAdapter<AdapterUART>(Adapter::Type::UART);
    if (!ad || !ad->impl()->getSerial()) {
        M5_LIB_LOGE("Illegal adapter");
        return false;
    }
    _serial.reset(new UUSerial(ad->impl()->getSerial()));

    if (_cfg.flushRX) {
        const uint32_t start = m5::utility::millis();
        while (m5::utility::millis() - start < 100) {
            ad->flushRX();
            m5::utility::delay(1);
        }
    }
    return true;
}

size_t UnitSP485::write(const uint8_t d, const bool flush)
{
    size_t n = _serial->write(d);
    if (flush && n > 0) {
        _serial->flush(true);
    }
    return n;
}

size_t UnitSP485::write(const uint8_t* buffer, const size_t size, const bool flush)
{
    size_t n = _serial->write(buffer, size);
    if (flush && n > 0) {
        _serial->flush(true);
    }
    return n;
}

size_t UnitSP485::write(const char* buffer, const size_t size, const bool flush)
{
    return write(reinterpret_cast<const uint8_t*>(buffer), size, flush);
}

size_t UnitSP485::write(const char* s, const bool flush)
{
    if (!s) {
        return 0;
    }
    return write(reinterpret_cast<const uint8_t*>(s), strlen(s), flush);
}

}  // namespace unit
}  // namespace m5
