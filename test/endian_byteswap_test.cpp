/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file endian_byteswap_test.cpp
  @brief Regression tests for UnitSP485's endian-aware byteswap dispatch
         (integral / enum / floating-point / aggregate).
 */
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <limits>
#include <unit/unit_SP485.hpp>

namespace {

// Expose the protected static template `byteswap_if_needed` for direct testing.
class ByteswapAccess : public m5::unit::UnitSP485 {
public:
    template <typename T>
    static T call(T v)
    {
        return byteswap_if_needed(v);
    }
};

}  // namespace

TEST(SP485ByteswapEndian, IntegralU16)
{
    EXPECT_EQ(ByteswapAccess::call(static_cast<uint16_t>(0x1234)), static_cast<uint16_t>(0x3412));
}

TEST(SP485ByteswapEndian, IntegralU32)
{
    EXPECT_EQ(ByteswapAccess::call(static_cast<uint32_t>(0x12345678u)), static_cast<uint32_t>(0x78563412u));
}

TEST(SP485ByteswapEndian, IntegralU64)
{
    EXPECT_EQ(ByteswapAccess::call(static_cast<uint64_t>(0x0123456789ABCDEFULL)),
              static_cast<uint64_t>(0xEFCDAB8967452301ULL));
}

TEST(SP485ByteswapEndian, IntegralSignedRoundTrip)
{
    const int32_t v = -12345678;
    EXPECT_EQ(ByteswapAccess::call(ByteswapAccess::call(v)), v);
}

TEST(SP485ByteswapEndian, EnumTreatedAsUnderlyingInteger)
{
    enum class E16 : uint16_t { X = 0x1234 };
    const E16 v       = E16::X;
    const E16 swapped = ByteswapAccess::call(v);
    EXPECT_EQ(static_cast<uint16_t>(swapped), static_cast<uint16_t>(0x3412));
}

TEST(SP485ByteswapEndian, Float32BitsReversed)
{
    // 1.0f = 0x3F800000 (big-endian bytes: 3F 80 00 00).
    const float f       = 1.0f;
    const float swapped = ByteswapAccess::call(f);
    uint32_t bits{};
    std::memcpy(&bits, &swapped, sizeof(bits));
    EXPECT_EQ(bits, static_cast<uint32_t>(0x0000803Fu));
}

TEST(SP485ByteswapEndian, Float32RoundTrip)
{
    const float f = 3.14159265f;
    EXPECT_EQ(ByteswapAccess::call(ByteswapAccess::call(f)), f);
}

TEST(SP485ByteswapEndian, Float32PosInfBitsReversed)
{
    // +inf = 0x7F800000; byte-reversed bit pattern = 0x0000807F.
    const float pinf    = std::numeric_limits<float>::infinity();
    const float swapped = ByteswapAccess::call(pinf);
    uint32_t bits{};
    std::memcpy(&bits, &swapped, sizeof(bits));
    EXPECT_EQ(bits, static_cast<uint32_t>(0x0000807Fu));
    // Round-trip via `==` is safe for ±inf (unlike NaN).
    EXPECT_EQ(ByteswapAccess::call(swapped), pinf);
}

TEST(SP485ByteswapEndian, Float32NegInfBitsReversed)
{
    // -inf = 0xFF800000; byte-reversed bit pattern = 0x000080FF.
    const float ninf    = -std::numeric_limits<float>::infinity();
    const float swapped = ByteswapAccess::call(ninf);
    uint32_t bits{};
    std::memcpy(&bits, &swapped, sizeof(bits));
    EXPECT_EQ(bits, static_cast<uint32_t>(0x000080FFu));
    EXPECT_EQ(ByteswapAccess::call(swapped), ninf);
}

TEST(SP485ByteswapEndian, Float32NaNRoundTripBitPreserving)
{
    // NaN != NaN by IEEE 754, so compare bit patterns via memcpy.
    const float nan       = std::numeric_limits<float>::quiet_NaN();
    const float roundtrip = ByteswapAccess::call(ByteswapAccess::call(nan));
    uint32_t nan_bits{}, rt_bits{};
    std::memcpy(&nan_bits, &nan, sizeof(nan_bits));
    std::memcpy(&rt_bits, &roundtrip, sizeof(rt_bits));
    EXPECT_EQ(rt_bits, nan_bits);
}

TEST(SP485ByteswapEndian, Float32NegativeZeroBitsReversed)
{
    // -0.0f = 0x80000000; byte-reversed bit pattern = 0x00000080.
    const float nzero   = -0.0f;
    const float swapped = ByteswapAccess::call(nzero);
    uint32_t bits{};
    std::memcpy(&bits, &swapped, sizeof(bits));
    EXPECT_EQ(bits, static_cast<uint32_t>(0x00000080u));
}

TEST(SP485ByteswapEndian, Double64BitsReversed)
{
    // 1.0 = 0x3FF0000000000000 (big-endian bytes: 3F F0 00 00 00 00 00 00).
    const double d       = 1.0;
    const double swapped = ByteswapAccess::call(d);
    uint64_t bits{};
    std::memcpy(&bits, &swapped, sizeof(bits));
    EXPECT_EQ(bits, static_cast<uint64_t>(0x000000000000F03FULL));
}

TEST(SP485ByteswapEndian, Double64RoundTrip)
{
    const double d = -2.718281828459045;
    EXPECT_EQ(ByteswapAccess::call(ByteswapAccess::call(d)), d);
}

TEST(SP485ByteswapEndian, AggregateReturnedAsIs)
{
    struct PODAggregate {
        uint16_t a;
        uint16_t b;
    };
    const PODAggregate v{0x1122, 0x3344};
    const PODAggregate result = ByteswapAccess::call(v);
    EXPECT_EQ(result.a, v.a);
    EXPECT_EQ(result.b, v.b);
}
