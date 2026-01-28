// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "AuxRoutines.h"
#include "FeatureExtractor.h"
#include "FeaturePruner.h"
#include "Logger.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdarg>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#define PRUNED_DIR "/var/cache/pruned"
#define UNFILTERED_DIR "/var/cache/unfiltered"
#define SCANNER_TAG "FeatureExtractor"
#define LOG_LINES 20

std::unordered_map<std::string, std::unordered_set<std::string>> FeatureExtractor::mTokenIgnoreMap;

static std::string format_string(const char *fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return std::string(buffer);
}

bool FeatureExtractor::IsValidPidViaProc(pid_t pid) {
    std::string procPath = "/proc/" + std::to_string(pid);
    struct stat info;
    return (stat(procPath.c_str(), &info) == 0 && S_ISDIR(info.st_mode));
}

std::string join_vector(const std::vector<std::string> &vec) {
    std::stringstream ss;
    for (const auto &s : vec) {
        ss << s << " ";
    }
    std::string res = ss.str();
    if (!res.empty())
        res.pop_back();
    return res;
}

int FeatureExtractor::CollectAndStoreData( pid_t pid,
    std::map<std::string, std::string> &output_data, bool dump_csv) {
    if (!IsValidPidViaProc(pid)) {
        LOGE(SCANNER_TAG,
             format_string("PID %d does not exist in /proc.", pid));
        return 1;
    }

    std::string delimiters = ".:";
    std::vector<std::string> context = ParseAttrCurrent(pid, delimiters);
    std::vector<std::string> lowerContext =
        FeaturePruner::toLowercaseVector(context);
    auto filtered_context = FeaturePruner::filterStrings(
        lowerContext, mTokenIgnoreMap.count("attr")
                          ? mTokenIgnoreMap.at("attr")
                          : std::unordered_set<std::string>());

    delimiters = ":\"/";
    std::vector<std::string> cgroup = ParseCgroup(pid, delimiters);
    std::vector<std::string> lowercgroup =
        FeaturePruner::toLowercaseVector(cgroup);
    auto filtered_cg = FeaturePruner::filterStrings(
        lowercgroup, mTokenIgnoreMap.count("cgroup")
                         ? mTokenIgnoreMap.at("cgroup")
                         : std::unordered_set<std::string>());
    FeaturePruner::normalize_numbers_inplace(filtered_cg);

    auto t1 = std::chrono::high_resolution_clock::now();
    delimiters = ".=/!";
    std::vector<std::string> cmdline = ParseCmdline(pid, delimiters);
    auto t2 = std::chrono::high_resolution_clock::now();
    LOGD(SCANNER_TAG,
         format_string(
             "cmdline took %f ms",
             std::chrono::duration<double, std::milli>(t2 - t1).count()));

    std::vector<std::string> lowercmdline =
        FeaturePruner::toLowercaseVector(cmdline);
    auto filtered_cmd = FeaturePruner::filterStrings(
        lowercmdline, mTokenIgnoreMap.count("cmdline")
                          ? mTokenIgnoreMap.at("cmdline")
                          : std::unordered_set<std::string>());
    FeaturePruner::removeDoubleDash(filtered_cmd);

    delimiters = ".";
    std::vector<std::string> comm = ParseComm(pid, delimiters);
    std::vector<std::string> lowercomm = FeaturePruner::toLowercaseVector(comm);
    auto filtered_comm = FeaturePruner::filterStrings(
        lowercomm, mTokenIgnoreMap.count("comm") ? mTokenIgnoreMap.at("comm")
                                           : std::unordered_set<std::string>());
    FeaturePruner::normalize_numbers_inplace(filtered_comm);

    t1 = std::chrono::high_resolution_clock::now();
    delimiters = "/()_:.";
    std::vector<std::string> maps = ParseMapFiles(pid, delimiters);
    t2 = std::chrono::high_resolution_clock::now();
    LOGD(SCANNER_TAG,
         format_string(
             "maps took %f ms",
             std::chrono::duration<double, std::milli>(t2 - t1).count()));

    std::vector<std::string> lowermaps = FeaturePruner::toLowercaseVector(maps);
    auto filtered_maps = FeaturePruner::filterStrings(
        lowermaps, mTokenIgnoreMap.count("map_files")
                       ? mTokenIgnoreMap.at("map_files")
                       : std::unordered_set<std::string>());
    FeaturePruner::normalize_numbers_inplace(filtered_maps);

    t1 = std::chrono::high_resolution_clock::now();
    delimiters = ":[]/()=";
    std::vector<std::string> fds = ParseFd(pid, delimiters);
    t2 = std::chrono::high_resolution_clock::now();
    LOGD(SCANNER_TAG,
         format_string(
             "fds took %f ms",
             std::chrono::duration<double, std::milli>(t2 - t1).count()));

    std::vector<std::string> lowerfds = FeaturePruner::toLowercaseVector(fds);
    auto filtered_fds = FeaturePruner::filterStrings(
        lowerfds, mTokenIgnoreMap.count("fds") ? mTokenIgnoreMap.at("fds")
                                         : std::unordered_set<std::string>());

    t1 = std::chrono::high_resolution_clock::now();
    delimiters = "=@;!-._/:, ";
    std::vector<std::string> environ = ParseEnviron(pid, delimiters);
    t2 = std::chrono::high_resolution_clock::now();
    LOGD(SCANNER_TAG,
         format_string(
             "environ took %f ms",
             std::chrono::duration<double, std::milli>(t2 - t1).count()));

    std::vector<std::string> lowerenviron =
        FeaturePruner::toLowercaseVector(environ);
    auto filtered_environ = FeaturePruner::filterStrings(
        lowerenviron, mTokenIgnoreMap.count("environ")
                          ? mTokenIgnoreMap.at("environ")
                          : std::unordered_set<std::string>());
    FeaturePruner::normalize_numbers_inplace(filtered_environ);

    delimiters = "/.";
    std::vector<std::string> exe = ParseExe(pid, delimiters);
    std::vector<std::string> lowerexe = FeaturePruner::toLowercaseVector(exe);
    auto filtered_exe = FeaturePruner::filterStrings(
        lowerexe, mTokenIgnoreMap.count("exe") ? mTokenIgnoreMap.at("exe")
                                         : std::unordered_set<std::string>());
    FeaturePruner::normalize_numbers_inplace(filtered_exe);

    delimiters = "=!'&/.,:- ";
    t1 = std::chrono::high_resolution_clock::now();
    auto journalctl_logs = ReadJournalForPid(pid, LOG_LINES);
    if (journalctl_logs.empty()) {
        LOGD(SCANNER_TAG, format_string("No logs found for PID %d", pid));
    }
    t2 = std::chrono::high_resolution_clock::now();
    LOGD(SCANNER_TAG,
         format_string(
             "journalctl took %f ms",
             std::chrono::duration<double, std::milli>(t2 - t1).count()));

    auto extracted_Logs = ExtractProcessNameAndMessage(journalctl_logs);
    std::vector<std::string> logs;
    for (const auto &entry : extracted_Logs) {
        auto tokens = ParseLog(entry, delimiters);
        for (const auto &c : tokens) {
            logs.push_back(c);
        }
    }

    std::vector<std::string> lowerlogs = FeaturePruner::toLowercaseVector(logs);
    auto filtered_logs = FeaturePruner::filterStrings(
        lowerlogs, mTokenIgnoreMap.count("logs") ? mTokenIgnoreMap.at("logs")
                                           : std::unordered_set<std::string>());
    FeaturePruner::removeDoubleQuotes(filtered_logs);

    output_data["attr"] = join_vector(filtered_context);
    output_data["cgroup"] = join_vector(filtered_cg);
    output_data["cmdline"] = join_vector(filtered_cmd);
    output_data["comm"] = join_vector(filtered_comm);
    output_data["maps"] = join_vector(filtered_maps);
    output_data["fds"] = join_vector(filtered_fds);
    output_data["environ"] = join_vector(filtered_environ);
    output_data["exe"] = join_vector(filtered_exe);
    output_data["logs"] = join_vector(filtered_logs);

    // Dump csv only if dump_csv
    if (!dump_csv)
        return 0;

    std::string prunedFolder = PRUNED_DIR;
    std::string unfilteredFolder = UNFILTERED_DIR;

    if(!AuxRoutines::fileExists(prunedFolder)) {
        mkdir(prunedFolder.c_str(), 0755);
        LOGI(SCANNER_TAG,
             format_string("New folder created: %s", prunedFolder.c_str()));
    }

    if(!AuxRoutines::fileExists(unfilteredFolder)) {
        mkdir(unfilteredFolder.c_str(), 0755);
    }

    std::string processName = comm.empty() ? "unknown_process" : comm[0];
    for (size_t i = 0; i < processName.size(); ++i) {
        processName[i] = std::tolower(processName[i]);
    }
    std::string fileName =
        processName + "_" + std::to_string(pid) + "_proc_info.csv";
    LOGD(SCANNER_TAG, format_string("FileName: %s", fileName.c_str()));

    // Unfiltered CSV
    std::string unfilteredFile =
        unfilteredFolder + "/" + fileName + "_unfiltered.csv";
    std::ofstream unfilteredCSV(unfilteredFile);
    if (!unfilteredCSV.is_open()) {
        LOGE(SCANNER_TAG, format_string("Failed to open unfiltered file: %s",
                                        unfilteredFile.c_str()));
    } else {
        unfilteredCSV
            << "PID,attr,cgroup,cmdline,comm,maps,fds,environ,exe,logs\n"
            << pid;
        // ... (CSV writing logic)
        // I will copy the CSV writing logic.
        auto write_csv_field = [&](const std::vector<std::string> &vec) {
            unfilteredCSV << ",\"";
            for (size_t i = 0; i < vec.size(); ++i) {
                unfilteredCSV << vec[i];
                if (i != vec.size() - 1)
                    unfilteredCSV << ",";
            }
            unfilteredCSV << "\"";
        };

        write_csv_field(lowerContext);
        write_csv_field(lowercgroup);  // cgroup
        write_csv_field(lowercmdline); // cmdline
        write_csv_field(lowercomm);    // comm
        write_csv_field(lowermaps);    // maps
        write_csv_field(lowerfds);     // fds

        // environ needs escaping quotes
        unfilteredCSV << ",\"";
        for (size_t i = 0; i < lowerenviron.size(); ++i) {
            for (char ch : lowerenviron[i]) {
                if (ch == '"')
                    unfilteredCSV << "\"\"";
                else
                    unfilteredCSV << ch;
            }
            if (i != lowerenviron.size() - 1)
                unfilteredCSV << ",";
        }
        unfilteredCSV << "\"";

        write_csv_field(lowerexe);  // exe
        write_csv_field(lowerlogs); // logs
        unfilteredCSV << "\n";
        unfilteredCSV.close();
    }

    // Filtered CSV
    std::string filteredFile = prunedFolder + "/" + fileName + "_filtered.csv";
    std::ofstream filteredCSV(filteredFile);
    if (!filteredCSV.is_open()) {
        LOGE(SCANNER_TAG, format_string("Failed to open filtered file: %s",
                                        filteredFile.c_str()));
    } else {
        filteredCSV
            << "PID,attr,cgroup,cmdline,comm,maps,fds,environ,exe,logs\n"
            << pid;
        auto write_filtered_field = [&](const std::vector<std::string> &vec) {
            filteredCSV << ",\"";
            for (size_t i = 0; i < vec.size(); ++i) {
                filteredCSV << vec[i];
                if (i != vec.size() - 1)
                    filteredCSV << ",";
            }
            filteredCSV << "\"";
        };
        write_filtered_field(filtered_context);
        write_filtered_field(filtered_cg);
        write_filtered_field(filtered_cmd);
        write_filtered_field(filtered_comm);
        write_filtered_field(filtered_maps);
        write_filtered_field(filtered_fds);

        // environ escaping
        filteredCSV << ",\"";
        for (size_t i = 0; i < filtered_environ.size(); ++i) {
            for (char ch : filtered_environ[i]) {
                if (ch == '"')
                    filteredCSV << "\"\"";
                else
                    filteredCSV << ch;
            }
            if (i != filtered_environ.size() - 1)
                filteredCSV << ",";
        }
        filteredCSV << "\"";

        write_filtered_field(filtered_exe);
        write_filtered_field(filtered_logs);
        filteredCSV << "\n";
        filteredCSV.close();
    }

    return 0;
}

