# M5Unit - RS485

## Overview

Library for RS485 using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5Stack Unit products.

### SKU:U034
Unit RS485 is a TTL-RS485 converter, used to convert TTL level signals to RS485 level signals. RS485 is a communication protocol standard that defines the electrical characteristics of drivers and receivers for serial communication systems. It supports multipoint systems and is used in industrial applications. When your project devices require communication control over RS485, using Unit RS485 as an interface converter is a good choice.

### SKU:U067
Hat RS485 is an RS485 converter compatible with StickC. It integrates SP485EEN internally, mainly consisting of a 485 automatic transceiver circuit and a DC-DC buck circuit (which can step down the input voltage to 5V). RS485 is a standard used to define the electrical characteristics of drivers and receivers for serial communication systems, widely used in industrial fields, supporting multi-point systems. This product is used to convert TTL standard to RS485 standard. If the external serial device uses the RS485 standard, this module performs TTL-RS485 level conversion to enable communication between devices.

### SKU:A131
Atomic RS485 Base is a TTL-RS485 converter based on the Atomic design, used for converting TTL level to RS485 level. RS485 is a widely used electrical standard in the industrial field, featuring stronger anti-interference capability and longer transmission distance. It has a built-in DC-DC step-down regulator chip, which can directly convert the 6 ~ 24V voltage of RS485 to 5V to power the ATOM, eliminating the trouble of separate power supply. When multiple devices require communication control, using the Atomic RS485 for interface type conversion will be a good choice.

### Built-in SIT3088
M5Stack Tab5 (ESP32-P4) has a built-in RS-485 transceiver (SIT3088) on the bottom edge, with RX=GPIO21 / TX=GPIO20 and manual direction control via DIR=GPIO34. Unlike SP485EEN-based products (U034 / U067 / A131), the DIR pin must be driven by software; `UnitSIT3088` handles this automatically via an internal decorator on the underlying Serial. `Tab5BuiltinRS485` is a thin subclass that bakes the Tab5-specific DIR pin (GPIO34) into config for out-of-the-box use.


## Caution
Reflections are more likely when the RS485 cable is long and/or the baud rate is relatively high.  
Add **120Ω termination at both ends** of the RS485 bus to suppress reflections.  
**External products (SKU:U034 / SKU:U067 / SKU:A131) do not include built-in termination resistors**, so add external termination when needed.  
**M5Stack Tab5 built-in RS-485 provides a switchable 120Ω terminator via a physical switch on the back of the device**; enable it when Tab5 sits at either end of the bus.

**Leaving the unit's TX pin unconnected blocks the whole bus.** On U034 / U067 / A131 the
transceiver's DI is tied to GND and its DE / /RE are driven from TX through a transistor (see the
Schematics on each SKU page), so the driver is enabled exactly while TX is low. A floating TX
leaves the driver enabled and holds the line at that level, and a unit plugged in with nothing on
its GROVE side then stops every other node from transmitting.

**The same thing happens while a node's power is coming up or going down.** A host that has just
lost power leaves TX low, so its driver stays enabled for as long as the unit's capacitors hold it
up, and the whole bus is unusable until they drain. Measured on hardware: with the peer unplugged
but still discharging, every message a second node sent came back to it corrupted; once the peer
had drained, the same node sent a hundred messages with nothing returning. Power a node up and the
transition drives the line briefly again. Plan for a device dropping off the bus to disturb it for
a moment rather than cleanly disappearing.

See also https://docs.m5stack.com/en/arduino/projects/atomic/atomic_rs485_232_base

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

| Example | Description | Frameworks |
|---|---|---|
| Transceiver | Sends and receives RS-485 messages using the unit's own API | Arduino / ESP-IDF |
| StreamAdapter | Wraps the unit in `UnitSP485Stream` to get Arduino `Stream` / `Print` (`print` / `printf` / `readStringUntil`), and to hand the link to any library taking a `Stream&` | Arduino only |

`UnitSP485Stream` derives from Arduino's `Stream`, which does not exist in ESP-IDF native builds.
Code that must build on both frameworks should call the unit directly, as Transceiver does.

### For Arduino IDE settings
You must choose a define symbol for the unit you will use.
(Rewrite source or specify with compile options)

- Transceiver / StreamAdapter (both carry the same block)
```cpp
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
```

### For ESP-IDF settings

> **NOTE:** The ESP-IDF native build (`idf.py`) targets ESP-IDF **5.1 or later** (5.x and 6.x).

The Transceiver example also builds with `idf.py`. The unit is selected from menuconfig instead of a
define symbol.

```sh
cd examples/UnitUnified/Transceiver
idf.py set-target <esp32|esp32s3|esp32c3|esp32c5|esp32c6|esp32h2|esp32p4>
idf.py menuconfig    # M5Unit-RS485 Transceiver example -> Target unit / board
idf.py build
idf.py -p <PORT> flash monitor
```

Which choices appear in menuconfig depends on the target, because each variant needs a board that
physically has that connector:

| Choice | Targets where it appears |
|---|---|
| UnitRS485 (U034, GROVE) | all targets |
| HatRS485 (U067, Hat header) | esp32 / esp32s3 / esp32c6 |
| Atomic RS485 Base (A131) | esp32 / esp32s3 |
| Tab5 built-in RS-485 (SIT3088) | esp32p4 |

menuconfig writes to the generated `sdkconfig`, so a choice made there takes effect on the next
build. Edits to `examples/UnitUnified/common/sdkconfig.defaults` do not: that file is only read
when no `sdkconfig` exists yet, so delete `sdkconfig` and re-run `idf.py set-target <target>` to
pick them up.

The chosen `CONFIG_EXAMPLE_USING_*` is mapped to the source-level `USING_*` macro by
`examples/UnitUnified/common/variant.cmake`, so no `#define` has to be edited for the ESP-IDF
build.


## Migration

**0.2.0 changes released APIs.** [MIGRATION.md](MIGRATION.md) lists what changed from 0.1.0 to
0.2.0, with code on both sides of each change, and carries the same list for later releases.


## Related Link
- [Unit RS485 & Datasheet](https://docs.m5stack.com/en/unit/rs485)
- [HAT RS485 & Datasheet](https://docs.m5stack.com/en/hat/hat-rs485)
- [Atomic RS485 Base & Datasheet](https://docs.m5stack.com/en/atom/Atomic%20RS485%20Base)
- [M5Stack Tab5 & Datasheet](https://docs.m5stack.com/en/core/Tab5)

## Required Libraries
- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

## License
- [M5Unit-RS485 MIT](LICENSE)


## Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-RS485/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

### Required
- [Doxygen](https://www.doxygen.nl/)
- [Git](https://git-scm.com/) (Output commit hash to html)
