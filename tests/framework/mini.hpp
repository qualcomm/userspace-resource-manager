
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <utility>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <iomanip>    // for std::setprecision
#include <algorithm>  // for std::max
#include <fstream>    // for reports
#include <map>        // for grouping reports

namespace mtest {
struct RunResult;
// ---------- Config ----------
#ifndef MTEST_ENABLE_EXCEPTIONS
#define MTEST_ENABLE_EXCEPTIONS 1
#endif

#ifndef MTEST_DEFAULT_OUTPUT_TAP
#define MTEST_DEFAULT_OUTPUT_TAP 0
#endif

#ifndef MTEST_DEFAULT_THREADS
#define MTEST_DEFAULT_THREADS 1
#endif

// ---------- Core types ----------
struct TestContext {
    const char* suite = nullptr;
    std::string name;               // dynamic instance names supported
    const char* tag   = nullptr;    // "unit", "module", "usecase", …
    const char* file  = nullptr;
    int         line  = 0;
    std::string message;
    bool        failed = false;

    // Runtime markers
    bool        mark_skip   = false;
    bool        mark_xfail  = false;
    std::string mark_reason;
};

using testfn = std::function<void(TestContext&)>;

struct TestCase {
    const char* suite;
    std::string name;   // allow dynamic instance names
    const char* tag;
    const char* file;
    int         line;
    testfn      fn;

    // Static markers
    bool        skip   = false;
    bool        xfail  = false;
    const char* reason = nullptr;
    bool        xfail_strict = false; // per-test strict override
};

enum class CaseStatus { PASS, FAIL, SKIP, XFAIL, XPASS };

// ---------- Registry ----------
inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

struct Registrar {
    // C-string name
    Registrar(const char* suite, const char* name, const char* tag,
              const char* file, int line, testfn fn,
              bool skip=false, bool xfail=false,
              const char* reason=nullptr, bool xfail_strict=false) {
        registry().push_back({suite, std::string(name), tag, file, line, std::move(fn),
                              skip, xfail, reason, xfail_strict});
    }
    // std::string name (parameterized cases)
    Registrar(const char* suite, const std::string& name, const char* tag,
              const char* file, int line, testfn fn,
              bool skip=false, bool xfail=false,
              const char* reason=nullptr, bool xfail_strict=false) {
        registry().push_back({suite, name, tag, file, line, std::move(fn),
                              skip, xfail, reason, xfail_strict});
    }
};

// ---------- Utilities ----------
namespace detail {
    // C++17-friendly to_string_any
    template <typename T>
    inline std::string to_string_any(const T& v) {
        std::ostringstream oss;
        oss << v;
        return oss.str();
    }

    // ANSI colors
    struct Colors {
        const char* reset  = "\x1b[0m";
        const char* bold   = "\x1b[1m";
        const char* red    = "\x1b[31m";
        const char* green  = "\x1b[32m";
        const char* yellow = "\x1b[33m";
        const char* cyan   = "\x1b[36m";
    };

    inline bool env_no_color() {
        const char* nc = std::getenv("NO_COLOR");
        return nc && *nc;
    }

    inline std::string fmt_pass(const TestCase& tc, double ms, bool tap, bool color) {
        Colors c;
        std::ostringstream oss;
        if (tap) {
            oss << "ok - " << tc.suite << "." << tc.name << " [" << tc.tag << "] ("
                << std::fixed << std::setprecision(3) << ms << " ms)\n";
        } else {
            if (color) oss << c.green;
            oss << "[PASS] ";
            if (color) oss << c.reset;
            oss << tc.suite << "." << tc.name << " [" << tc.tag << "] ("
                << std::fixed << std::setprecision(3) << ms << " ms)\n";
        }
        return oss.str();
    }

    inline std::string fmt_fail(const TestCase& tc, double ms, const TestContext& ctx,
                                bool tap, bool color) {
        Colors c;
        std::ostringstream oss;
        if (tap) {
            oss << "not ok - " << tc.suite << "." << tc.name << " [" << tc.tag << "] ("
                << std::fixed << std::setprecision(3) << ms << " ms)\n";
            oss << "  ---\n  file: " << tc.file << "\n  line: " << tc.line
                << "\n  msg: " << ctx.message << "\n  ...\n";
        } else {
            if (color) oss << c.red;
            oss << "[FAIL] ";
            if (color) oss << c.reset;
            oss << tc.suite << "." << tc.name << " [" << tc.tag << "] ("
                << std::fixed << std::setprecision(3) << ms << " ms)\n  "
                << tc.file << ":" << tc.line << "\n  " << ctx.message << "\n";
        }
        return oss.str();
    }

    inline std::string fmt_skip(const TestCase& tc, double ms, const char* reason,
                                bool tap, bool color, int tap_index) {
        Colors c;
        std::ostringstream oss;
        if (tap) {
            oss << "ok " << tap_index << " - " << tc.suite << "." << tc.name
                << " [" << tc.tag << "] # SKIP " << (reason ? reason : "") << "\n";
        } else {
            if (color) oss << c.yellow;
            oss << "[SKIP] ";
            if (color) oss << c.reset;
            oss << tc.suite << "." << tc.name << " [" << tc.tag << "] ("
                << std::fixed << std::setprecision(3) << ms
                << " ms)  " << (reason ? reason : "") << "\n";
        }
        return oss.str();
    }

    inline std::string fmt_xfail(const TestCase& tc, double ms, const char* reason,
                                 bool tap, bool color, int tap_index) {
        Colors c;
        std::ostringstream oss;
        if (tap) {
            oss << "not ok " << tap_index << " - " << tc.suite << "." << tc.name
                << " [" << tc.tag << "] (" << std::fixed << std::setprecision(3) << ms
                << " ms) # TODO " << (reason ? reason : "expected failure") << "\n";
        } else {
            if (color) oss << c.yellow;
            oss << "[XFAIL] ";
            if (color) oss << c.reset;
            oss << tc.suite << "." << tc.name << " [" << tc.tag << "] ("
                << std::fixed << std::setprecision(3) << ms
                << " ms)  " << (reason ? reason : "") << "\n";
        }
        return oss.str();
    }