std::vector<std::string>
FeatureExtractor::ParseAttrCurrent(const uint32_t pid,
                                   const std::string &delimiters) {
    std::vector<std::string> context_parts;
    std::string path = "/proc/" + std::to_string(pid) + "/attr/current";
    std::ifstream infile(path);
    if (!infile.is_open()) {
        LOGE(SCANNER_TAG, format_string("Failed to open %s", path.c_str()));
        return context_parts;
    }
    std::string line;
    if (std::getline(infile, line)) {
        line = std::regex_replace(line, std::regex(R"(\s*\(enforce\))"), "");
        context_parts = FeaturePruner::splitString(line, delimiters);
    }
    return context_parts;
}

std::vector<std::string>
FeatureExtractor::ParseCgroup(pid_t pid, const std::string &delimiters) {
    std::vector<std::string> tokens;
    std::string path = "/proc/" + std::to_string(pid) + "/cgroup";
    std::ifstream infile(path);
    if (!infile.is_open()) {
        LOGE(SCANNER_TAG, format_string("Failed to open %s", path.c_str()));
        return tokens;
    }
    std::string line;
    while (std::getline(infile, line)) {
        std::vector<std::string> lineTokens =
            FeaturePruner::splitString(line, delimiters);
        tokens.insert(tokens.end(), lineTokens.begin(), lineTokens.end());
    }
    return tokens;
}

