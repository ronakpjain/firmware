add_library(
    dashboard_menu_under_test OBJECT
    "${FIRMWARE_SOURCE_DIR}/source/dashboard/driver_interface/menu_system.c"
    "${TEST_SUPPORT_DIR}/nextion_fake.c"
)
configure_test_target(dashboard_menu_under_test)
target_include_directories(
    dashboard_menu_under_test BEFORE PRIVATE
    "${FIRMWARE_SOURCE_DIR}/source/dashboard/driver_interface"
)
target_sources(
    unit_tests PRIVATE
    "${FIRMWARE_SOURCE_DIR}/source/dashboard/driver_interface/menu_system_tests.cpp"
    $<TARGET_OBJECTS:dashboard_menu_under_test>
)
