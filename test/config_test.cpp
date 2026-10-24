/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file config_test.cpp
  @brief Tests for the begin-time settings of the RS-485 units

  config_t is a begin-time setting: what the unit acts on is captured by begin(), and later writes
  to config() must not reach it. The parts that need no begin() are checked here; whether begin()
  really applies them needs an adapter, which only exists on a framework, so those cases live in
  the embedded suites.
 */
#include <gtest/gtest.h>
#include <type_traits>
#include <M5UnitUnifiedRS485.hpp>  // Tab5BuiltinRS485 subclass
#include <unit/unit_SIT3088.hpp>
#include <unit/unit_SP485.hpp>

namespace {

// Exposes the DIR pin that begin() captured, which is protected because users have no business
// reading it.
class DirPinAccess : public m5::unit::UnitSIT3088 {
public:
    int8_t capturedDirPin() const
    {
        return configured_dir_pin();
    }
};

}  // namespace

TEST(RS485ComponentConfig, FlushRXRoundTrips)
{
    m5::unit::UnitSP485 unit;

    EXPECT_TRUE(unit.config().flushRX);

    auto cfg    = unit.config();
    cfg.flushRX = false;
    unit.config(cfg);
    EXPECT_FALSE(unit.config().flushRX);
}

TEST(SIT3088Config, DirPinRoundTrips)
{
    m5::unit::UnitSIT3088 unit;

    // A bare UnitSIT3088 has no DIR pin: begin() has to refuse it rather than toggle GPIO -1
    EXPECT_EQ(unit.config().dir_pin, -1);

    auto cfg    = unit.config();
    cfg.dir_pin = 7;
    unit.config(cfg);
    EXPECT_EQ(unit.config().dir_pin, 7);
}

TEST(SIT3088Config, InheritedFlushRXReachesTheBaseStorage)
{
    m5::unit::UnitSIT3088 unit;

    auto cfg    = unit.config();
    cfg.dir_pin = 7;
    cfg.flushRX = false;
    unit.config(cfg);

    // UnitSIT3088::config() hides the base one, so the inherited field has to be copied down for
    // RS485Component::begin() to see it
    EXPECT_FALSE(static_cast<m5::unit::RS485Component&>(unit).config().flushRX);
}

TEST(SIT3088Config, DirPinIsNotCapturedBeforeBegin)
{
    DirPinAccess unit;

    auto cfg    = unit.config();
    cfg.dir_pin = 7;
    unit.config(cfg);

    // Writing config() must not reach the pin the decorator drives; only begin() captures it
    EXPECT_EQ(unit.capturedDirPin(), -1);
}

TEST(Tab5BuiltinRS485Config, BakesDirPin34)
{
    m5::unit::Tab5BuiltinRS485 unit;

    EXPECT_EQ(unit.config().dir_pin, 34);
}

// A unit must not be movable: UnitSIT3088 installs a serial backend that drives the DIR pin through
// the unit itself, so a moved-to unit would keep using the moved-from one, and a moved-from unit
// would be left without a backend at all. M5_UNIT_COMPONENT_HPP_BUILDER defaults the move members,
// so these assertions catch a base class that stops deleting them.
static_assert(!std::is_move_constructible<m5::unit::UnitSP485>::value, "a unit must not be movable");
static_assert(!std::is_move_constructible<m5::unit::UnitSIT3088>::value, "a unit must not be movable");
static_assert(!std::is_move_assignable<m5::unit::UnitSP485>::value, "a unit must not be move-assignable");
static_assert(!std::is_move_assignable<m5::unit::UnitSIT3088>::value, "a unit must not be move-assignable");

TEST(RS485ComponentConfig, UnitsAreNotMovable)
{
    EXPECT_FALSE(std::is_move_constructible<m5::unit::UnitSP485>::value);
    EXPECT_FALSE(std::is_move_constructible<m5::unit::UnitSIT3088>::value);
    EXPECT_FALSE(std::is_move_assignable<m5::unit::UnitSP485>::value);
}

TEST(SIT3088BeginGuard, FailsWhenDirPinUnset)
{
    m5::unit::UnitSIT3088 unit;

    // The guard runs before anything framework-specific, so this holds on the host too
    EXPECT_FALSE(unit.begin());
}