    inline std::string fmt_xpass(const TestCase& tc, double ms, const char* reason,
                                 bool tap, bool color, bool strict, int tap_index) {
        Colors c;
        std::ostringstream oss;
        if (tap) {
            oss << "ok " << tap_index << " - " << tc.suite << "." << tc.name
                << " [" << tc.tag << "] (" << std::fixed << std::setprecision(3) << ms
                << " ms) # TODO XPASS: " << (reason ? reason : "unexpected pass") << "\n";
        } else {
            if (strict) {
                if (color) oss << c.red;
                oss << "[XPASS(strict)] ";
                if (color) oss << c.reset;
            } else {
                if (color) oss << c.cyan;
                oss << "[XPASS] ";
                if (color) oss << c.reset;
            }
            oss << tc.suite << "." << tc.name << " [" << tc.tag << "] ("
                << std::fixed << std::setprecision(3) << ms
                << " ms)  " << (reason ? reason : "") << "\n";
        }
        return oss.str();
    }

    // -------- Reporting helpers (prototypes only; impls appear later) --------
    // Forward declare writers so run_all can call them; implementations are below


    void write_json_report(
        const char* path,
        const std::vector<int>& selected,
        const std::vector<TestCase>& cases,
        const std::vector<CaseStatus>& statuses,
        const std::vector<double>& durations,
        const std::vector<std::string>& reasons,
        const std::vector<std::string>& messages,
        const ::mtest::RunResult& summary);

    void write_junit_report(
        const char* path,
        const std::vector<int>& selected,
        const std::vector<TestCase>& cases,
        const std::vector<CaseStatus>& statuses,
        const std::vector<double>& durations,
        const std::vector<std::string>& reasons,
        const std::vector<std::string>& messages,
        const ::mtest::RunResult& summary,
        bool xfail_as_skip);

    void write_markdown_report(
        const char* path,
        const std::vector<int>& selected,
        const std::vector<TestCase>& cases,
        const std::vector<CaseStatus>& statuses,
        const std::vector<double>& durations,
        const std::vector<std::string>& reasons,
        const std::vector<std::string>& messages,
        const ::mtest::RunResult& summary);


} // namespace detail

// ---------- Assertions ----------
#define MTEST_STRINGIFY(x) #x
#define MTEST_STR(x) MTEST_STRINGIFY(x)

#define MT_REQUIRE(ctx, expr) do { \
    if (!(expr)) { \
        (ctx).failed = true; \
        (ctx).message = std::string("REQUIRE failed: ") + #expr; \
        if (MTEST_ENABLE_EXCEPTIONS) throw std::runtime_error((ctx).message); \
        return; \
    } \
} while(0)

#define MT_CHECK(ctx, expr) do { \
    if (!(expr)) { \
        (ctx).failed = true; \
        (ctx).message = std::string("CHECK failed: ") + #expr; \
    } \
} while(0)

#define MT_REQUIRE_EQ(ctx, a, b) do { \
    auto va = (a); auto vb = (b); \
    if (!(va == vb)) { \
        (ctx).failed = true; \
        std::ostringstream _m; \
        _m << "REQUIRE_EQ failed: " << #a << " == " << #b \
            << "  (got " << va << " vs " << vb << ")"; \
        (ctx).message = _m.str(); \
        if (MTEST_ENABLE_EXCEPTIONS) throw std::runtime_error((ctx).message); \
        return; \
    } \
} while(0)

#define MT_CHECK_EQ(ctx, a, b) do { \
    auto va = (a); auto vb = (b); \
    if (!(va == vb)) { \
        (ctx).failed = true; \
        std::ostringstream _m; \
        _m << "CHECK_EQ failed: " << #a << " == " << #b \
            << "  (got " << va << " vs " << vb << ")"; \
        (ctx).message = _m.str(); \
    } \
} while(0)

#define MT_FAIL(ctx, msg) do { \
    (ctx).failed = true; \
    (ctx).message = std::string("FAIL: ") + (msg); \
    if (MTEST_ENABLE_EXCEPTIONS) throw std::runtime_error((ctx).message); \
    return; \
} while(0)


// ---------- Markers (runtime) ----------
#define MT_SKIP(ctx, reason) do { \
    (ctx).mark_skip = true; \
    (ctx).mark_reason = (reason); \
    return; \
} while(0)

#define MT_MARK_XFAIL(ctx, reason) do { \
    (ctx).mark_xfail = true; \
    (ctx).mark_reason = (reason); \
} while(0)


#ifndef MTEST_OVERRIDE_ASSERT
#define MTEST_OVERRIDE_ASSERT 1
#endif
#if MTEST_OVERRIDE_ASSERT
namespace detail {
    inline void assert_true(bool cond, const char* expr, const char* file, int line) {
        if (!cond) {
#if MTEST_ENABLE_EXCEPTIONS
            throw std::runtime_error(std::string("assert(") + expr + ") failed at "
                                     + file + ":" + std::to_string(line));
#else
            std::fprintf(stderr, "assert(%s) failed at %s:%d\n", expr, file, line);
#endif
        }
    }
}
#ifdef assert
#undef assert
#endif
#define assert(expr) ::mtest::detail::assert_true((expr), #expr, __FILE__, __LINE__)
#endif


// ---------- Fixtures ----------
struct Fixture {
    virtual ~Fixture() = default;
    virtual void setup(TestContext&) {}
    virtual void teardown(TestContext&) {}
};

template <typename F>
inline void with_fixture(TestContext& ctx,
    const std::function<void(F&, TestContext&)>& body) {
    F f;
    f.setup(ctx);
    body(f, ctx);
    f.teardown(ctx);
}

