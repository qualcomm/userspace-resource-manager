// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "TestBaseline.h"
#include "TestUtils.h"

static int8_t isKey(const std::string& keyName) {
    if(keyName == TEST_ROOT) return true;
    if(keyName == TARGET_NAME_LIST) return true;
    if(keyName == CLUSTER_EXPECTATIONS) return true;
    if(keyName == TARGET_CLUSTER_INFO_LOGICAL_ID) return true;
    if(keyName == TARGET_CLUSTER_INFO_PHYSICAL_ID) return true;
    if(keyName == NUM_CLUSERS) return true;
    if(keyName == NUM_CORES) return true;

    return false;
}

ClusterExpectationBuilder::ClusterExpectationBuilder() {
    this->mClusterExpectation = new(std::nothrow) ClusterExpection;
    if(this->mClusterExpectation == nullptr) {
        return;
    }

    this->mClusterExpectation->mLogicalID = -1;
    this->mClusterExpectation->mPhysicalID = -1;
}

ErrCode ClusterExpectationBuilder::setLogicalID(const std::string& logicalIDString) {
    this->mClusterExpectation->mLogicalID = -1;
    try {
        this->mClusterExpectation->mLogicalID = std::stoi(logicalIDString);
        return RC_SUCCESS;

    } catch(const std::exception& e) {
        return RC_INVALID_VALUE;
    }

    return RC_INVALID_VALUE;
}

ErrCode ClusterExpectationBuilder::setPhysicalID(const std::string& physicalIDString) {
    this->mClusterExpectation->mPhysicalID = -1;
    try {
        this->mClusterExpectation->mPhysicalID = std::stoi(physicalIDString);
        return RC_SUCCESS;

    } catch(const std::exception& e) {
        return RC_INVALID_VALUE;
    }

    return RC_INVALID_VALUE;
}

ClusterExpection* ClusterExpectationBuilder::build() {
    return this->mClusterExpectation;
}

ErrCode TestBaseline::parseTestConfigYamlNode(const std::string& filePath) {
    SETUP_LIBYAML_PARSING(filePath);

    // Check if there exists a Target Config for this particular target in the Common Configs.
    // Skip this check if the BU has provided their own Target Configs
    std::string currTargetName =
        AuxRoutines::readFromFile(UrmSettings::mDeviceNamePath);

    int8_t parsingDone = false;
    int8_t docMarker = false;
    int8_t parsingClusterExpectation = false;
    int8_t isConfigForCurrentTarget = false;
    int8_t deviceParsingDone = false;

    std::string value;
    std::string topKey;
    std::stack<std::string> keyTracker;

    ClusterExpectationBuilder* clusterExpectationBuilder = nullptr;

    while(!parsingDone) {
        if(!yaml_parser_parse(&parser, &event)) {
            return RC_YAML_PARSING_ERROR;
        }

        switch(event.type) {
            case YAML_STREAM_END_EVENT:
                parsingDone = true;
                break;

            case YAML_SEQUENCE_END_EVENT:
                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                keyTracker.pop();
                break;

            case YAML_MAPPING_START_EVENT:
                if(!docMarker) {
                    docMarker = true;
                } else {
                    parsingClusterExpectation = true;
                    clusterExpectationBuilder = new(std::nothrow) ClusterExpectationBuilder;
                    if(clusterExpectationBuilder == nullptr) {
                        return RC_YAML_INVALID_SYNTAX;
                    }
                }

                break;

            case YAML_MAPPING_END_EVENT:
                if(keyTracker.empty()) {
                    break;
                } else if(parsingClusterExpectation) {
                    parsingClusterExpectation = false;

                    if(isConfigForCurrentTarget) {
                        ClusterExpection* info = clusterExpectationBuilder->build();
                        this->mLogicalToPhysicalClusterMapping[info->mLogicalID] = info->mPhysicalID;
                    }

                    delete clusterExpectationBuilder;
                    clusterExpectationBuilder = nullptr;
                }

                break;

            case YAML_SCALAR_EVENT:
                if(event.data.scalar.value != nullptr) {
                    value = reinterpret_cast<char*>(event.data.scalar.value);
                }

                if(isKey(value)) {
                    if(value == TARGET_NAME_LIST) {
                        isConfigForCurrentTarget = false;
                    }
                    keyTracker.push(value);
                    break;
                }

                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                topKey = keyTracker.top();
                if(topKey != TARGET_NAME_LIST &&
                   topKey != CLUSTER_EXPECTATIONS) {
                    keyTracker.pop();
                }

                if(topKey == TARGET_NAME_LIST) {
                    if(value == currTargetName) {
                        if(!deviceParsingDone) {
                            isConfigForCurrentTarget = true;
                            deviceParsingDone = true;
                        }
                    } else if(currTargetName.length() == 0 && value == "*") {
                        if(!deviceParsingDone) {
                            isConfigForCurrentTarget = true;
                            deviceParsingDone = true;
                        }
                    }
                } else if(topKey == TARGET_CLUSTER_INFO_LOGICAL_ID) {
                    clusterExpectationBuilder->setLogicalID(value);
                } else if(topKey == TARGET_CLUSTER_INFO_PHYSICAL_ID) {
                    clusterExpectationBuilder->setPhysicalID(value);
                } else if(topKey == NUM_CLUSERS) {
                    if(isConfigForCurrentTarget) {
                        this->mTotalClusterCount = -1;
                        try {
                            this->mTotalClusterCount = std::stoi(value);
                        } catch(const std::exception& e) {}
                    }
                } else if(topKey == NUM_CORES) {
                    if(isConfigForCurrentTarget) {
                        this->mTotalCoreCount = -1;
                        try {
                            this->mTotalCoreCount = std::stoi(value);
                        } catch(const std::exception& e) {}
                    }
                }

            default:
                break;
        }
        yaml_event_delete(&event);
    }

    TEARDOWN_LIBYAML_PARSING
    return RC_SUCCESS;
}

ErrCode TestBaseline::fetchBaseline() {
    return parseTestConfigYamlNode(baselineYamlFilePath);
}

void TestBaseline::displayBaseline() {
    std::cout<<"Total Cluster Count: "<<this->getExpectedClusterCount()<<std::endl;
    std::cout<<"Total Core Count: "<<this->getExpectedCoreCount()<<std::endl;

    for(auto it = this->mLogicalToPhysicalClusterMapping.begin();
             it != this->mLogicalToPhysicalClusterMapping.end(); it++) {
        std::cout<<"Logical ID "<<it->first<<" mapped to Physical ID: "<<it->second<<std::endl;
    }
}

int32_t TestBaseline::getExpectedClusterCount() {
    return this->mTotalClusterCount;
}

int32_t TestBaseline::getExpectedCoreCount() {
    return this->mTotalCoreCount;
}

int32_t TestBaseline::getExpectedPhysicalCluster(int32_t logicalID) {
    if(this->mLogicalToPhysicalClusterMapping.find(logicalID) ==
       this->mLogicalToPhysicalClusterMapping.end()) {
        return -1;
    }

    return this->mLogicalToPhysicalClusterMapping[logicalID];
}
