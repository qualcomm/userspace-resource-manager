// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <iostream>
#include <unistd.h>
#include <thread>
#include <exception>
#include <sstream>
#include <memory>
#include <getopt.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "Utils.h"
#include "UrmAPIs.h"

int8_t parseResources(const std::string& input,
                      std::vector<std::pair<uint32_t,
                      std::pair<int32_t, std::vector<int32_t>>>>& resourceVec) {
    std::unordered_map<uint32_t, std::pair<int32_t, std::vector<int32_t>>> resourceMap;
    std::istringstream resourceStream(input);
    std::string token;

    while(std::getline(resourceStream, token, ';')) {
        std::string resInfo = "";
        size_t colonPos = token.find(':');
        if(colonPos == std::string::npos) {
            return -1;
        }

        std::string opCode = token.substr(0, colonPos);
        std::string valuesStr = token.substr(colonPos + 1);

        size_t sepPos = opCode.find('#');
        if(sepPos != std::string::npos) {
            resInfo = opCode.substr(sepPos + 1);
            opCode = opCode.substr(0, sepPos);
        }

        if(resInfo.length() > 0) {
            resourceMap[(uint32_t)stol(opCode, nullptr, 0)].first = (int32_t)stoi(resInfo, nullptr, 0);
        }

        std::istringstream valuesStream(valuesStr);
        std::string value;

        while(std::getline(valuesStream, value, ',')) {
            try {
                resourceMap[(uint32_t)stol(opCode, nullptr, 0)].second.push_back(std::stoi(value));
            } catch (const std::invalid_argument&) {
                return -1;
            } catch (const std::out_of_range&) {
                return -1;
            }
        }
    }

    for(std::pair<uint32_t, std::pair<int32_t, std::vector<int32_t>>> entry: resourceMap) {
        resourceVec.push_back({entry.first, {entry.second.first, entry.second.second}});
    }

    return 0;
}

void sendTuneRequest(int64_t duration, int32_t priority, int32_t count, const std::string& resourceInfo) {
    std::vector<std::pair<uint32_t, std::pair<int32_t, std::vector<int32_t>>>> resourceVec;
    if(parseResources(resourceInfo, resourceVec) == -1 || resourceVec.size() > count) {
        std::cout<<"Failed to parse Resource List"<<std::endl;
        return;
    }

    std::cout<<"Number of unique resources in the request: "<<resourceVec.size()<<std::endl;
    SysResource* resourceList = new SysResource[resourceVec.size()];

    for(int32_t i = 0; i < resourceVec.size(); i++) {
        memset(&resourceList[i], 0, sizeof(SysResource));
        resourceList[i].mResCode = resourceVec[i].first;
        resourceList[i].mResInfo = resourceVec[i].second.first;
        resourceList[i].mNumValues = resourceVec[i].second.second.size();
        if(resourceList[i].mNumValues == 1) {
            resourceList[i].mResValue.value = resourceVec[i].second.second[0];
        } else {
            resourceList[i].mResValue.values = new int32_t[resourceList[i].mNumValues];
            for(int32_t k = 0; k < resourceList[i].mNumValues; k++) {
                resourceList[i].mResValue.values[k] = resourceVec[i].second.second[k];
            }
        }
    }

    // Log the resources
    for(int32_t idx = 0; idx < resourceVec.size(); idx++) {
        std::cout<<"Printing Resource at index = "<<idx<<std::endl;
        std::cout<<"ResCode for this Resource = "<<resourceList[idx].mResCode<<std::endl;
        std::cout<<"ResInfo for this Resource = "<<resourceList[idx].mResInfo<<std::endl;
        std::cout<<"Number of Values for this Resource = "<<resourceList[idx].mNumValues<<std::endl;

        std::cout<<"Printing values"<<std::endl;
        if(resourceList[idx].mNumValues == 1) {
            std::cout<<resourceList[idx].mResValue.value<<std::endl;
        } else {
            for(int32_t j = 0; j < resourceList[idx].mNumValues; j++) {
                std::cout<<resourceList[idx].mResValue.values[j]<<std::endl;
            }
        }
    }

    int64_t handle = tuneResources(duration, priority, resourceVec.size(), resourceList);
    if(handle == -1) {
        std::cout<<"Failed to send Tune Request"<<std::endl;
    } else {
        std::cout<<"Handle Received from Server is: "<<handle<<std::endl;
    }
}

void sendRetuneRequest(int64_t handle, int64_t duration) {
    int8_t status = retuneResources(handle, duration);
    if(status == 0) {
        std::cout<<"Retune Request Successfully Submitted"<<std::endl;
    } else if(status == -1) {
        std::cout<<"Retune Request Could not be sent"<<std::endl;
    }
}

