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

// Hat port UART pins (RX=SCL, TX=SDA of Hat connector)
#if defined(USING_HAT_RS485)
namespace hat {
struct UartPins {
    int rx;
    int tx;
};

UartPins get_hat_uart_pins(const m5::board_t board)
{
    switch (board) {
        case m5::board_t::board_M5StickS3:
            return {0, 8};
        case m5::board_t::board_M5StackCoreInk:
            return {26, 25};
        case m5::board_t::board_M5StickC:
        case m5::board_t::board_M5StickCPlus:
        case m5::board_t::board_M5StickCPlus2:
            return {26, 0};
        case m5::board_t::board_ArduinoNessoN1:
            return {7, 6};
        default:
            return {-1, -1};
    }
}
}  // namespace hat
#endif

// AtomBase UART pins
#if defined(USING_ATOMIC_RS485_BASE)
namespace atombase {
struct UartPins {
    int rx;
    int tx;
};

UartPins get_atombase_uart_pins(const m5::board_t board)
{
    switch (board) {
        case m5::board_t::board_M5AtomLite:
        case m5::board_t::board_M5AtomMatrix:
            return {22, 19};
        case m5::board_t::board_M5AtomS3:
        case m5::board_t::board_M5AtomS3Lite:
        case m5::board_t::board_M5AtomS3R:
        case m5::board_t::board_M5AtomEchoS3R:
        case m5::board_t::board_M5AtomS3RCam:
        case m5::board_t::board_M5AtomS3RExt:
            return {5, 6};
        default:
            return {-1, -1};
    }
}
}  // namespace atombase
#endif

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

    virtual HardwareSerial* init_serial() override
    {
#if defined(USING_HAT_RS485)
        auto board       = M5.getBoard();
        const auto pins  = hat::get_hat_uart_pins(board);
        auto pin_num_in  = pins.rx;
        auto pin_num_out = pins.tx;
#elif defined(USING_ATOMIC_RS485_BASE)
        auto board       = M5.getBoard();
        const auto pins  = atombase::get_atombase_uart_pins(board);
        auto pin_num_in  = pins.rx;
        auto pin_num_out = pins.tx;
#else
        auto pin_num_in  = M5.getPin(m5::pin_name_t::port_c_rxd);
        auto pin_num_out = M5.getPin(m5::pin_name_t::port_c_txd);
        if (pin_num_in < 0 || pin_num_out < 0) {
            if (M5.getBoard() == m5::board_t::board_M5NanoC6) {
                M5.Ex_I2C.release();
            }
            Wire.end();
            pin_num_in  = M5.getPin(m5::pin_name_t::port_a_pin1);
            pin_num_out = M5.getPin(m5::pin_name_t::port_a_pin2);
        }
#endif

        // clang-format off
#if defined(CONFIG_IDF_TARGET_ESP32C6)
    auto& s = Serial1;
#elif SOC_UART_NUM > 2
    auto& s = Serial2;
#elif SOC_UART_NUM > 1
    auto& s = Serial1;
#else
#error "Not enough Serial"
#endif
        // clang-format on

        s.end();
        s.begin(19200, SERIAL_8N1, pin_num_in, pin_num_out);
        while (s.available()) {
            s.read();
        }
        return &s;
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
