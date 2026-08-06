# M5Unit - RS485

## Overview

Library for RS485 using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5Stack Unit products.

### SKU:U034
Unit RS485 is a TTL-RS485 converter, used to convert TTL level signals to RS485 level signals. RS485 is a communication protocol standard that defines the electrical characteristics of drivers and receivers for serial communication systems. It supports multipoint systems and is used in industrial applications. When your project devices require communication control over RS485, using Unit RS485 as an interface converter is a good choice.

### SKU:U067
Hat RS485 is an RS485 converter compatible with M5StickC. It integrates SP485EEN internally, mainly consisting of a 485 automatic transceiver circuit and a DC-DC buck circuit (which can step down the input voltage to 5V). RS485 is a standard used to define the electrical characteristics of drivers and receivers for serial communication systems, widely used in industrial fields, supporting multi-point systems. This product is used to convert TTL standard to RS485 standard. If the external serial device is RS485 standard, communication between devices implementing TTL to RS485 protocol conversion can be achieved through this module.

### SKU:A131
Atomic RS485 Base is a TTL-RS485 converter based on the Atomic design, used for converting TTL level to RS485 level. RS485 is a widely used electrical standard in the industrial field, featuring stronger anti-interference capability and longer transmission distance. It has a built-in DC-DC step-down regulator chip, which can directly convert the 12V voltage of RS485 to 5V to power the ATOM, eliminating the trouble of separate power supply. When multiple devices require communication control, using the Atomic RS485 for interface type conversion will be a good choice.

### Tab5 built-in RS-485 (SIT3088)
M5Stack Tab5 (ESP32-P4) has a built-in RS-485 transceiver (SIT3088) on the bottom edge, with RX=GPIO21 / TX=GPIO20 and manual direction control via DIR=GPIO34. Unlike SP485EEN-based products (U034 / U067 / A131), the DIR pin must be driven by software; `UnitSIT3088` (aliased as `Tab5BuiltinRS485`) handles this automatically via an internal decorator on the underlying Serial.


## Caution
Reflections are more likely when the RS485 cable is long and/or the baud rate is relatively high.  
Add **120Ω termination at both ends** of the RS485 bus to suppress reflections.  
**These products (SKU:U034 / SKU:U067 / SKU:A131 and Tab5 built-in) do not include built-in termination resistors**, so add external termination when needed.

See also https://docs.m5stack.com/en/arduino/projects/atomic/atomic_rs485_232_base

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

### For Arduino IDE settings
You must choose a define symbol for the unit you will use.
(Rewrite source or specify with compile options)

- Transceiver
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


## Related Link
- [Unit RS485 & Datasheet](https://docs.m5stack.com/en/unit/rs485)
- [HAT RS485 & Datasheet](https://docs.m5stack.com/en/hat/hat-rs485)
- [Atomic RS485 Base & Datasheet](https://docs.m5stack.com/en/atom/Atomic%20RS485%20Base)
- [M5Stack Tab5 & Datasheet](https://docs.m5stack.com/en/core/Tab5)

## Required Libraries:
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
