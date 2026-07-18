add_firmware_test(
    test_a_box
    SOURCES
        "${FIRMWARE_SOURCE_DIR}/source/a_box/adbms/commands.c"
        "${FIRMWARE_SOURCE_DIR}/source/a_box/adbms/commands_tests.cpp"
        "${FIRMWARE_SOURCE_DIR}/source/a_box/adbms/pec.c"
        "${FIRMWARE_SOURCE_DIR}/source/a_box/adbms/pec_tests.cpp"
        "${FIRMWARE_SOURCE_DIR}/source/a_box/thermistor/thermistor.c"
        "${FIRMWARE_SOURCE_DIR}/source/a_box/thermistor/thermistor_tests.cpp"
    LIBRARIES LERP_LUT
)