// ---------- Registration macros ----------
#define MT_TEST(suite, name, tag) \
    static void suite##_##name##_impl(mtest::TestContext&); \
    static mtest::Registrar suite##_##name##_reg( \
        MTEST_STRINGIFY(suite), MTEST_STRINGIFY(name), tag, \
        __FILE__, __LINE__, suite##_##name##_impl \
    ); \
    static void suite##_##name##_impl(mtest::TestContext& ctx)

#define MT_TEST_F(suite, name, tag, FixtureType) \
    static void suite##_##name##_body(FixtureType&, mtest::TestContext&); \
    static void suite##_##name##_impl(mtest::TestContext&); \
    static mtest::Registrar suite##_##name##_reg( \
        MTEST_STRINGIFY(suite), MTEST_STRINGIFY(name), tag, \
        __FILE__, __LINE__, suite##_##name##_impl \
    ); \
    static void suite##_##name##_impl(mtest::TestContext& ctx) { \
        mtest::with_fixture<FixtureType>(ctx, std::bind( \
            suite##_##name##_body, std::placeholders::_1, std::placeholders::_2)); \
    } \
    static void suite##_##name##_body(FixtureType& f, mtest::TestContext& ctx)

#define MT_TEST_F_END

// Parameterized tests
#define MT_TEST_P(suite, name, tag, ParamType, params) \
    static void suite##_##name##_impl(mtest::TestContext&, const ParamType& param); \
    namespace suite##_##name##_param_ns { \
        static struct registrar_t { \
            registrar_t() { \
                for (const ParamType& _p : params) { \
                    std::string _n = std::string(MTEST_STRINGIFY(name)) + "(" + mtest::detail::to_string_any(_p) + ")"; \
                    mtest::Registrar( \
                        MTEST_STRINGIFY(suite), _n, tag, __FILE__, __LINE__, \
                        std::bind(suite##_##name##_impl, std::placeholders::_1, _p) \
                    ); \
                } \
            } \
        } _registrar_instance; \
    } \
    static void suite##_##name##_impl(mtest::TestContext& ctx, const ParamType& param)

#define MT_TEST_P_LIST(suite, name, tag, ParamType, ...) \
    static void suite##_##name##_impl(mtest::TestContext&, const ParamType& param); \
    namespace suite##_##name##_param_list_ns { \
        static struct registrar_t { \
            registrar_t() { \
                const ParamType _params[] = { __VA_ARGS__ }; \
                for (const ParamType& _p : _params) { \
                    std::string _n = std::string(MTEST_STRINGIFY(name)) + "(" + mtest::detail::to_string_any(_p) + ")"; \
                    mtest::Registrar( \
                        MTEST_STRINGIFY(suite), _n, tag, __FILE__, __LINE__, \
                        std::bind(suite##_##name##_impl, std::placeholders::_1, _p) \
                    ); \
                } \
            } \
        } _registrar_instance; \
    } \
    static void suite##_##name##_impl(mtest::TestContext& ctx, const ParamType& param)

#define MT_TEST_FP(suite, name, tag, FixtureType, ParamType, params) \
    static void suite##_##name##_body(FixtureType&, mtest::TestContext&, const ParamType&); \
    static void suite##_##name##_impl(mtest::TestContext&); \
    namespace suite##_##name##_fp_ns { \
        static struct registrar_t { \
            registrar_t() { \
                for (const ParamType& _p : params) { \
                    std::string _n = std::string(MTEST_STRINGIFY(name)) + "(" + mtest::detail::to_string_any(_p) + ")"; \
                    mtest::Registrar( \
                        MTEST_STRINGIFY(suite), _n, tag, __FILE__, __LINE__, \
                        std::bind(suite##_##name##_impl, std::placeholders::_1) \
                    ); \
                } \
            } \
        } _registrar_instance; \
    } \
    static void suite##_##name##_impl(mtest::TestContext& ctx) { \
        (void)ctx; \
    } \
    static void suite##_##name##_body(FixtureType& f, mtest::TestContext& ctx, const ParamType& param)

#define MT_TEST_FP_LIST(suite, name, tag, FixtureType, ParamType, ...) \
    static void suite##_##name##_body(FixtureType&, mtest::TestContext&, const ParamType&); \
    static void suite##_##name##_impl_instance(mtest::TestContext&, const ParamType&); \
    namespace suite##_##name##_fp_list_ns { \
        static struct registrar_t { \
            registrar_t() { \
                const ParamType _params[] = { __VA_ARGS__ }; \
                for (const ParamType& _p : _params) { \
                    std::string _n = std::string(MTEST_STRINGIFY(name)) + "(" + mtest::detail::to_string_any(_p) + ")"; \
                    mtest::Registrar( \
                        MTEST_STRINGIFY(suite), _n, tag, __FILE__, __LINE__, \
                        std::bind(suite##_##name##_impl_instance, std::placeholders::_1, _p) \
                    ); \
                } \
            } \
        } _registrar_instance; \
    } \
    static void suite##_##name##_impl_instance(mtest::TestContext& ctx, const ParamType& param) { \
        mtest::with_fixture<FixtureType>(ctx, std::bind( \
            suite##_##name##_body, std::placeholders::_1, std::placeholders::_2, param)); \
    } \
    static void suite##_##name##_body(FixtureType& f, mtest::TestContext& ctx, const ParamType& param)


// ---------- Static marker macros ----------
#define MT_TEST_XFAIL(suite, name, tag, reason) \
    static void suite##_##name##_impl(mtest::TestContext&); \
    static mtest::Registrar suite##_##name##_reg( \
        MTEST_STRINGIFY(suite), MTEST_STRINGIFY(name), tag, \
        __FILE__, __LINE__, suite##_##name##_impl, \
        /*skip*/false, /*xfail*/true, reason, /*xfail_strict*/false); \
    static void suite##_##name##_impl(mtest::TestContext& ctx)

