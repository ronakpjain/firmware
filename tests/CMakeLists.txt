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

function(configure_test_target TARGET_NAME)
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
        "${TEST_FAKE_DIR}"
        "${TEST_SUPPORT_DIR}"
        "${FIRMWARE_SOURCE_DIR}"
    )
endfunction()

function(configure_g4_test_target TARGET_NAME)
    configure_test_target(${TARGET_NAME})
    target_compile_definitions(${TARGET_NAME} PRIVATE STM32G474xx)
endfunction()

add_executable(unit_tests "${CMAKE_CURRENT_LIST_DIR}/unit_test_main.cpp")
configure_test_target(unit_tests)
target_link_libraries(unit_tests PRIVATE GTest::gtest)

file(GLOB TEST_MODULES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/modules/*.cmake")
foreach(TEST_MODULE IN LISTS TEST_MODULES)
    include("${TEST_MODULE}")
endforeach()
