add_firmware_test(
    g4_gpio_tests
    G4
    SOURCES
        "${FIRMWARE_SOURCE_DIR}/common/phal_G4/gpio/gpio.c"
        "${FIRMWARE_SOURCE_DIR}/common/phal_G4/gpio/tests/gpio_tests.cpp"
        "${TEST_SUPPORT_DIR}/g4_register_fake.c"
        "${TEST_SUPPORT_DIR}/g4_test_runtime.c"
    INCLUDE_DIRECTORIES "${TEST_FAKE_DIR}/g4"
)