#define MT_TEST_SKIP(suite, name, tag, reason) \
    static void suite##_##name##_impl(mtest::TestContext&); \
    static mtest::Registrar suite##_##name##_reg( \
        MTEST_STRINGIFY(suite), MTEST_STRINGIFY(name), tag, \
        __FILE__, __LINE__, suite##_##name##_impl, \
        /*skip*/true, /*xfail*/false, reason, /*xfail_strict*/false); \
    static void suite##_##name##_impl(mtest::TestContext& ctx)

#define MT_TEST_F_XFAIL(suite, name, tag, FixtureType, reason) \
    static void suite##_##name##_body(FixtureType&, mtest::TestContext&); \
    static void suite##_##name##_impl(mtest::TestContext&); \
    static mtest::Registrar suite##_##name##_reg( \
        MTEST_STRINGIFY(suite), MTEST_STRINGIFY(name), tag, \
        __FILE__, __LINE__, suite##_##name##_impl, \
        /*skip*/false, /*xfail*/true, reason, /*xfail_strict*/false); \
    static void suite##_##name##_impl(mtest::TestContext& ctx) { \
        mtest::with_fixture<FixtureType>(ctx, std::bind( \
            suite##_##name##_body, std::placeholders::_1, std::placeholders::_2)); \
    } \
    static void suite##_##name##_body(FixtureType& f, mtest::TestContext& ctx)

#define MT_TEST_F_SKIP(suite, name, tag, FixtureType, reason) \
    static void suite##_##name##_body(FixtureType&, mtest::TestContext&); \
    static void suite##_##name##_impl(mtest::TestContext&); \
    static mtest::Registrar suite##_##name##_reg( \
        MTEST_STRINGIFY(suite), MTEST_STRINGIFY(name), tag, \
        __FILE__, __LINE__, suite##_##name##_impl, \
        /*skip*/true, /*xfail*/false, reason, /*xfail_strict*/false); \
    static void suite##_##name##_impl(mtest::TestContext& ctx) { \
        mtest::with_fixture<FixtureType>(ctx, std::bind( \
            suite##_##name##_body, std::placeholders::_1, std::placeholders::_2)); \
    } \
    static void suite##_##name##_body(FixtureType& f, mtest::TestContext& ctx)

#define MT_TEST_P_XFAIL_LIST(suite, name, tag, ParamType, reason, ...) \
    static void suite##_##name##_impl(mtest::TestContext&, const ParamType& param); \
    namespace suite##_##name##_param_xfail_list_ns { \
        static struct registrar_t { \
            registrar_t() { \
                const ParamType _params[] = { __VA_ARGS__ }; \
                for (const ParamType& _p : _params) { \
                    std::string _n = std::string(MTEST_STRINGIFY(name)) + "(" + mtest::detail::to_string_any(_p) + ")"; \
                    mtest::Registrar( \
                        MTEST_STRINGIFY(suite), _n, tag, __FILE__, __LINE__, \
                        std::bind(suite##_##name##_impl, std::placeholders::_1, _p), \
                        /*skip*/false, /*xfail*/true, reason, /*xfail_strict*/false); \
                } \
            } \
        } _registrar_instance; \
    } \
    static void suite##_##name##_impl(mtest::TestContext& ctx, const ParamType& param)

#define MT_TEST_P_SKIP_LIST(suite, name, tag, ParamType, reason, ...) \
    static void suite##_##name##_impl(mtest::TestContext&, const ParamType& param); \
    namespace suite##_##name##_param_skip_list_ns { \
        static struct registrar_t { \
            registrar_t() { \
                const ParamType _params[] = { __VA_ARGS__ }; \
                for (const ParamType& _p : _params) { \
                    std::string _n = std::string(MTEST_STRINGIFY(name)) + "(" + mtest::detail::to_string_any(_p) + ")"; \
                    mtest::Registrar( \
                        MTEST_STRINGIFY(suite), _n, tag, __FILE__, __LINE__, \
                        std::bind(suite##_##name##_impl, std::placeholders::_1, _p), \
                        /*skip*/true, /*xfail*/false, reason, /*xfail_strict*/false); \
                } \
            } \
        } _registrar_instance; \
    } \
    static void suite##_##name##_impl(mtest::TestContext& ctx, const ParamType& param)

#define MT_TEST_FP_LIST_XFAIL(suite, name, tag, FixtureType, ParamType, reason, ...) \
    static void suite##_##name##_body(FixtureType&, mtest::TestContext&, const ParamType&); \
    static void suite##_##name##_impl_instance(mtest::TestContext&, const ParamType&); \
    namespace suite##_##name##_fp_list_xfail_ns { \
        static struct registrar_t { \
            registrar_t() { \
                const ParamType _params[] = { __VA_ARGS__ }; \
                for (const ParamType& _p : _params) { \
                    std::string _n = std::string(MTEST_STRINGIFY(name)) + "(" + mtest::detail::to_string_any(_p) + ")"; \
                    mtest::Registrar( \
                        MTEST_STRINGIFY(suite), _n, tag, __FILE__, __LINE__, \
                        std::bind(suite##_##name##_impl_instance, std::placeholders::_1, _p), \
                        /*skip*/false, /*xfail*/true, reason, /*xfail_strict*/false); \
                } \
            } \
        } _registrar_instance; \
    } \
    static void suite##_##name##_impl_instance(mtest::TestContext& ctx, const ParamType& param) { \
        mtest::with_fixture<FixtureType>(ctx, std::bind( \
            suite##_##name##_body, std::placeholders::_1, std::placeholders::_2, param)); \
    } \
    static void suite##_##name##_body(FixtureType& f, mtest::TestContext& ctx, const ParamType& param)

#define MT_TEST_FP_LIST_SKIP(suite, name, tag, FixtureType, ParamType, reason, ...) \
    static void suite##_##name##_body(FixtureType&, mtest::TestContext&, const ParamType&); \
    static void suite##_##name##_impl_instance(mtest::TestContext&, const ParamType&); \
    namespace suite##_##name##_fp_list_skip_ns { \
        static struct registrar_t { \
            registrar_t() { \
                const ParamType _params[] = { __VA_ARGS__ }; \
                for (const ParamType& _p : _params) { \
                    std::string _n = std::string(MTEST_STRINGIFY(name)) + "(" + mtest::detail::to_string_any(_p) + ")"; \
                    mtest::Registrar( \
                        MTEST_STRINGIFY(suite), _n, tag, __FILE__, __LINE__, \
                        std::bind(suite##_##name##_impl_instance, std::placeholders::_1, _p), \
                        /*skip*/true, /*xfail*/false, reason, /*xfail_strict*/false); \
                } \
            } \
        } _registrar_instance; \
    } \
    static void suite##_##name##_impl_instance(mtest::TestContext& ctx, const ParamType& param) { \
        mtest::with_fixture<FixtureType>(ctx, std::bind( \
            suite##_##name##_body, std::placeholders::_1, std::placeholders::_2, param)); \
    } \
    static void suite##_##name##_body(FixtureType& f, mtest::TestContext& ctx, const ParamType& param)


// ---------- Runner ----------
struct RunOptions {
    const char* name_contains = nullptr;   // substring filter on test name
    const char* tag_equals    = nullptr;   // exact tag filter
    bool        tap_output    = MTEST_DEFAULT_OUTPUT_TAP != 0;
    bool        stop_on_fail  = false;
    bool        summary_only  = false;
    bool        color_output  = true;      // colored output
    int         threads       = MTEST_DEFAULT_THREADS; // parallelism

    // XFAIL strict mode: XPASS counts as failure when true
    bool        xfail_strict  = false;

    // Reports
    const char* report_json   = nullptr;   // --report-json=<path>
    const char* report_junit  = nullptr;   // --report-junit=<path>
    const char* report_md     = nullptr;   // --report-md=<path>
    bool        junit_xfail_as_skip = true; // treat XFAIL as skipped in JUnit
};

struct RunResult {
    int total   = 0;
    int failed  = 0;
    int passed  = 0;
    double ms_total = 0.0;

    // New counters
    int skipped = 0;
    int xfailed = 0;
    int xpassed = 0;
};

inline bool match_filter(const TestCase& tc, const RunOptions& opt) {
    if (opt.name_contains) {
        if (tc.name.find(opt.name_contains) == std::string::npos)
            return false;
    }
    if (opt.tag_equals) {
        if (std::strcmp(tc.tag, opt.tag_equals) != 0)
            return false;
    }
    return true;
}

struct CaseResult {
    CaseStatus  status = CaseStatus::PASS;
    double      ms   = 0.0;
    std::string out;
    std::string msg;
    bool        strict_xpass = false;
    std::string reason;
};

inline CaseResult run_one(const TestCase& tc, const RunOptions& opt, int tap_index) {
    CaseResult cr;
    TestContext ctx;
    ctx.suite = tc.suite; ctx.name = tc.name; ctx.tag = tc.tag;
    ctx.file  = tc.file;  ctx.line = tc.line;

    auto t0 = std::chrono::steady_clock::now();

    // Static skip: short-circuit
    if (tc.skip) {
        auto t1s = std::chrono::steady_clock::now();
        cr.ms = std::chrono::duration<double, std::milli>(t1s - t0).count();
        cr.status = CaseStatus::SKIP;
        cr.reason = (tc.reason ? tc.reason : "");
        cr.out = detail::fmt_skip(tc, cr.ms, tc.reason, opt.tap_output,
                                  opt.color_output && !detail::env_no_color(),
                                  tap_index);
        return cr;
    }

#if MTEST_ENABLE_EXCEPTIONS
    try {
        tc.fn(ctx);
    } catch (const std::exception& e) {
        ctx.failed = true;
        ctx.message = std::string("uncaught: ") + e.what();
    } catch (...) {
        ctx.failed = true;
        ctx.message = "uncaught: unknown exception";
    }
#else
    tc.fn(ctx);
#endif
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    cr.ms = ms;

    // Runtime skip?
    if (ctx.mark_skip) {
        cr.status = CaseStatus::SKIP;
        cr.reason = ctx.mark_reason;
        cr.out = detail::fmt_skip(tc, ms, ctx.mark_reason.c_str(),
                                  opt.tap_output,
                                  opt.color_output && !detail::env_no_color(),
                                  tap_index);
        return cr;
    }

    // XFAIL logic (static or runtime)
    bool is_xfail = tc.xfail || ctx.mark_xfail;
    const char* why = tc.reason ? tc.reason : (ctx.mark_reason.empty() ? nullptr : ctx.mark_reason.c_str());
    bool strict = tc.xfail_strict || opt.xfail_strict;

    if (is_xfail) {
        cr.reason = (why ? why : "");
        if (ctx.failed) {
            cr.status = CaseStatus::XFAIL;
            cr.msg = ctx.message;
            cr.out = detail::fmt_xfail(tc, ms, why, opt.tap_output,
                                       opt.color_output && !detail::env_no_color(),
                                       tap_index);
        } else {
            cr.status = CaseStatus::XPASS;
            cr.strict_xpass = strict;
            cr.out = detail::fmt_xpass(tc, ms, why, opt.tap_output,
                                       opt.color_output && !detail::env_no_color(),
                                       strict, tap_index);
        }
        return cr;
    }

    // Normal pass/fail
    if (ctx.failed) {
        cr.status = CaseStatus::FAIL;
        cr.msg = ctx.message;
        if (opt.tap_output) {
            std::ostringstream oss;
            oss << "not ok " << tap_index << " - " << tc.suite << "." << tc.name
                << " [" << tc.tag << "] (" << std::fixed << std::setprecision(3) << ms << " ms)\n";
            oss << "  ---\n  file: " << tc.file << "\n  line: " << tc.line
                << "\n  msg: " << ctx.message << "\n  ...\n";
            cr.out = oss.str();
        } else {
            cr.out = detail::fmt_fail(tc, ms, ctx, /*tap*/false,
                                      opt.color_output && !detail::env_no_color());
        }
    } else {
        cr.status = CaseStatus::PASS;
        cr.out = opt.tap_output
            ? ([&]{
                std::ostringstream oss;
                oss << "ok " << tap_index << " - " << tc.suite << "." << tc.name
                    << " [" << tc.tag << "] (" << std::fixed << std::setprecision(3) << ms << " ms)\n";
                return oss.str();
              })()
            : detail::fmt_pass(tc, ms, /*tap*/false,
                               opt.color_output && !detail::env_no_color());
    }
    return cr;
}

// Worker functor for parallel execution (no lambdas)
struct Worker {
    const std::vector<int>* selected;
    const std::vector<TestCase>* cases;
    const RunOptions* opt;
    std::vector<CaseResult>* results;
    std::atomic<size_t>* next_index;

    Worker(const std::vector<int>* sel,
           const std::vector<TestCase>* rcases,
           const RunOptions* ropt,
           std::vector<CaseResult>* res,
           std::atomic<size_t>* next)
        : selected(sel), cases(rcases), opt(ropt), results(res), next_index(next) {}

    void operator()() {
        while (true) {
            size_t k = next_index->fetch_add(1);
            if (k >= selected->size()) break;
            int idx = (*selected)[k];
            (*results)[k] = run_one((*cases)[idx], *opt, (int)k + 1);
        }
    }
};

inline RunResult run_all(const RunOptions& opt = {}) {
    RunResult rr{};
    const auto& r = registry();

    // Collect selected cases
    std::vector<int> selected;
    selected.reserve(r.size());
    for (int i = 0; i < (int)r.size(); ++i) {
        if (match_filter(r[i], opt)) selected.push_back(i);
    }

    if (opt.tap_output) {
        std::printf("TAP version 13\n");
        std::printf("1..%zu\n", selected.size());
    }

    // Results store in deterministic order
    std::vector<CaseResult> results(selected.size());

    // If stop_on_fail or threads <= 1, run sequential
    if (opt.stop_on_fail || opt.threads <= 1) {
        for (size_t k = 0; k < selected.size(); ++k) {
            int idx = selected[k];
            auto cr = run_one(r[idx], opt, (int)k + 1);
            results[k] = std::move(cr);
            rr.total++;
            rr.ms_total += results[k].ms;

            switch (results[k].status) {
                case CaseStatus::PASS:  rr.passed++; break;
                case CaseStatus::FAIL:  rr.failed++; break;
                case CaseStatus::SKIP:  rr.skipped++; break;
                case CaseStatus::XFAIL: rr.xfailed++; break;
                case CaseStatus::XPASS:
                    if (results[k].strict_xpass) rr.failed++;
                    else rr.xpassed++;
                    break;
            }

            if (!opt.summary_only) {
                std::fwrite(results[k].out.data(), 1, results[k].out.size(), stdout);
            }
            if (opt.stop_on_fail && results[k].status == CaseStatus::FAIL) break;
        }
    } else {
        // Parallel execution with deterministic output ordering
        std::atomic<size_t> next{0};
        int threads = std::max(1, opt.threads);
        std::vector<std::thread> workers;
        workers.reserve(threads);

        for (int t = 0; t < threads; ++t) {
            workers.emplace_back(Worker(&selected, &r, &opt, &results, &next));
        }
        for (std::thread& th : workers) th.join();

        // Aggregate + print in order
        for (size_t k = 0; k < results.size(); ++k) {
            rr.total++;
            rr.ms_total += results[k].ms;
            switch (results[k].status) {
                case CaseStatus::PASS:  rr.passed++; break;
                case CaseStatus::FAIL:  rr.failed++; break;
                case CaseStatus::SKIP:  rr.skipped++; break;
                case CaseStatus::XFAIL: rr.xfailed++; break;
                case CaseStatus::XPASS:
                    if (results[k].strict_xpass) rr.failed++;
                    else rr.xpassed++;
                    break;
            }
            if (!opt.summary_only) {
                std::fwrite(results[k].out.data(), 1, results[k].out.size(), stdout);
            }
        }
    }

    if (!opt.summary_only) {
        bool all_ok = (rr.failed == 0);
        if (opt.color_output && !detail::env_no_color()) {
            const char* col = all_ok ? "\x1b[32m" : "\x1b[31m";
            std::printf(
                "\n%sSummary:%s total=%d, passed=%d, failed=%d, skipped=%d, xfail=%d, xpass=%d, time=%.3f ms\n",
                col, "\x1b[0m", rr.total, rr.passed, rr.failed, rr.skipped, rr.xfailed, rr.xpassed, rr.ms_total);
        } else {
            std::printf(
                "\nSummary: total=%d, passed=%d, failed=%d, skipped=%d, xfail=%d, xpass=%d, time=%.3f ms\n",
                rr.total, rr.passed, rr.failed, rr.skipped, rr.xfailed, rr.xpassed, rr.ms_total);
        }
    }

    // -------- Reports --------
    if (opt.report_json || opt.report_junit || opt.report_md) {
        // Prepare linear arrays for writers
        std::vector<CaseStatus> statuses(selected.size());
        std::vector<double>     durations(selected.size());
        std::vector<std::string> reasons(selected.size());
        std::vector<std::string> messages(selected.size());
        for (size_t i = 0; i < selected.size(); ++i) {
            statuses[i] = results[i].status;
            durations[i] = results[i].ms;
            reasons[i]   = results[i].reason;
            messages[i]  = results[i].msg;
        }
        if (opt.report_json) {
            detail::write_json_report(opt.report_json, selected, r,
                                      statuses, durations, reasons, messages, rr);
        }
        if (opt.report_junit) {
            detail::write_junit_report(opt.report_junit, selected, r,
                                       statuses, durations, reasons, messages, rr,
                                       opt.junit_xfail_as_skip);
        }
        if (opt.report_md) {
            detail::write_markdown_report(opt.report_md, selected, r,
                                          statuses, durations, reasons, messages, rr);
        }
    }

    return rr;
}

// -------- Report writers (implementations placed after RunResult is defined) --------
namespace detail {

inline const char* status_to_str(CaseStatus s) {
    switch (s) {
        case CaseStatus::PASS:  return "pass";
        case CaseStatus::FAIL:  return "fail";
        case CaseStatus::SKIP:  return "skip";
        case CaseStatus::XFAIL: return "xfail";
        case CaseStatus::XPASS: return "xpass";
    }
    return "unknown";
}

inline std::string escape_xml(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
            case '&': o << "&amp;"; break;
            case '<': o << "&lt;";  break;
            case '>': o << "&gt;";  break;
            case '"': o << "&quot;";break;
            case '\'':o << "&apos;";break;
            default:  o << c;       break;
        }
    }
    return o.str();
}

inline std::string json_escape(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
            case '"':  o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\n': o << "\\n";  break;
            case '\r': o << "\\r";  break;
            case '\t': o << "\\t";  break;
            default:   o << c;      break;
        }
    }
    return o.str();
}

