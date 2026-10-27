# Map the Kconfig "Target unit / board" choice (common/Kconfig.variant) to the source-level macro
# that both the Arduino and ESP-IDF builds use. Include this from an example's main/CMakeLists.txt
# *after* idf_component_register() (it needs ${COMPONENT_LIB}).
set(M5UNIT_VARIANT "")
if(CONFIG_EXAMPLE_USING_UNIT_RS485)
    set(M5UNIT_VARIANT USING_UNIT_RS485)
elseif(CONFIG_EXAMPLE_USING_HAT_RS485)
    set(M5UNIT_VARIANT USING_HAT_RS485)
elseif(CONFIG_EXAMPLE_USING_ATOMIC_RS485_BASE)
    set(M5UNIT_VARIANT USING_ATOMIC_RS485_BASE)
elseif(CONFIG_EXAMPLE_USING_TAB5_BUILTIN_RS485)
    set(M5UNIT_VARIANT USING_TAB5_BUILTIN_RS485)
endif()
if(M5UNIT_VARIANT)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ${M5UNIT_VARIANT})
endif()
