/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SP485.cpp
  @brief SP485 unit for M5UnitUnified
 */
#include "unit_SP485.hpp"
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;

namespace m5 {
namespace unit {

// class UnitSP485
const char UnitSP485::name[] = "UnitSP485";
const types::uid_t UnitSP485::uid{"UnitSP485"_mmh3};
const types::attr_t UnitSP485::attr{attribute::AccessUART};

UnitSP485::UnitSP485() : RS485Component(DEFAULT_ADDRESS)
{
}

}  // namespace unit
}  // namespace m5
