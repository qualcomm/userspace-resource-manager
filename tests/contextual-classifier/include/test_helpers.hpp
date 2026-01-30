// tests/include/test_helpers.hpp
#ifndef TEST_HELPERS_HPP_INCLUDED
#define TEST_HELPERS_HPP_INCLUDED

#include <array>
#include <chrono>
#include <cstdio>
#include <regex>
#include <string>

namespace test_helpers {

// -------- Settings / constants --------
inline constexpr const char* kService      = "urm";

// ContextualClassifierInit.cpp (Init trace you added):
inline constexpr const char* kInitEnter    = "InitTrace: ENTER module init";
inline constexpr const char* kInitCall     = "InitTrace: calling cc_init()";

// ContextualClassifier.cpp (Init path):
inline constexpr const char* kInitOk1      = "Classifier module init.";
inline constexpr const char* kInitOk2      = "Now listening for process events.";

// ContextualClassifier.cpp (worker consumes events):
inline const std::regex kClassifyStartRe(
    R"(ClassifyProcess:\s*Starting classification for PID:\s*\d+)",
    std::regex::icase);

// MLInference.cpp (FastText):
inline constexpr const char* kFtModelLoaded = "fastText model loaded. Embedding dimension:";
inline constexpr const char* kFtPredictDone = "Prediction complete. PID:";

// URM cgroup move logs:
inline constexpr const char* kCgrpWriteTry  = "moveProcessToCGroup: Writing to Node:";
inline constexpr const char* kCgrpWriteFail = "moveProcessToCGroup: Call to write, Failed";

// Time windows (tuned for your box)
inline constexpr int kSinceSeconds = 240;   // query last N seconds of journald
inline constexpr int kSettleMs     = 1800;  // wait after service restart
inline constexpr int kEventWaitMs  = 1500;  // wait after spawning an event

// -------- Shell helpers --------
struct CmdResult {
    int exit_code{-1};
    std::string out;
};

// Run a shell command, capture stdout, and return exit code + output.
// Stderr redirected to /dev/null to keep logs stable.
static inline CmdResult run_cmd(const std::string& cmd) {
    CmdResult cr;
    std::array<char, 4096> buf{};
    std::string full = cmd + " 2>/dev/null";
    if (FILE* fp = popen(full.c_str(), "r")) {
        while (size_t n = fread(buf.data(), 1, buf.size(), fp)) {
            cr.out.append(buf.data(), n);
        }
        cr.exit_code = pclose(fp);
    } else {
        cr.exit_code = -1;
    }
    return cr;
}

static inline void spawn_short_process() {
    (void)run_cmd("bash -lc 'sleep 1 & disown'");
}

// -------- Service / journald helpers --------
static inline bool service_restart() {
    (void)run_cmd("systemctl daemon-reload");
    auto rc = run_cmd(std::string("systemctl restart ") + kService);
    return (rc.exit_code == 0);
}

static inline std::string read_journal_last_seconds(int seconds) {
    // Try unprivileged read first
    {
        std::string cmd = "journalctl -u " + std::string(kService) +
                          " --since \"-" + std::to_string(seconds) + "s\" -o short-iso";
        CmdResult r = run_cmd(cmd);
        if (!r.out.empty()) return r.out;
    }
    // Fallback with sudo
    {
        std::string cmd = "sudo journalctl -u " + std::string(kService) +
                          " --since \"-" + std::to_string(seconds) + "s\" -o short-iso";
        return run_cmd(cmd).out;
    }
}

// -------- Log search helpers --------
static inline bool contains_line(const std::string& logs, const char* needle) {
    return logs.find(needle) != std::string::npos;
}

static inline bool contains_regex(const std::string& logs, const std::regex& re) {
    return std::regex_search(logs, re);
}

} // namespace test_helpers

#endif // TEST_HELPERS_HPP_INCLUDED
