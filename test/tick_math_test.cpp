/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file tick_math_test.cpp
  @brief Tests for the milliseconds-to-ticks conversion of the ESP-IDF serial backend

  The backend builds only for ESP-IDF, where a wrong conversion shows up as a read that does not
  wait, or one that waits forever, rather than as anything visible on a serial monitor. The
  conversion is pure arithmetic, so it is checked here with the tick rate and the "block forever"
  value passed in, standing in for CONFIG_FREERTOS_HZ and portMAX_DELAY.
 */
#include <gtest/gtest.h>
#include <cstdint>
#include <unit/rs485_tick_math.hpp>

namespace {

using m5::unit::rs485::detail::ticks_from_ms;

// The ESP-IDF defaults: 100 Hz means one tick is 10 ms, which is what makes the floor necessary.
constexpr uint32_t HZ_100{100};
constexpr uint32_t HZ_1000{1000};
constexpr uint32_t MAX_DELAY{0xFFFFFFFFu};

}  // namespace

TEST(TickMath, ZeroStaysNonBlocking)
{
    // 0 ms is the caller asking not to block, which must survive the floor below
    EXPECT_EQ(ticks_from_ms<uint32_t>(0, HZ_100, MAX_DELAY), 0u);
    EXPECT_EQ(ticks_from_ms<uint32_t>(0, HZ_1000, MAX_DELAY), 0u);
}

TEST(TickMath, SubTickTimeoutsAreFlooredToOneTick)
{
    // At 100 Hz everything under 10 ms truncates to zero ticks; left alone it would turn a short
    // wait into a non-blocking read, which is the opposite of what the caller asked for.
    for (uint32_t ms = 1; ms < 10; ++ms) {
        EXPECT_EQ(ticks_from_ms<uint32_t>(ms, HZ_100, MAX_DELAY), 1u) << "ms=" << ms;
    }
    EXPECT_EQ(ticks_from_ms<uint32_t>(10, HZ_100, MAX_DELAY), 1u);  // exactly one tick, not floored
    EXPECT_EQ(ticks_from_ms<uint32_t>(11, HZ_100, MAX_DELAY), 1u);  // truncation, not rounding
    EXPECT_EQ(ticks_from_ms<uint32_t>(20, HZ_100, MAX_DELAY), 2u);
}

TEST(TickMath, OrdinaryTimeoutsScaleWithTheTickRate)
{
    EXPECT_EQ(ticks_from_ms<uint32_t>(1000, HZ_100, MAX_DELAY), 100u);
    EXPECT_EQ(ticks_from_ms<uint32_t>(1000, HZ_1000, MAX_DELAY), 1000u);
    EXPECT_EQ(ticks_from_ms<uint32_t>(1, HZ_1000, MAX_DELAY), 1u);
}

TEST(TickMath, BlockForeverIsNeverReachable)
{
    // max_delay means "wait without end", so a timeout must stop one tick short of it however
    // large it is -- including the values where a 32-bit multiply would have wrapped.
    // At 1000 Hz the largest timeout a uint32_t can hold lands exactly on max_delay, so this is
    // where the clamp earns its place: without it the read would block forever.
    EXPECT_EQ(ticks_from_ms<uint32_t>(MAX_DELAY, HZ_1000, MAX_DELAY), MAX_DELAY - 1);
    EXPECT_LT(ticks_from_ms<uint32_t>(MAX_DELAY - 1, HZ_1000, MAX_DELAY), MAX_DELAY);
    EXPECT_EQ(ticks_from_ms<uint32_t>(MAX_DELAY / 1000 * 1000, HZ_1000, MAX_DELAY), MAX_DELAY / 1000 * 1000);

    // At 100 Hz a 32-bit tick type cannot be driven to max_delay at all: the largest uint32_t
    // millisecond value is only ~4.3e8 ticks, about 49 days. The 64-bit multiply is what keeps it
    // from wrapping to something short; a 32-bit one would overflow well before here.
    EXPECT_EQ(ticks_from_ms<uint32_t>(MAX_DELAY, HZ_100, MAX_DELAY), 429496729u);
    EXPECT_LT(ticks_from_ms<uint32_t>(MAX_DELAY, HZ_100, MAX_DELAY), MAX_DELAY);
}

TEST(TickMath, A16BitTickTypeSaturatesToo)
{
    // TickType_t is 16-bit when FreeRTOS is configured for it, which makes saturation reachable
    // from timeouts a user might really pass.
    constexpr uint16_t max16{0xFFFFu};
    EXPECT_EQ(ticks_from_ms<uint16_t>(1000, HZ_100, max16), 100u);
    EXPECT_EQ(ticks_from_ms<uint16_t>(700000, HZ_100, max16), static_cast<uint16_t>(max16 - 1));
    EXPECT_EQ(ticks_from_ms<uint16_t>(5, HZ_100, max16), 1u);
    EXPECT_EQ(ticks_from_ms<uint16_t>(0, HZ_100, max16), 0u);
}
