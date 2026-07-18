add_firmware_test(
    a_box_tests
    SOURCES
        "${FIRMWARE_SOURCE_DIR}/source/a_box/adbms/commands.c"
        "${FIRMWARE_SOURCE_DIR}/source/a_box/adbms/tests/commands_tests.cpp"
        "${FIRMWARE_SOURCE_DIR}/source/a_box/adbms/pec.c"
        "${FIRMWARE_SOURCE_DIR}/source/a_box/adbms/tests/pec_tests.cpp"
        "${FIRMWARE_SOURCE_DIR}/source/a_box/thermistor/thermistor.c"
        "${FIRMWARE_SOURCE_DIR}/source/a_box/thermistor/tests/thermistor_tests.cpp"
    LIBRARIES LERP_LUT
)
