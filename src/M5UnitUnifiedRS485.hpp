/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file M5UnitUnifiedRS485.hpp
  @brief Main header of M5Unit-RS485 using M5UnitUnified

  @mainpage M5Unit-RS485
  Library for UnitRS485 / HatRS485 / AtomicRS485Base / Tab5BuiltinRS485 using M5UnitUnified.
*/
#ifndef M5_UNIT_UNIFIED_RS485_HPP
#define M5_UNIT_UNIFIED_RS485_HPP

#include "unit/unit_SP485.hpp"
#include "unit/unit_SIT3088.hpp"
#include "unit/unit_SP485_stream.hpp"

/*!
  @namespace m5
  @brief Top level namespace of M5stack
 */
namespace m5 {

/*!
  @namespace unit
  @brief Unit-related namespace
 */
namespace unit {

/*!
  @brief UnitRS485 alias of UnitSP485 (U034; SP485EEN auto-direction)
 */
using UnitRS485 = m5::unit::UnitSP485;
/*!
  @brief HatRS485 alias of UnitSP485 (U067; SP485EEN auto-direction)
 */
using HatRS485 = m5::unit::UnitSP485;
/*!
  @brief AtomicRS485Base alias of UnitSP485 (A131; SP485EEN auto-direction)
 */
using AtomicRS485Base = m5::unit::UnitSP485;
/*!
  @class Tab5BuiltinRS485
  @brief M5Stack Tab5 built-in RS-485 (SIT3088 with DIR pin fixed to GPIO34)
  @details Thin UnitSIT3088 subclass that bakes the Tab5-specific DIR pin (G34) into
           config() so that `Tab5BuiltinRS485 unit;` alone yields a ready-to-begin
           instance. The wiring helper `m5::unit::rs485::wiring::addTab5BuiltinRS485UART`
           still handles Serial pins and baud.
 */
class Tab5BuiltinRS485 : public m5::unit::UnitSIT3088 {
public:
    Tab5BuiltinRS485()
    {
        auto cfg    = config();
        cfg.dir_pin = 34;  // Tab5 built-in fixed pin
        config(cfg);
    }
    virtual ~Tab5BuiltinRS485() = default;
};

}  // namespace unit
}  // namespace m5
#endif
