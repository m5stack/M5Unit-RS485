# ESP-IDF component registration for M5Unit-RS485.
# Upstream: https://github.com/m5stack/M5Unit-RS485
#
# The public headers pull in both M5UnitUnified and M5Utility, because the endian-aware
# readValue / writeValue templates are built on m5_utility/stl/byteswap.hpp and endianness.hpp.
# driver is private: only unit_SIT3088.cpp uses driver/gpio.h for the direction pin.
file(GLOB_RECURSE SRCS "src/*.cpp")
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "src"
    REQUIRES M5UnitUnified M5Utility
    PRIV_REQUIRES driver
)
