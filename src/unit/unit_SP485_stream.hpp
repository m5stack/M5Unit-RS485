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
#include "unit_SP485.hpp"
#include <Stream.h>

namespace m5 {
namespace unit {

/*!
  @class UnitSP485Stream
  @brief Thin Arduino Stream/Print adapter for UnitSP485
  @note Available only in Arduino builds.
  @details Provides Arduino-compatible Stream/Print APIs (e.g. print/println/readString)
           while keeping UnitSP485 itself free from Arduino dependencies.
 */
class UnitSP485Stream final : public Stream {
public:
    /*!
      @brief Constructs a Stream adapter for UnitSP485
      @param unit UnitSP485 instance to wrap
     */
    explicit UnitSP485Stream(UnitSP485& unit) : _unit(unit)
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
    UnitSP485& _unit;
};

}  // namespace unit
}  // namespace m5
#endif
#endif
