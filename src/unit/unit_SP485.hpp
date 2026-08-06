/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SP485.hpp
  @brief SP485 unit for M5UnitUnified
 */
#ifndef M5_UNIT_RS485_UNIT_SP485_HPP
#define M5_UNIT_RS485_UNIT_SP485_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/stl/byteswap.hpp>
#include <m5_utility/stl/endianness.hpp>
#include <cstring>
#include <type_traits>

class HardwareSerial;

namespace m5 {
namespace unit {

/*!
  @class UnitSP485
  @brief RS-485 unit using SP485 transceiver
 */
class UnitSP485 : public Component {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitSP485, 0x00 /*UART*/);

public:
    ///@cond
    struct ISerial {
        virtual ~ISerial()                                         = default;
        virtual int available()                                    = 0;
        virtual int availableForWrite()                            = 0;
        virtual int peek()                                         = 0;
        virtual int read()                                         = 0;
        virtual size_t read(uint8_t *buf, const size_t len)        = 0;
        virtual size_t readBytes(uint8_t *buf, const size_t len)   = 0;
        virtual void flush()                                       = 0;
        virtual void flush(const bool txOnly)                      = 0;
        virtual size_t write(const uint8_t *buf, const size_t len) = 0;
        virtual size_t write(uint8_t b)                            = 0;
        virtual uint32_t baudRate() const                          = 0;
    };
    ///@endcond

    /*!
      @brief Constructor
     */
    UnitSP485();
    //! @brief Destructor
    virtual ~UnitSP485() = default;

    /*!
      @brief Begin using the registered UART adapter
      @return True if the adapter is valid and ready
     */
    virtual bool begin() override;

    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        bool flushRX{true};  //!< Flush RX buffer on begin?
    };

    ///@name Settings for begin
    ///@{
    /*!
      @brief Gets the configuration
      @return Current configuration
     */
    inline config_t config()
    {
        return _cfg;
    }
    /*!
      @brief Sets the configuration
      @param cfg Configuration settings
     */
    inline void config(const config_t &cfg)
    {
        _cfg = cfg;
    }
    ///@}

    ///@note See also Arduino documents
    ///@name Arduino Serial Compatibility API
    ///@{
    //! @brief Returns true if the unit is registered
    inline operator bool() const
    {
        return isRegistered();
    }
    /*!
      @brief Returns the number of available bytes to read
      @return Number of bytes available
     */
    inline int available()
    {
        return _serial->available();
    }
    /*!
      @brief Returns the number of bytes that can be written without blocking
      @return Number of bytes available for write
     */
    inline int availableForWrite()
    {
        return _serial->availableForWrite();
    }
    /*!
      @brief Peeks the next incoming byte without removing it
      @return Next byte or -1 if none
     */
    inline int peek()
    {
        return _serial->peek();
    }
    /*!
      @brief Reads one byte
      @return Byte read or -1 if none
     */
    inline int read()
    {
        return _serial->read();
    }
    /*!
      @brief Reads up to size bytes into buffer
      @param buffer Destination buffer
      @param size Maximum number of bytes to read
      @return Number of bytes read
     */
    inline size_t read(uint8_t *buffer, const size_t size)
    {
        return _serial->read(buffer, size);
    }
    /*!
      @brief Reads up to size bytes into buffer
      @param buffer Destination buffer
      @param size Maximum number of bytes to read
      @return Number of bytes read
     */
    inline size_t read(char *buffer, const size_t size)
    {
        return read(reinterpret_cast<uint8_t *>(buffer), size);
    }
    /*!
      @brief Reads bytes with timeout defined by underlying Serial
      @param buffer Destination buffer
      @param length Number of bytes to read
      @return Number of bytes actually read
     */
    size_t readBytes(uint8_t *buffer, const size_t length)
    {
        return _serial->readBytes(buffer, length);
    }
    /*!
      @brief Reads bytes with timeout defined by underlying Serial
      @param buffer Destination buffer
      @param length Number of bytes to read
      @return Number of bytes actually read
     */
    inline size_t readBytes(char *buffer, const size_t length)
    {
        return readBytes(reinterpret_cast<uint8_t *>(buffer), length);
    }
    /*!
      @brief Flushes the serial port
     */
    inline void flush()
    {
        _serial->flush();
    }
    /*!
      @brief Flushes the serial port
      @param txOnly If true, flush only TX
     */
    inline void flush(const bool txOnly)
    {
        _serial->flush(txOnly);
    }
    /*!
      @brief Writes one byte
      @param d Byte to write
      @param flush If true, flush TX before returning
      @return Number of bytes written (0 if receive buffer is not empty due to half-duplex constraint)
      @note Returns 0 without writing when available() > 0, as RS485 half-duplex cannot transmit while data is pending
     */
    size_t write(const uint8_t d, const bool flush = true);
    /*!
      @brief Writes bytes from buffer
      @param buffer Source buffer
      @param size Number of bytes to write
      @param flush If true, flush TX before returning
      @return Number of bytes written
     */
    size_t write(const uint8_t *buffer, const size_t size, const bool flush = true);
    /*!
      @brief Writes bytes from buffer
      @param buffer Source buffer
      @param size Number of bytes to write
      @param flush If true, flush TX before returning
      @return Number of bytes written
     */
    size_t write(const char *buffer, const size_t size, const bool flush = true);
    /*!
      @brief Writes a null-terminated string
      @param s Null-terminated string
      @param flush If true, flush TX before returning
      @return Number of bytes written
     */
    size_t write(const char *s, const bool flush = true);
    ///@}

