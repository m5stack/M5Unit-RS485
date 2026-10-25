/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Transceiver example using M5UnitUnified for UnitRS485
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedRS485.h>
#include <M5Utility.hpp>
#include <wiring/m5_unit_unified_wiring.hpp>        // core wiring: addUART / addHatUART
#include <wiring/m5_unit_unified_rs485_wiring.hpp>  // RS485-local wiring: addAtomicBaseUART
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_RS485) && !defined(USING_HAT_RS485) && !defined(USING_ATOMIC_RS485_BASE) && \
    !defined(USING_TAB5_BUILTIN_RS485)
// For UnitRS485 (U034)
// #define USING_UNIT_RS485
// For HatRS485 (U067)
// #define USING_HAT_RS485
// For Atomic RS485 Base (A131)
// #define USING_ATOMIC_RS485_BASE
// For Tab5 built-in RS-485 (SIT3088)
// #define USING_TAB5_BUILTIN_RS485
#endif

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
#if defined(USING_UNIT_RS485)
#pragma message("Using UnitRS485")
m5::unit::UnitRS485 unit;
#elif defined(USING_HAT_RS485)
#pragma message("Using HatRS485")
m5::unit::HatRS485 unit;
#elif defined(USING_ATOMIC_RS485_BASE)
#pragma message("Using AtomicRS485Base")
m5::unit::AtomicRS485Base unit;
#elif defined(USING_TAB5_BUILTIN_RS485)
#pragma message("Using Tab5BuiltinRS485")
m5::unit::Tab5BuiltinRS485 unit;
#else
#error Please choose unit!
#endif

/*
 **** Caution ****
 Reflections are more likely when the RS485 cable is long and/or the baud rate is relatively high.
 Add 120Ω termination at both ends of the RS485 bus to suppress reflections.
 These products (SKU:U034 / SKU:U067 / SKU:A131) do not include built-in termination resistors, so add external
 termination when needed.
 */
constexpr uint32_t UART_BAUD{57600};
// Receiver RX buffer, sized for this demo's ~640-byte message bursts (the default is 256 bytes on
// the native backend and the HardwareSerial default on Arduino).
constexpr size_t UART_RX_BUFFER_SIZE{2048};
constexpr auto UART_CONFIG{m5::unit::wiring::UartConfig::Default};

//
constexpr char message_0[] =
    "Unit RS485 is a TTL-RS485 converter, used to convert TTL level signals to RS485 level signals.\n"
    "RS485 is a communication protocol standard that defines the electrical characteristics of drivers and receivers "
    "for serial communication systems.\n"
    " It supports multipoint systems and is used in industrial applications.\n"
    " When your project devices require communication control over RS485, using Unit RS485 as an interface converter "
    "is a good choice.\n";
constexpr char message_1[] =
    "Hat RS485 is an RS485 converter compatible with M5StickC.\n"
    "It integrates SP485EEN internally, mainly consisting of a 485 automatic transceiver circuit and a DC-DC buck "
    "circuit (which can step down the input voltage to 5V).\n"
    "RS485 is a standard used to define the electrical characteristics of drivers and receivers for serial "
    "communication systems, widely used in industrial fields, supporting multi-point systems.\n"
    "This product is used to convert TTL standard to RS485 standard.\n"
    "If the external serial device is RS485 standard, communication between devices implementing TTL to RS485 protocol "
    "conversion can be achieved through this module.\n";
constexpr char message_2[] =
    "Atomic RS485 Base is a TTL-RS485 converter based on the Atomic design, used for converting TTL level to RS485 "
    "level.\n"
    "RS485 is a widely used electrical standard in the industrial field, featuring stronger anti-interference "
    "capability and longer transmission distance.\n"
    "It has a built-in DC-DC step-down regulator chip, which can directly convert the 12V voltage of RS485 to 5V to "
    "power the ATOM, eliminating the trouble of separate power supply.\n"
    "When multiple devices require communication control, using the Atomic RS485 for interface type conversion will be "
    "a good choice.\n";
constexpr const char* message_table[] = {
    message_0,
    message_1,
    message_2,
};
std::vector<uint8_t> receive_bytes{};
size_t receive_batch_total{};

struct Sample0 {
    uint16_t a{0xE1D2};
    float b{-12.75f};
};

size_t write_chunked(m5::unit::RS485Component& s, const uint8_t* data, const size_t len,
                     const uint32_t timeout_ms = 200)
{
    if (data == nullptr || len == 0) {
        return 0;
    }

    const auto start = m5::utility::millis();
    size_t total{};

    while (total < len && !m5::utility::hasElapsed(start, timeout_ms)) {
        const int available = s.availableForWrite();
        if (available <= 0) {
            m5::utility::delay(1);
            continue;
        }

        size_t to_write = static_cast<size_t>(available);
        if (to_write > (len - total)) {
            to_write = len - total;
        }

        const size_t wrote = s.write(data + total, to_write);
        if (wrote == 0) {
            m5::utility::delay(1);
            continue;
        }
        total += wrote;
    }
    // write() only queues the bytes; flush() is what waits for them to leave the wire. Show both
    // halves, because that split is what the two-argument write() means since 0.2.0.
    if (total > 0) {
        s.flush(true);  // true = TX only
    }
    return total;
}

