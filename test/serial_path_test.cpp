/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file serial_path_test.cpp
  @brief Tests for the Serial-compatible path of RS485Component

  None of it needs a peer: ISerial is the seam. A fake implementation feeds a byte queue to
  read() / readBytes() / readValue() and records what write() hands over, so the whole
  Arduino-compatible API can be exercised without hardware, an adapter or begin(). The same fake
  also pins how the backend is owned, handed over by a move, and released.
 */
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>
#include <unit/unit_SP485.hpp>

namespace {

// ISerial backed by two byte queues: _rx is what the peer sent, _tx is what the unit wrote.
struct FakeSerial : public m5::unit::RS485Component::ISerial {
    static int _destructed;  // how many instances have been destroyed so far
    std::vector<uint8_t> _rx{}, _tx{};
    size_t _pos{};
    m5::unit::RS485Component* _owner{};  // last component this backend was rebound to
    uint32_t _last_timeout_ms{};
    uint32_t _baud{19200};
    size_t _flush_count{}, _flush_tx_only_count{};
    // Every read funnels into take(), so without these the four entry points are
    // indistinguishable and nothing pins which one readValue() goes through.
    size_t _read_count{}, _read_buf_count{}, _read_bytes_count{}, _read_bytes_timeout_count{};

    void feed(const uint8_t* data, const size_t len)
    {
        _rx.assign(data, data + len);
        _pos = 0;
    }
    size_t remaining() const
    {
        return _rx.size() - _pos;
    }

    int available() override
    {
        return static_cast<int>(remaining());
    }
    int availableForWrite() override
    {
        return 64;
    }
    int peek() override
    {
        return (_pos < _rx.size()) ? static_cast<int>(_rx[_pos]) : -1;
    }
    int read() override
    {
        ++_read_count;
        return (_pos < _rx.size()) ? static_cast<int>(_rx[_pos++]) : -1;
    }
    size_t read(uint8_t* b, const size_t n) override
    {
        ++_read_buf_count;
        return take(b, n);
    }
    size_t readBytes(uint8_t* b, const size_t n) override
    {
        ++_read_bytes_count;
        return take(b, n);
    }
    size_t readBytes(uint8_t* b, const size_t n, const uint32_t timeout_ms) override
    {
        ++_read_bytes_timeout_count;
        _last_timeout_ms = timeout_ms;
        return take(b, n);
    }
    void flush() override
    {
        ++_flush_count;
    }
    void flush(const bool txOnly) override
    {
        ++_flush_count;
        if (txOnly) {
            ++_flush_tx_only_count;
        }
    }
    size_t write(const uint8_t* b, const size_t n) override
    {
        _tx.insert(_tx.end(), b, b + n);
        return n;
    }
    size_t write(const uint8_t b) override
    {
        _tx.push_back(b);
        return 1;
    }
    uint32_t baudRate() const override
    {
        return _baud;
    }
    ~FakeSerial() override
    {
        ++_destructed;
    }
    void rebind(m5::unit::RS485Component* new_owner) noexcept override
    {
        _owner = new_owner;
    }

protected:
    size_t take(uint8_t* b, const size_t n)
    {
        size_t got{};
        while (got < n && _pos < _rx.size()) {
            b[got++] = _rx[_pos++];
        }
        return got;
    }
};

int FakeSerial::_destructed{};

// Exposes the protected ISerial seam, like ByteswapAccess in endian_byteswap_test.cpp does for
// the byteswap dispatch.
class SerialAccess : public m5::unit::UnitSP485 {
public:
    FakeSerial* install()
    {
        FakeSerial* fake = new FakeSerial;
        install_serial(std::unique_ptr<ISerial>(fake));
        return fake;
    }
    ISerial* current()
    {
        return iserial();
    }
    void installNothing()
    {
        install_serial(std::unique_ptr<ISerial>());
    }
};

class TestSerialPath : public ::testing::Test {
protected:
    virtual void SetUp() override
    {
        fake = unit.install();
    }
    SerialAccess unit{};
    FakeSerial* fake{};
};

const uint8_t WIRE[] = {0x12, 0x34, 0x56, 0x78};

}  // namespace

