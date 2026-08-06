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
#include <wiring/m5_unit_unified_wiring.hpp>        // core wiring: addUART / addHatUART / defaultUartSerial
#include <wiring/m5_unit_unified_rs485_wiring.hpp>  // RS485-local wiring: addAtomicBaseUART
#include <chrono>
#include <cmath>
#include <iostream>
#include <type_traits>

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

namespace {
struct SampleTrivial {
    uint16_t a;
    uint32_t b;
};

static_assert(std::is_trivially_copyable<SampleTrivial>::value && std::is_standard_layout<SampleTrivial>::value,
              "SampleTrivial must be trivially copyable and standard-layout");

}  // namespace

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
