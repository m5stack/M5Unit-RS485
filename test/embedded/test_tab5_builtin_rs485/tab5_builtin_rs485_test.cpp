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
#include <M5UnitUnifiedRS485.hpp>  // Tab5BuiltinRS485 subclass
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

TEST_F(TestTab5BuiltinRS485, WriteValueUInt16)
{
    SCOPED_TRACE(ustr);
    EXPECT_NE(serial, nullptr);
    if (!serial) {
        return;
    }
    while (unit->available()) {
        unit->read();
    }
    EXPECT_TRUE((unit->writeValue<uint16_t, true>(0x1234)));
    EXPECT_TRUE((unit->writeValue<uint16_t, false>(0x1234)));
}

TEST_F(TestTab5BuiltinRS485, WriteValueUInt32)
{
    SCOPED_TRACE(ustr);
    EXPECT_NE(serial, nullptr);
    if (!serial) {
        return;
    }
    while (unit->available()) {
        unit->read();
    }
    EXPECT_TRUE((unit->writeValue<uint32_t, true>(0x12345678u)));
    EXPECT_TRUE((unit->writeValue<uint32_t, false>(0x12345678u)));
}

TEST_F(TestTab5BuiltinRS485, WriteValueUInt64)
{
    SCOPED_TRACE(ustr);
    EXPECT_NE(serial, nullptr);
    if (!serial) {
        return;
    }
    while (unit->available()) {
        unit->read();
    }
    EXPECT_TRUE((unit->writeValue<uint64_t, true>(0x0123456789ABCDEFULL)));
    EXPECT_TRUE((unit->writeValue<uint64_t, false>(0x0123456789ABCDEFULL)));
}

TEST_F(TestTab5BuiltinRS485, WriteValueFloat)
{
    SCOPED_TRACE(ustr);
    EXPECT_NE(serial, nullptr);
    if (!serial) {
        return;
    }
    while (unit->available()) {
        unit->read();
    }
    EXPECT_TRUE((unit->writeValue<float, true>(3.14159265f)));
    EXPECT_TRUE((unit->writeValue<float, false>(3.14159265f)));
}

TEST_F(TestTab5BuiltinRS485, WriteNullReturnsZero)
{
    SCOPED_TRACE(ustr);
    EXPECT_NE(serial, nullptr);
    if (!serial) {
        return;
    }
    EXPECT_EQ(unit->write(static_cast<const char*>(nullptr)), 0u);
}

// Verifies that Tab5BuiltinRS485's constructor alone (without the wiring helper)
// bakes DIR = GPIO34 into config. Distinguishes ctor-set from wiring-set.
TEST(Tab5BuiltinRS485Ctor, BakesDirPin34)
{
    m5::unit::Tab5BuiltinRS485 unit;
    EXPECT_EQ(unit.config().dir_pin, 34);
}

// Negative test: bare UnitSIT3088 (not Tab5BuiltinRS485) has dir_pin=-1 by default,
// so begin() must fail. Confirms the safety guard in UnitSIT3088::begin().
TEST(UnitSIT3088BeginGuard, FailsWhenDirPinUnset)
{
    m5::unit::UnitUnified units;
    m5::unit::UnitSIT3088 unit;
    EXPECT_FALSE(unit.begin());
    (void)units;
}