inline void write_json_report(
    const char* path,
    const std::vector<int>& selected,
    const std::vector<TestCase>& cases,
    const std::vector<CaseStatus>& statuses,
    const std::vector<double>& durations,
    const std::vector<std::string>& reasons,
    const std::vector<std::string>& messages,
    const ::mtest::RunResult& summary)
{
    if (!path) return;
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << "{\n";
    f << "  \"summary\": {"
      << "\"total\": "    << summary.total    << ", "
      << "\"passed\": "   << summary.passed   << ", "
      << "\"failed\": "   << summary.failed   << ", "
      << "\"skipped\": "  << summary.skipped  << ", "
      << "\"xfail\": "    << summary.xfailed  << ", "
      << "\"xpass\": "    << summary.xpassed  << ", "
      << "\"time_ms\": "  << std::fixed << std::setprecision(3) << summary.ms_total
      << "},\n";
    f << "  \"tests\": [\n";
    for (size_t k = 0; k < selected.size(); ++k) {
        const TestCase& tc = cases[selected[k]];
        f << "    {\n";
        f << "      \"suite\": \"" << tc.suite << "\",\n";
        f << "      \"name\": \""  << tc.name  << "\",\n";
        f << "      \"tag\": \""   << tc.tag   << "\",\n";
        f << "      \"file\": \""  << tc.file  << "\",\n";
        f << "      \"line\": "    << tc.line  << ",\n";
        f << "      \"status\": \""<< status_to_str(statuses[k]) << "\",\n";
        f << "      \"duration_ms\": " << std::fixed << std::setprecision(3) << durations[k] << ",\n";
        
        f << "      \"reason\": "  << (reasons[k].empty() ? "null" : ("\"" + json_escape(reasons[k]) + "\"")) << ",\n";
        f << "      \"message\": " << (messages[k].empty() ? "null" : ("\"" + json_escape(messages[k]) + "\"")) << "\n";

        f << "    }" << (k + 1 < selected.size() ? "," : "") << "\n";
    }
    f << "  ]\n";
    f << "}\n";
}

