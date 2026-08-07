/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file m5_unit_unified_rs485_wiring.hpp
  @brief Opt-in, header-only board-aware connection helpers specific to M5Unit-RS485
  @details Mirrors the M5UnitUnified core wiring helper (`m5::unit::wiring`) but holds variants
           that are product-specific to this library and therefore do NOT belong in the generic,
           survey-derived core tables:
           - AtomicRS485Base (A131): plugs into the Atom-family bottom base, whose UART GPIOs
             differ per Atom board and are not exposed by the generic PortA/PortC helpers.
           - Tab5 built-in RS-485 (SIT3088 with manual DIR pin): fixed to M5Stack Tab5
             (ESP32-P4, RX=G21, TX=G20, DIR=G34).
  @note Include this LAST (it self-includes the core wiring helper, which detects M5Unified via
        `__M5UNIFIED_HPP__`). Example/test must `#include <M5Unified.h>` before this header.
*/
#ifndef M5_UNIT_UNIFIED_RS485_WIRING_HPP
#define M5_UNIT_UNIFIED_RS485_WIRING_HPP

#include "../unit/unit_SIT3088.hpp"
#include <M5UnitUnified.hpp>
#include <wiring/m5_unit_unified_wiring.hpp>  // core wiring: defaultUartSerial / uartPortHandle / defaultUartPort

#if defined(ARDUINO)
#include <HardwareSerial.h>
#endif