TEST_F(TestSerialPath, PeekDoesNotConsume)
{
    fake->feed(WIRE, sizeof(WIRE));

    EXPECT_EQ(unit.available(), static_cast<int>(sizeof(WIRE)));
    EXPECT_EQ(unit.peek(), 0x12);
    EXPECT_EQ(unit.peek(), 0x12);
    EXPECT_EQ(unit.available(), static_cast<int>(sizeof(WIRE)));
    EXPECT_EQ(unit.read(), 0x12);
    EXPECT_EQ(unit.available(), static_cast<int>(sizeof(WIRE)) - 1);
}

TEST_F(TestSerialPath, ReadReturnsMinusOneWhenEmpty)
{
    EXPECT_EQ(unit.available(), 0);
    EXPECT_EQ(unit.peek(), -1);
    EXPECT_EQ(unit.read(), -1);
}

TEST_F(TestSerialPath, ReadIntoBuffer)
{
    fake->feed(WIRE, sizeof(WIRE));

    uint8_t buf[3]{};
    EXPECT_EQ(unit.read(buf, sizeof(buf)), sizeof(buf));
    EXPECT_EQ(buf[0], 0x12);
    EXPECT_EQ(buf[2], 0x56);
    // Short read: only one byte is left
    EXPECT_EQ(unit.read(buf, sizeof(buf)), 1u);
    EXPECT_EQ(buf[0], 0x78);
}

TEST_F(TestSerialPath, ReadIntoCharBuffer)
{
    fake->feed(WIRE, sizeof(WIRE));

    char buf[4]{};
    EXPECT_EQ(unit.read(buf, sizeof(buf)), sizeof(buf));
    EXPECT_EQ(static_cast<uint8_t>(buf[3]), 0x78);
}

TEST_F(TestSerialPath, ReadBytesOverloads)
{
    uint8_t buf[2]{};
    char cbuf[2]{};

    fake->feed(WIRE, sizeof(WIRE));
    EXPECT_EQ(unit.readBytes(buf, sizeof(buf)), sizeof(buf));
    EXPECT_EQ(buf[0], 0x12);

    fake->feed(WIRE, sizeof(WIRE));
    EXPECT_EQ(unit.readBytes(cbuf, sizeof(cbuf)), sizeof(cbuf));
    EXPECT_EQ(static_cast<uint8_t>(cbuf[1]), 0x34);

    // The three-argument forms hand the timeout to the backend instead of relying on its state
    fake->feed(WIRE, sizeof(WIRE));
    EXPECT_EQ(unit.readBytes(buf, sizeof(buf), 250), sizeof(buf));
    EXPECT_EQ(fake->_last_timeout_ms, 250u);

    fake->feed(WIRE, sizeof(WIRE));
    EXPECT_EQ(unit.readBytes(cbuf, sizeof(cbuf), 1), sizeof(cbuf));
    EXPECT_EQ(fake->_last_timeout_ms, 1u);

    // Two calls per form, and neither form may fall back to the other or to read()
    EXPECT_EQ(fake->_read_bytes_count, 2u);
    EXPECT_EQ(fake->_read_bytes_timeout_count, 2u);
    EXPECT_EQ(fake->_read_count, 0u);
}

TEST_F(TestSerialPath, ReadValueLittleEndian)
{
    const uint8_t wire[] = {0x34, 0x12};
    fake->feed(wire, sizeof(wire));

    uint16_t v{};
    EXPECT_TRUE((unit.readValue<uint16_t, true>(v)));
    EXPECT_EQ(v, 0x1234);

    // readValue() must wait for the bytes, which is what the two-argument readBytes() does. Reading
    // the value byte by byte, or with an explicit timeout, would return the same data here, so the
    // route is asserted rather than the result.
    EXPECT_EQ(fake->_read_bytes_count, 1u);
    EXPECT_EQ(fake->_read_count, 0u);
    EXPECT_EQ(fake->_read_buf_count, 0u);
    EXPECT_EQ(fake->_read_bytes_timeout_count, 0u);
}