std::vector<std::string>
FeatureExtractor::ParseCmdline(pid_t pid, const std::string &delimiters) {
    std::vector<std::string> tokens;
    std::string path = "/proc/" + std::to_string(pid) + "/cmdline";
    std::ifstream infile(path, std::ios::binary);
    if (!infile.is_open()) {
        LOGE(SCANNER_TAG, format_string("Failed to open %s", path.c_str()));
        return tokens;
    }
    std::string content((std::istreambuf_iterator<char>(infile)),
                        std::istreambuf_iterator<char>());
    size_t start = 0;
    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '\0') {
            if (i > start) {
                std::string arg = content.substr(start, i - start);
                for (const auto &raw :
                     FeaturePruner::splitString(arg, delimiters)) {
                    std::string cleaned;
                    for (char c : raw) {
                        if (delimiters.find(c) == std::string::npos) {
                            cleaned += c;
                        }
                    }
                    cleaned = FeaturePruner::trim(cleaned);
                    if (!cleaned.empty() &&
                        !FeaturePruner::isDigitsOnly(cleaned) &&
                        cleaned.size() > 1) {
                        tokens.push_back(cleaned);
                    }
                }
            }
            start = i + 1;
        }
    }
    return tokens;
}

std::vector<std::string>
FeatureExtractor::ParseComm(pid_t pid, const std::string &delimiters) {
    std::vector<std::string> tokens;
    std::string path = "/proc/" + std::to_string(pid) + "/comm";
    std::ifstream infile(path);
    if (!infile.is_open()) {
        LOGE(SCANNER_TAG, format_string("Failed to open %s", path.c_str()));
        return tokens;
    }
    std::string comm;
    std::getline(infile, comm);
    for (const auto &t : FeaturePruner::splitString(comm, delimiters)) {
        std::string cleaned = FeaturePruner::trim(t);
        if (!cleaned.empty() && cleaned.size() > 1) {
            tokens.push_back(cleaned);
        }
    }
    return tokens;
}