// Counterpart of write_chunked(): reads a block with an explicit timeout instead of polling
// read() one byte at a time. readBytes() is also how a value larger than readValue()'s 256-byte
// limit has to be taken off the wire.
// The timeout is per chunk, not an overall budget: it ends the read when the peer stops sending,
// which is how RS-485 frames are delimited (Modbus RTU uses an idle gap of 3.5 character times).
size_t read_chunked(m5::unit::RS485Component& s, uint8_t* buffer, const size_t len, const uint32_t timeout_ms = 200)
{
    if (buffer == nullptr || len == 0) {
        return 0;
    }

    size_t total{};
    while (total < len) {
        const size_t got = s.readBytes(buffer + total, len - total, timeout_ms);
        if (got == 0) {
            break;  // nothing more arrived within the timeout
        }
        total += got;
    }
    return total;
}

//
struct LogView {
    explicit LogView(LGFX_Device& gfx) : _gfx{gfx}, _canvas{&gfx}
    {
    }

    void configure(const int16_t x = 0, const int16_t y = 0, const uint16_t w = 0, const uint16_t h = 0,
                   const uint16_t fg = TFT_WHITE, const uint16_t bg = TFT_BLACK)
    {
        _x = x;
        _y = y;
        _w = (w == 0) ? _gfx.width() : w;
        _h = (h == 0) ? _gfx.height() : h;
        if (_w == 0 || _h == 0) {
            _w = 1;
            _h = 1;
        }
        _fg = fg;
        _bg = bg;

        _canvas.setColorDepth(1);
        _canvas.setPsram(false);
        _canvas.createSprite(_w, _h);
        _canvas.setPaletteColor(0, _bg);
        _canvas.setPaletteColor(1, _fg);
        _canvas.setTextColor(1, 0);
        _canvas.setFont(_gfx.getFont());
        _canvas.setTextScroll(true);
        clear();
    }

    void clear()
    {
        _canvas.fillScreen(0);
        _dirty = true;
    }

    void append(const char* s)
    {
        if (!s) {
            return;
        }
        while (*s) {
            if (*s == '\n') {
                _canvas.print("\r\n");
            } else {
                _canvas.write(static_cast<uint8_t>(*s));
            }
            ++s;
        }
        _dirty = true;
    }

    void append(char c)
    {
        if (c == '\n') {
            _canvas.print("\r\n");
        } else {
            _canvas.write(static_cast<uint8_t>(c));
        }
        _dirty = true;
    }

    void appendLine(const char* s)
    {
        if (!s) {
            return;
        }
        _canvas.printf("%s\r\n", s);
        _dirty = true;
    }

    void render()
    {
        if (!_dirty) {
            return;
        }
        _canvas.pushSprite(&_gfx, _x, _y);
        _dirty = false;
    }

private:
    LGFX_Device& _gfx;
    M5Canvas _canvas;
    int16_t _x{};
    int16_t _y{};
    uint16_t _w{};
    uint16_t _h{};
    uint16_t _fg{TFT_WHITE};
    uint16_t _bg{TFT_BLACK};
    bool _dirty{true};
};

LogView log_view{lcd};

void flush_received(LogView& view)
{
    if (receive_bytes.empty()) {
        return;
    }

    const unsigned int total = static_cast<unsigned int>(receive_batch_total);

    std::vector<std::string> ascii_lines;
    ascii_lines.reserve((receive_bytes.size() + 15) / 16);

    for (size_t line_start = 0; line_start < receive_bytes.size(); line_start += 16) {
        const size_t line_end     = std::min(line_start + 16, receive_bytes.size());
        const unsigned int offset = static_cast<unsigned int>(line_start);

        std::string hex_line;
        hex_line.reserve((line_end - line_start) * 5);
        std::string lcd_hex;
        lcd_hex.reserve((line_end - line_start) * 3);
        std::string ascii_line;
        ascii_line.reserve(line_end - line_start);

        for (size_t i = line_start; i < line_end; ++i) {
            char buf[6]{};
            std::snprintf(buf, sizeof(buf), "0x%02X", receive_bytes[i]);
            hex_line.append(buf);
            hex_line.push_back(' ');

            std::snprintf(buf, sizeof(buf), "%02X", receive_bytes[i]);
            lcd_hex.append(buf);
            lcd_hex.push_back(' ');

            const uint8_t c = receive_bytes[i];
            if (c >= 0x20 && c <= 0x7E) {
                ascii_line.push_back(static_cast<char>(c));
            } else {
                ascii_line.push_back('.');
            }
        }

        M5.Log.printf("|%03u/%03u|%s\n", offset, total, hex_line.c_str());
        view.append(lcd_hex.c_str());
        view.append("\n");
        ascii_lines.push_back(ascii_line);
    }

    for (size_t line_start = 0, idx = 0; line_start < receive_bytes.size(); line_start += 16, ++idx) {
        const unsigned int offset = static_cast<unsigned int>(line_start);
        M5.Log.printf("|%03u/%03u|%s|\n", offset, total, ascii_lines[idx].c_str());
    }

    receive_bytes.clear();
}

}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    {
        auto cfg    = unit.config();
        cfg.flushRX = true;
        unit.config(cfg);
    }

    // The wiring helpers size the RX buffer themselves, before the port is opened.
