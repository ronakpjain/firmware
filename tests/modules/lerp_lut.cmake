add_subdirectory(
    "${FIRMWARE_SOURCE_DIR}/common/lerp_lut"
    "${CMAKE_CURRENT_BINARY_DIR}/lerp_lut"
)
target_sources(
    unit_tests PRIVATE
    "${FIRMWARE_SOURCE_DIR}/common/lerp_lut/tests/lerp_lut_tests.cpp"
)
target_link_libraries(unit_tests PRIVATE LERP_LUT)
