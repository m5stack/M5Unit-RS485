/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file read_path_test.cpp
  @brief Tests for the receive side of RS485Component

  Receiving needs no peer: ISerial is the seam. A fake implementation feeds a byte queue to
  read() / readBytes() / readValue() and records what write() hands over, so the whole
  Arduino-compatible API can be exercised without hardware, an adapter or begin().
 */
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <unit/unit_SP485.hpp>

namespace {

// ISerial backed by two byte queues: rx is what the peer sent, tx is what the unit wrote.
struct FakeSerial : public m5::unit::RS485Component::ISerial {
    std::vector<uint8_t> rx{}, tx{};
    size_t pos{};
    uint32_t last_timeout_ms{};
    uint32_t baud{19200};
    size_t flush_count{}, flush_tx_only_count{};

    void feed(const uint8_t* data, const size_t len)
    {
        rx.assign(data, data + len);
        pos = 0;
    }
    size_t remaining() const
    {
        return rx.size() - pos;
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
        return (pos < rx.size()) ? static_cast<int>(rx[pos]) : -1;
    }
    int read() override
    {
        return (pos < rx.size()) ? static_cast<int>(rx[pos++]) : -1;
    }
    size_t read(uint8_t* b, const size_t n) override
    {
        return take(b, n);
    }
    size_t readBytes(uint8_t* b, const size_t n) override
    {
        return take(b, n);
    }
    size_t readBytes(uint8_t* b, const size_t n, const uint32_t timeout_ms) override
    {
        last_timeout_ms = timeout_ms;
        return take(b, n);
    }
    void flush() override
    {
        ++flush_count;
    }
    void flush(const bool txOnly) override
    {
        ++flush_count;
        if (txOnly) {
            ++flush_tx_only_count;
        }
    }
    size_t write(const uint8_t* b, const size_t n) override
    {
        tx.insert(tx.end(), b, b + n);
        return n;
    }
    size_t write(uint8_t b) override
    {
        tx.push_back(b);
        return 1;
    }
    uint32_t baudRate() const override
    {
        return baud;
    }

protected:
    size_t take(uint8_t* b, const size_t n)
    {
        size_t got{};
        while (got < n && pos < rx.size()) {
            b[got++] = rx[pos++];
        }
        return got;
    }
};

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
    uint32_t serialBaudRate()
    {
        return iserial()->baudRate();
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

class TestReadPath : public ::testing::Test {
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

TEST_F(TestReadPath, PeekDoesNotConsume)
{
    fake->feed(WIRE, sizeof(WIRE));

    EXPECT_EQ(unit.available(), static_cast<int>(sizeof(WIRE)));
    EXPECT_EQ(unit.peek(), 0x12);
    EXPECT_EQ(unit.peek(), 0x12);
    EXPECT_EQ(unit.available(), static_cast<int>(sizeof(WIRE)));
    EXPECT_EQ(unit.read(), 0x12);
    EXPECT_EQ(unit.available(), static_cast<int>(sizeof(WIRE)) - 1);
}

TEST_F(TestReadPath, ReadReturnsMinusOneWhenEmpty)
{
    EXPECT_EQ(unit.available(), 0);
    EXPECT_EQ(unit.peek(), -1);
    EXPECT_EQ(unit.read(), -1);
}

TEST_F(TestReadPath, ReadIntoBuffer)
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

TEST_F(TestReadPath, ReadIntoCharBuffer)
{
    fake->feed(WIRE, sizeof(WIRE));

    char buf[4]{};
    EXPECT_EQ(unit.read(buf, sizeof(buf)), sizeof(buf));
    EXPECT_EQ(static_cast<uint8_t>(buf[3]), 0x78);
}

TEST_F(TestReadPath, ReadBytesOverloads)
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
    EXPECT_EQ(fake->last_timeout_ms, 250u);

    fake->feed(WIRE, sizeof(WIRE));
    EXPECT_EQ(unit.readBytes(cbuf, sizeof(cbuf), 1), sizeof(cbuf));
    EXPECT_EQ(fake->last_timeout_ms, 1u);
}

TEST_F(TestReadPath, ReadValueLittleEndian)
{
    const uint8_t wire[] = {0x34, 0x12};
    fake->feed(wire, sizeof(wire));

    uint16_t v{};
    EXPECT_TRUE((unit.readValue<uint16_t, true>(v)));
    EXPECT_EQ(v, 0x1234);
}

TEST_F(TestReadPath, ReadValueBigEndian)
{
    const uint8_t wire[] = {0x12, 0x34};
    fake->feed(wire, sizeof(wire));

    uint16_t v{};
    EXPECT_TRUE((unit.readValue<uint16_t, false>(v)));
    EXPECT_EQ(v, 0x1234);
}

TEST_F(TestReadPath, ReadValueFloatBigEndian)
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

TEST_F(TestReadPath, ReadValueFailsOnShortReadAndClearsValue)
{
    const uint8_t wire[] = {0xAA};
    fake->feed(wire, sizeof(wire));

    uint16_t v{0x5A5A};
    EXPECT_FALSE(unit.readValue<uint16_t>(v));
    // The value is cleared before the read is attempted, so a failure never leaves stale data
    EXPECT_EQ(v, 0u);
}

TEST_F(TestReadPath, FlushIsForwarded)
{
    unit.flush();
    EXPECT_EQ(fake->flush_count, 1u);
    EXPECT_EQ(fake->flush_tx_only_count, 0u);

    unit.flush(true);
    EXPECT_EQ(fake->flush_count, 2u);
    EXPECT_EQ(fake->flush_tx_only_count, 1u);
}

TEST_F(TestReadPath, AvailableForWriteAndBaudRateAreForwarded)
{
    EXPECT_EQ(unit.availableForWrite(), 64);
    EXPECT_EQ(unit.serialBaudRate(), 19200u);
}

TEST_F(TestReadPath, WriteQueuesWithoutWaiting)
{
    EXPECT_EQ(unit.write(WIRE, sizeof(WIRE)), sizeof(WIRE));
    EXPECT_EQ(fake->tx.size(), sizeof(WIRE));
    // write() only queues; nothing waits for the transmission to complete
    EXPECT_EQ(fake->flush_count, 0u);

    EXPECT_EQ(unit.write(static_cast<uint8_t>(0x5A)), 1u);
    EXPECT_EQ(fake->tx.back(), 0x5A);
    EXPECT_EQ(fake->flush_count, 0u);

    EXPECT_EQ(unit.write("hi"), 2u);
    EXPECT_EQ(fake->tx.back(), 'i');
    EXPECT_EQ(fake->flush_count, 0u);
}

TEST_F(TestReadPath, WriteValueWaitsForTheTransmission)
{
    EXPECT_TRUE((unit.writeValue<uint16_t, true>(0x1234)));
    EXPECT_EQ(fake->tx.size(), sizeof(uint16_t));
    EXPECT_EQ(fake->tx[0], 0x34);
    // Unlike write(), writeValue() drains TX before reporting success
    EXPECT_EQ(fake->flush_tx_only_count, 1u);
}

// Every Serial-compatible method dereferences the backend unconditionally, so a component must
// never be left without one: the fallback is shared, and installing nothing keeps it in place.
TEST(ReadPathSerialSeam, FallbackIsSharedAndSurvivesAnEmptyInstall)
{
    SerialAccess one{}, another{};
    auto* fallback = one.current();
    EXPECT_NE(fallback, nullptr);
    EXPECT_EQ(another.current(), fallback);

    one.installNothing();
    EXPECT_EQ(one.current(), fallback);
    EXPECT_EQ(one.available(), 0);
}