TEST_F(TestSerialPath, ReadValueBigEndian)
{
    const uint8_t wire[] = {0x12, 0x34};
    fake->feed(wire, sizeof(wire));

    uint16_t v{};
    EXPECT_TRUE((unit.readValue<uint16_t, false>(v)));
    EXPECT_EQ(v, 0x1234);
}

TEST_F(TestSerialPath, ReadValueFloatBigEndian)
{
    float src{-12.75f};
    uint8_t wire[sizeof(float)]{};
    std::memcpy(wire, &src, sizeof(wire));
    for (size_t i = 0; i < sizeof(wire) / 2; ++i) {
        const uint8_t t            = wire[i];
        wire[i]                    = wire[sizeof(wire) - 1 - i];
        wire[sizeof(wire) - 1 - i] = t;
    }
    fake->feed(wire, sizeof(wire));

    float v{};
    EXPECT_TRUE((unit.readValue<float, false>(v)));
    EXPECT_FLOAT_EQ(v, src);
}

TEST_F(TestSerialPath, ReadValueFailsOnShortReadAndClearsValue)
{
    const uint8_t wire[] = {0xAA};
    fake->feed(wire, sizeof(wire));

    uint16_t v{0x5A5A};
    EXPECT_FALSE(unit.readValue<uint16_t>(v));
    // The value is cleared before the read is attempted, so a failure never leaves stale data
    EXPECT_EQ(v, 0u);
}

TEST_F(TestSerialPath, FlushIsForwarded)
{
    unit.flush();
    EXPECT_EQ(fake->_flush_count, 1u);
    EXPECT_EQ(fake->_flush_tx_only_count, 0u);

    unit.flush(true);
    EXPECT_EQ(fake->_flush_count, 2u);
    EXPECT_EQ(fake->_flush_tx_only_count, 1u);
}

TEST_F(TestSerialPath, AvailableForWriteAndBaudRateAreForwarded)
{
    EXPECT_EQ(unit.availableForWrite(), 64);
    EXPECT_EQ(unit.baudRate(), 19200u);
}

// The guards in RS485Component::write() -- a null buffer or a zero length -- were covered only by
// the embedded suites, where a failure costs a hardware run to find.
TEST_F(TestSerialPath, WriteRejectsNullAndEmptyBuffers)
{
    EXPECT_EQ(unit.write(WIRE, 0), 0u);
    EXPECT_EQ(unit.write(static_cast<const uint8_t*>(nullptr), 5), 0u);
    EXPECT_EQ(unit.write(static_cast<const char*>(nullptr), 5), 0u);
    EXPECT_EQ(unit.write(static_cast<const char*>(nullptr)), 0u);

    EXPECT_TRUE(fake->_tx.empty());
    EXPECT_EQ(fake->_flush_count, 0u);
}

TEST_F(TestSerialPath, WriteQueuesWithoutWaiting)
{
    EXPECT_EQ(unit.write(WIRE, sizeof(WIRE)), sizeof(WIRE));
    EXPECT_EQ(fake->_tx.size(), sizeof(WIRE));
    // write() only queues; nothing waits for the transmission to complete
    EXPECT_EQ(fake->_flush_count, 0u);

    EXPECT_EQ(unit.write(static_cast<uint8_t>(0x5A)), 1u);
    EXPECT_EQ(fake->_tx.back(), 0x5A);
    EXPECT_EQ(fake->_flush_count, 0u);

    EXPECT_EQ(unit.write("hi"), 2u);
    EXPECT_EQ(fake->_tx.back(), 'i');
    EXPECT_EQ(fake->_flush_count, 0u);
}

TEST_F(TestSerialPath, WriteValueWaitsForTheTransmission)
{
    EXPECT_TRUE((unit.writeValue<uint16_t, true>(0x1234)));
    EXPECT_EQ(fake->_tx.size(), sizeof(uint16_t));
    EXPECT_EQ(fake->_tx[0], 0x34);
    // Unlike write(), writeValue() drains TX before reporting success
    EXPECT_EQ(fake->_flush_tx_only_count, 1u);
}