namespace m5 {
namespace unit {
namespace rs485 {
namespace wiring {

#if defined(__M5UNIFIED_HPP__)

/*!
  @struct AtomicBasePinPair
  @brief UART pin assignments (rx, tx) for AtomicRS485Base on the Atom-family bottom base
*/
struct AtomicBasePinPair {
    int8_t rx;  //!< UART RX pin on the Atom-family bottom base
    int8_t tx;  //!< UART TX pin on the Atom-family bottom base
};

/*!
  @brief Board-aware lookup for AtomicRS485Base UART pins
  @return AtomicBasePinPair for the current board; {-1, -1} for unsupported boards
*/
inline AtomicBasePinPair atomicBaseUartPins()
{
    switch (M5.getBoard()) {
        case m5::board_t::board_M5AtomLite:
        case m5::board_t::board_M5AtomMatrix:
            return {22, 19};
        case m5::board_t::board_M5AtomS3:
        case m5::board_t::board_M5AtomS3Lite:
        case m5::board_t::board_M5AtomS3R:
        case m5::board_t::board_M5AtomEchoS3R:
        case m5::board_t::board_M5AtomS3RCam:
        case m5::board_t::board_M5AtomS3RExt:
            return {5, 6};
        default:
            return {-1, -1};
    }
}

#if defined(ARDUINO)
/*!
  @brief Add a UnitSP485 (aliased as AtomicRS485Base) on the Atom-family bottom base UART
  @param units UnitUnified manager
  @param unit UnitSP485 / AtomicRS485Base instance
  @param baud Baud rate (default 115200)
  @param config Serial config (default `SERIAL_8N1`)
  @return True if successful; false on unsupported board
  @note The HardwareSerial is selected via m5::unit::wiring::defaultUartSerial() (mirrors addUART).
*/
inline bool addAtomicBaseUART(UnitUnified& units, Component& unit, const uint32_t baud = 115200,
                              const uint32_t config = SERIAL_8N1)
{
    const auto p = atomicBaseUartPins();
    if (p.rx < 0 || p.tx < 0) {
        M5_LIB_LOGE("wiring: addAtomicBaseUART unsupported board=0x%02x", static_cast<int>(M5.getBoard()));
        return false;
    }
    M5_LIB_LOGI("wiring: addAtomicBaseUART board=0x%02x rx=%d tx=%d baud=%lu", static_cast<int>(M5.getBoard()),
                static_cast<int>(p.rx), static_cast<int>(p.tx), static_cast<unsigned long>(baud));

    HardwareSerial& serial = m5::unit::wiring::defaultUartSerial();
    serial.end();
    serial.begin(baud, config, p.rx, p.tx);
    return units.add(unit, serial);
}
#else   // ESP-IDF native
/*!
  @brief Add a UnitSP485 (aliased as AtomicRS485Base) on the Atom-family bottom base UART (ESP-IDF native)
  @param units UnitUnified manager
  @param unit UnitSP485 / AtomicRS485Base instance
  @param baud Baud rate (default 115200)
  @param config UartConfig (default `UartConfig::Default` = 8N1)
  @return True if successful; false on unsupported board / UART install failure
  @note The UART port is installed via the M5UnitUnified native helper, mirroring m5::unit::wiring::addUART.
*/
inline bool addAtomicBaseUART(UnitUnified& units, Component& unit, const uint32_t baud = 115200,
                              const m5::unit::wiring::UartConfig config = m5::unit::wiring::UartConfig::Default)
{
    const auto p = atomicBaseUartPins();
    if (p.rx < 0 || p.tx < 0) {
        M5_LIB_LOGE("wiring: addAtomicBaseUART unsupported board=0x%02x", static_cast<int>(M5.getBoard()));
        return false;
    }
    M5_LIB_LOGI("wiring(ESP-IDF): addAtomicBaseUART board=0x%02x rx=%d tx=%d baud=%u", static_cast<int>(M5.getBoard()),
                static_cast<int>(p.rx), static_cast<int>(p.tx), static_cast<unsigned>(baud));

    auto port = m5::unit::wiring::uartPortHandle(m5::unit::wiring::defaultUartPort(), static_cast<gpio_num_t>(p.rx),
                                                 static_cast<gpio_num_t>(p.tx), baud, config);
    if (port == UART_NUM_MAX) {
        return false;
    }
    return units.add(unit, port);
}
#endif  // ARDUINO

///@name Tab5 built-in RS-485 (SIT3088)
///@{
/*!
  @struct Tab5BuiltinRS485Pins
  @brief Pin assignments (rx, tx, dir) for M5Stack Tab5 built-in SIT3088 RS-485
*/
struct Tab5BuiltinRS485Pins {
    int8_t rx;   //!< UART RX (Tab5 built-in: GPIO21)
    int8_t tx;   //!< UART TX (Tab5 built-in: GPIO20)
    int8_t dir;  //!< SIT3088 DIR (DE/RE tied, Tab5 built-in: GPIO34)
};

/*!
  @brief Fixed pin assignments for Tab5 built-in RS-485.
  @return Pins for Tab5; {-1, -1, -1} on any other board.
*/
inline Tab5BuiltinRS485Pins tab5BuiltinRS485Pins()
{
    switch (M5.getBoard()) {
        case m5::board_t::board_M5Tab5:
            return {21, 20, 34};
        default:
            return {-1, -1, -1};
    }
}

#if defined(ARDUINO)
/*!
  @brief Add a UnitSIT3088 (aliased as Tab5BuiltinRS485) on Tab5's built-in RS-485.
  @param units UnitUnified manager
  @param unit UnitSIT3088 / Tab5BuiltinRS485 instance (dir_pin is set via config())
  @param baud Baud rate (default 115200)
  @param config Serial config (default `SERIAL_8N1`)
  @return True if successful; false on unsupported board
  @note The HardwareSerial is selected via m5::unit::wiring::defaultUartSerial() (mirrors addUART).
*/
inline bool addTab5BuiltinRS485UART(UnitUnified& units, UnitSIT3088& unit, const uint32_t baud = 115200,
                                    const uint32_t config = SERIAL_8N1)
{
    const auto p = tab5BuiltinRS485Pins();
    if (p.rx < 0 || p.tx < 0 || p.dir < 0) {
        M5_LIB_LOGE("wiring: addTab5BuiltinRS485UART unsupported board=0x%02x", static_cast<int>(M5.getBoard()));
        return false;
    }
    M5_LIB_LOGI("wiring: addTab5BuiltinRS485UART board=0x%02x rx=%d tx=%d dir=%d baud=%lu",
                static_cast<int>(M5.getBoard()), static_cast<int>(p.rx), static_cast<int>(p.tx),
                static_cast<int>(p.dir), static_cast<unsigned long>(baud));

    auto cfg    = unit.config();
    cfg.dir_pin = p.dir;
    unit.config(cfg);

    HardwareSerial& serial = m5::unit::wiring::defaultUartSerial();
    serial.end();
    serial.begin(baud, config, p.rx, p.tx);
    return units.add(unit, serial);
}
#else   // ESP-IDF native
/*!
  @brief Add a UnitSIT3088 (aliased as Tab5BuiltinRS485) on Tab5's built-in RS-485 (ESP-IDF native)
  @note ESP-IDF native support for UnitSIT3088 is not yet implemented; this helper returns false.
*/
inline bool addTab5BuiltinRS485UART(UnitUnified& units, UnitSIT3088& unit, const uint32_t baud = 115200,
                                    const m5::unit::wiring::UartConfig config = m5::unit::wiring::UartConfig::Default)
{
    (void)units;
    (void)unit;
    (void)baud;
    (void)config;
    M5_LIB_LOGE("wiring(ESP-IDF): addTab5BuiltinRS485UART is not yet supported");
    return false;
}
#endif  // ARDUINO
///@}

#endif  // __M5UNIFIED_HPP__

}  // namespace wiring
}  // namespace rs485
}  // namespace unit
}  // namespace m5
#endif  // M5_UNIT_UNIFIED_RS485_WIRING_HPP
