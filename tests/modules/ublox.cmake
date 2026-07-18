target_sources(
    unit_tests PRIVATE
    "${FIRMWARE_SOURCE_DIR}/common/ublox/nav_pvt.c"
    "${FIRMWARE_SOURCE_DIR}/common/ublox/nav_relposned.c"
    "${FIRMWARE_SOURCE_DIR}/common/ublox/ublox_tests.cpp"
)
