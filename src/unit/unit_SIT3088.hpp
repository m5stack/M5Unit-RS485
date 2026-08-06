/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SIT3088.hpp
  @brief SIT3088 unit for M5UnitUnified (manual direction-controlled RS-485 transceiver)
  @details The SIT3088 transceiver does NOT auto-control DE/RE like SP485EEN;
           the host must drive a DIR pin (typically DE and /RE tied) high before
           transmitting and low after transmission completes.
           Used by M5Stack Tab5's built-in RS-485 (DIR = GPIO34).
  @note The public API is framework-agnostic. Framework-specific direction-pin control
        (pinMode / digitalWrite on Arduino, gpio_set_level on ESP-IDF native etc.) is
        confined to the translation unit.
 */
#ifndef M5_UNIT_RS485_UNIT_SIT3088_HPP
#define M5_UNIT_RS485_UNIT_SIT3088_HPP

#include "rs485_component.hpp"

namespace m5 {
namespace unit {

/*!
  @class UnitSIT3088
  @brief RS-485 unit using the SIT3088 transceiver (manual direction control via a DIR pin)
 */
class UnitSIT3088 : public RS485Component {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitSIT3088, 0x00 /*UART*/);

public:
    /*!
      @brief Constructor
     */
    UnitSIT3088();
    //! @brief Destructor
    virtual ~UnitSIT3088() = default;

    /*!
      @brief Sets the DIR (DE/RE tied) pin used to drive the SIT3088 direction.
      @param pin GPIO number for DIR, or -1 to leave unset.
      @note Must be called before begin(). Wiring helpers such as
            m5::unit::rs485::wiring::addTab5BuiltinRS485UART() set this automatically.
     */
    inline void dirPin(const int8_t pin)
    {
        _dir_pin = pin;
    }
    /*!
      @brief Gets the DIR pin currently configured.
      @return DIR pin GPIO number, or -1 if unset.
     */
    inline int8_t dirPin() const
    {
        return _dir_pin;
    }

protected:
    // Install a direction-pin-controlled ISerial decorator around the raw HardwareSerial.
    virtual std::unique_ptr<ISerial> make_serial(AdapterUART *ad) override;

private:
    int8_t _dir_pin{-1};
};

}  // namespace unit
}  // namespace m5
#endif