    ///@name Read/Write any value
    ///@{
    /*!
      @brief Read the any number value
      @tparam T Type (must be trivially copyable and standard-layout)
      @tparam LittleEndian Incoming data is little-endian if true
      @param value Receive variable
      @return True if successful
      @note LittleEndian byteswap is applied for integral / enum types and for 32-bit / 64-bit
      floating-point types (via bit-cast). Aggregate / struct types are stored as-is in host
      byte order; the LittleEndian parameter is ignored for those.
      @note LittleEndian only declares the wire byte order; the peer must agree on the same
      convention. Floating-point special values (NaN, +/-inf, +/-0) are byte-reversed as raw
      bit patterns without special-casing.
     */
    template <typename T, bool LittleEndian = true>
    bool readValue(T &value)
    {
        static_assert(std::is_trivially_copyable<T>::value && std::is_standard_layout<T>::value,
                      "readValue requires trivially copyable, standard-layout types only");
        value = T{};
        uint8_t buf[sizeof(T)]{};
        size_t got = _serial->readBytes(buf, sizeof(T));
        if (got != sizeof(T)) {
            return false;
        }
        std::memcpy(&value, buf, sizeof(T));
        if (LittleEndian != m5::endian::little) {
            value = byteswap_if_needed(value);
        }
        return true;
    }
    /*!
      @brief Write the any number value
      @tparam T Type (must be trivially copyable and standard-layout)
      @tparam LittleEndian Outgoing data is little-endian if true
      @param value Value to write
      @return True if successful
      @note LittleEndian byteswap is applied for integral / enum types and for 32-bit / 64-bit
      floating-point types (via bit-cast). Aggregate / struct types are sent as-is in host
      byte order; the LittleEndian parameter is ignored for those.
      @note LittleEndian only declares the wire byte order; the peer must agree on the same
      convention. Floating-point special values (NaN, +/-inf, +/-0) are byte-reversed as raw
      bit patterns without special-casing.
     */
    template <typename T, bool LittleEndian = true>
    bool writeValue(const T value)
    {
        static_assert(std::is_trivially_copyable<T>::value && std::is_standard_layout<T>::value,
                      "writeValue requires trivially copyable, standard-layout types only");
        T tmp = value;
        if (LittleEndian != m5::endian::little) {
            tmp = byteswap_if_needed(tmp);
        }
        uint8_t buf[sizeof(T)]{};
        std::memcpy(buf, &tmp, sizeof(T));
        return write(buf, sizeof(T), true) == sizeof(T);
    }
    ///@}

protected:
    inline ISerial *iserial()
    {
        return _serial.get();
    }

    // 3-way byteswap dispatch tags: integral/enum, floating-point, other (aggregate).
    struct byteswap_int_tag {};
    struct byteswap_float_tag {};
    struct byteswap_none_tag {};

    template <typename T>
    struct byteswap_tag_of {
        using type = typename std::conditional<
            std::is_integral<T>::value || std::is_enum<T>::value, byteswap_int_tag,
            typename std::conditional<std::is_floating_point<T>::value, byteswap_float_tag,
                                      byteswap_none_tag>::type>::type;
    };

    template <typename T>
    static T byteswap_if_needed(T v)
    {
        return byteswap_if_needed_impl(v, typename byteswap_tag_of<T>::type{});
    }

    // Integral / enum: delegated to m5::stl::byteswap.
    template <typename T>
    static T byteswap_if_needed_impl(T v, byteswap_int_tag)
    {
        return m5::stl::byteswap(v);
    }

    // Floating-point: bit-cast to same-size unsigned integer, byteswap, bit-cast back.
    template <typename T>
    static T byteswap_if_needed_impl(T v, byteswap_float_tag)
    {
        static_assert(sizeof(T) == 4 || sizeof(T) == 8,
                      "byteswap_if_needed: only 32-bit / 64-bit floating-point supported");
        typename std::conditional<sizeof(T) == 4, uint32_t, uint64_t>::type u{};
        std::memcpy(&u, &v, sizeof(T));
        u = m5::stl::byteswap(u);
        T out{};
        std::memcpy(&out, &u, sizeof(T));
        return out;
    }

    // Aggregate / struct: byte-reverse of the whole object is not meaningful; return as-is.
    template <typename T>
    static T byteswap_if_needed_impl(T v, byteswap_none_tag)
    {
        return v;
    }

private:
    std::unique_ptr<ISerial> _serial{};
    config_t _cfg{};
};

}  // namespace unit
}  // namespace m5
#endif
