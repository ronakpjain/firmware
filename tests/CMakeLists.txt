cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(PER_FIRMWARE_UNIT_TESTS LANGUAGES C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
    URL_HASH SHA256=9a56a54ae784394ff664cd55e8f4c9a03b503ebf0cb99576321c78ab3d87ca84
)
FetchContent_MakeAvailable(googletest)

set(FIRMWARE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(TEST_FAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/fakes")
set(TEST_SUPPORT_DIR "${CMAKE_CURRENT_LIST_DIR}/support")

function(add_firmware_test TARGET_NAME)
    cmake_parse_arguments(
        TEST
        "G4"
        ""
        "SOURCES;DEFINITIONS;INCLUDE_DIRECTORIES;LIBRARIES"
        ${ARGN}
    )

    add_executable(${TARGET_NAME} ${TEST_SOURCES})
    target_link_libraries(${TARGET_NAME} PRIVATE GTest::gtest_main ${TEST_LIBRARIES})
    target_compile_options(
        ${TARGET_NAME}
        PRIVATE
        $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:-Wall>
        $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:-Wextra>
        $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:-Werror>
        $<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang,GNU>:-Wall>
        $<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang,GNU>:-Wextra>
        $<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang,GNU>:-Werror>
    )
    target_include_directories(
        ${TARGET_NAME}
        BEFORE PRIVATE
        ${TEST_INCLUDE_DIRECTORIES}
        "${TEST_SUPPORT_DIR}"
        "${FIRMWARE_SOURCE_DIR}"
    )
    target_compile_definitions(${TARGET_NAME} PRIVATE ${TEST_DEFINITIONS})
    if(TEST_G4)
        target_compile_definitions(${TARGET_NAME} PRIVATE STM32G474xx)
    endif()

    set_property(GLOBAL APPEND PROPERTY FIRMWARE_TEST_TARGETS ${TARGET_NAME})
endfunction()

add_subdirectory(
    "${FIRMWARE_SOURCE_DIR}/common/lerp_lut"
    "${CMAKE_CURRENT_BINARY_DIR}/lerp_lut"
)

add_subdirectory(boards/common)
add_subdirectory(boards/a_box)
add_subdirectory(boards/dashboard)
add_subdirectory(boards/main_module)
add_subdirectory(boards/driveline)

get_property(FIRMWARE_TEST_TARGETS GLOBAL PROPERTY FIRMWARE_TEST_TARGETS)
add_custom_target(unit_tests DEPENDS ${FIRMWARE_TEST_TARGETS})

set(TEST_EXECUTABLE_MANIFEST "")
foreach(TEST_TARGET IN LISTS FIRMWARE_TEST_TARGETS)
    string(APPEND TEST_EXECUTABLE_MANIFEST "$<TARGET_FILE:${TEST_TARGET}>\n")
endforeach()
file(
    GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/test_executables.txt"
    CONTENT "${TEST_EXECUTABLE_MANIFEST}"
)
