target_sources(
    unit_tests PRIVATE
    "${FIRMWARE_SOURCE_DIR}/source/a_box/adbms/commands.c"
    "${FIRMWARE_SOURCE_DIR}/source/a_box/adbms/commands_tests.cpp"
    "${FIRMWARE_SOURCE_DIR}/source/a_box/adbms/pec.c"
    "${FIRMWARE_SOURCE_DIR}/source/a_box/adbms/pec_tests.cpp"
)
