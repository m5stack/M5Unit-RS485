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
        (ESP-IDF gpio_config / gpio_set_level, HardwareSerial on Arduino) is confined
        to the translation unit.
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
      @struct config_t
      @brief Settings for begin (extends RS485Component::config_t with the SIT3088 DIR pin)
      @note Following M5UnitUnified convention, config is a begin-time setting: changing any
            field after begin() has succeeded has no effect (the DIR pin is captured during
            begin() and the ISerial decorator is already installed).
     */
    struct config_t : public RS485Component::config_t {
        int8_t dir_pin{-1};  //!< DIR (DE/RE tied) GPIO number; -1 = unset (begin() fails)
    };

    /*!
      @brief Default constructor. dir_pin remains unset; set via config() before begin().
     */
    UnitSIT3088();
    /*!
      @brief Constructor that bakes the DIR pin into config().
      @param dir_pin DIR (DE/RE tied) GPIO number for this board.
     */
    explicit UnitSIT3088(const int8_t dir_pin);
    //! @brief Destructor
    virtual ~UnitSIT3088() = default;

    /*!
      @brief Configure the DIR pin, then begin using the registered UART adapter.
      @return True if successful; false if dir_pin is unset or the adapter is invalid.
     */
    virtual bool begin() override;

    ///@name Settings for begin
    ///@{
    /*!
      @brief Gets the configuration
      @return Current configuration
     */
    inline config_t config()
    {
        return _sit_cfg;
    }
    /*!
      @brief Sets the configuration
      @param cfg Configuration settings
      @note Call before Units.begin(). Post-begin mutations have no effect.
     */
    inline void config(const config_t &cfg)
    {
        _sit_cfg = cfg;
        // Sync inherited fields (flushRX) to the parent's storage so RS485Component::begin() sees them.
        RS485Component::config(cfg);
    }
    ///@}

protected:
    /*!
      @brief One-time setup of the DIR pin as an output (called by begin()).
      @param pin GPIO number to configure.
      @note Override to customize pin setup (e.g. use hardware RS-485 RTS mode via
            uart_set_mode).
     */
    virtual void configure_dir_pin(const int8_t pin);
    /*!
      @brief Switch the transceiver into transmit (true) or receive (false) mode.
      @param tx_enable true = drive DIR high (TX), false = drive DIR low (RX).
      @note Called on every write from the ISerial decorator. Must be fast (a single
            GPIO write) so as not to eat into the RS-485 turn-around budget.
     */
    virtual void set_transmit(const bool tx_enable);
    // Install a direction-pin-controlled ISerial decorator around the raw HardwareSerial.
    virtual std::unique_ptr<ISerial> make_serial(AdapterUART *ad) override;

    inline int8_t configured_dir_pin() const
    {
        return _configured_dir_pin;
    }

private:
    config_t _sit_cfg{};
    int8_t _configured_dir_pin{-1};  // captured during begin(); immune to post-begin config changes

#if defined(ARDUINO)
    friend struct DirControlledSerial;  // internal decorator needs set_transmit() access
#endif
};

}  // namespace unit
}  // namespace m5
#endif
