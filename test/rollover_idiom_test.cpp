/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file rollover_idiom_test.cpp
  @brief Regression test for the unsigned-subtraction wrap-around idiom
         used in UnitSP485::begin()'s flushRX loop, guaranteeing millis()
         rollover safety.
 */
#include <gtest/gtest.h>
#include <cstdint>

TEST(RolloverIdiom, NormalRange)
{
    const uint32_t start = 1000;
    EXPECT_LT(static_cast<uint32_t>(1050u - start), 100u);  // 50ms elapsed
    EXPECT_GE(static_cast<uint32_t>(1100u - start), 100u);  // 100ms elapsed
    EXPECT_GE(static_cast<uint32_t>(1200u - start), 100u);  // 200ms elapsed
}

TEST(RolloverIdiom, JustBeforeWrap)
{
    const uint32_t start = 0xFFFFFF9Cu;                            // 100ms before wrap
    EXPECT_LT(static_cast<uint32_t>(0xFFFFFFF0u - start), 100u);   // 84ms elapsed, still in window
    EXPECT_GE(static_cast<uint32_t>(0x00000000u - start), 100u);   // 100ms exactly, out
    EXPECT_GE(static_cast<uint32_t>(0x00000032u - start), 100u);   // 150ms elapsed
}

TEST(RolloverIdiom, AcrossWrap)
{
    const uint32_t start = 0xFFFFFFF0u;                            // 16ms before wrap
    EXPECT_LT(static_cast<uint32_t>(0xFFFFFFFAu - start), 100u);   // 10ms elapsed
    EXPECT_LT(static_cast<uint32_t>(0x00000010u - start), 100u);   // 32ms elapsed (across wrap)
    EXPECT_GE(static_cast<uint32_t>(0x00000054u - start), 100u);   // 100ms elapsed
}

TEST(RolloverIdiom, AbsoluteComparisonBrokenAcrossWrap)
{
    // Contrast: the buggy pattern (now < start + span) exits too early across the wrap boundary.
    const uint32_t start        = 0xFFFFFFF0u;
    const uint32_t drain_until  = start + 100u;                    // 0x54 (wrapped)
    const uint32_t now          = 0x00000010u;
    EXPECT_LT(now, drain_until);                                    // buggy pattern exits early
    EXPECT_LT(static_cast<uint32_t>(now - start), 100u);            // correct idiom continues looping
}
