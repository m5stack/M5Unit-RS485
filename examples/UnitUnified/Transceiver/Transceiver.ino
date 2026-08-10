/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Transceiver example using M5UnitUnified for UnitRS485
*/
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
#include "main/Transceiver.cpp"
