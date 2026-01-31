##  New Enhancements
- **XFAIL & SKIP support**:
  - Static: `MT_TEST_XFAIL`, `MT_TEST_SKIP`.
  - Runtime: `MT_MARK_XFAIL(ctx, "reason" )`, `MT_SKIP(ctx, "reason")`.
  - CLI: `--xfail-strict` (XPASS counts as failure).
- **Reports**:
  - JSON, JUnit XML, Markdown.
  - CLI flags: `--report-json`, `--report-junit`, `--report-md`.
- **Auto-report defaults**:
  - Uses `TEST_REPORT_DIR` env var or current directory if no flags provided.

---

## Example 
```cpp
#include "mini.hpp"

MT_TEST(math, addition, "unit") {
    MT_REQUIRE_EQ(ctx, 2 + 3, 5);
}
```

Build & run:
```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make
./tests/resource-tuner/RestuneComponentTests
./tests/resource-tuner/RestuneIntegrationTests
./tests/resource-tuner/RestuneUnitTests
```

---

##  Running Tests
```bash
# Filter by tag
./RestuneIntegrationTests --tag=cgroup

# Generate reports manually
TEST_REPORT_DIR=./build/test-reports ./RestuneIntegrationTests
```
Reports:
```
build/test-reports/
  RestuneIntegrationTests.json
  RestuneIntegrationTests.junit.xml
  RestuneIntegrationTests.md
```

---

##  CLI Options
- `--filter=<substring>` – name filter.
- `--tag=<tag>` – exact tag match.
- `--threads=N` – parallel execution.
- `--tap` / `--no-tap` – TAP output toggle.
- `--xfail-strict` – XPASS counts as failure.
- `--report-json`, `--report-junit`, `--report-md` – report paths.

---

##  Example Summary
```
Summary: total=47, passed=29, failed=11, skipped=0, xfail=7, xpass=0, time=314566.983 ms
```

---

##  Marker Options
You can mark tests as **expected failure (XFAIL)** or **skip** using these macros:

### Static Markers
```cpp
MT_TEST_XFAIL(SuiteName, TestName, "tag", "reason") {
    // Test logic (will be reported as XFAIL if it fails)
}

MT_TEST_SKIP(SuiteName, TestName, "tag", "reason") {
    // This test will be skipped entirely
}
```

### Runtime Markers
```cpp
MT_TEST(SuiteName, TestName, "tag") {
    if (!condition) {
        MT_SKIP(ctx, "Skipping due to condition");
    }
    if (known_issue) {
        MT_MARK_XFAIL(ctx, "Known issue: expected failure");
    }
    // Normal test logic
}
```

Summary output includes `skipped`, `xfail`, and `xpass` counts.

---

##  CTest Integration
To run tests and generate reports automatically via CTest:

### Example CMake snippet
```cmake
set(TEST_REPORT_DIR "${CMAKE_BINARY_DIR}/test-reports")
file(MAKE_DIRECTORY "${TEST_REPORT_DIR}")

add_test(NAME RestuneIntegrationTests_All
         COMMAND RestuneIntegrationTests
                 --report-json=${TEST_REPORT_DIR}/integration_all.json
                 --report-junit=${TEST_REPORT_DIR}/integration_all.junit.xml
                 --report-md=${TEST_REPORT_DIR}/integration_all.md)

set_tests_properties(RestuneIntegrationTests_All PROPERTIES
    ENVIRONMENT "TEST_REPORT_DIR=${TEST_REPORT_DIR};NO_COLOR=")
```

Run all tests:
```bash
ctest -V
```

Reports will appear in `build/test-reports/`.

