// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef TEST_BASELINE_H
#define TEST_BASELINE_H

#include <unordered_map>
#include <stack>

#include "YamlParser.h"
#include "UrmSettings.h"

#define TEST_ROOT "TestConfigs"
#define TARGET_NAME_LIST "TargetName"
#define CLUSTER_EXPECTATIONS "ClusterExpectations"
#define TARGET_CLUSTER_INFO_LOGICAL_ID "LgcId"
#define TARGET_CLUSTER_INFO_PHYSICAL_ID "PhyId"
#define NUM_CLUSERS "NumClusters"
#define NUM_CORES "NumCores"

const static std::string baselineYamlFilePath = "/etc/urm/tests/configs/Baseline.yaml";

typedef struct {
    int32_t mLogicalID;
    int32_t mPhysicalID;
} ClusterExpection;

class ClusterExpectationBuilder {
private:
    ClusterExpection* mClusterExpectation;

public:
    ClusterExpectationBuilder();

    ErrCode setLogicalID(const std::string& logicalIDString);
    ErrCode setPhysicalID(const std::string& physicalIDString);

    ClusterExpection* build();
};

class TestBaseline {
private:
    std::unordered_map<int32_t, int32_t> mLogicalToPhysicalClusterMapping;
    int32_t mTotalClusterCount;
    int32_t mTotalCoreCount;

    ErrCode parseTestConfigYamlNode(const std::string& filePath);

public:
    TestBaseline(): mTotalClusterCount(-1), mTotalCoreCount(-1) {}

    ErrCode fetchBaseline();
    int32_t getExpectedClusterCount();
    int32_t getExpectedCoreCount();
    int32_t getExpectedPhysicalCluster(int32_t logicalID);

    void displayBaseline();
};

#endif
