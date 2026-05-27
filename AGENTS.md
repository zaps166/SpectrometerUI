# OpenCode Agent Instructions

## Architecture & Tech Stack
- **Language**: C++23 and QML.
- **Framework**: Qt 6.11 (QuickControls2, SerialPort).
- **Build System**: CMake (minimum 3.31).
- **Platform**: Desktop (Linux/Windows/macOS) and Android.

## Build Instructions
Quick CMake build (recommended):
```bash
cmake -B build/ai -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/ai
```

Notes:
- To enable AddressSanitizer set `-DUSE_ASAN=ON` in the CMake configure command (see examples below).
- On platforms that require `libusb` for Android builds set `-DANDROID_LIBUSB_DIR=/path/to/libusb`.

### Important CMake Options
- `-DUSE_ASAN=ON`: Enables Address Sanitizer (default OFF).
- `-DBUILD_TESTS=ON`: Build the test targets (default OFF).
- `-DUSE_QT_SERIAL_PORT=ON/OFF`: Toggles between `QSerialPort` and `libusb-1.0`. Defaults to `ON` for Desktop, `OFF` for Android.
- `-DANDROID_LIBUSB_DIR=/path/to/libusb`: Required when building for Android to locate libusb.

## Code Conventions & Constraints
- **Strings (CRITICAL)**: `QT_NO_CAST_FROM_ASCII` is strictly enforced. You must wrap raw C-strings when passing to Qt APIs. Prefer the Qt string literal operator `u"text"_s` (enabled via `using namespace Qt::StringLiterals;`) or `QString::fromUtf8("text")`. Do not pass `"text"` directly to functions expecting `QString`.
- **String Concatenation**: `QT_USE_QSTRINGBUILDER` is enabled globally, so `operator+` is already optimized via QStringBuilder. Always use `+` for concatenation (e.g., `u"a"_s + u"b"_s`). Do **not** use `%`. Never assign a QStringBuilder expression to `auto` — the builder type is a temporary proxy and can cause dangling references; always assign to `QString` explicitly.

- **Centralized header**: This project provides a single shared header at `Headers.hpp` (root of the repo) that pulls common Qt/std headers and also declares global using directives:
  - `using namespace Qt::StringLiterals;` (enables `u""_s` literals)
  - `using namespace std;`

  If you change how string literals are used (or edit `Headers.hpp`), consider refreshing it and recompiling the project so all translation units pick up the change. Avoid making accidental edits to the global `using` directives without coordinating with the team — changing them can silently affect many files.
- **Compiler Warnings**: `-Wsign-compare` is explicitly enabled. Ensure signed/unsigned types match in comparisons.
- **QML Architecture**: The project uses Qt 6's `qt_add_qml_module` for QML compilation to C++. `AppQmlSingleton.qml` is configured as a singleton type.

## Testing & Verification
- Verify successful compilation locally after modifying C++ or QML files.
- Run the executable locally from the build directory to ensure QML engine loads correctly.
- Tests live in the `tests/` directory (for example `tests/AlgorithmTests.cpp`) and are built by the CMake project.

How to run tests:
- Using the `build/ai` directory from the build example above:
  - Note: tests are only built if `-DBUILD_TESTS=ON` is passed to CMake (default OFF).
  - Configure & build tests: `cmake -B build/ai -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON && cmake --build build/ai`
  - Run all tests: `ctest --test-dir build/ai --output-on-failure`
  - List tests: `ctest --test-dir build/ai -N`
  - Run tests matching a regex: `ctest --test-dir build/ai -R <regex>`
  - Or run a test binary directly (typically under `build/ai/tests/`).

ASAN (AddressSanitizer):
- Enable ASAN at configure time: `cmake -B build/ai -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DUSE_ASAN=ON -DBUILD_TESTS=ON` then build and run tests. ASAN builds are useful for finding memory issues; when running under ASAN prefer single-threaded test execution for clearer reports: `ctest --test-dir build/ai -j1 --output-on-failure`.