std::vector<std::string>
FeatureExtractor::ParseMapFiles(pid_t pid, const std::string &delimiters) {
    std::vector<std::string> results;
    std::string dir_path = "/proc/" + std::to_string(pid) + "/map_files";
    DIR *dir = opendir(dir_path.c_str());
    if (!dir) {
        LOGE(SCANNER_TAG, format_string("Failed to open %s", dir_path.c_str()));
        return results;
    }
    struct dirent *entry;
    char link_target[PATH_MAX];
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.')
            continue;
        std::string link_path = dir_path + "/" + entry->d_name;
        ssize_t len =
            readlink(link_path.c_str(), link_target, sizeof(link_target) - 1);
        if (len != -1) {
            link_target[len] = '\0';
            std::string target_str(link_target);
            for (const auto &tok :
                 FeaturePruner::splitString(target_str, delimiters)) {
                std::string simplified =
                    FeaturePruner::normalizeLibraryName(tok);
                if (simplified.empty() || simplified.size() <= 1)
                    continue;
                if (FeaturePruner::isDigitsOnly(simplified))
                    continue;
                if (std::find(results.begin(), results.end(), simplified) ==
                    results.end()) {
                    results.push_back(simplified);
                }
            }
        }
    }
    closedir(dir);
    return results;
}

