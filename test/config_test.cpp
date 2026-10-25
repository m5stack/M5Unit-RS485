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
#include <utility>
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

// A unit must be movable, and the move must not throw: the base class defaults the move members
// with noexcept, which a base whose own move can throw would turn into deleted ones.
// Tab5BuiltinRS485 declares a destructor, so it has to declare the move members as well.
static_assert(std::is_nothrow_move_constructible<m5::unit::UnitSP485>::value, "a unit must be movable");
static_assert(std::is_nothrow_move_constructible<m5::unit::UnitSIT3088>::value, "a unit must be movable");
static_assert(std::is_nothrow_move_constructible<m5::unit::Tab5BuiltinRS485>::value, "a unit must be movable");
static_assert(std::is_nothrow_move_assignable<m5::unit::UnitSP485>::value, "a unit must be move-assignable");
static_assert(std::is_nothrow_move_assignable<m5::unit::UnitSIT3088>::value, "a unit must be move-assignable");
static_assert(std::is_nothrow_move_assignable<m5::unit::Tab5BuiltinRS485>::value, "a unit must be move-assignable");

// Copying is prohibited: two components cannot own one serial backend. UnitSIT3088 and
// Tab5BuiltinRS485 get it from declaring move members, which is easy to undo by accident.
static_assert(!std::is_copy_constructible<m5::unit::UnitSP485>::value, "a unit must not be copyable");
static_assert(!std::is_copy_assignable<m5::unit::UnitSP485>::value, "a unit must not be copy-assignable");
static_assert(!std::is_copy_constructible<m5::unit::UnitSIT3088>::value, "a unit must not be copyable");
static_assert(!std::is_copy_constructible<m5::unit::Tab5BuiltinRS485>::value, "a unit must not be copyable");

// config() reads the settings, so it must be callable on a const unit, and it returns a copy so
// that a caller cannot write through it.
static_assert(
    std::is_same<decltype(std::declval<const m5::unit::UnitSP485&>().config()), m5::unit::UnitSP485::config_t>::value,
    "config() must be const and return by value");
static_assert(std::is_same<decltype(std::declval<const m5::unit::UnitSIT3088&>().config()),
                           m5::unit::UnitSIT3088::config_t>::value,
              "config() must be const and return by value");

TEST(RS485ComponentConfig, UnitsAreMovableButNotCopyable)
{
    EXPECT_TRUE(std::is_nothrow_move_constructible<m5::unit::UnitSP485>::value);
    EXPECT_TRUE(std::is_nothrow_move_constructible<m5::unit::UnitSIT3088>::value);
    EXPECT_TRUE(std::is_nothrow_move_assignable<m5::unit::UnitSP485>::value);
    EXPECT_FALSE(std::is_copy_constructible<m5::unit::UnitSP485>::value);

    // The DIR pin setting survives a move
    m5::unit::Tab5BuiltinRS485 unit;
    m5::unit::Tab5BuiltinRS485 moved{std::move(unit)};
    EXPECT_EQ(moved.config().dir_pin, 34);
}

// Move assignment carries the configuration as well. The two halves need separate units to be
// observed: RS485Component::operator= copies the base config_t by hand, which only a unit that
// does not shadow config() exposes, while the derived config_t rides on the unit's own defaulted
// operator.
TEST(RS485ComponentConfig, MoveAssignmentCarriesTheBaseConfiguration)
{
    m5::unit::UnitSP485 source;
    auto source_cfg    = source.config();
    source_cfg.flushRX = false;
    source.config(source_cfg);

    m5::unit::UnitSP485 target;
    EXPECT_TRUE(target.config().flushRX);

    target = std::move(source);
    EXPECT_FALSE(target.config().flushRX);
}

TEST(RS485ComponentConfig, MoveAssignmentCarriesTheDerivedConfiguration)
{
    m5::unit::Tab5BuiltinRS485 source;
    auto source_cfg    = source.config();
    source_cfg.dir_pin = 21;
    source.config(source_cfg);

    m5::unit::Tab5BuiltinRS485 target;
    EXPECT_EQ(target.config().dir_pin, 34);

    target = std::move(source);
    EXPECT_EQ(target.config().dir_pin, 21);
}

TEST(SIT3088BeginGuard, FailsWhenDirPinUnset)
{
    m5::unit::UnitSIT3088 unit;

    // The guard runs before anything framework-specific, so this holds on the host too
    EXPECT_FALSE(unit.begin());
}
