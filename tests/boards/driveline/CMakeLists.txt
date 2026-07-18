add_firmware_test(
    driveline_tests
    SOURCES
        "${FIRMWARE_SOURCE_DIR}/source/driveline/oil_temps/oil_temps_table.c"
        "${FIRMWARE_SOURCE_DIR}/source/driveline/oil_temps/tests/oil_temps_table_tests.cpp"
    LIBRARIES LERP_LUT
)