void sendUntuneRequest(int64_t handle) {
    int8_t status = untuneResources(handle);
    if(status == 0) {
        std::cout<<"Untune Request Successfully Submitted"<<std::endl;
    } else if(status == -1) {
        std::cout<<"Untune Request Could not be sent"<<std::endl;
    }
}

static void sendPropRequest(const char* prop) {
    char buffer[1024];
    int8_t status = getProp(prop, buffer, sizeof(buffer), "prop_not_found");
    if(status == 0) {
        std::cout<<"Prop Retrieved, value is: "<<buffer<<std::endl;
    } else if(status == -1) {
        std::cout<<"Get Prop Request Could not be sent"<<std::endl;
    }
}

static void sendTuneSignal(const char* scode) {
    std::cout<<"Sending tuneSignal request for SigCode: "<<scode<<std::endl;
    uint32_t sigCode = (uint32_t)stol(std::string(scode), nullptr, 0);
    int64_t handle = tuneSignal(sigCode, 0, 0, 0, "", "", 0, nullptr);
    if(handle == -1) {
        std::cout<<"Failed to send tuneSignal Request"<<std::endl;
    } else {
        std::cout<<"Handle Received from Server is: "<<handle<<std::endl;
    }
}

static int8_t processCommands() {
    std::string input;

    // read line
    std::getline(std::cin, input);

    if(input == "") {
        return true;
    }

    if(input == "exit" || input == "stop") {
        return false;
    }

    if(input == "help") {
        std::cout<<"Available commands: tune, retune, untune"<<std::endl;
        return true;
    }

    // break into tokens
    std::stringstream ss(input);
    std::vector<std::string> tokens;
    std::string token;
    while(std::getline(ss, token, ' ')) {
        tokens.push_back(token);
    }

    if(tokens[0] == "tune") {
        if(tokens.size() != 5) {
            std::cout<<"Invalid number of arguments for tune request"<<std::endl;
            std::cout<<"Usage: tune <duration> <priority> <numRes> <resCode>:<value>,<resCode>:<value>...."<<std::endl;
            std::cout<<"Example: tune 4000 3 2 1:1,2:2"<<std::endl;
            return true;
        }

        if(tokens[1].find(":") == std::string::npos) {
            std::cout<<"Invalid tune request"<<std::endl;
            std::cout<<"Usage: tune <duration> <priority> <numRes> <resCode>:<value>,<resCode>:<value>"<<std::endl;
            std::cout<<"Example: tune 4000 3 2 1:1,2:2"<<std::endl;
            return true;
        }

        int64_t duration = std::stoi(tokens[1]);
        int32_t priority = std::stoi(tokens[2]);
        int32_t count = std::stoi(tokens[3]);

        if(duration < -1 || duration == 0 || count <= 0) {
            std::cout<<"Invalid Params for Retune request" <<std::endl;
            std::cout<<"Usage: tune <duration> <priority> <numRes> <resCode>:<value>,<resCode>:<value>"<<std::endl;
            return true;
        } else {
            sendTuneRequest(duration, priority, count, tokens[4]);
        }


    } else if(tokens[0] == "untune") {
        if(tokens.size() != 2) {
            std::cout<<"Invalid number of arguments for Untune request" <<std::endl;
            std::cout<<"Usage: untune <handle>"<<std::endl;
            std::cout<<"Example: untune 4"<<std::endl;
            return true;
        }

        int64_t handle = std::stoi(tokens[1]);
        if(handle <= 0) {
            std::cout<<"Invalid Params for untune request" <<std::endl;
            std::cout<<"Usage: untune <handle>"<<std::endl;
            return true;

        } else {
            sendUntuneRequest(handle);
        }

        return true;

    } else if(tokens[0] == "retune") {
        if(tokens.size() != 3) {
            std::cout<<"Invalid number of arguments for Retune request"<<std::endl;
            std::cout<<"Usage: retune <handle> <duration>"<<std::endl;
            std::cout<<"Example: retune 1 5000" << std::endl;
            return true;
        }

        int64_t handle = std::stoi(tokens[1]);
        int32_t duration = std::stoi(tokens[2]);

        if(handle <= 0 || duration == 0 || duration < -1) {
            std::cout<<"Invalid Params for Retune request" <<std::endl;
            std::cout<<"Usage: retune <handle> <duration>"<<std::endl;
            return true;

        } else {
            sendRetuneRequest(handle, duration);
        }

        return true;

    } else {
        std::cout<<"Invalid command"<<std::endl;
        return true;
    }

    return true;
}

