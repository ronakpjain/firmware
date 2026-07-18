target_sources(
    unit_tests PRIVATE
    "${TEST_SUPPORT_DIR}/common_utils_test_adapter.c"
    "${FIRMWARE_SOURCE_DIR}/common/utils/utils_tests.cpp"
)
