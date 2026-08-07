/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for SP485
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <unit/unit_SP485.hpp>
#include <unit/unit_SP485_stream.hpp>               // UnitSP485Stream Arduino Stream adapter
#include <wiring/m5_unit_unified_wiring.hpp>        // core wiring: addUART / addHatUART / defaultUartSerial
#include <wiring/m5_unit_unified_rs485_wiring.hpp>  // RS485-local wiring: addAtomicBaseUART
#include <chrono>
#include <cmath>
#include <iostream>

#if !defined(USING_UNIT_RS485) && !defined(USING_HAT_RS485) && !defined(USING_ATOMIC_RS485_BASE)
// For UnitRS485
// #define USING_UNIT_RS485
// For HatRS485
// #define USING_HAT_RS485
// For AtomRS485
// #define USING_ATOMIC_RS485_BASE
#endif

using namespace m5::unit::googletest;
using namespace m5::unit;

class TestSP485 : public UARTComponentTestBase<UnitSP485> {
protected:
    virtual UnitSP485* get_instance() override
    {
        auto ptr = new m5::unit::UnitSP485();
        return ptr;
    }

    // Overridden begin() takes over the whole UART bring-up; init_serial() is unused.
    virtual HardwareSerial* init_serial() override
    {
        return nullptr;
    }

    virtual bool begin() override
    {
        bool ok = false;
#if defined(USING_HAT_RS485)
        ok = m5::unit::wiring::addHatUART(Units, *unit, 19200);
#elif defined(USING_ATOMIC_RS485_BASE)
        ok = m5::unit::rs485::wiring::addAtomicBaseUART(Units, *unit, 19200);
#else  // USING_UNIT_RS485
        ok = m5::unit::wiring::addUART(Units, *unit, 19200);
#endif
        if (!ok) {
            return false;
        }
        serial = &m5::unit::wiring::defaultUartSerial();
        return Units.begin();
    }
};

TEST_F(TestSP485, WriteFlushFlag)
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

TEST_F(TestSP485, WriteValueReturnsTrue)
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

TEST_F(TestSP485, WriteNullReturnsZero)
{
    SCOPED_TRACE(ustr);
    EXPECT_NE(serial, nullptr);
    if (!serial) {
        return;
    }
    // write(char*) has an explicit null guard.
    EXPECT_EQ(unit->write(static_cast<const char*>(nullptr)), 0u);
}

TEST_F(TestSP485, WriteValueFloat)
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

TEST_F(TestSP485, WriteValueUInt32)
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

TEST_F(TestSP485, WriteValueUInt64)
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

TEST_F(TestSP485, WriteBufferSize)
{
    SCOPED_TRACE(ustr);
    EXPECT_NE(serial, nullptr);
    if (!serial) {
        return;
    }
    while (unit->available()) {
        unit->read();
    }
    const uint8_t buf[3] = {0x01, 0x02, 0x03};
    EXPECT_EQ(unit->write(buf, sizeof(buf), false), sizeof(buf));
    EXPECT_EQ(unit->write(buf, sizeof(buf), true), sizeof(buf));
    // Zero-size and null-buffer guards
    EXPECT_EQ(unit->write(buf, 0u), 0u);
    EXPECT_EQ(unit->write(static_cast<const uint8_t*>(nullptr), 5u), 0u);
}

TEST_F(TestSP485, WriteCharBufferSize)
{
    SCOPED_TRACE(ustr);
    EXPECT_NE(serial, nullptr);
    if (!serial) {
        return;
    }
    while (unit->available()) {
        unit->read();
    }
    const char buf[] = "abc";
    EXPECT_EQ(unit->write(buf, 3u), 3u);
}

TEST_F(TestSP485, WriteNonNullCString)
{
    SCOPED_TRACE(ustr);
    EXPECT_NE(serial, nullptr);
    if (!serial) {
        return;
    }
    while (unit->available()) {
        unit->read();
    }
    EXPECT_EQ(unit->write("hello"), 5u);
    EXPECT_EQ(unit->write(""), 0u);
}

// UnitSP485Stream (Arduino Stream adapter) smoke test: verifies the adapter forwards
// write() calls to the underlying RS485Component.
TEST_F(TestSP485, StreamAdapterForwardsWrite)
{
    SCOPED_TRACE(ustr);
    EXPECT_NE(serial, nullptr);
    if (!serial) {
        return;
    }
    m5::unit::UnitSP485Stream stream(*unit);
    while (unit->available()) {
        unit->read();
    }
    EXPECT_GT(stream.write(static_cast<uint8_t>(0x5A)), 0u);
    const uint8_t buf[3] = {0x01, 0x02, 0x03};
    EXPECT_EQ(stream.write(buf, sizeof(buf)), sizeof(buf));
    // Print API delegates to write().
    EXPECT_EQ(stream.print("hi"), 2u);
}
