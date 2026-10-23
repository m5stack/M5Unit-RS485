/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file write_overload_resolution_test.cpp
  @brief Regression tests for which write() overload a given second argument selects

  The second argument of the two-argument write() is a byte count. Every integer length has to
  reach the overload taking a size, and the deleted guard overloads have to keep rejecting what
  cannot be a count. A rejected call is a compile error by design, so both directions are
  asserted at compile time through a detection trait rather than by calling write().
 */
#include <gtest/gtest.h>
#include <bitset>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>
#include <unit/unit_SP485.hpp>

namespace {

// True when write(const uint8_t *, Size) is callable, i.e. Size is accepted as a byte count.
template <typename Size, typename = void>
struct writable_bytes : std::false_type {
};
template <typename Size>
struct writable_bytes<Size, decltype((void)std::declval<m5::unit::UnitSP485 &>().write(
                                std::declval<const uint8_t *>(), std::declval<Size>()))> : std::true_type {
};

// Same for write(const char *, Size).
template <typename Size, typename = void>
struct writable_chars : std::false_type {
};
template <typename Size>
struct writable_chars<Size, decltype((void)std::declval<m5::unit::UnitSP485 &>().write(
                                std::declval<const char *>(), std::declval<Size>()))> : std::true_type {
};

// Integer lengths: accepted. A signed int is the shape most user code has, and the one that used
// to be ambiguous while write() still took a trailing bool.
static_assert(writable_bytes<int>::value, "a signed length must resolve to write(const uint8_t *, size_t)");
static_assert(writable_bytes<unsigned int>::value, "an unsigned length must resolve");
static_assert(writable_bytes<uint8_t>::value, "a uint8_t length must resolve");
static_assert(writable_bytes<size_t>::value, "a size_t length must resolve");
static_assert(writable_chars<int>::value, "a signed length must resolve to write(const char *, size_t)");
static_assert(writable_chars<unsigned int>::value, "an unsigned length must resolve");
static_assert(writable_chars<size_t>::value, "a size_t length must resolve");

// Character types: rejected, because write(buffer, 'a') would otherwise mean 97 bytes.
static_assert(!writable_bytes<char>::value, "a char must not be taken as a byte count");
static_assert(!writable_bytes<signed char>::value, "a signed char must not be taken as a byte count");
static_assert(!writable_chars<char>::value, "a char must not be taken as a byte count");
static_assert(!writable_chars<signed char>::value, "a signed char must not be taken as a byte count");

// Non-integral types: rejected. The bool-convertible proxies are what the removed flush flag used
// to accept, and would otherwise silently mean a one-byte write.
static_assert(!writable_bytes<double>::value, "a non-integral length must be rejected");
static_assert(!writable_chars<double>::value, "a non-integral length must be rejected");
static_assert(!writable_chars<std::vector<bool>::reference>::value, "a vector<bool> proxy must be rejected");
static_assert(!writable_chars<std::bitset<8>::reference>::value, "a bitset proxy must be rejected");

}  // namespace

// The assertions above hold at compile time; this case reports them in the suite's results.
TEST(WriteOverloadResolution, SecondArgumentIsAByteCount)
{
    EXPECT_TRUE(writable_bytes<int>::value);
    EXPECT_TRUE(writable_chars<size_t>::value);
    EXPECT_FALSE(writable_chars<char>::value);
    EXPECT_FALSE(writable_chars<std::vector<bool>::reference>::value);
}
