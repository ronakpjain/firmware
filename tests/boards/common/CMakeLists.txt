set(G4_GPIO_TEST_SOURCES
    "${FIRMWARE_SOURCE_DIR}/common/phal_G4/gpio/gpio.c"
    "${FIRMWARE_SOURCE_DIR}/common/phal_G4/gpio/gpio_tests.cpp"
    "${TEST_SUPPORT_DIR}/g4_register_fake.c"
    "${TEST_SUPPORT_DIR}/g4_test_runtime.c"
)
set_source_files_properties(
    ${G4_GPIO_TEST_SOURCES}
    PROPERTIES COMPILE_DEFINITIONS STM32G474xx
)

add_firmware_test(
    test_common
    SOURCES
        "${FIRMWARE_SOURCE_DIR}/can_library/tests/can_codec_test_adapter.c"
        "${FIRMWARE_SOURCE_DIR}/can_library/tests/can_codec_tests.cpp"
        "${FIRMWARE_SOURCE_DIR}/common/amk/amk.c"
        "${FIRMWARE_SOURCE_DIR}/common/amk/amk_tests.cpp"
        "${FIRMWARE_SOURCE_DIR}/common/bangbang/bangbang.c"
        "${FIRMWARE_SOURCE_DIR}/common/bangbang/bangbang_tests.cpp"
        "${FIRMWARE_SOURCE_DIR}/common/lerp_lut/tests/lerp_lut_tests.cpp"
        ${G4_GPIO_TEST_SOURCES}
        "${FIRMWARE_SOURCE_DIR}/common/strbuf/strbuf.c"
        "${FIRMWARE_SOURCE_DIR}/common/strbuf/strbuf_tests.cpp"
        "${FIRMWARE_SOURCE_DIR}/common/ublox/nav_pvt.c"
        "${FIRMWARE_SOURCE_DIR}/common/ublox/nav_relposned.c"
        "${FIRMWARE_SOURCE_DIR}/common/ublox/ublox_tests.cpp"
        "${TEST_SUPPORT_DIR}/common_utils_test_adapter.c"
        "${FIRMWARE_SOURCE_DIR}/common/utils/utils_tests.cpp"
    INCLUDE_DIRECTORIES "${TEST_FAKE_DIR}/g4"
    LIBRARIES LERP_LUT
)
