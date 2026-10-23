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
#include "rs485_idf_serial.hpp"

namespace m5 {
namespace unit {
namespace {

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
    inline size_t readBytes(uint8_t *, const size_t, const uint32_t) override
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

}  // namespace

// class RS485Component
RS485Component::RS485Component(const uint8_t addr) : Component(addr)
{
    _serial.reset(new NullSerial);
}

std::unique_ptr<RS485Component::ISerial> RS485Component::make_serial(AdapterUART *ad)
{
#if defined(ARDUINO)
    return std::unique_ptr<ISerial>(new UUSerial(ad->impl()->getSerial()));
#elif defined(ESP_PLATFORM)
    // AdapterUART::uartPort() reports UART_NUM_MAX for an adapter that was not built from a port
    // (i.e. one holding a HardwareSerial), which this backend cannot drive.
    const auto port = ad->uartPort();
    if (port == UART_NUM_MAX || !uart_is_driver_installed(port)) {
        M5_LIB_LOGE("Illegal adapter: no installed ESP-IDF UART port");
        return std::unique_ptr<ISerial>();
    }
    return std::unique_ptr<ISerial>(new IDFSerial(port));
#else
    (void)ad;
    return std::unique_ptr<ISerial>(new NullSerial);
#endif
}

bool RS485Component::begin()
{
#if defined(ARDUINO) || defined(ESP_PLATFORM)
    auto ad = asAdapter<AdapterUART>(Adapter::Type::UART);
    if (!ad) {
        M5_LIB_LOGE("Illegal adapter");
        return false;
    }
#if defined(ARDUINO)
    if (!ad->impl()->getSerial()) {
        M5_LIB_LOGE("Illegal adapter");
        return false;
    }
#endif
    _serial = make_serial(ad);
    if (!_serial) {
        return false;
    }

    if (_cfg.flushRX) {
        const auto start = m5::utility::millis();
        while (!m5::utility::hasElapsed(start, 100)) {
            ad->flushRX();
            m5::utility::delay(1);
        }
    }
    return true;
#else
    M5_LIB_LOGE("RS485Component::begin(): this framework is not supported");
    return false;
#endif
}

size_t RS485Component::write(const uint8_t d)
{
    return _serial->write(d);
}

size_t RS485Component::write(const uint8_t *buffer, const size_t size)
{
    if (!buffer || size == 0) {
        return 0;
    }
    return _serial->write(buffer, size);
}

size_t RS485Component::write(const char *buffer, const size_t size)
{
    return write(reinterpret_cast<const uint8_t *>(buffer), size);
}

size_t RS485Component::write(const char *s)
{
    if (!s) {
        return 0;
    }
    return write(reinterpret_cast<const uint8_t *>(s), std::strlen(s));
}

size_t RS485Component::write_waiting(const uint8_t *buffer, const size_t size)
{
    const size_t n = write(buffer, size);
    if (n > 0) {
        _serial->flush(true);
    }
    return n;
}

}  // namespace unit
}  // namespace m5
