/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file rs485_component.hpp
  @brief Common base for RS-485 transceiver units in M5UnitUnified
  @details Non-instantiable intermediate that holds the Arduino Serial-compatible API,
           the ISerial abstraction, endian-aware readValue / writeValue templates and the
           begin() flushRX policy shared between all RS-485 transceiver units in this
           library (UnitSP485, UnitSIT3088, ...). Chip-specific behavior is injected by
           overriding make_serial().
  @note The public API is framework-agnostic (no Arduino types leak through this header).
        Framework-specific bits (HardwareSerial, digitalWrite, ...) live in the derived
        classes' translation units.
 */
#ifndef M5_UNIT_RS485_RS485_COMPONENT_HPP
#define M5_UNIT_RS485_RS485_COMPONENT_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/stl/byteswap.hpp>
#include <m5_utility/stl/endianness.hpp>
#include <cstring>
#include <memory>
#include <type_traits>

namespace m5 {
namespace unit {

class AdapterUART;  // forward decl: framework-agnostic UART adapter

/*!
  @class RS485Component
  @brief Non-instantiable base class for RS-485 transceiver units
 */
class RS485Component : public Component {
public:
    ///@cond
    struct ISerial {
        virtual ~ISerial()                                       = default;
        virtual int available()                                  = 0;
        virtual int availableForWrite()                          = 0;
        virtual int peek()                                       = 0;
        virtual int read()                                       = 0;
        virtual size_t read(uint8_t *buf, const size_t len)      = 0;
        virtual size_t readBytes(uint8_t *buf, const size_t len) = 0;
        /*
          Read with an explicit timeout instead of the backend's sticky timeout state. Required by
          the ESP-IDF backend, where uart_read_bytes() takes ticks_to_wait as an argument and there
          is no HardwareSerial object holding a timeout to inherit.
        */
        virtual size_t readBytes(uint8_t *buf, const size_t len, const uint32_t timeout_ms) = 0;
        virtual void flush()                                                                = 0;
        virtual void flush(const bool txOnly)                                               = 0;
        virtual size_t write(const uint8_t *buf, const size_t len)                          = 0;
        virtual size_t write(const uint8_t b)                                               = 0;
        virtual uint32_t baudRate() const                                                   = 0;
        // Called on the backend when its owning component has been moved. A backend that holds a
        // pointer back to its owner (see UnitSIT3088) overrides this to follow the move; one that
        // does not needs no implementation, which is why this is not pure. It runs inside the
        // component's noexcept move, hence noexcept: an override may only store the pointer, not
        // act on the owner, which is not fully constructed yet when a move constructor calls this.
        virtual void rebind(RS485Component *owner) noexcept
        {
            (void)owner;
        }
    };
    ///@endcond

    //! @brief Destructor
    virtual ~RS485Component() = default;

    ///@warning COPY PROHIBITED
    ///@name Constructor
    ///@{
    //! @brief Deleted: a component owns its serial backend, so it cannot be copied
    RS485Component(const RS485Component &) = delete;
    ///@}