#if defined(USING_HAT_RS485)
    const bool wired = m5::unit::wiring::addHatUART(Units, unit, UART_BAUD, UART_CONFIG, UART_RX_BUFFER_SIZE);
#elif defined(USING_ATOMIC_RS485_BASE)
    const bool wired =
        m5::unit::rs485::wiring::addAtomicBaseUART(Units, unit, UART_BAUD, UART_CONFIG, UART_RX_BUFFER_SIZE);
#elif defined(USING_TAB5_BUILTIN_RS485)
    const bool wired =
        m5::unit::rs485::wiring::addTab5BuiltinRS485UART(Units, unit, UART_BAUD, UART_CONFIG, UART_RX_BUFFER_SIZE);
#else  // USING_UNIT_RS485
    const bool wired = m5::unit::wiring::addUART(Units, unit, UART_BAUD, UART_CONFIG, UART_RX_BUFFER_SIZE);
#endif

    if (!wired || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();  // fills the screen red itself
    }

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());

    log_view.configure();
    log_view.render();

    M5.Log.printf("READY\n");
    log_view.appendLine("READY");
    log_view.render();
}

void loop()
{
    M5.update();
    Units.update();

    // Send
    if (M5.BtnA.wasClicked()) {
        static uint32_t step{};
        M5.Log.printf("Send:>>>>\n");
        size_t sent{};
        if (step < m5::stl::size(message_table)) {
            const char* msg = message_table[step];
            M5.Log.printf("%s\n", msg);
            const size_t len = std::strlen(msg);
            sent             = write_chunked(unit, reinterpret_cast<const uint8_t*>(msg), len);
            if (sent != len) {
                M5_LOGW("TX timeout: %u/%u bytes", static_cast<uint32_t>(sent), static_cast<uint32_t>(len));
            }
        } else if (step == m5::stl::size(message_table)) {
            Sample0 s0{};
            M5.Log.printf("Sample0 (LE): a=0x%04X b=%f\n", s0.a, s0.b);
            sent = unit.writeValue<uint16_t, true>(s0.a) ? sizeof(s0.a) : 0;
            sent += unit.writeValue<float, true>(s0.b) ? sizeof(s0.b) : 0;
        } else {
            Sample0 s0{};
            M5.Log.printf("Sample0 (BE): a=0x%04X b=%f\n", s0.a, s0.b);
            sent = unit.writeValue<uint16_t, false>(s0.a) ? sizeof(s0.a) : 0;
            sent += unit.writeValue<float, false>(s0.b) ? sizeof(s0.b) : 0;
        }
        M5.Log.printf("<<<< %zu sent\n", sent);
        step = (step + 1) % (m5::stl::size(message_table) + 2);
    }

    // Receive
    if (unit.available()) {
        receive_bytes.resize(static_cast<size_t>(unit.available()));
        receive_bytes.resize(read_chunked(unit, receive_bytes.data(), receive_bytes.size()));

        receive_batch_total = receive_bytes.size();
        flush_received(log_view);
    }
    log_view.render();
}

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#if CONFIG_FREERTOS_UNICORE
// Single-core SoCs run loop() on the only CPU, so the IDLE task never gets scheduled while loop()
// busy-waits. Yielding every iteration would break the RS-485 turnaround timing, so hand the CPU
// over briefly at a long interval instead: just often enough for IDLE to feed the task watchdog.
static void feed_idle_task_periodically()
{
    constexpr uint32_t FEED_INTERVAL_MS{2000};
    constexpr TickType_t FEED_SLEEP_TICKS{pdMS_TO_TICKS(5)};
    static m5::utility::elapsed_time_t last_feed{};
    if (m5::utility::hasElapsed(last_feed, FEED_INTERVAL_MS)) {
        last_feed = m5::utility::millis();
        vTaskDelay(FEED_SLEEP_TICKS);
    }
}
#endif

extern "C" void app_main(void)
{
    setup();
    for (;;) {
#if CONFIG_FREERTOS_UNICORE
        feed_idle_task_periodically();
#endif
        loop();
    }
}
#endif
