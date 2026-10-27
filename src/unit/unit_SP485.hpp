/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SP485.hpp
  @brief SP485-family unit for M5UnitUnified (board-side direction control)
 */
#ifndef M5_UNIT_RS485_UNIT_SP485_HPP
#define M5_UNIT_RS485_UNIT_SP485_HPP

#include "rs485_component.hpp"

namespace m5 {
namespace unit {

/*!
  @class UnitSP485
  @brief RS-485 unit whose transceiver direction is controlled on the board, not by software
  @details Covers the SP485-family products: SP485EEN on UnitRS485 (U034) and HatRS485 (U067),
           SP3485EN on Atomic RS485 Base (A131). On all three the transceiver's DI is tied to GND
           and DE / /RE are driven from TX by a transistor, so no direction pin reaches the host
           and the unit needs no direction handling (contrast UnitSIT3088).
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
