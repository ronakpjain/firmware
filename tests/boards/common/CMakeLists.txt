add_firmware_test(
    common_tests
    SOURCES
        "${FIRMWARE_SOURCE_DIR}/can_library/tests/can_codec_test_adapter.c"
        "${FIRMWARE_SOURCE_DIR}/can_library/tests/can_codec_tests.cpp"
)