std::vector<std::string>
FeatureExtractor::ParseFd(pid_t pid, const std::string &delimiters) {
    std::vector<std::string> results;
    std::string dir_path = "/proc/" + std::to_string(pid) + "/fd";
    DIR *dir = opendir(dir_path.c_str());
    if (!dir) {
        LOGE(SCANNER_TAG,
             format_string("Unable to open fd directory %s", dir_path.c_str()));
        return results;
    }
    struct dirent *entry;
    char link_target[PATH_MAX];
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.')
            continue;
        std::string link_path = dir_path + "/" + entry->d_name;
        ssize_t len =
            readlink(link_path.c_str(), link_target, sizeof(link_target) - 1);
        if (len != -1) {
            link_target[len] = '\0';
            std::string target_str(link_target);
            std::vector<std::string> tokens =
                FeaturePruner::splitString(target_str, delimiters);
            for (const auto &tok : tokens) {
                if (tok.empty())
                    continue;
                std::string cleaned =
                    FeaturePruner::removeDatesAndTimesFromToken(tok);
                if (cleaned.empty())
                    continue;
                bool isNumber =
                    std::all_of(cleaned.begin(), cleaned.end(), ::isdigit);
                if (isNumber)
                    continue;
                if (std::find(results.begin(), results.end(), cleaned) ==
                    results.end()) {
                    results.push_back(cleaned);
                }
            }
        }
    }
    closedir(dir);
    return results;
}

std::vector<std::string>
FeatureExtractor::ParseEnviron(pid_t pid, const std::string &delimiters) {
    std::vector<std::string> out;
    std::string path = "/proc/" + std::to_string(pid) + "/environ";
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        LOGE(SCANNER_TAG, format_string("Failed to open: %s", path.c_str()));
        return out;
    }
    std::string entry;
    while (std::getline(in, entry, '\0')) {
        if (entry.empty())
            continue;
        std::vector<std::string> tokens =
            FeaturePruner::splitString(entry, delimiters);
        for (auto &token : tokens) {
            for (size_t i = 0; i < token.size();) {
                if (delimiters.find(token[i]) != std::string::npos) {
                    token.erase(i, 1);
                } else {
                    ++i;
                }
            }
            if (FeaturePruner::isAllSpecialChars(token))
                continue;
            if (!token.empty() && !FeaturePruner::hasDigit(token)) {
                out.push_back(token);
            }
        }
    }
    return out;
}