inline void write_junit_report(
    const char* path,
    const std::vector<int>& selected,
    const std::vector<TestCase>& cases,
    const std::vector<CaseStatus>& statuses,
    const std::vector<double>& durations,
    const std::vector<std::string>& reasons,
    const std::vector<std::string>& messages,
    const ::mtest::RunResult& summary,
    bool xfail_as_skip)
{
    if (!path) return;
    std::ofstream f(path);
    if (!f.is_open()) return;

    // Group by suite for multiple <testsuite>
    std::map<std::string, std::vector<size_t>> by_suite;
    for (size_t i = 0; i < selected.size(); ++i) {
        by_suite[cases[selected[i]].suite].push_back(i);
    }

    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    f << "<testsuites tests=\"" << summary.total
      << "\" failures=\"" << summary.failed
      << "\" skipped=\"" << summary.skipped + summary.xfailed
      << "\" time=\"" << std::fixed << std::setprecision(3) << summary.ms_total/1000.0 << "\">\n";

    for (const auto& kv : by_suite) {
        const std::string& suite = kv.first;
        const auto& idxs = kv.second;

        // per-suite stats
        int tests=0, failures=0, skipped=0;
        double time_sec = 0.0;
        for (size_t i : idxs) {
            ++tests;
            time_sec += durations[i] / 1000.0;
            const auto st = statuses[i];
            if (st == CaseStatus::FAIL) failures++;
            if (st == CaseStatus::SKIP) skipped++;
            if (st == CaseStatus::XFAIL && xfail_as_skip) skipped++;
            if (st == CaseStatus::XPASS) {
                // XPASS(strict) has been counted as failure in summary already
            }
        }

        f << "  <testsuite name=\"" << escape_xml(suite)
          << "\" tests=\""   << tests
          << "\" failures=\""<< failures
          << "\" skipped=\"" << skipped
          << "\" time=\""    << std::fixed << std::setprecision(3) << time_sec << "\">\n";

        for (size_t i : idxs) {
            const TestCase& tc = cases[selected[i]];
            const auto& st = statuses[i];
            const auto& dur = durations[i];
            const auto& rsn = reasons[i];
            const auto& msg = messages[i];

            f << "    <testcase classname=\"" << escape_xml(tc.suite)
              << "\" name=\"" << escape_xml(tc.name)
              << "\" time=\"" << std::fixed << std::setprecision(3) << dur/1000.0 << "\">";

            if (st == CaseStatus::FAIL) {
                f << "\n      <failure message=\"" << escape_xml(msg) << "\">"
                  << escape_xml(tc.file) << ":" << tc.line << "</failure>\n";
            } else if (st == CaseStatus::SKIP) {
                f << "\n      <skipped message=\"" << escape_xml(rsn) << "\"/>\n";
            } else if (st == CaseStatus::XFAIL) {
                if (xfail_as_skip) {
                    f << "\n      <skipped message=\"" << escape_xml(rsn.empty() ? "expected failure" : rsn) << "\"/>\n";
                } else {
                    f << "\n      <failure message=\"XFAIL: " << escape_xml(rsn) << "\"/>\n";
                }
            }
            f << "    </testcase>\n";
        }
        f << "  </testsuite>\n";
    }
    f << "</testsuites>\n";
}

