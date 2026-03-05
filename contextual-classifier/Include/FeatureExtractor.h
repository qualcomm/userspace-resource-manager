// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include <cstdint>
#include <map>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "FeaturePruner.h"

#define CLASSIFIER_CONF_DIR "/etc/urm/classifier/"
const std::string IGNORE_TOKENS_PATH = CLASSIFIER_CONF_DIR "ignore-tokens.txt";

class FeatureExtractor {
private:
	static std::unordered_map<std::string, std::unordered_set<std::string>> mTokenIgnoreMap;

	static std::vector<std::string> ParseAttrCurrent(const uint32_t pid,
		                                             const std::string &delimiters);

	static std::vector<std::string> ParseCgroup(pid_t pid,
												const std::string &delimiters);

	static std::vector<std::string> ParseCmdline(pid_t pid,
												 const std::string &delimiters);

	static std::vector<std::string> ParseComm(pid_t pid,
											  const std::string &delimiters);

	static std::vector<std::string> ParseMapFiles(pid_t pid,
		                                          const std::string &delimiters);

	static std::vector<std::string> ParseFd(pid_t pid,
											const std::string &delimiters);

	static std::vector<std::string> ParseEnviron(pid_t pid,
												 const std::string &delimiters);

	static std::vector<std::string> ParseExe(pid_t pid,
											 const std::string &delimiters);

	static std::vector<std::string> ReadJournalForPid(pid_t pid,
													  uint32_t numLines);

	static std::vector<std::string> ParseLog(const std::string &input,
											 const std::string &delimiters);

	static std::vector<std::string>
	ExtractProcessNameAndMessage(const std::vector<std::string> &journalLines);

	static int8_t IsValidPidViaProc(pid_t pid);

public:
	FeatureExtractor() {
		std::vector<std::string> labels = {
			"attr", "cgroup",  "cmdline",
			"comm", "environ", "exe",
			"logs", "fds", "map_files"
		};
		mTokenIgnoreMap = FeaturePruner::loadIgnoreMap(IGNORE_TOKENS_PATH, labels);
	}

	static int CollectAndStoreData(
		pid_t pid,
		std::map<std::string, std::string> &output_data,
		int8_t dump_csv);
};

#endif // FEATURE_EXTRACTOR_H
