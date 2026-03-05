// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "FeaturePruner.h"
#include "Logger.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

static std::string escapeForCharClass(const std::string &d) {
    std::string out;
    out.reserve(d.size() * 2);
    for (char c : d) {
        switch (c) {
        case '\\':
        case ']':
        case '^':
        case '-':
            out.push_back('\\');
            [[fallthrough]];
        default:
            out.push_back(c);
        }
    }
    return out;
}

std::vector<std::string>
FeaturePruner::splitString(const std::string &input,
                           const std::string &delimiters) {
    std::string regexPattern = "[" + escapeForCharClass(delimiters) + "]";
    std::regex re(regexPattern);
    std::sregex_token_iterator it(input.begin(), input.end(), re, -1);
    std::sregex_token_iterator end;
    std::vector<std::string> result;
    for (; it != end; ++it) {
        if (!it->str().empty()) {
            result.push_back(it->str());
        }
    }
    return result;
}

std::string FeaturePruner::trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::string FeaturePruner::normalizeLibraryName(const std::string &s) {
    std::string result = trim(s);
    if (result.empty())
        return result;
    size_t pos = result.find(".so");
    if (pos != std::string::npos) {
        result = result.substr(0, pos);
    }
    while (!result.empty() &&
           std::isdigit(static_cast<unsigned char>(result.back()))) {
        result.pop_back();
    }
    while (!result.empty() && (result.back() == '-' || result.back() == '_' ||
                               result.back() == '.')) {
        result.pop_back();
    }
    result = trim(result);
    if (result == "so")
        return "";
    return result;
}

std::string
FeaturePruner::removeDatesAndTimesFromToken(const std::string &input) {
    std::string out = input;
    static const std::regex date_numeric(
        R"((\b\d{1,2}[-/.]\d{1,2}[-/.]\d{2,4}\b)|(\b\d{4}[-/.]\d{1,2}[-/.]\d{1,2}\b))",
        std::regex::ECMAScript | std::regex::icase);
    static const std::regex date_month_name(
        R"(\b(?:(?:jan(?:uary)?|feb(?:ruary)?|mar(?:ch)?|apr(?:il)?|may|jun(?:e)?|jul(?:y)?|aug(?:ust)?|sep(?:t|tember)?|oct(?:ober)?|nov(?:ember)?|dec(?:ember)?))\s+\d{1,2}(?:,\s*)?\s+\d{2,4}\b|\b\d{1,2}\s+(?:jan(?:uary)?|feb(?:ruary)?|mar(?:ch)?|apr(?:il)?|may|jun(?:e)?|jul(?:y)?|aug(?:ust)?|sep(?:t|tember)?|oct(?:ober)?|nov(?:ember)?|dec(?:ember)?)(?:,\s*)?\s+\d{2,4}\b)",
        std::regex::ECMAScript | std::regex::icase);
    static const std::regex time_hm(R"(\b\d{1,2}:\d{2}(:\d{2})?\s*(AM|PM)?\b)",
                                    std::regex::ECMAScript | std::regex::icase);
    out = std::regex_replace(out, date_numeric, "");
    out = std::regex_replace(out, date_month_name, "");
    out = std::regex_replace(out, time_hm, "");
    out = std::regex_replace(out, std::regex(R"(\s{2,})"), " ");
    return out;
}

std::string FeaturePruner::removePunctuation(const std::string &s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(s[i]);
        if (!std::ispunct(ch)) {
            out.push_back(static_cast<char>(ch));
        }
    }
    return out;
}

int8_t FeaturePruner::isSingleCharToken(const std::string &s) {
    return s.size() == 1;
}

int8_t FeaturePruner::isAllSpecialChars(const std::string &token) {
    if (token.empty())
        return false;
    for (size_t i = 0; i < token.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(token[i]);
        if (std::isalnum(ch)) {
            return false;
        }
    }
    return true;
}

int8_t FeaturePruner::hasDigit(const std::string &str) {
    std::regex digitRegex("[0-9]");
    return std::regex_search(str, digitRegex);
}

int8_t FeaturePruner::isDigitsOnly(const std::string &str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
}

std::vector<std::string>
FeaturePruner::filterStrings(const std::vector<std::string> &input,
                             const std::unordered_set<std::string> &ignoreSet) {
    std::vector<std::string> result;
    std::copy_if(input.begin(), input.end(), std::back_inserter(result),
                 [&ignoreSet](const std::string &s) {
                     return ignoreSet.find(s) == ignoreSet.end();
                 });
    return result;
}

void FeaturePruner::removeDoubleQuotes(std::vector<std::string> &vec) {
    for (auto &str : vec) {
        str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());
    }
}

std::vector<std::string>
FeaturePruner::toLowercaseVector(const std::vector<std::string> &input) {
    std::vector<std::string> result = input;
    for (size_t i = 0; i < result.size(); ++i) {
        for (size_t j = 0; j < result[i].size(); ++j) {
            result[i][j] = std::tolower(result[i][j]);
        }
    }
    return result;
}

void FeaturePruner::removeDoubleDash(std::vector<std::string> &vec) {
    auto it = vec.begin();
    while (it != vec.end()) {
        if (*it == "--") {
            it = vec.erase(it);
        } else {
            size_t pos;
            while ((pos = it->find("--")) != std::string::npos) {
                it->erase(pos, 2);
            }
            ++it;
        }
    }
}

static const std::regex
    kUuidRe(R"(\b[0-9a-fA-F]{8}(?:-[0-9a-fA-F]{4}){3}-[0-9a-fA-F]{12}\b)");
static const std::regex kHexRunRe(R"(\b[0-9a-fA-F]{4,}\b)");
static const std::regex kDecRe(R"(\b[+-]?\d+\b)");

static std::string replace_numbers_and_hex_with_N(const std::string &in) {
    std::string s = std::regex_replace(in, kUuidRe, "n");
    s = std::regex_replace(s, kHexRunRe, "n");
    s = std::regex_replace(s, kDecRe, "n");
    return s;
}

void FeaturePruner::normalize_numbers_inplace(
    std::vector<std::string> &tokens) {
    for (auto &s : tokens) {
        s = replace_numbers_and_hex_with_N(s);
    }
}

std::unordered_map<std::string, std::unordered_set<std::string>>
FeaturePruner::loadIgnoreMap(const std::string &filename,
                             const std::vector<std::string> &labels) {
    std::ifstream file(filename);
    std::string line;
    std::unordered_map<std::string, std::unordered_set<std::string>> ignoreMap;

    if (!file.is_open()) {
        LOGE("FeaturePruner", "Error opening file: " + filename);
        return ignoreMap;
    }

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key, values;

        if (std::getline(iss, key, ':') && std::getline(iss, values)) {
            if (std::find(labels.begin(), labels.end(), key) != labels.end()) {
                std::istringstream valStream(values);
                std::string val;
                while (std::getline(valStream, val, ',')) {
                    val.erase(0, val.find_first_not_of(" \t\n\r"));
                    val.erase(val.find_last_not_of(" \t\n\r") + 1);
                    if (!val.empty()) {
                        ignoreMap[key].emplace(val);
                    }
                }
            }
        }
    }
    return ignoreMap;
}