inline void write_markdown_report(
    const char* path,
    const std::vector<int>& selected,
    const std::vector<TestCase>& cases,
    const std::vector<CaseStatus>& statuses,
    const std::vector<double>& durations,
    const std::vector<std::string>& reasons,
    const std::vector<std::string>& messages,
    const ::mtest::RunResult& summary)
{
    if (!path) return;
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << "# Mini.hpp Test Report\n\n";
    f << "**Summary**  \n";
    f << "- Total: "   << summary.total   << "\n";
    f << "- Passed: "  << summary.passed  << "\n";
    f << "- Failed: "  << summary.failed  << "\n";
    f << "- Skipped: " << summary.skipped << "\n";
    f << "- XFAIL: "   << summary.xfailed << "\n";
    f << "- XPASS: "   << summary.xpassed << "\n";
    f << "- Time: "    << std::fixed << std::setprecision(3) << summary.ms_total << " ms\n\n";

    // Group by suite for readability
    std::map<std::string, std::vector<size_t>> by_suite;
    for (size_t i = 0; i < selected.size(); ++i) {
        by_suite[cases[selected[i]].suite].push_back(i);
    }

    for (const auto& kv : by_suite) {
        f << "## Suite: " << kv.first << "\n\n";
        for (size_t i : kv.second) {
            const TestCase& tc = cases[selected[i]];
            f << "### " << tc.name << "  \n";
            f << "- **Tag**: "     << tc.tag << "  \n";
            f << "- **Status**: "  << status_to_str(statuses[i]) << "  \n";
            f << "- **Duration**: " << std::fixed << std::setprecision(3) << durations[i] << " ms  \n";
            f << "- **Location**: " << tc.file << ":" << tc.line << "  \n";
            if (!reasons[i].empty()) f << "- **Reason**: " << reasons[i] << "  \n";
            if (!messages[i].empty()) f << "- **Message**: " << messages[i] << "  \n";
            f << "\n";
        }
    }
}

} // namespace detail



