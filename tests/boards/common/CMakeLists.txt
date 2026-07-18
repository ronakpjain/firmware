add_firmware_test(
    common_tests
    SOURCES
        "${FIRMWARE_SOURCE_DIR}/can_library/tests/can_codec_test_adapter.c"
        "${FIRMWARE_SOURCE_DIR}/can_library/tests/can_codec_tests.cpp"
        "${FIRMWARE_SOURCE_DIR}/common/amk/amk.c"
        "${FIRMWARE_SOURCE_DIR}/common/amk/tests/amk_tests.cpp"
        "${FIRMWARE_SOURCE_DIR}/common/bangbang/bangbang.c"
        "${FIRMWARE_SOURCE_DIR}/common/bangbang/tests/bangbang_tests.cpp"
        "${FIRMWARE_SOURCE_DIR}/common/lerp_lut/tests/lerp_lut_tests.cpp"
        "${FIRMWARE_SOURCE_DIR}/common/strbuf/strbuf.c"
        "${FIRMWARE_SOURCE_DIR}/common/strbuf/tests/strbuf_tests.cpp"
        "${FIRMWARE_SOURCE_DIR}/common/ublox/nav_pvt.c"
        "${FIRMWARE_SOURCE_DIR}/common/ublox/nav_relposned.c"
        "${FIRMWARE_SOURCE_DIR}/common/ublox/tests/ublox_tests.cpp"
        "${TEST_SUPPORT_DIR}/common_utils_test_adapter.c"
        "${FIRMWARE_SOURCE_DIR}/common/utils/tests/utils_tests.cpp"
    INCLUDE_DIRECTORIES "${TEST_FAKE_DIR}/g4"
    LIBRARIES LERP_LUT
)
