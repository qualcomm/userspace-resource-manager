// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "RAMConfigGenerator.h"
#include "Logger.h"

// Low RAM Profile (< 4GB)
// Conservative settings to minimize memory usage
RAMConfigGenerator::ConfigProfile RAMConfigGenerator::getLowRAMProfile() {
    ConfigProfile profile;
    profile.maxConcurrentRequests = 30;
    profile.maxResourcesPerRequest = 10;
    profile.threadPoolDesiredCapacity = 3;
    profile.threadPoolMaxScalingCapacity = 6;
    profile.pulseDuration = 30000;
    profile.garbageCollectionDuration = 1500;
    profile.garbageCollectionBatchSize = 10;
    profile.rateLimiterDelta = 3;
    profile.penaltyFactor = 2.5;
    profile.rewardFactor = 0.3;
    return profile;
}

// Medium RAM Profile (4GB - 8GB)
// Balanced settings - similar to current defaults
RAMConfigGenerator::ConfigProfile RAMConfigGenerator::getMediumRAMProfile() {
    ConfigProfile profile;
    profile.maxConcurrentRequests = 60;
    profile.maxResourcesPerRequest = 20;
    profile.threadPoolDesiredCapacity = 5;
    profile.threadPoolMaxScalingCapacity = 10;
    profile.pulseDuration = 23000;
    profile.garbageCollectionDuration = 1000;
    profile.garbageCollectionBatchSize = 20;
    profile.rateLimiterDelta = 5;
    profile.penaltyFactor = 2.0;
    profile.rewardFactor = 0.4;
    return profile;
}

// High RAM Profile (8GB - 64GB)
// Aggressive settings for better performance
RAMConfigGenerator::ConfigProfile RAMConfigGenerator::getHighRAMProfile() {
    ConfigProfile profile;
    profile.maxConcurrentRequests = 75;
    profile.maxResourcesPerRequest = 30;
    profile.threadPoolDesiredCapacity = 8;
    profile.threadPoolMaxScalingCapacity = 16;
    profile.pulseDuration = 18000;
    profile.garbageCollectionDuration = 800;
    profile.garbageCollectionBatchSize = 30;
    profile.rateLimiterDelta = 7;
    profile.penaltyFactor = 1.8;
    profile.rewardFactor = 0.5;
    return profile;
}

// Very High RAM Profile (> 64GB)
// Maximum performance settings
RAMConfigGenerator::ConfigProfile RAMConfigGenerator::getVeryHighRAMProfile() {
    ConfigProfile profile;
    profile.maxConcurrentRequests = 100;
    profile.maxResourcesPerRequest = 50;
    profile.threadPoolDesiredCapacity = 12;
    profile.threadPoolMaxScalingCapacity = 24;
    profile.pulseDuration = 15000;
    profile.garbageCollectionDuration = 600;
    profile.garbageCollectionBatchSize = 40;
    profile.rateLimiterDelta = 10;
    profile.penaltyFactor = 1.5;
    profile.rewardFactor = 0.6;
    return profile;
}

RAMConfigGenerator::ConfigProfile RAMConfigGenerator::getProfileForTier(const std::string& ramTier) {
    if (ramTier == "low") {
        LOGI("RAM_CONFIG_GEN", "Using LOW RAM configuration profile");
        return getLowRAMProfile();
    } else if (ramTier == "medium") {
        LOGI("RAM_CONFIG_GEN", "Using MEDIUM RAM configuration profile");
        return getMediumRAMProfile();
    } else if (ramTier == "high") {
        LOGI("RAM_CONFIG_GEN", "Using HIGH RAM configuration profile");
        return getHighRAMProfile();
    } else if (ramTier == "very_high") {
        LOGI("RAM_CONFIG_GEN", "Using VERY_HIGH RAM configuration profile");
        return getVeryHighRAMProfile();
    } else {
        LOGW("RAM_CONFIG_GEN", "Unknown RAM tier: " + ramTier + ", using MEDIUM profile");
        return getMediumRAMProfile();
    }
}

std::string RAMConfigGenerator::getConfigValue(const std::string& propertyName,
                                               const std::string& ramTier) {
    ConfigProfile profile = getProfileForTier(ramTier);

    if (propertyName == "resource_tuner.maximum.concurrent.requests") {
        return std::to_string(profile.maxConcurrentRequests);
    } else if (propertyName == "resource_tuner.maximum.resources.per.request") {
        return std::to_string(profile.maxResourcesPerRequest);
    } else if (propertyName == "resource_tuner.thread_pool.desired_capacity") {
        return std::to_string(profile.threadPoolDesiredCapacity);
    } else if (propertyName == "resource_tuner.thread_pool.max_scaling_capacity") {
        return std::to_string(profile.threadPoolMaxScalingCapacity);
    } else if (propertyName == "resource_tuner.pulse.duration") {
        return std::to_string(profile.pulseDuration);
    } else if (propertyName == "resource_tuner.garbage_collection.duration") {
        return std::to_string(profile.garbageCollectionDuration);
    } else if (propertyName == "resource_tuner.garbage_collection.batch_size") {
        return std::to_string(profile.garbageCollectionBatchSize);
    } else if (propertyName == "resource_tuner.rate_limiter.delta") {
        return std::to_string(profile.rateLimiterDelta);
    } else if (propertyName == "resource_tuner.penalty.factor") {
        return std::to_string(profile.penaltyFactor);
    } else if (propertyName == "resource_tuner.reward.factor") {
        return std::to_string(profile.rewardFactor);
    }

    return "";
}

std::unordered_map<std::string, std::string>
RAMConfigGenerator::getAllConfigs(const std::string& ramTier) {
    std::unordered_map<std::string, std::string> configs;
    ConfigProfile profile = getProfileForTier(ramTier);

    configs["resource_tuner.maximum.concurrent.requests"] =
        std::to_string(profile.maxConcurrentRequests);
    configs["resource_tuner.maximum.resources.per.request"] =
        std::to_string(profile.maxResourcesPerRequest);
    configs["resource_tuner.thread_pool.desired_capacity"] =
        std::to_string(profile.threadPoolDesiredCapacity);
    configs["resource_tuner.thread_pool.max_scaling_capacity"] =
        std::to_string(profile.threadPoolMaxScalingCapacity);
    configs["resource_tuner.pulse.duration"] =
        std::to_string(profile.pulseDuration);
    configs["resource_tuner.garbage_collection.duration"] =
        std::to_string(profile.garbageCollectionDuration);
    configs["resource_tuner.garbage_collection.batch_size"] =
        std::to_string(profile.garbageCollectionBatchSize);
    configs["resource_tuner.rate_limiter.delta"] =
        std::to_string(profile.rateLimiterDelta);
    configs["resource_tuner.penalty.factor"] =
        std::to_string(profile.penaltyFactor);
    configs["resource_tuner.reward.factor"] =
        std::to_string(profile.rewardFactor);

    return configs;
}