    ///@warning COPY PROHIBITED
    ///@name Assignment
    ///@{
    //! @brief Deleted: a component owns its serial backend, so it cannot be copied
    RS485Component &operator=(const RS485Component &) = delete;
    ///@}

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
    inline config_t config() const
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
    ///@note write() only queues the bytes; flush() is what waits for the transmission to
    /// complete. A subclass that drives a direction pin (UnitSIT3088) has to drain inside its own
    /// ISerial before releasing the line, so its write() blocks even though this one does not.
    ///@name Arduino Serial Compatibility API
    ///@{
    //! @brief Returns true if the unit is registered
    inline operator bool() const
    {
        return isRegistered();
    }
    /*!
      @brief Returns the baud rate the serial port is running at
      @return Baud rate in bps, or 0 while no serial backend is installed, or if the driver
      cannot report it
      @note No backend is installed before the first successful begin(), and on a component
            that has been moved from. A begin() that fails leaves the backend already installed.
     */
    inline uint32_t baudRate() const
    {
        return _serial->baudRate();
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
      @brief Reads bytes with an explicit timeout
      @param buffer Destination buffer
      @param length Number of bytes to read
      @param timeout_ms Maximum time to wait for the requested bytes
      @return Number of bytes actually read
      @note Prefer this over the two-argument form: it does not depend on the backend's timeout
      state, which is only reachable through framework-specific API.
     */
    inline size_t readBytes(uint8_t *buffer, const size_t length, const uint32_t timeout_ms)
    {
        return _serial->readBytes(buffer, length, timeout_ms);
    }
    /*!
      @brief Reads bytes with an explicit timeout
      @param buffer Destination buffer
      @param length Number of bytes to read
      @param timeout_ms Maximum time to wait for the requested bytes
      @return Number of bytes actually read
     */
    inline size_t readBytes(char *buffer, const size_t length, const uint32_t timeout_ms)
    {
        return readBytes(reinterpret_cast<uint8_t *>(buffer), length, timeout_ms);
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
      @return Number of bytes written
      @note Queues the byte, as Arduino's Print::write() does. Call flush() when the caller has
      to know the byte has left the wire.
     */
    size_t write(const uint8_t d);
    /*!
      @brief Writes bytes from buffer
      @param buffer Source buffer
      @param size Number of bytes to write
      @return Number of bytes written
      @note Queues the bytes; call flush() to wait for the transmission to complete.
     */
    size_t write(const uint8_t *buffer, const size_t size);
    /*!
      @brief Writes bytes from buffer
      @param buffer Source buffer
      @param size Number of bytes to write
      @return Number of bytes written
      @note Queues the bytes; call flush() to wait for the transmission to complete.
     */
    size_t write(const char *buffer, const size_t size);
    /*!
      @brief Writes a null-terminated string
      @param s Null-terminated string
      @return Number of bytes written
      @note Queues the bytes; call flush() to wait for the transmission to complete.
     */
    size_t write(const char *s);

    /*!
      @brief Deleted: the second argument of write() is a byte count
      @details Rejects what cannot be a count -- char and signed char (write(buffer, 'a') would
      otherwise mean 97 bytes), bool, and any non-integral type, including the bool-convertible
      proxies of std::vector<bool> / std::bitset<N>, which used to slip through as a length. An
      integer length is filtered out by SFINAE and keeps resolving to write(const uint8_t *, size_t).
     */
    template <typename Size,
              typename std::enable_if<!std::is_integral<Size>::value || std::is_same<Size, char>::value ||
                                          std::is_same<Size, signed char>::value || std::is_same<Size, bool>::value,
                                      int>::type = 0>
    size_t write(const uint8_t *buffer, const Size size) = delete;
    /*!
      @brief Deleted: the second argument of write() is a byte count
      @details The same rule as write(const uint8_t *, const Size), except that bool is still
      accepted here: write(s, wait) is a released signature, so the deprecated overload below has to
      keep binding it. For a uint8_t buffer the wait form was always write(buffer, size, wait), so
      there bool is rejected.
     */
    // When the deprecated write(s, wait) overload is removed, add std::is_same<Size, bool> here too,
    // or write(s, true) would silently become a one-byte write. The static_assert on
    // deprecated_wait_form in test/write_overload_resolution_test.cpp fails on that day.
    template <typename Size,
              typename std::enable_if<!std::is_integral<Size>::value || std::is_same<Size, char>::value ||
                                          std::is_same<Size, signed char>::value,
                                      int>::type = 0>
    size_t write(const char *buffer, const Size size) = delete;
    ///@}

    ///@name Deprecated Arduino Serial Compatibility API
    ///@{
    /*!
      @brief Writes one byte, optionally waiting for the transmission to complete
      @param d Byte to write
      @param wait If true, wait for TX to drain before returning
      @return Number of bytes written
      @deprecated write() no longer waits; write the bytes and call flush() yourself.
     */
    template <typename Wait, typename std::enable_if<std::is_same<Wait, bool>::value, int>::type = 0>
    [[deprecated("write() no longer waits for TX; call flush() after writing")]] size_t write(const uint8_t d,
                                                                                              const Wait wait)
    {
        return wait ? write_waiting(&d, 1) : write(d);
    }
    /*!
      @brief Writes bytes from buffer, optionally waiting for the transmission to complete
      @param buffer Source buffer
      @param size Number of bytes to write
      @param wait If true, wait for TX to drain before returning
      @return Number of bytes written
      @deprecated write() no longer waits; write the bytes and call flush() yourself.
     */
    [[deprecated("write() no longer waits for TX; call flush() after writing")]] inline size_t write(
        const uint8_t *buffer, const size_t size, const bool wait)
    {
        return wait ? write_waiting(buffer, size) : write(buffer, size);
    }
    /*!
      @brief Writes bytes from buffer, optionally waiting for the transmission to complete
      @param buffer Source buffer
      @param size Number of bytes to write
      @param wait If true, wait for TX to drain before returning
      @return Number of bytes written
      @deprecated write() no longer waits; write the bytes and call flush() yourself.
     */
    [[deprecated("write() no longer waits for TX; call flush() after writing")]] inline size_t write(const char *buffer,
                                                                                                     const size_t size,
                                                                                                     const bool wait)
    {
        return wait ? write_waiting(reinterpret_cast<const uint8_t *>(buffer), size) : write(buffer, size);
    }
    /*!
      @brief Writes a null-terminated string, optionally waiting for the transmission to complete
      @param s Null-terminated string
      @param wait If true, wait for TX to drain before returning
      @return Number of bytes written
      @deprecated write() no longer waits; write the bytes and call flush() yourself.
      @note Only an exact bool binds here, so write(buffer, length) with a plain integer length
      is never mistaken for this overload.
     */
    template <typename Wait, typename std::enable_if<std::is_same<Wait, bool>::value, int>::type = 0>
    [[deprecated("write() no longer waits for TX; call flush() after writing")]] size_t write(const char *s,
                                                                                              const Wait wait)
    {
        if (!wait) {
            return write(s);
        }
        return write_waiting(reinterpret_cast<const uint8_t *>(s), s ? std::strlen(s) : 0);
    }
    ///@}

    ///@name Read/Write a trivially copyable value
    ///@{
    /*!
      @brief Read a trivially copyable value
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
      @note Waits for sizeof(T) bytes with the backend's own timeout, as the two-argument
      readBytes() does: the one set by HardwareSerial::setTimeout() on Arduino, and a fixed
      1000 ms on ESP-IDF native. Use readBytes() with an explicit timeout to control it.
      @note sizeof(T) must be 256 bytes or less: the value goes through a stack buffer, and 256
      bytes is the largest Modbus RTU frame as well as this library's default RX buffer size.
     */
    template <typename T, bool LittleEndian = true>
    bool readValue(T &value)
    {
        static_assert(std::is_trivially_copyable<T>::value && std::is_standard_layout<T>::value,
                      "readValue requires trivially copyable, standard-layout types only");
        static_assert(sizeof(T) <= 256, "readValue reads T through a stack buffer; sizeof(T) must be <= 256");
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
      @brief Write a trivially copyable value
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
      @note Unlike write(), this waits for the transmission to complete before returning, so a
      value is always fully on the wire when it reports success.
      @note sizeof(T) must be 256 bytes or less: the value goes through a stack buffer, and 256
      bytes is the largest Modbus RTU frame as well as this library's default RX buffer size.
     */
    template <typename T, bool LittleEndian = true>
    bool writeValue(const T value)
    {
        static_assert(std::is_trivially_copyable<T>::value && std::is_standard_layout<T>::value,
                      "writeValue requires trivially copyable, standard-layout types only");
        static_assert(sizeof(T) <= 256, "writeValue writes T through a stack buffer; sizeof(T) must be <= 256");
        T tmp = value;
        if (LittleEndian != m5::endian::little) {
            tmp = byteswap_if_needed(tmp);
        }
        uint8_t buf[sizeof(T)]{};
        std::memcpy(buf, &tmp, sizeof(T));
        return write_waiting(buf, sizeof(T)) == sizeof(T);
    }
    ///@}

protected:
    //! @brief Base constructor (concrete subclasses pass their DEFAULT_ADDRESS)
    explicit RS485Component(const uint8_t addr);

    /*!
      @brief Move constructor
      @param other Component to move from
      @details The serial backend is taken over and told about its new owner, so a backend that
      points back at the component it serves (UnitSIT3088 drives the DIR pin through the unit)
      follows the move. The moved-from component is left with the null-behavior backend, so its
      Serial-compatible methods keep answering.
      @note Protected on purpose: moving through a reference to this base would hand a backend
            built for one unit to a component of another type. The concrete units declare their
            own move members, so a unit is movable as a unit.
      @note Do not move a unit once UnitUnified::add() has registered it: UnitUnified keeps the
            address it was given, and the links to a parent or child component are copied rather
            than redirected.
      @note Recreate any UnitSP485Stream that wraps the unit: it holds a reference to the
            component it was built from, which after a move answers through the null backend.
     */
    RS485Component(RS485Component &&other) noexcept;

    /*!
      @brief Move assignment
      @param other Component to move from
      @return Reference to this component
      @details Hands over the serial backend as the move constructor does, and releases the
      backend this component held.
      @note The same three cautions as for the move constructor apply.
     */
    RS485Component &operator=(RS485Component &&other) noexcept;

    // Writes bytes and waits for the transmission to complete
    size_t write_waiting(const uint8_t *buffer, const size_t size);

    inline ISerial *iserial()
    {
        return _serial.get();
    }

    /*!
      @brief Installs a serial backend, taking over ownership and replacing the current one
      @param serial Backend to install, typically the result of make_serial()
      @note A null argument is ignored: every Serial-compatible method dereferences the backend
            unconditionally, so the component must never be left without one.
     */
    void install_serial(std::unique_ptr<ISerial> serial);

    /*!
      @brief Factory hook: build the concrete ISerial to be used by begin().
      @param ad AdapterUART obtained via asAdapter<AdapterUART>() (never null at call site)
      @return Owning pointer to the ISerial implementation, or a null-behavior implementation on
              frameworks not yet supported.
      @note Default implementation wraps the Arduino HardwareSerial exposed by the adapter.
            Chip-specific subclasses (e.g. UnitSIT3088) override to install a decorator that adds
            direction-pin control, etc.
     */
    virtual std::unique_ptr<ISerial> make_serial(AdapterUART *ad);

    // 2-way byteswap dispatch tags: types m5::stl::byteswap_value accepts, and other (aggregate).
    struct byteswap_value_tag {
    };
    struct byteswap_none_tag {
    };

    template <typename T>
    struct byteswap_tag_of {
        using type = typename std::conditional<std::is_integral<T>::value || std::is_enum<T>::value ||
                                                   std::is_floating_point<T>::value,
                                               byteswap_value_tag, byteswap_none_tag>::type;
    };

    template <typename T>
    static T byteswap_if_needed(T v)
    {
        return byteswap_if_needed_impl(v, typename byteswap_tag_of<T>::type{});
    }

    // Integral / enum / floating-point: delegated to m5::stl::byteswap_value.
    template <typename T>
    static T byteswap_if_needed_impl(T v, byteswap_value_tag)
    {
        return m5::stl::byteswap_value(v);
    }

    // Aggregate / struct: byte-reverse of the whole object is not meaningful; return as-is.
    // m5::stl::byteswap_value deletes these, so the dispatch must keep them out of it.
    template <typename T>
    static T byteswap_if_needed_impl(T v, byteswap_none_tag)
    {
        return v;
    }

    config_t _cfg{};

private:
    // The fallback backend lives in static storage and must outlive every component, while a
    // backend handed over by install_serial() is owned. One pointer carries both, so that no
    // Serial-compatible method has to test for null and moving a component needs no allocation.
    struct SerialDeleter {
        // No default member initializer: that would stop SerialDeleter being an aggregate in
        // C++11, and with it the SerialDeleter{true} / SerialDeleter{false} initializations.
        // Value-initializing the pointer (SerialPtr _serial{}) already zeroes this.
        bool owned;
        inline void operator()(ISerial *serial) const
        {
            if (owned) {
                delete serial;
            }
        }
    };
    using SerialPtr = std::unique_ptr<ISerial, SerialDeleter>;

    // Tells an owned backend which component it belongs to now.
    void rebind_serial() noexcept;

    SerialPtr _serial{};
};

}  // namespace unit
}  // namespace m5
#endif
