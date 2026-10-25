# Migration

The API changes of each release, newest first, with code on both sides of each change.


## 0.1.0 -> 0.2.0

### write() no longer waits for the TX to drain

Every `write()` took a trailing `bool flush = true`, so a write waited for the transceiver to
finish sending before returning. The byte-count overloads now return as soon as the bytes are
queued, as Arduino's `Print::write()` does:

```cpp
// 0.1.0 -- waited by default
unit.write(buffer, size);

// 0.2.0 -- queue, then wait explicitly if the next step needs the line free
unit.write(buffer, size);
unit.flush(true);          // true = TX only
```

The forms taking a trailing `bool` are kept as `[[deprecated]]` overloads, so existing code
still compiles and still waits. They are scheduled for removal in 0.3.0.

A second argument that cannot be a byte count is now a compile error instead of a silent
one-byte write: `write(buffer, 'a')` and `write(bytes, true)` are rejected.

### The wiring helpers take a UartConfig and an RX buffer size

`addAtomicBaseUART()` / `addTab5BuiltinRS485UART()` took a raw Arduino `SERIAL_8N1` on Arduino and
a `wiring::UartConfig` on ESP-IDF. Both now take `UartConfig`, so one call builds on either
framework, and both gained an optional RX buffer size:

```cpp
// 0.1.0 (Arduino)
m5::unit::wiring::addAtomicBaseUART(units, unit, 115200, SERIAL_8N1);

// 0.2.0 (both frameworks)
using m5::unit::wiring::UartConfig;
m5::unit::wiring::addAtomicBaseUART(units, unit, 115200, UartConfig::Default);
m5::unit::wiring::addAtomicBaseUART(units, unit, 115200, UartConfig::Default, 512);
```

The raw `SERIAL_*` form is kept as a `[[deprecated]]` overload, also scheduled for removal in 0.3.0.

### readValue / writeValue are limited to 256 bytes

Both templates copy the value through a stack buffer of `sizeof(T)` bytes, which had no upper
bound. They now reject anything over 256 bytes at compile time -- the largest Modbus RTU frame,
and this library's default RX buffer size. Aggregates stay supported up to that size; a larger
one has to be read in pieces with `readBytes()`.

### A unit can be moved, but only before it is added

`UnitRS485` / `UnitSP485` / `UnitSIT3088` / `Tab5BuiltinRS485` are movable, so a unit can be
returned from a factory or kept in a container. Two rules come with it:

- **Do not move a unit after `UnitUnified::add()`.** `add()` records the unit it was given, and
  moving the object afterwards leaves that record pointing at the old one.
- **Recreate any `UnitSP485Stream`** built from the unit; the adapter refers to the object it was
  constructed with.

`RS485Component`'s own move members are `protected`, so a unit cannot be moved through a base
reference, which would leave a derived unit's own state behind. Copying is deleted: a unit owns
its serial backend.

### Deriving from RS485Component: _serial is private

A unit that supplies its own backend by overriding `make_serial()` no longer assigns `_serial`:

```cpp
// 0.1.0
_serial = std::unique_ptr<ISerial>(new MySerial(...));

// 0.2.0
install_serial(std::unique_ptr<ISerial>(new MySerial(...)));
```

`iserial()` still returns the backend in use. Before `begin()`, and after a `begin()` that failed,
that is a shared no-op backend rather than `nullptr`, so the Serial-compatible calls stay safe at
any time.

`ISerial` gained one pure virtual, `readBytes(buf, len, timeout_ms)`: an ESP-IDF UART port has no
`HardwareSerial` object holding a timeout to inherit, so the timeout is passed per call. An
`ISerial` implementation outside this library has to add it.

### Requires M5UnitUnified 0.6.4 or later

The dependency floor moved from 0.5.5 to 0.6.4, because the ESP-IDF backend now takes its UART
port from `AdapterUART::uartPort()`, which 0.6.4 is the first release to expose. `M5Utility`
(`>= 0.3.0`) is declared explicitly as well, and `library.json` lists `espidf` among its
frameworks so that the library resolves as a dependency of an ESP-IDF project.
