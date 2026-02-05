/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file M5UnitUnifiedRS485.hpp
  @brief Main header of M5Unit-RS485 using M5UnitUnified

  @mainpage M5Unit-RS485
  Library for UnitRS485 using M5UnitUnified.
*/
#ifndef M5_UNIT_UNIFIED_RS485_HPP
#define M5_UNIT_UNIFIED_RS485_HPP

#include "unit/unit_SP485.hpp"
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
  @brief UnitRS485 alias of UnitSP485
 */
using UnitRS485 = m5::unit::UnitSP485;
/*!
  @brief HatRS485 alias of UnitSP485
 */
using HatRS485 = m5::unit::UnitSP485;
/*!
  @brief AtomicRS485Base alias of UnitSP485
 */
using AtomicRS485Base = m5::unit::UnitSP485;

}  // namespace unit
}  // namespace m5
#endif
