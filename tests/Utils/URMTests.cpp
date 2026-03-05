// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "URMTests.h"
#include <sstream>
#include <fstream>

#include <algorithm>

#define TESTS_LOG_HTML "tests_report.html"
#define GET_TEST_KEY(name, category) category + "#" + name

uint32_t TestAggregator::mTestsCount = 0;
int32_t TestAggregator::mFailCount = 0;
int32_t TestAggregator::mSkipCount = 0;
int32_t TestAggregator::mPassCount = 0;

std::map<std::string, URMTest> TestAggregator::mTests {};

TestAggregator::TestAggregator(URMTestCase testCb,
                               const std::string& name,
                               const std::string& testCat,
                               const std::string& tag) {
    URMTest test = {
        .testCallback = testCb,
        .classLabel = tag,
        .status = TestStatus::NOT_RUN,
        .comment = "",
        .flags = 0,
    };
    mTests[GET_TEST_KEY(name, testCat)] = test;
}

void TestAggregator::addPass(const std::string& name,
                             const std::string& testCat) {
    mTests[GET_TEST_KEY(name, testCat)].status = TestStatus::PASSED;
    mPassCount++;
}

void TestAggregator::addFail(const std::string& name,
                             const std::string& testCat) {
    mTests[GET_TEST_KEY(name, testCat)].status = TestStatus::FAILED;
    mFailCount++;
}

void TestAggregator::addSkip(const std::string& name,
                             const std::string& testCat) {
    mTests[GET_TEST_KEY(name, testCat)].status = TestStatus::SKIPPED;
    mSkipCount++;
}

int32_t TestAggregator::runAll(const std::string& name) {
    mPassCount = mFailCount = mSkipCount = 0;

    // Run all the test cases in the given class.
    for(std::pair<std::string, URMTest> test: mTests) {
        mTestsCount++;
        if(test.second.classLabel == name) {
            test.second.testCallback();
        }
    }

    std::ofstream fileStream(TESTS_LOG_HTML, std::ios::out | std::ios::trunc);
    if(fileStream.is_open()) {
        fileStream<<"<h1>Tests Summary</h1>"<<std::endl;
        fileStream<<"<h3>Total: "<<mTestsCount<<"</h3>"<<std::endl;
        fileStream<<"<h3>Passed: "<<mPassCount<<"</h3>"<<std::endl;
        fileStream<<"<h3>Failed: "<<mFailCount<<"</h3>"<<std::endl;
        fileStream<<"<h3>Skipped: "<<mSkipCount<<"</h3>"<<std::endl;

        fileStream<<std::endl;
        fileStream<<"<table border=\"1px solid\">"<<std::endl;

        std::string header = "\
            <thead> \
                <tr>    \
                <th style=\"padding: 5px;\">Category</th>   \
                <th style=\"padding: 5px;\">TestCase</th>   \
                <th style=\"padding: 5px;\">Result</th> \
                </tr>   \
            </thead>    \
        ";

        fileStream<<header<<std::endl;
        fileStream<<"<tbody>"<<std::endl;
        for(std::pair<std::string, URMTest> test: mTests) {
            fileStream<<"<tr>"<<std::endl;

            std::string curToken;
            std::vector<std::string> tokens;
            std::istringstream nameStream(test.first);
            while(std::getline(nameStream, curToken, '#')) {
                tokens.push_back(curToken);
            }

            if(tokens.size() < 2) {
                continue;
            }

            fileStream<<"<td style=\"padding: 5px;\"><b>"<<tokens[0]<<"</b></td>"<<std::endl;
            fileStream<<"<td style=\"padding: 5px;\"><b>"<<tokens[1]<<"</b></td>"<<std::endl;
            if(test.second.status == TestStatus::FAILED) {
                fileStream<<"<td style=\"padding: 5px; color:red\">Failed</td>"<<std::endl;
            } else if(test.second.status == TestStatus::SKIPPED) {
                fileStream<<"<td style=\"padding: 5px; color:orange\">Skipped</td>"<<std::endl;
            } else {
                fileStream<<"<td style=\"padding: 5px; color:green\">Passed</td>"<<std::endl;
            }
            fileStream<<"</tr>"<<std::endl;
        }
        fileStream<<"</tbody>"<<std::endl;
        fileStream<<"</table>"<<std::endl;
        fileStream.close();
    }

    // Log essential information to console as well
    std::cout<<"\nSummary:"<<std::endl;
    std::cout<<"Ran "<<mTestsCount<<" Test Cases"<<std::endl;
    std::cout<<"Pass: "<<mPassCount<<std::endl;
    std::cout<<"Fail: "<<mFailCount<<std::endl;
    std::cout<<"Skip: "<<mSkipCount<<std::endl;

    if(mFailCount > 0) {
        return 1;
    }

    return 0;
}
