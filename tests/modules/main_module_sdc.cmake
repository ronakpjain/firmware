add_library(
    main_module_sdc_under_test OBJECT
    "${FIRMWARE_SOURCE_DIR}/source/main_module/sdc/sdc.c"
    "${TEST_SUPPORT_DIR}/board_phal_fake.c"
    "${TEST_SUPPORT_DIR}/main_module_sdc_test_runtime.c"
)
configure_g4_test_target(main_module_sdc_under_test)
target_include_directories(
    main_module_sdc_under_test BEFORE PRIVATE
    "${TEST_FAKE_DIR}/board"
    "${FIRMWARE_SOURCE_DIR}/source/main_module"
    "${FIRMWARE_SOURCE_DIR}/source/main_module/sdc"
)
target_sources(
    unit_tests PRIVATE
    "${FIRMWARE_SOURCE_DIR}/source/main_module/sdc/sdc_tests.cpp"
    $<TARGET_OBJECTS:main_module_sdc_under_test>
)
