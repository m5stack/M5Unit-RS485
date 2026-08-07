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

#if defined(ESP_PLATFORM)
#include <driver/gpio.h>  // gpio_config / gpio_set_level / gpio_num_t
#endif

#if defined(ARDUINO)
#include <HardwareSerial.h>  // decorator wraps HardwareSerial; no Arduino.h dependency needed
#endif

using namespace m5::utility::mmh3;
using namespace m5::unit::types;

namespace m5 {
namespace unit {

#if defined(ARDUINO)
//
// DIR-pin-controlled ISerial decorator for SIT3088.
// Read side passes through to HardwareSerial. Write side calls the unit's protected
// set_transmit() before the transfer, blocks on flush() to guarantee the last stop bit
// is on the wire, then calls set_transmit(false) to re-enable the receiver.
// Timing per SIT3088E datasheet: driver tPHZ / tPLZ <= 85 ns, receiver enable from
// shutdown <= 500 ns, so the small extra pin toggle stays within RS-485 turn-around.
//
struct DirControlledSerial final : public RS485Component::ISerial {
    DirControlledSerial(HardwareSerial *hs, UnitSIT3088 *unit) : _hs(hs), _unit(unit)
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
    size_t write(const uint8_t *b, const size_t n) override
    {
        if (n == 0) {
            return 0;
        }
        _unit->set_transmit(true);
        const size_t written = _hs->write(b, n);
        _hs->flush();  // block until TX complete before releasing DIR
        _unit->set_transmit(false);
        return written;
    }
    size_t write(uint8_t b) override
    {
        _unit->set_transmit(true);
        const size_t written = _hs->write(b);
        _hs->flush();
        _unit->set_transmit(false);
        return written;
    }
    inline uint32_t baudRate() const override
    {
        return _hs->baudRate();
    }

    HardwareSerial *_hs{};
    UnitSIT3088 *_unit{};
};
#endif  // ARDUINO

// class UnitSIT3088
const char UnitSIT3088::name[] = "UnitSIT3088";
const types::uid_t UnitSIT3088::uid{"UnitSIT3088"_mmh3};
const types::attr_t UnitSIT3088::attr{attribute::AccessUART};

UnitSIT3088::UnitSIT3088() : RS485Component(DEFAULT_ADDRESS)
{
}

bool UnitSIT3088::begin()
{
#if defined(ESP_PLATFORM)
    if (_sit_cfg.dir_pin < 0 || static_cast<int>(_sit_cfg.dir_pin) >= GPIO_NUM_MAX) {
        M5_LIB_LOGE("UnitSIT3088::begin(): dir_pin out of range (%d)", _sit_cfg.dir_pin);
        return false;
    }
#else
    if (_sit_cfg.dir_pin < 0) {
        M5_LIB_LOGE("UnitSIT3088::begin(): dir_pin must be set via config()");
        return false;
    }
#endif
    configure_dir_pin(_sit_cfg.dir_pin);
    _configured_dir_pin = _sit_cfg.dir_pin;
    set_transmit(false);  // start in RX mode
    return RS485Component::begin();
}

void UnitSIT3088::configure_dir_pin(const int8_t pin)
{
#if defined(ESP_PLATFORM)
    gpio_config_t cfg{};
    cfg.pin_bit_mask = 1ULL << static_cast<uint64_t>(pin);
    cfg.mode         = GPIO_MODE_OUTPUT;
    cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&cfg);
#else
    (void)pin;
#endif
}

void UnitSIT3088::set_transmit(const bool tx_enable)
{
#if defined(ESP_PLATFORM)
    gpio_set_level(static_cast<gpio_num_t>(_configured_dir_pin), tx_enable ? 1 : 0);
#else
    (void)tx_enable;
#endif
}

std::unique_ptr<UnitSIT3088::ISerial> UnitSIT3088::make_serial(AdapterUART *ad)
{
#if defined(ARDUINO)
    return std::unique_ptr<ISerial>(new DirControlledSerial(ad->impl()->getSerial(), this));
#else
    (void)ad;
    M5_LIB_LOGE("UnitSIT3088::make_serial(): non-Arduino frameworks are not yet supported");
    return std::unique_ptr<ISerial>();
#endif
}

}  // namespace unit
}  // namespace m5
