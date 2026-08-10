/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SP485.hpp
  @brief SP485 unit for M5UnitUnified
 */
#ifndef M5_UNIT_RS485_UNIT_SP485_HPP
#define M5_UNIT_RS485_UNIT_SP485_HPP

#include "rs485_component.hpp"

namespace m5 {
namespace unit {

/*!
  @class UnitSP485
  @brief RS-485 unit using the SP485EEN transceiver (auto direction control)
 */
class UnitSP485 : public RS485Component {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitSP485, 0x00 /*UART*/);

public:
    /*!
      @brief Constructor
     */
    UnitSP485();
    //! @brief Destructor
    virtual ~UnitSP485() = default;
};

}  // namespace unit
}  // namespace m5
#endif
