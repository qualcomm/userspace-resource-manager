// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef RAM_CONFIG_GENERATOR_H
#define RAM_CONFIG_GENERATOR_H

#include <string>
#include <unordered_map>
#include <cstdint>

/**
 * @brief RAMConfigGenerator
 * @details Generates optimized configuration values based on system RAM tier.
 *          This class provides RAM-aware configuration values for various
 *          resource tuner parameters.
 */
class RAMConfigGenerator {
public:
    /**
     * @brief Get configuration value for a given property based on RAM tier
     * @param propertyName The name of the property
     * @param ramTier The RAM tier (low, medium, high, very_high)
     * @return The optimized value for the property, or empty string if not found
     */
    static std::string getConfigValue(const std::string& propertyName,
                                     const std::string& ramTier);

    /**
     * @brief Get all RAM-based configurations for a given tier
     * @param ramTier The RAM tier (low, medium, high, very_high)
     * @return Map of property names to their optimized values
     */
    static std::unordered_map<std::string, std::string>
        getAllConfigs(const std::string& ramTier);

private:
    // Configuration profiles for different RAM tiers
    struct ConfigProfile {
        uint32_t maxConcurrentRequests;
        uint32_t maxResourcesPerRequest;
        uint32_t threadPoolDesiredCapacity;
        uint32_t threadPoolMaxScalingCapacity;
        uint32_t pulseDuration;
        uint32_t garbageCollectionDuration;
        uint32_t garbageCollectionBatchSize;
        uint32_t rateLimiterDelta;
        double penaltyFactor;
        double rewardFactor;
    };

    static ConfigProfile getLowRAMProfile();
    static ConfigProfile getMediumRAMProfile();
    static ConfigProfile getHighRAMProfile();
    static ConfigProfile getVeryHighRAMProfile();
    static ConfigProfile getProfileForTier(const std::string& ramTier);
};

#endif
