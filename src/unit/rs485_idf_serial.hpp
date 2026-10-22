/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file rs485_idf_serial.hpp
  @brief ESP-IDF UART-driver-backed RS485Component::ISerial (library-internal)
  @note Not part of the public API: this header is included by the library's translation units
        only, so that driver/uart.h never leaks into the framework-agnostic public headers.
 */
#ifndef M5_UNIT_RS485_RS485_IDF_SERIAL_HPP
#define M5_UNIT_RS485_RS485_IDF_SERIAL_HPP

#if !defined(ARDUINO) && defined(ESP_PLATFORM)

#include "rs485_component.hpp"
#include <m5_unit_component/adapter_uart.hpp>
#include <driver/uart.h>
#include <freertos/FreeRTOS.h>
#include <soc/soc_caps.h>

namespace m5 {
namespace unit {

//
// ESP-IDF UART-driver-backed ISerial.
//
// Arduino's Stream contract is reproduced on top of driver/uart.h: read() / read(buf, len) do not
// block, readBytes() waits, and flush() waits for TX to drain. peek() has no driver counterpart,
// so one byte is held back here.
//
struct IDFSerial : public RS485Component::ISerial {
    explicit IDFSerial(const uart_port_t port) : _port(port)
    {
    }

    int available() override
    {
        size_t len{};
        if (uart_get_buffered_data_len(_port, &len) != ESP_OK) {
            return (_peeked >= 0) ? 1 : 0;
        }
        return static_cast<int>(len) + ((_peeked >= 0) ? 1 : 0);
    }
    int availableForWrite() override
    {
        size_t free_size{};
        // The port is installed without a TX ring buffer, so uart_write_bytes() copies straight
        // into the hardware FIFO and blocks. Report the FIFO length as the writable chunk size.
        if (uart_get_tx_buffer_free_size(_port, &free_size) != ESP_OK || free_size == 0) {
            return SOC_UART_FIFO_LEN;
        }
        return static_cast<int>(free_size);
    }
    int peek() override
    {
        if (_peeked < 0) {
            uint8_t b{};
            if (uart_read_bytes(_port, &b, 1, 0) == 1) {
                _peeked = b;
            }
        }
        return _peeked;
    }
    int read() override
    {
        if (_peeked >= 0) {
            const int value = _peeked;
            _peeked         = -1;
            return value;
        }
        uint8_t b{};
        return (uart_read_bytes(_port, &b, 1, 0) == 1) ? static_cast<int>(b) : -1;
    }
    size_t read(uint8_t *b, const size_t n) override
    {
        return read_with_wait(b, n, 0);
    }
    size_t readBytes(uint8_t *b, const size_t n) override
    {
        return read_with_wait(b, n, ticks_of(_timeout_ms));
    }
    size_t readBytes(uint8_t *b, const size_t n, const uint32_t timeout_ms) override
    {
        return read_with_wait(b, n, ticks_of(timeout_ms));
    }
    void flush() override
    {
        uart_wait_tx_done(_port, portMAX_DELAY);
    }
    void flush(const bool txOnly) override
    {
        uart_wait_tx_done(_port, portMAX_DELAY);
        if (!txOnly) {
            uart_flush_input(_port);
            _peeked = -1;
        }
    }
    size_t write(const uint8_t *b, const size_t n) override
    {
        if (b == nullptr || n == 0) {
            return 0;
        }
        const int written = uart_write_bytes(_port, b, n);
        return (written > 0) ? static_cast<size_t>(written) : 0;
    }
    size_t write(uint8_t b) override
    {
        return write(&b, 1);
    }
    uint32_t baudRate() const override
    {
        uint32_t baud{};
        return (uart_get_baudrate(_port, &baud) == ESP_OK) ? baud : 0;
    }

protected:
    // pdMS_TO_TICKS() truncates towards zero, so with the ESP-IDF default CONFIG_FREERTOS_HZ=100
    // every timeout below 10 ms would become 0 ticks, i.e. a non-blocking read. Keep at least one
    // tick so a caller asking for any wait at all gets one.
    static TickType_t ticks_of(const uint32_t timeout_ms)
    {
        const TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
        return (ticks == 0 && timeout_ms > 0) ? 1 : ticks;
    }

    size_t read_with_wait(uint8_t *b, const size_t n, const TickType_t ticks)
    {
        if (b == nullptr || n == 0) {
            return 0;
        }
        size_t total{};
        if (_peeked >= 0) {
            b[0]    = static_cast<uint8_t>(_peeked);
            _peeked = -1;
            ++total;
        }
        if (total >= n) {
            return total;
        }
        const int got = uart_read_bytes(_port, b + total, n - total, ticks);
        return (got > 0) ? total + static_cast<size_t>(got) : total;
    }

    uart_port_t _port{UART_NUM_1};
    int _peeked{-1};
    // Timeout of the two-argument readBytes() (and therefore of readValue<T>()). Fixed at the
    // Arduino Stream default: this backend has no HardwareSerial object whose setTimeout() could
    // be inherited. Pass an explicit timeout to the three-argument readBytes() to override it.
    uint32_t _timeout_ms{1000};
};

//
// Recover the uart_port_t the adapter was built on.
//
// AdapterUART exposes the port only through AdapterUART::ESPIDFImpl::uartPort(), and ESP-IDF builds
// with -fno-rtti so dynamic_cast is unavailable. On this platform ESPIDFImpl is the only UARTImpl
// AdapterUART ever installs (SerialImpl exists under ARDUINO only), so the downcast is sound.
//
inline uart_port_t uart_port_of(AdapterUART *ad)
{
    return static_cast<AdapterUART::ESPIDFImpl *>(ad->impl())->uartPort();
}

}  // namespace unit
}  // namespace m5

#endif  // !ARDUINO && ESP_PLATFORM
#endif
