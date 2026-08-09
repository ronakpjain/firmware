# Host tests

The host test suite uses GoogleTest and CTest for firmware unit tests.

## Requirements

- Python 3.11 or newer
- CMake 3.21 or newer
- A C23 and C++17 host compiler
- Network access for initial GoogleTest dependency downloads
- For coverage: `gcov`, `lcov`, and `genhtml` on `PATH` (`sudo apt-get install lcov` on Ubuntu or `brew install lcov` on macOS)

## Running the tests

Run tests through `per_build.py` from the repository root:

| Command | Action |
| --- | --- |
| `python3 per_build.py tests` | Build and run all host unit tests |
| `python3 per_build.py tests unit` | Build and run unit tests |
| `python3 per_build.py tests --sanitizers` | Run unit tests with AddressSanitizer and UBSan |
| `python3 per_build.py tests unit --sanitizers` | Run unit tests with AddressSanitizer and UBSan |
| `python3 per_build.py tests unit --coverage` | Run tests and generate an HTML coverage report |

Coverage is available for GNU, Clang, and AppleClang builds. It resets
counters, runs CTest, captures gcov data with lcov, filters test and dependency
sources, and writes the report to
`firmware/build/host-tests/coverage/html/index.html`. Coverage cannot be
combined with sanitizers.

`per_build.py` configures `tests/CMakeLists.txt`, builds the selected tests,
and runs CTest with failure output enabled. Build artifacts are stored in
`firmware/build/host-tests`.

The `host_tests.yml` GitHub Actions workflow runs the unit tests with coverage
on pull requests and pushes to `master`, and uploads the generated HTML report
as the `host-test-coverage` artifact.

The same steps can be run directly:

```sh
cmake -S tests -B firmware/build/host-tests \
  -DPER_TEST_LAYER=unit \
  -DPER_TEST_SANITIZERS=OFF
cmake --build firmware/build/host-tests
ctest --test-dir firmware/build/host-tests --output-on-failure
```

`PER_TEST_LAYER` accepts only `all` or `unit`.

To run coverage directly with CMake, configure with
`-DPER_TEST_COVERAGE=ON` and build the `coverage` target:

```sh
cmake -S tests -B firmware/build/host-tests \
  -DPER_TEST_LAYER=unit \
  -DPER_TEST_SANITIZERS=OFF \
  -DPER_TEST_COVERAGE=ON
cmake --build firmware/build/host-tests --target coverage
```

## Existing tests

Unit tests live under `tests/unit/firmware`:

- `lerp_lut_test.cpp` covers exact lookup points, interpolation, and upper and lower clamping in `firmware/common/lerp_lut/lerp_lut.c`.
- `can_codec_test.cpp` covers payload loading and storage, byte swapping, signal packing and unpacking, sign extension, and float bit conversion in `firmware/can_library/can_codec.h`. A C23 shim ensures these header-only inline functions are compiled as C rather than as part of the C++17 GoogleTest translation unit.

`tests/cmake/FirmwareUnitTest.cmake` provides `add_firmware_unit_test`. It
configures production C sources as C23 static libraries, test sources as C++17,
strict compiler warnings, GoogleTest discovery, the CTest `unit` label, and
optional sanitizers or coverage instrumentation.

## Directory layout

```text
tests/
├── ARCHITECTURE.md
├── CMakeLists.txt
├── cmake/
│   └── FirmwareUnitTest.cmake
└── unit/
    ├── CMakeLists.txt
    └── firmware/
        ├── can_codec_test.cpp
        └── lerp_lut_test.cpp
```

## Adding a unit test

1. Add a GoogleTest source file under `tests/unit/firmware`.
2. Register a target in `tests/unit/CMakeLists.txt` with `add_firmware_unit_test`:

   ```cmake
   add_firmware_unit_test(
       NAME example_test
       SOURCES "${CMAKE_CURRENT_LIST_DIR}/../../firmware/common/example/example.c"
       TEST_SOURCES "${CMAKE_CURRENT_LIST_DIR}/firmware/example_test.cpp"
       INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../firmware/common/example"
   )
   ```

3. Use `SOURCES` for production `.c` files. For header-only C modules, add a
   `.c` shim to `SOURCES` and call it from the C++ test so inline implementation
   code is compiled under C23 rather than C++17.
4. Run `python3 per_build.py tests unit --sanitizers`.

CTest discovers each GoogleTest case from the registered target automatically.
