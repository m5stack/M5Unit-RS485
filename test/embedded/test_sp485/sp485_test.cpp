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

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

class TestSP485 : public UARTComponentTestBase<UnitSP485, bool> {
protected:
    virtual UnitSP485* get_instance() override
    {
        auto ptr = new m5::unit::UnitSP485();
        return ptr;
    }

    virtual bool is_using_hal() const override
    {
        return GetParam();
    };

    virtual HardwareSerial* init_serial() override
    {
        auto pin_num_in  = M5.getPin(m5::pin_name_t::port_c_rxd);
        auto pin_num_out = M5.getPin(m5::pin_name_t::port_c_txd);
        if (pin_num_in < 0 || pin_num_out < 0) {
            Wire.end();
            pin_num_in  = M5.getPin(m5::pin_name_t::port_a_pin1);
            pin_num_out = M5.getPin(m5::pin_name_t::port_a_pin2);
        }

#if SOC_UART_NUM > 2
        auto& s = Serial2;
#elif SOC_UART_NUM > 1
        auto& s = Serial1;
#else
#error "Not enough Serial"
#endif

        s.end();
        s.begin(19200, SERIAL_8N1, pin_num_in, pin_num_out);
        while (s.available()) {
            s.read();
        }
        return &s;
    }
};

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSP485, ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSP485, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestSP485, ::testing::Values(false));

namespace {
}  // namespace

TEST_P(TestSP485, WriteFlushFlag)
{
    SCOPED_TRACE(ustr);
    ASSERT_NE(serial, nullptr);

    while (unit->available()) {
        unit->read();
    }

    const uint8_t b = 0x5A;
    EXPECT_GT(unit->write(b, false), 0u);
    EXPECT_GT(unit->write(b, true), 0u);
}

TEST_P(TestSP485, WriteValueReturnsTrue)
{
    SCOPED_TRACE(ustr);
    ASSERT_NE(serial, nullptr);

    while (unit->available()) {
        unit->read();
    }
    EXPECT_TRUE((unit->writeValue<uint16_t, true>(0x1234)));
    EXPECT_TRUE((unit->writeValue<uint16_t, false>(0x1234)));
}