inline int run_main(int argc, const char* argv[]) {
    RunOptions opt{};
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        // Existing CLI parsing:
        if (std::strncmp(a, "--filter=", 9) == 0) opt.name_contains = a + 9;
        else if (std::strncmp(a, "--tag=", 6) == 0) opt.tag_equals = a + 6;
        else if (std::strcmp(a, "--tap") == 0) opt.tap_output = true;
        else if (std::strcmp(a, "--stop-on-fail") == 0) opt.stop_on_fail = true;
        else if (std::strcmp(a, "--summary") == 0) opt.summary_only = true;
        else if (std::strcmp(a, "--no-tap") == 0) opt.tap_output = false;
        else if (std::strcmp(a, "--no-color") == 0) opt.color_output = false;
        else if (std::strcmp(a, "--color") == 0) opt.color_output = true;
        else if (std::strncmp(a, "--threads=", 10) == 0) {
            int v = std::atoi(a + 10);
            if (v > 0) opt.threads = v;
        }
        else if (std::strcmp(a, "--xfail-strict") == 0) {
            opt.xfail_strict = true;
        }
        else if (std::strncmp(a, "--report-json=", 14) == 0) {
            opt.report_json = a + 14;
        }
        else if (std::strncmp(a, "--report-junit=", 15) == 0) {
            opt.report_junit = a + 15;
        }
        else if (std::strncmp(a, "--report-md=", 12) == 0) {
            opt.report_md = a + 12;
        }
        else if (std::strcmp(a, "--junit-xfail-as-failure") == 0) {
            opt.junit_xfail_as_skip = false;
        }
    }

    // ✅ Add auto-report defaults here:
    if (!opt.report_json && !opt.report_junit && !opt.report_md) {
        const char* dir = std::getenv("TEST_REPORT_DIR");
        std::string outdir = dir ? dir : "."; // fallback to current dir

        const char* exe = (argc > 0 && argv[0]) ? argv[0] : "mini";
        const char* base = std::strrchr(exe, '/');
#ifdef _WIN32
        const char* back = std::strrchr(exe, '\\');
        if (back && (!base || back > base)) base = back;
#endif
        base = base ? (base + 1) : exe;

        static std::string json  = outdir + "/" + std::string(base) + ".json";
        static std::string junit = outdir + "/" + std::string(base) + ".junit.xml";
        static std::string md    = outdir + "/" + std::string(base) + ".md";

        opt.report_json  = json.c_str();
        opt.report_junit = junit.c_str();
        opt.report_md    = md.c_str();
    }

    auto rr = run_all(opt);
    return rr.failed ? 1 : 0;
}

} // namespace mtest


// Global main wrapper (can be disabled with -DMTEST_NO_MAIN)
#ifndef MTEST_NO_MAIN
int main(int argc, const char* argv[]) {
    return mtest::run_main(argc, argv);
}
#endif