// Every Serial-compatible method dereferences the backend unconditionally, so a component must
// never be left without one: the fallback is shared, and installing nothing keeps it in place.
// baudRate() answers through whatever backend is installed, so a unit that never reached a
// successful begin() reports 0 through the shared fallback. This is the contract the accessor's
// documentation states, and the only one observable without hardware.
TEST(SerialSeam, BaudRateIsZeroWithoutAnInstalledBackend)
{
    SerialAccess fresh{};
    EXPECT_EQ(fresh.baudRate(), 0u);

    SerialAccess moved_from{};
    moved_from.install();
    SerialAccess target{std::move(moved_from)};
    EXPECT_EQ(target.baudRate(), 19200u);
    EXPECT_EQ(moved_from.baudRate(), 0u);
}

TEST(SerialSeam, FallbackIsSharedAndSurvivesAnEmptyInstall)
{
    SerialAccess one{}, another{};
    auto* fallback = one.current();
    EXPECT_NE(fallback, nullptr);
    EXPECT_EQ(another.current(), fallback);

    one.installNothing();
    EXPECT_EQ(one.current(), fallback);
    EXPECT_EQ(one.available(), 0);
}

// "A null argument is ignored" has to hold over a real backend too, not just over the fallback.
TEST(SerialSeam, EmptyInstallKeepsAnOwnedBackend)
{
    SerialAccess unit{};
    FakeSerial* fake = unit.install();
    unit.installNothing();
    EXPECT_EQ(unit.current(), fake);
}

// Moving a component hands the backend over and tells it who owns it now, which is what lets
// UnitSIT3088 keep driving the DIR pin through the unit. The moved-from component falls back to
// the shared null backend instead of being left with none.
TEST(SerialSeam, MovingAComponentRebindsTheBackend)
{
    SerialAccess source{};
    FakeSerial* fake = source.install();
    EXPECT_EQ(fake->_owner, nullptr);

    SerialAccess untouched{};
    const auto* fallback = untouched.current();

    SerialAccess moved{std::move(source)};
    EXPECT_EQ(moved.current(), fake);
    EXPECT_EQ(fake->_owner, &moved);
    EXPECT_EQ(source.current(), fallback);

    SerialAccess target{};
    target = std::move(moved);
    EXPECT_EQ(target.current(), fake);
    EXPECT_EQ(fake->_owner, &target);
    EXPECT_EQ(moved.current(), fallback);
}

// The deleter carries the ownership flag with the pointer, so an owned backend has to be released
// exactly once however it leaves the component: replaced by a move, replaced by a new install, or
// carried to the end of the _owner's life. The shared fallback must never be released at all.
TEST(SerialSeam, OwnedBackendsAreFreedExactlyOnce)
{
    FakeSerial::_destructed = 0;
    {
        SerialAccess target{}, source{};
        target.install();  // the backend that move assignment will replace
        FakeSerial* handed_over = source.install();

        target = std::move(source);
        EXPECT_EQ(FakeSerial::_destructed, 1);
        EXPECT_EQ(target.current(), handed_over);

        target.install();  // installing over an owned backend releases it
        EXPECT_EQ(FakeSerial::_destructed, 2);
    }
    // Leaving the scope destroys the last owned backend; source holds the fallback, which is shared
    // and must survive.
    EXPECT_EQ(FakeSerial::_destructed, 3);
}

// Self-move assignment must leave the component alone: the guard in operator= is the only thing
// standing between it and a component that drops its own backend.
TEST(SerialSeam, SelfMoveAssignmentKeepsTheBackend)
{
    FakeSerial::_destructed = 0;
    SerialAccess unit{};
    FakeSerial* fake = unit.install();
    fake->_rx.push_back(0x5a);

    SerialAccess& alias = unit;  // through a reference, so that -Wself-move does not fire
    unit                = std::move(alias);

    EXPECT_EQ(unit.current(), fake);
    EXPECT_EQ(FakeSerial::_destructed, 0);
    EXPECT_EQ(unit.available(), 1);
    EXPECT_EQ(unit.read(), 0x5a);
}
