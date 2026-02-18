// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef PROPERTIES_REGISTRY_H
#define PROPERTIES_REGISTRY_H

#include <unordered_map>
#include <string>
#include <system_error>
#include <memory>
#include <shared_mutex>

/**
 * @brief PropertiesRegistry
 * @details Stores and manages all the properties parsed from the Properties Config files.
 */
class PropertiesRegistry {
private:
    static std::shared_ptr<PropertiesRegistry> propRegistryInstance;
    std::unordered_map<std::string, std::string> mProperties;
    std::shared_timed_mutex mPropRegistryMutex;

    PropertiesRegistry();

public:
    ~PropertiesRegistry();

    /**
     * @brief Create a property with the given name (key) and value.
     * @param propertyName Property Name or Key
     * @param propertyValue Property Value
     * @return int8_t:\n
     *            - 1: if the property was successfully created\n
     *            - 0: otherwise
     */
    int8_t createProperty(const std::string& propertyName, const std::string& propertyValue);

    /**
     * @brief Get the Property value corresponding to the given key
     * @param propertyName Property Name or Key
     * @param result If the property exists, the value will be stored in this argument
     * @return size_t: Number of bytes written
     */
    size_t queryProperty(const std::string& propertyName, std::string& result);

    /**
     * @brief Modify the value of the property with the given name
     * @param propertyName Property Name or Key
     * @param propertyValue New Property Value
     * @return int8_t:\n
     *            - 1: if a property was successfully modified\n
     *            - 0: otherwise
     */
    int8_t modifyProperty(const std::string& propertyName, const std::string& propertyValue);

    /**
     * @brief Delete the Property with the given name (key)
     * @param propertyName Property Name or Key
     * @return int8_t:\n
     *            - 1: if a property was successfully deleted\n
     *            - 0: otherwise
     */
    int8_t deleteProperty(const std::string& propertyName);

    int32_t getPropertiesCount();

    static std::shared_ptr<PropertiesRegistry> getInstance() {
        if(propRegistryInstance == nullptr) {
            std::shared_ptr<PropertiesRegistry> localpropRegistryInstance(new PropertiesRegistry());
            localpropRegistryInstance.swap(propRegistryInstance);
        }
        return propRegistryInstance;
    }
};

#endif
