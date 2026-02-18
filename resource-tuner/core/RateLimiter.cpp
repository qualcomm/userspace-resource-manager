// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "RateLimiter.h"

std::shared_ptr<RateLimiter> RateLimiter::mRateLimiterInstance = nullptr;
std::mutex RateLimiter::instanceProtectionLock{};

RateLimiter::RateLimiter() {
    this->mDelta = UrmSettings::metaConfigs.mDelta;
    this->mPenaltyFactor = UrmSettings::metaConfigs.mPenaltyFactor;
    this->mRewardFactor = UrmSettings::metaConfigs.mRewardFactor;
}

int8_t RateLimiter::shouldBeProcessed(pid_t clientTID) {
    this->mRateLimiterMutex.lock();

    double health = ClientDataManager::getInstance()->getHealthByClientID(clientTID);
    if(health <= 0) {
        // Repeat offender, total block
        this->mRateLimiterMutex.unlock();
        return false;
    }

    int64_t currentMillis = AuxRoutines::getCurrentTimeInMilliseconds();
    // If this is the First Request, don't update the Health
    if(ClientDataManager::getInstance()->getLastRequestTimestampByClientID(clientTID) != 0) {
        int64_t requestDelta = currentMillis - ClientDataManager::getInstance()->getLastRequestTimestampByClientID(clientTID);

        if(requestDelta < mDelta) {
            health -= mPenaltyFactor;
        } else {
            // Increase in health
            health = std::min(100.0, health + this->mRewardFactor);
        }
    }

    ClientDataManager::getInstance()->updateHealthByClientID(clientTID, health);

    // Check if client health is positive
    if(health <= 0) {
        this->mRateLimiterMutex.unlock();
        return false;
    }

    // Request can be accepted
    ClientDataManager::getInstance()->updateLastRequestTimestampByClientID(clientTID, currentMillis);
    this->mRateLimiterMutex.unlock();

    return true;
}

int8_t RateLimiter::isRateLimitHonored(pid_t clientTID) {
    return shouldBeProcessed(clientTID);
}

int8_t RateLimiter::isGlobalRateLimitHonored() {
    int64_t currActiveReqCount = RequestManager::getInstance()->getActiveReqeustsCount();
    // Cover this check as part of Rate Limiter
    if(currActiveReqCount >= UrmSettings::metaConfigs.mMaxConcurrentRequests) {
        return false;
    }
    return true;
}
