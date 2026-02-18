// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "URMTests.h"

uint32_t TestAggregator::mTestsCount = 0;
std::vector<URMTest> TestAggregator::mTests {};
std::vector<std::string> TestAggregator::mFailed {};

TestAggregator::TestAggregator(URMTestCase testCallback, const std::string& tag) {
    mTests.push_back({testCallback, tag});
}

std::vector<URMTest> TestAggregator::getAllTests() {
    return mTests;
}

void TestAggregator::addFail(const std::string& name) {
    mFailed.push_back(name);
}

int32_t TestAggregator::runAll(const std::string& name) {
    for(URMTest test: mTests) {
        mTestsCount++;
        if(test.second == name) {
            test.first();
        }
    }

    std::cout<<"\nSummary:"<<std::endl;
    std::cout<<"Ran "<<mTestsCount<<" Test Cases"<<std::endl;

    uint32_t failCount = (uint32_t)mFailed.size();
    uint32_t passCount = mTestsCount - failCount;
    std::cout<<"Pass: "<<passCount<<std::endl;
    std::cout<<"Fail: "<<failCount<<std::endl;

    if(failCount > 0) {
        std::cout<<"\nFollowing Tests Failed: "<<std::endl;
        for(std::string& name: mFailed) {
            std::cout<<name<<std::endl;
        }
        return 1;
    }

    return 0;
}
