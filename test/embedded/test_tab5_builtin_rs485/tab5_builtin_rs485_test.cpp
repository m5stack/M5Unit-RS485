/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for Tab5 built-in RS-485 (UnitSIT3088)
*/
#include <gtest/gtest.h>
#include <utility>  // std::move
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <M5UnitUnifiedRS485.hpp>  // Tab5BuiltinRS485 subclass
#include <googletest/test_template.hpp>
#include <unit/unit_SIT3088.hpp>
#include <wiring/m5_unit_unified_wiring.hpp>        // core wiring: defaultUartSerial
#include <wiring/m5_unit_unified_rs485_wiring.hpp>  // RS485-local wiring: addTab5BuiltinRS485UART

using namespace m5::unit::googletest;
using namespace m5::unit;

// Exposes what begin() captured, and counts the DIR writes the ISerial decorator makes. Both are
// protected on the unit, because users have no business touching them.
class Tab5Probe : public m5::unit::Tab5BuiltinRS485 {
public:
    int8_t capturedDirPin() const
    {
        return configured_dir_pin();
    }
    size_t transmitCalls() const
    {
        return _transmit_calls;
    }
    void resetTransmitCalls()
    {
        _transmit_calls = 0;
    }

protected:
    void set_transmit(const bool tx_enable) override
    {
        ++_transmit_calls;
        Tab5BuiltinRS485::set_transmit(tx_enable);
    }

private:
    size_t _transmit_calls{};
};

// Brings a Tab5Probe up the way the wiring helper does. Each test gets a fresh unit from SetUp(),
// so re-initialising the shared serial here is harmless.
inline bool begin_probe(m5::unit::UnitUnified& units, Tab5Probe& probe)
{
    return m5::unit::rs485::wiring::addTab5BuiltinRS485UART(units, probe, 19200) && units.begin();
}

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

TEST_F(TestTab5BuiltinRS485, WriteQueuesThenFlush)
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
    EXPECT_GT(unit->write(b), 0u);
    // write() queues; flush() is what waits for the transmission to complete
    unit->flush();
    EXPECT_GT(unit->write(b), 0u);
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

// config_t is documented as a begin-time setting: begin() captures dir_pin and later writes to
// config() must not move the pin the decorator drives. Only the pre-begin state is checked on the
// host, because begin() cannot succeed there.
TEST_F(TestTab5BuiltinRS485, BeginCapturesDirPinAndIgnoresLaterChanges)
{
    SCOPED_TRACE(ustr);

    m5::unit::UnitUnified units;
    Tab5Probe probe;
    ASSERT_TRUE(begin_probe(units, probe));
    EXPECT_EQ(probe.capturedDirPin(), 34);

    auto cfg    = probe.config();
    cfg.dir_pin = 7;  // a pin the Tab5 does not use for DIR
    probe.config(cfg);
    EXPECT_EQ(probe.config().dir_pin, 7);   // the setting took
    EXPECT_EQ(probe.capturedDirPin(), 34);  // ... but what begin() captured did not move

    // And the decorator still drives the captured pin on every write
    probe.resetTransmitCalls();
    const uint8_t buf[2] = {0x5A, 0xA5};
    EXPECT_EQ(probe.write(buf, sizeof(buf)), sizeof(buf));
    EXPECT_GE(probe.transmitCalls(), 2u);  // one for transmit, one to release the line
}

// The decorator holds a pointer back to the unit it drives, and rebind() is what re-points it when
// the unit moves. Deleting either rebind() override passes every host test, because the host never
// installs a real decorator -- this is the only place the real one is exercised.
//
// Moving a unit after UnitUnified::add() is documented as unsupported (UnitUnified keeps the
// address it was given), so this checks defensive behaviour, not a supported usage: whatever else
// breaks, the decorator must drive the unit it was handed to and not the carcass of the old one.
TEST_F(TestTab5BuiltinRS485, AMovedUnitKeepsDrivingTheDirPin)
{
    SCOPED_TRACE(ustr);

    m5::unit::UnitUnified units;
    Tab5Probe source;
    ASSERT_TRUE(begin_probe(units, source));
    source.resetTransmitCalls();

    Tab5Probe moved{std::move(source)};
    moved.resetTransmitCalls();

    const uint8_t buf[2] = {0x33, 0xCC};
    EXPECT_EQ(moved.write(buf, sizeof(buf)), sizeof(buf));

    EXPECT_GE(moved.transmitCalls(), 2u) << "the decorator did not follow the move";
    EXPECT_EQ(source.transmitCalls(), 0u) << "the decorator still drives the moved-from unit";
}
