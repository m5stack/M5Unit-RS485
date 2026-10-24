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
#include "rs485_hs_serial.hpp"
#include "rs485_idf_serial.hpp"
#include <M5Utility.hpp>

#if defined(ESP_PLATFORM)
#include <driver/gpio.h>  // gpio_config / gpio_set_level / gpio_num_t
#endif

using namespace m5::utility::mmh3;
using namespace m5::unit::types;

namespace m5 {
namespace unit {

#if defined(ARDUINO)
//
// DIR-pin-controlled ISerial decorator for SIT3088. Only the write side differs from the plain
// HardwareSerial backend, so the read side is inherited as-is: set_transmit() is called before the
// transfer, flush() blocks until the last stop bit is on the wire, then the receiver is re-enabled.
// Timing per SIT3088E datasheet: driver disable tPHZ / tPLZ <= 85 ns and receiver enable
// tRPZH / tRPZL <= 40 ns. DE and /RE are tied, so the 500 ns shutdown-exit path (tRPSH / tRPSL)
// cannot be entered. The extra pin toggle therefore stays within the RS-485 turn-around budget.
//
struct UnitSIT3088::DirControlledSerial final : public rs485::detail::HSSerial {
    DirControlledSerial(HardwareSerial *hs, UnitSIT3088 *unit) : rs485::detail::HSSerial(hs), _unit(unit)
    {
    }

    size_t write(const uint8_t *b, const size_t n) override
    {
        if (b == nullptr || n == 0) {
            return 0;
        }
        _unit->set_transmit(true);
        const size_t written = HSSerial::write(b, n);
        HSSerial::flush();  // block until TX complete before releasing DIR
        _unit->set_transmit(false);
        return written;
    }
    size_t write(uint8_t b) override
    {
        return write(&b, 1);
    }

protected:
    UnitSIT3088 *_unit{};
};
#endif  // ARDUINO

#if !defined(ARDUINO) && defined(ESP_PLATFORM)
//
// ESP-IDF counterpart of DirControlledSerial: IDFSerial plus the DIR pin handling. Only write()
// differs, so the read side is inherited as-is.
//
struct UnitSIT3088::DirControlledIDFSerial final : public rs485::detail::IDFSerial {
    DirControlledIDFSerial(const uart_port_t port, UnitSIT3088 *unit) : rs485::detail::IDFSerial(port), _unit(unit)
    {
    }

    size_t write(const uint8_t *b, const size_t n) override
    {
        if (b == nullptr || n == 0) {
            return 0;
        }
        _unit->set_transmit(true);
        const size_t written = IDFSerial::write(b, n);
        IDFSerial::flush();  // block until TX complete before releasing DIR
        _unit->set_transmit(false);
        return written;
    }
    size_t write(uint8_t b) override
    {
        return write(&b, 1);
    }

protected:
    UnitSIT3088 *_unit{};
};
#endif  // !ARDUINO && ESP_PLATFORM

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
#elif defined(ESP_PLATFORM)
    const auto port = ad->uartPort();
    if (port == UART_NUM_MAX || !uart_is_driver_installed(port)) {
        M5_LIB_LOGE("Illegal adapter: no installed ESP-IDF UART port");
        return std::unique_ptr<ISerial>();
    }
    return std::unique_ptr<ISerial>(new DirControlledIDFSerial(port, this));
#else
    (void)ad;
    M5_LIB_LOGE("UnitSIT3088::make_serial(): this framework is not supported");
    return std::unique_ptr<ISerial>();
#endif
}

}  // namespace unit
}  // namespace m5
