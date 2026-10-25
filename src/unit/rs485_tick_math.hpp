/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file rs485_tick_math.hpp
  @brief Milliseconds-to-ticks conversion for the ESP-IDF serial backend

  Kept free of ESP-IDF headers so the edges below can be exercised on the host: the backend itself
  only builds for ESP-IDF, where a wrong conversion is invisible from a serial monitor.
 */
#ifndef M5_UNIT_RS485_RS485_TICK_MATH_HPP
#define M5_UNIT_RS485_RS485_TICK_MATH_HPP

#include <cstdint>

namespace m5 {
namespace unit {
namespace rs485 {
namespace detail {

/*!
  @brief Converts a caller-supplied timeout to scheduler ticks
  @param timeout_ms Timeout in milliseconds; 0 means "do not block"
  @param tick_rate_hz Scheduler tick rate (CONFIG_FREERTOS_HZ)
  @param max_delay The value that means "block forever" and must not be reachable
  @return Ticks to wait, in [0, max_delay - 1]
  @details Avoids both ends pdMS_TO_TICKS() gets wrong for a caller-supplied timeout. It truncates
  towards zero, so with the ESP-IDF default tick rate of 100 Hz any timeout below 10 ms would
  become a non-blocking read; and it multiplies in 32 bits, so a timeout above
  2^32 / tick_rate_hz milliseconds would wrap to an unrelated, usually much shorter, wait.
  Saturating just below max_delay keeps "block forever" out of reach of a timeout argument.
 */
template <typename Tick>
inline Tick ticks_from_ms(const uint32_t timeout_ms, const uint32_t tick_rate_hz, const Tick max_delay)
{
    const uint64_t ticks = (static_cast<uint64_t>(timeout_ms) * tick_rate_hz) / 1000U;
    if (ticks == 0) {
        return (timeout_ms > 0) ? static_cast<Tick>(1) : static_cast<Tick>(0);
    }
    return (ticks >= static_cast<uint64_t>(max_delay)) ? static_cast<Tick>(max_delay - 1) : static_cast<Tick>(ticks);
}

}  // namespace detail
}  // namespace rs485
}  // namespace unit
}  // namespace m5
#endif