std::vector<std::string>
FeatureExtractor::ParseExe(pid_t pid, const std::string &delimiters) {
    std::vector<std::string> out;
    std::string path = "/proc/" + std::to_string(pid) + "/exe";
    char buf[PATH_MAX];
    ssize_t len = readlink(path.c_str(), buf, sizeof(buf) - 1);
    if (len != -1) {
        buf[len] = '\0';
        std::string exePath(buf);
        for (const auto &part :
             FeaturePruner::splitString(exePath, delimiters)) {
            if (!FeaturePruner::isDigitsOnly(part)) {
                out.push_back(part);
            }
        }
    } else {
        LOGE(SCANNER_TAG, format_string("Failed to readlink %s for PID %d",
                                        path.c_str(), pid));
    }
    return out;
}

std::vector<std::string>
FeatureExtractor::ReadJournalForPid(pid_t pid, uint32_t numLines) {
    std::vector<std::string> lines;
    std::string comm;
    std::ifstream commFile("/proc/" + std::to_string(pid) + "/comm");
    if (commFile.is_open()) {
        std::getline(commFile, comm);
        commFile.close();
    } else {
        LOGE(SCANNER_TAG, format_string("Failed to open /proc/%d/comm", pid));
        return lines;
    }
    std::string cmd = "journalctl --no-pager -n " + std::to_string(numLines) +
                      " _COMM=" + comm;
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        LOGE(SCANNER_TAG, format_string("popen failed: %s", strerror(errno)));
        return lines;
    }
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        lines.emplace_back(buffer);
    }
    int rc = pclose(pipe);
    if (rc == -1) {
        LOGE(SCANNER_TAG, format_string("pclose failed: %s", strerror(errno)));
    }
    return lines;
}

std::vector<std::string>
FeatureExtractor::ParseLog(const std::string &input,
                           const std::string &delimiters) {
    std::vector<std::string> tokens;
    std::string token;
    std::unordered_set<char> delimSet(delimiters.begin(), delimiters.end());
    std::regex bracketedTag(R"(\[\s*(info|warn|error|debug|trace)?\s*\]?)",
                            std::regex_constants::icase);
    std::string cleaned_input = std::regex_replace(input, bracketedTag, "");
    cleaned_input.erase(
        std::remove(cleaned_input.begin(), cleaned_input.end(), '\n'),
        cleaned_input.end());
    for (char ch : cleaned_input) {
        if (delimSet.count(ch)) {
            if (!token.empty()) {
                token = FeaturePruner::removePunctuation(token);
                if (!token.empty() &&
                    !FeaturePruner::isSingleCharToken(token) &&
                    !FeaturePruner::isDigitsOnly(token)) {
                    tokens.push_back(token);
                }
            }
            token.clear();
        } else {
            token += ch;
        }
    }
    if (!token.empty()) {
        token = FeaturePruner::removePunctuation(token);
        if (!token.empty() && !FeaturePruner::isSingleCharToken(token) &&
            !FeaturePruner::isDigitsOnly(token)) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

std::vector<std::string> FeatureExtractor::ExtractProcessNameAndMessage(
    const std::vector<std::string> &journalLines) {
    std::vector<std::string> filtered;
    std::regex pattern(R"(.*? (\S+)\[(\d+)\]: (.*))");
    for (const auto &line : journalLines) {
        std::smatch match;
        if (std::regex_search(line, match, pattern) && match.size() == 4) {
            std::string processName = match[1];
            std::string message = match[3];
            filtered.push_back(processName + ": " + message);
        }
    }
    return filtered;
}
