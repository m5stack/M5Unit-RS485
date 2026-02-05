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
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <m5_utility/stl/byteswap.hpp>
#include <string>

// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_RS485) && !defined(USING_HAT_RS485) && !defined(USING_ATOMIC_RS485_BASE)
// For UnitRS485
// #define USING_UNIT_RS485
// For HatRS485
// #define USING_HAT_RS485
// For AtomRS485
// #define USING_ATOMIC_RS485_BASE
#endif

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
#if defined(USING_UNIT_RS485)
#pragma message "Using UnitRS485"
m5::unit::UnitRS485 unit;
#elif defined(USING_HAT_RS485)
#pragma message "Using HatRS485"
m5::unit::HatRS485 unit;
#elif defined(USING_ATOMIC_RS485_BASE)
#pragma message "Using AtomicRS485Base"
m5::unit::AtomicRS485Base unit;
#else
#error Please choose unit!
#endif
m5::unit::UnitSP485Stream stream{unit};  // Class inherited from Arduino Stream

/*
 **** Caution ****
 Reflections are more likely when the RS485 cable is long and/or the baud rate is relatively high.
 Add 120Ω termination at both ends of the RS485 bus to suppress reflections.
 These products (SKU:034 / SKU:067 / SLU:A131) do not include built-in termination resistors, so add external
 termination when needed.
 */
constexpr uint32_t UART_BAUD{51200};

//
constexpr char message_0[] =
    "Unit RS485 is a TTL-RS485 converter, used to convert TTL level signals to RS485 level signals.\n"
    "RS485 is a communication protocol standard that defines the electrical characteristics of drivers and receivers "
    "for serial communication systems.\n"
    " It supports multipoint systems and is used in industrial applications.\n"
    " When your project devices require communication control over RS485, using Unit RS485 as an interface converter "
    "is a good choice.\n";
constexpr char message_1[] =
    "Hat RS485 is an RS485 converter compatible with M5SticKC.\n"
    "It integrates SP485EEN internally, mainly consisting of a 485 automatic transceiver circuit and a DC-DC buck "
    "circuit (which can step down the input voltage to 5V)n"
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

void write_float_bytes(const float v, const bool little_endian)
{
    uint8_t tmp[sizeof(float)]{};
    std::memcpy(tmp, &v, sizeof(tmp));
    if (!little_endian) {
        for (size_t i = 0; i < sizeof(tmp) / 2; ++i) {
            std::swap(tmp[i], tmp[sizeof(tmp) - 1 - i]);
        }
    }
    unit.write(tmp, sizeof(tmp));
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

bool is_ascii_ok(const uint8_t c)
{
    return (c == '\r' || c == '\n' || c == '\t' || (c >= 0x20 && c <= 0x7E));
}

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
            if (is_ascii_ok(c) && c >= 0x20 && c <= 0x7E) {
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

#if defined(USING_HAT_RS485) || defined(USING_ATOMIC_RS485_BASE)
struct UartPins {
    int rx;
    int tx;
};

#if defined(USING_HAT_RS485)
UartPins get_hat_uart_pins(const m5::board_t board)
{
    switch (board) {
        case m5::board_t::board_M5StickS3:
            return {0, 8};
        case m5::board_t::board_M5StackCoreInk:
            return {26, 25};
        case m5::board_t::board_M5StickC:
        case m5::board_t::board_M5StickCPlus:
        case m5::board_t::board_M5StickCPlus2:
            return {26, 0};
        case m5::board_t::board_ArduinoNessoN1:
            return {7, 6};
        default:
            return {-1, -1};
    }
}
#endif

#if defined(USING_ATOMIC_RS485_BASE)
UartPins get_atombase_uart_pins(const m5::board_t board)
{
    switch (board) {
        case m5::board_t::board_M5AtomLite:
        case m5::board_t::board_M5AtomMatrix:
            return {22, 19};
        case m5::board_t::board_M5AtomS3:
        case m5::board_t::board_M5AtomS3Lite:
        case m5::board_t::board_M5AtomS3R:
        case m5::board_t::board_M5AtomEchoS3R:
        case m5::board_t::board_M5AtomS3RCam:
        case m5::board_t::board_M5AtomS3RExt:
            return {5, 6};
        default:
            return {-1, -1};
    }
}
#endif
#endif
}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

#if defined(USING_UNIT_RS485)
    auto pin_num_in  = M5.getPin(m5::pin_name_t::port_c_rxd);
    auto pin_num_out = M5.getPin(m5::pin_name_t::port_c_txd);
    if (pin_num_in < 0 || pin_num_out < 0) {
        M5_LOGW("PortC is not available");
        Wire.end();
        pin_num_in  = M5.getPin(m5::pin_name_t::port_a_pin1);
        pin_num_out = M5.getPin(m5::pin_name_t::port_a_pin2);
    }
#elif defined(USING_HAT_RS485)
    // HAT interface
    const auto pins  = get_hat_uart_pins(M5.getBoard());
    auto pin_num_in  = pins.rx;
    auto pin_num_out = pins.tx;
#elif defined(USING_ATOMIC_RS485_BASE)
    // AtomBase interface
    const auto pins  = get_atombase_uart_pins(M5.getBoard());
    auto pin_num_in  = pins.rx;
    auto pin_num_out = pins.tx;
#endif

    M5_LOGI("Pin Rx:%d,Tx:%d", pin_num_in, pin_num_out);
    if (pin_num_in < 0 || pin_num_out < 0) {
        M5_LOGE("Illegal Pin number");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    // clang-format off
#if defined(CONFIG_IDF_TARGET_ESP32C6)
    auto& s = Serial1;
#elif SOC_UART_NUM > 2
    auto& s = Serial2;
#elif SOC_UART_NUM > 1
    auto& s = Serial1;
#else
#error "Not enough Serial"
#endif
    // clang-format on

    s.begin(UART_BAUD, SERIAL_8N1, pin_num_in, pin_num_out);
    {
        auto cfg    = unit.config();
        cfg.flushRX = true;
        unit.config(cfg);
    }
    if (!Units.add(unit, s) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());
    lcd.fillScreen(TFT_DARKGREEN);

    log_view.configure();
    log_view.render();

    M5.Log.printf("READY\n");
    lcd.drawString("READY", 0, 0);
}

void loop()
{
    M5.update();
    Units.update();

    // Send
    if (M5.BtnA.wasClicked()) {
        static uint32_t step{};
        M5.Log.printf("Send:>>>>\n");
        if (step < m5::stl::size(message_table)) {
            const char* msg = message_table[step];
            M5.Log.printf("%s\n", msg);
            stream.print(msg);
        } else if (step == m5::stl::size(message_table)) {
            Sample0 s0{};
            M5.Log.printf("Sample0 (LE): a=0x%04X b=%f\n", s0.a, s0.b);
            unit.writeValue<uint16_t, true>(s0.a);
            write_float_bytes(s0.b, true);
        } else {
            Sample0 s0{};
            M5.Log.printf("Sample0 (BE): a=0x%04X b=%f\n", s0.a, s0.b);
            unit.writeValue<uint16_t, false>(s0.a);
            write_float_bytes(s0.b, false);
        }
        M5.Log.printf("<<<<\n");
        step = (step + 1) % (m5::stl::size(message_table) + 2);
    }

    // Receive
    if (stream.available()) {
        receive_bytes.clear();
        receive_bytes.reserve(static_cast<size_t>(stream.available()));
        while (stream.available()) {
            receive_bytes.push_back(static_cast<uint8_t>(stream.read()));
        }

        receive_batch_total = receive_bytes.size();
        flush_received(log_view);
    }
    log_view.render();
}
