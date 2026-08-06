/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for Tab5 built-in RS-485 (UnitSIT3088)
*/
#include <gtest/gtest.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <unit/unit_SIT3088.hpp>
#include <wiring/m5_unit_unified_wiring.hpp>        // core wiring: defaultUartSerial
#include <wiring/m5_unit_unified_rs485_wiring.hpp>  // RS485-local wiring: addTab5BuiltinRS485UART
#include <type_traits>

using namespace m5::unit::googletest;
using namespace m5::unit;

class TestTab5BuiltinRS485 : public UARTComponentTestBase<UnitSIT3088> {
protected:
    virtual UnitSIT3088* get_instance() override
    {
        return new m5::unit::UnitSIT3088();
    }

    // Overridden begin() takes over the whole UART bring-up; init_serial() is unused.
    virtual HardwareSerial* init_serial() override
    {
        return nullptr;
    }

    virtual bool begin() override
    {
        if (!m5::unit::rs485::wiring::addTab5BuiltinRS485UART(Units, *unit, 19200)) {
            return false;
        }
        serial = &m5::unit::wiring::defaultUartSerial();
        return Units.begin();
    }
};

TEST_F(TestTab5BuiltinRS485, WriteFlushFlag)
{
    SCOPED_TRACE(ustr);
    EXPECT_NE(serial, nullptr);
    if (!serial) {
        return;
    }

    while (unit->available()) {
        unit->read();
    }

    const uint8_t b = 0x5A;
    EXPECT_GT(unit->write(b, false), 0u);
    EXPECT_GT(unit->write(b, true), 0u);
}

TEST_F(TestTab5BuiltinRS485, DirPinConfigured)
{
    SCOPED_TRACE(ustr);
    // Wiring helper must have installed DIR = GPIO34 for Tab5.
    EXPECT_EQ(unit->config().dir_pin, 34);
}
