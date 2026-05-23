// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef FEATURE_PRUNER_H
#define FEATURE_PRUNER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

class FeaturePruner {
public:
	static std::string trim(const std::string &s);
	static std::vector<std::string> splitString(const std::string &input,
												const std::string &delimiters);

	static std::string normalizeLibraryName(const std::string &s);

	static std::vector<std::string>
	filterStrings(const std::vector<std::string> &input,
				  const std::unordered_set<std::string> &ignoreSet);

	static void removeDoubleQuotes(std::vector<std::string> &vec);

	static std::vector<std::string>
	toLowercaseVector(const std::vector<std::string> &input);

	static void removeDoubleDash(std::vector<std::string> &vec);

	static void normalize_numbers_inplace(std::vector<std::string> &tokens);

	static int8_t isDigitsOnly(const std::string &str);

	static int8_t hasDigit(const std::string &str);

	static int8_t isAllSpecialChars(const std::string &token);

	static std::string removeDatesAndTimesFromToken(const std::string &input);

	static std::string removePunctuation(const std::string &s);

	static int8_t isSingleCharToken(const std::string &s);

	static std::unordered_map<std::string, std::unordered_set<std::string>>
	loadIgnoreMap(const std::string &filename,
				  const std::vector<std::string> &labels);
};

#endif // FEATURE_PRUNER_H
