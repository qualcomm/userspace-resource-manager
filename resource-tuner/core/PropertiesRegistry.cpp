// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "PropertiesRegistry.h"
#include "RAMConfigGenerator.h"
#include "Logger.h"

std::shared_ptr<PropertiesRegistry> PropertiesRegistry::propRegistryInstance = nullptr;

PropertiesRegistry::PropertiesRegistry() {}

int8_t PropertiesRegistry::createProperty(const std::string& propertyName, const std::string& propertyValue) {
    if(propertyName.length() == 0 || propertyValue.length() == 0) {
        return false;
    }

    this->mProperties[propertyName] = propertyValue;
    return true;
}

size_t PropertiesRegistry::queryProperty(const std::string& propertyName, std::string& result) {
    if(propertyName.length() == 0) {
        return 0;
    }

    this->mPropRegistryMutex.lock_shared();
    if(this->mProperties.find(propertyName) == this->mProperties.end()) {
        this->mPropRegistryMutex.unlock_shared();
        return 0;
    }

    result = this->mProperties[propertyName];
    this->mPropRegistryMutex.unlock_shared();

    return result.length();
}

int8_t PropertiesRegistry::modifyProperty(const std::string& propertyName, const std::string& propertyValue) {
    if(propertyName.length() == 0 || propertyValue.length() == 0) {
        return false;
    }

    std::string tmpResult;
    if(queryProperty(propertyName, tmpResult) == false) {
        return false;
    }

    try {
        this->mPropRegistryMutex.lock();
        this->mProperties[propertyName] = propertyValue;
        this->mPropRegistryMutex.unlock();

    } catch(const std::system_error& e) {
        return false;
    }

    return true;
}

int8_t PropertiesRegistry::deleteProperty(const std::string& propertyName) {
    if(propertyName.length() == 0) {
        return false;
    }

    std::string tmpResult;
    if(queryProperty(propertyName, tmpResult) == false) {
        return false;
    }

    try {
        this->mPropRegistryMutex.lock();
        this->mProperties.erase(propertyName);
        this->mPropRegistryMutex.unlock();

    } catch(const std::system_error& e) {
        return false;
    }

    return true;
}

int32_t PropertiesRegistry::getPropertiesCount() {
    return this->mProperties.size();
}

int8_t PropertiesRegistry::applyRAMBasedConfigs(const std::string& ramTier) {
    LOGI("PROPERTIES_REGISTRY", "Applying RAM-based configurations for tier: " + ramTier);

    // Get all RAM-based configurations
    auto ramConfigs = RAMConfigGenerator::getAllConfigs(ramTier);

    int32_t appliedCount = 0;
    for (const auto& config : ramConfigs) {
        const std::string& propName = config.first;
        const std::string& propValue = config.second;

        // Check if property already exists
        std::string existingValue;
        if (queryProperty(propName, existingValue) > 0) {
            // Property exists, modify it
            if (modifyProperty(propName, propValue)) {
                LOGI("PROPERTIES_REGISTRY",
                     "Updated property: " + propName + " = " + propValue +
                     " (was: " + existingValue + ")");
                appliedCount++;
            }
        } else {
            // Property doesn't exist, create it
            if (createProperty(propName, propValue)) {
                LOGI("PROPERTIES_REGISTRY",
                     "Created property: " + propName + " = " + propValue);
                appliedCount++;
            }
        }
    }

    LOGI("PROPERTIES_REGISTRY",
         "Applied " + std::to_string(appliedCount) + " RAM-based configurations");

    return appliedCount > 0 ? true : false;
}

PropertiesRegistry::~PropertiesRegistry() {}
