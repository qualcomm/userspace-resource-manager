// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef COMPONENT_REGISTRY_H
#define COMPONENT_REGISTRY_H

#include <unordered_map>

#include "Utils.h"

typedef struct {
    EventCallback mInit;
    EventCallback mTear;
} ModuleInfo;

/**
 * @brief ComponentRegistry
 * @details Keeps track of the Resource Tuner modules, used to check if a module
 *          is enabled or not and stores the module's registered callbacks if it is enabled.
 */
class ComponentRegistry {
private:
    static std::unordered_map<ModuleID, ModuleInfo> mModuleRegistry;

public:
    ComponentRegistry(ModuleID moduleID,
                      EventCallback registrationCallback,
                      EventCallback terardownCallback);

    static int8_t isModuleEnabled(ModuleID moduleID);
    static ModuleInfo getModuleInfo(ModuleID moduleID);
};

#define URM_REGISTER_MODULE(identifier, registration, teardown) \
        static ComponentRegistry CONCAT(_module, identifier)(identifier, registration, teardown);


#endif
