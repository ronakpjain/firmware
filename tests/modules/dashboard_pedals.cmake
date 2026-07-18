add_library(
    dashboard_pedals_under_test OBJECT
    "${FIRMWARE_SOURCE_DIR}/source/dashboard/pedals/pedals.c"
    "${TEST_SUPPORT_DIR}/dashboard_pedals_test_runtime.c"
)
configure_g4_test_target(dashboard_pedals_under_test)
target_include_directories(
    dashboard_pedals_under_test BEFORE PRIVATE
    "${TEST_FAKE_DIR}/board"
    "${FIRMWARE_SOURCE_DIR}/source/dashboard"
    "${FIRMWARE_SOURCE_DIR}/source/dashboard/pedals"
)
target_sources(
    unit_tests PRIVATE
    "${FIRMWARE_SOURCE_DIR}/source/dashboard/pedals/pedals_tests.cpp"
    $<TARGET_OBJECTS:dashboard_pedals_under_test>
)
