/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SP485_stream.hpp
  @brief Arduino Stream adapter for UnitSP485
 */
#ifndef M5_UNIT_RS485_UNIT_SP485_STREAM_HPP
#define M5_UNIT_RS485_UNIT_SP485_STREAM_HPP

#if defined(ARDUINO)
#include "rs485_component.hpp"
#include <Stream.h>

namespace m5 {
namespace unit {

/*!
  @class UnitSP485Stream
  @brief Thin Arduino Stream/Print adapter for any RS485Component (UnitSP485 / UnitSIT3088 / ...)
  @note Available only in Arduino builds.
  @details Provides Arduino-compatible Stream/Print APIs (e.g. print/println/readString)
           while keeping the underlying RS485 unit free from Arduino dependencies.
 */
class UnitSP485Stream final : public Stream {
public:
    /*!
      @brief Constructs a Stream adapter for any RS485Component-derived unit
      @param unit RS485Component-derived instance to wrap (UnitSP485, UnitSIT3088, aliases thereof)
     */
    explicit UnitSP485Stream(RS485Component& unit) : _unit(unit)
    {
    }

    /*!
      @brief Returns the number of bytes available to read
      @return Number of bytes available
     */
    int available() override
    {
        return _unit.available();
    }
    /*!
      @brief Reads one byte
      @return Byte read or -1 if none
     */
    int read() override
    {
        return _unit.read();
    }
    /*!
      @brief Peeks the next incoming byte without removing it
      @return Next byte or -1 if none
     */
    int peek() override
    {
        return _unit.peek();
    }
    /*!
      @brief Writes one byte
      @param b Byte to write
      @return Number of bytes written
     */
    size_t write(uint8_t b) override
    {
        return _unit.write(b);
    }
    /*!
      @brief Writes bytes from buffer
      @param buffer Source buffer
      @param size Number of bytes to write
      @return Number of bytes written
     */
    size_t write(const uint8_t* buffer, size_t size) override
    {
        return _unit.write(buffer, size);
    }
    /*!
      @brief Flushes the serial port
     */
    void flush() override
    {
        _unit.flush();
    }

    /*!
      @brief Returns available buffer space for write
      @return Number of bytes that can be written without blocking
     */
    int availableForWrite()
    {
        return _unit.availableForWrite();
    }

    using Print::write;

private:
    RS485Component& _unit;
};

}  // namespace unit
}  // namespace m5
#endif
#endif