static void startPersistentMode() {
    while(true) {
        try {
            if(!processCommands()) {
                return;
            }
        } catch(const std::bad_alloc& e) {
            std::cout<<"Invalid Command"<<std::endl;
        }
    }
}

int32_t main(int32_t argc, char* argv[]) {
    const char* shortPrompts = "turd:p:l:n:h:s:gk:qm:";
    const struct option longPrompts[] = {
        {"tune", no_argument, nullptr, 't'},
        {"untune", no_argument, nullptr, 'u'},
        {"retune", no_argument, nullptr, 'r'},
        {"duration", required_argument, nullptr, 'd'},
        {"handle", required_argument, nullptr, 'h'},
        {"priority", required_argument, nullptr, 'p'},
        {"res", required_argument, nullptr, 'l'},
        {"num", required_argument, nullptr, 'n'},
        {"getProp", no_argument, nullptr, 'g'},
        {"key", required_argument, nullptr, 'k'},
        {"persistent", no_argument, nullptr, 's'},
        {"signal", no_argument, nullptr, 'q'},
        {"scode", required_argument, nullptr, 'm'},
        {nullptr, no_argument, nullptr, 0}
    };

    int32_t c;

    int8_t requestType = -1;
    int64_t handle = -1;
    int64_t duration = -1;
    int32_t priority = -1;
    int32_t numResources = -1;
    const char* resources = nullptr;
    const char* propKey = nullptr;
    const char* sigCode = nullptr;
    int8_t persistent = false;

    while((c = getopt_long(argc, argv, shortPrompts, longPrompts, nullptr)) != -1) {
        switch(c) {
            case 't':
                requestType = REQ_RESOURCE_TUNING;
                break;
            case 'u':
                requestType = REQ_RESOURCE_UNTUNING;
                break;
            case 'r':
                requestType = REQ_RESOURCE_RETUNING;
                break;
            case 'd':
                duration = std::stoi(optarg);
                break;
            case 'p':
                priority = std::stoi(optarg);
                break;
            case 'h':
                handle = std::stoi(optarg);
                break;
            case 'l':
                resources = optarg;
                break;
            case 'n':
                numResources = std::stoi(optarg);
                break;
            case 's':
                persistent = true;
                break;
            case 'g':
                requestType = REQ_PROP_GET;
                break;
            case 'k':
                propKey = optarg;
                break;
            case 'q':
                requestType = REQ_SIGNAL_TUNING;
                break;
            case 'm':
                sigCode = optarg;
                break;
            default:
                break;
        }
    }

    if(persistent) {
        startPersistentMode();
        return 0;
    }

    switch(requestType) {
        case REQ_RESOURCE_TUNING:
            if(duration == 0 || duration < -1 || numResources <= 0 || priority == -1 ||
               resources == nullptr) {
                std::cout<<"Invalid Params for Tune Request"<<std::endl;
                std::cout<<"Usage: --tune --duration <duration> --priority <priority> --num <numRes> --res \"<resCode[#resInfo]>:<value(s)>;<resCode[#resInfo]>:<value(s)>\""<<std::endl;
                std::cout<<"Example: --tune --duration 5000 --priority 0 --num 1 --res \"0x0005a0d1:897\""<<std::endl;
                break;
            }
            if(resources != nullptr) {
                sendTuneRequest(duration, priority, numResources, resources);
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
            break;

        case REQ_RESOURCE_RETUNING:
            if(duration == 0 || duration < -1 || handle <= 0) {
                std::cout<<"Invalid Params for Retune Request"<<std::endl;
                std::cout<<"Usage: --retune --handle <handle> --duration <duration>"<< std::endl;
                break;
            }
            sendRetuneRequest(handle, duration);
            break;

        case REQ_RESOURCE_UNTUNING:
            if(handle <= 0) {
                std::cout<<"Invalid Params for Untune request"<<std::endl;
                std::cout<<"Usage: --untune --handle <handle>"<<std::endl;
                break;
            }
            sendUntuneRequest(handle);
            break;

        case REQ_PROP_GET:
            if(propKey == nullptr) {
                std::cout<<"Invalid Params for Get Prop request"<< std::endl;
                std::cout<<"Usage: --getProp --key <key>"<<std::endl;
                break;
            }
            sendPropRequest(propKey);
            break;

        case REQ_SIGNAL_TUNING:
            if(sigCode == nullptr) {
                std::cout<<"Invalid Params for tune signal request"<< std::endl;
                std::cout<<"Usage: --signal --scode <key>"<<std::endl;
                break;
            }

            sendTuneSignal(sigCode);
            std::this_thread::sleep_for(std::chrono::seconds(3));
            break;

        default:
            return -1;
    }

    return 0;
}
