add_library(
    g4_gpio_under_test OBJECT
    "${FIRMWARE_SOURCE_DIR}/common/phal_G4/gpio/gpio.c"
    "${TEST_SUPPORT_DIR}/g4_test_runtime.c"
)
configure_g4_test_target(g4_gpio_under_test)
target_sources(
    unit_tests PRIVATE
    "${FIRMWARE_SOURCE_DIR}/common/phal_G4/gpio/gpio_tests.cpp"
    $<TARGET_OBJECTS:g4_gpio_under_test>
)
target_compile_definitions(unit_tests PRIVATE STM32G474xx)
