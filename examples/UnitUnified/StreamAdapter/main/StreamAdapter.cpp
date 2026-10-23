/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  StreamAdapter example using M5UnitUnified for UnitRS485

  Shows what UnitSP485Stream adds on top of the unit's own API: the Arduino Stream / Print
  interface. That makes two things possible which the raw unit cannot do:

    - formatted output (print / println / printf) and line-oriented input (readStringUntil)
    - handing the RS-485 link to any Arduino library that takes a Stream& (Modbus stacks, GPS
      parsers, AT-command drivers, ...)

  Arduino only: Stream does not exist in ESP-IDF native builds. Code meant to build on both
  frameworks should use the unit's API directly -- see the Transceiver example.
*/
#if !defined(ARDUINO)
#error This example is Arduino-only (UnitSP485Stream wraps Arduino's Stream). See Transceiver for a portable example.
#endif

#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedRS485.h>
#include <wiring/m5_unit_unified_wiring.hpp>        // core wiring: addUART / addHatUART
#include <wiring/m5_unit_unified_rs485_wiring.hpp>  // RS485-local wiring: addAtomicBaseUART

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

// The adapter is what this example is about: it turns the unit into an Arduino Stream.
m5::unit::UnitSP485Stream stream{unit};

constexpr uint32_t UART_BAUD{57600};
constexpr uint32_t SEND_INTERVAL_MS{2000};
constexpr uint32_t LINE_TIMEOUT_MS{500};

/*
  Anything that takes a Stream& works with the unit through the adapter. This stands in for a
  third-party Arduino library (a Modbus stack, a GPS parser, ...) that you cannot modify.
 */
void sendReport(Stream& out, const uint32_t sequence)
{
    out.print("seq=");
    out.print(sequence);
    out.print(" uptime=");
    out.print(millis() / 1000);
    out.println("s");
}

uint32_t sequence{};
uint32_t last_send_at{};

}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    {
        auto cfg    = unit.config();
        cfg.flushRX = true;
        unit.config(cfg);
    }

#if defined(USING_HAT_RS485)
    const bool wired = m5::unit::wiring::addHatUART(Units, unit, UART_BAUD);
#elif defined(USING_ATOMIC_RS485_BASE)
    const bool wired = m5::unit::rs485::wiring::addAtomicBaseUART(Units, unit, UART_BAUD);
#elif defined(USING_TAB5_BUILTIN_RS485)
    const bool wired = m5::unit::rs485::wiring::addTab5BuiltinRS485UART(Units, unit, UART_BAUD);
#else  // USING_UNIT_RS485
    const bool wired = m5::unit::wiring::addUART(Units, unit, UART_BAUD);
#endif

    if (!wired || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();  // fills the screen red itself
    }

    M5_LOGI("M5UnitUnified initialized");

    // setTimeout() is a Stream member (not Print) and governs readStringUntil() below.
    stream.setTimeout(LINE_TIMEOUT_MS);

    lcd.fillScreen(TFT_DARKGREEN);
    lcd.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    lcd.setCursor(0, 0);
    lcd.println("UnitSP485Stream");
}

void loop()
{
    M5.update();
    Units.update();

    if (m5::utility::hasElapsed(last_send_at, SEND_INTERVAL_MS)) {
        last_send_at = m5::utility::millis();

        // printf on the RS-485 link itself, courtesy of Print.
        stream.printf("tick %u\r\n", static_cast<unsigned int>(sequence));
        // ... and the same link handed to a Stream&-taking function.
        sendReport(stream, sequence);

        M5.Log.printf(">>>> sent seq=%u\n", static_cast<unsigned int>(sequence));
        ++sequence;
    }

    // Line-oriented receive: readStringUntil() belongs to Stream, not to the unit.
    if (stream.available()) {
        const String line = stream.readStringUntil('\n');
        if (line.length()) {
            M5.Log.printf("<<<< %s\n", line.c_str());
            lcd.printf("%s\n", line.c_str());
        }
    }
}
