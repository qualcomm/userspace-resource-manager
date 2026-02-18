// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

/*!
 * \file  RateLimiter.h
 */

/*!
 * \ingroup  RATE_LIMITER
 * \defgroup RATE_LIMITER Rate Limiter
 * \details To Prevent abuse of the system, Rate Limiting checks have been added.\n
 *          This is implemented via the “RateLimiter” component.\n\n
 *          Each client on initialization gets a pre-defined “health” of 100.
 *          We follow a Reward / Punish mechanism. Where a punishment implies a decrease in the\
 *          health and a reward result in an increment in health (upto 100 max).
 *          If the client health drops to a value <= 0, then the client shall be dropped, i.e. any
 *          further requests sent by the client will be dropped without any further processing.\n\n
 *          How are Punishment and Rewards Defined: RateLimiter provides a time interval “delta”, say 5 ms. If a client sends 2 requests within a time interval smaller than delta, then we punish the client. If consecutive client requests are suitably spaced out, we reward the client for good behavior.
 *
 * @{
 */

#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include <mutex>
#include <shared_mutex>
#include <memory>

#include "RequestManager.h"
#include "ClientDataManager.h"
#include "UrmSettings.h"

/**
 * @brief RateLimiter
 * @details Responsible for Tracking Client Behaviour, and Protect against System Abuse.
 */
class RateLimiter {
private:
    static std::shared_ptr<RateLimiter> mRateLimiterInstance;
    static std::mutex instanceProtectionLock;
    std::shared_timed_mutex mRateLimiterMutex;

    uint32_t mDelta;
    double mPenaltyFactor;
    double mRewardFactor;
    int8_t shouldBeProcessed(pid_t clientPID);

    RateLimiter();

public:
    /**
     * @brief Checks if rate limit is honored.
     * @details RateLimiter uses the notion of Client Health (initialize to 100),
     *          which changes based on Client Behaviour (through Reward or Punishment).
     *          If this value Reaches 0, then any further Requests from the Client
     *          shall be dropped.
     * @param clientTID TID of the client
     * @return int8_t:\n
     *            - 1: If the Request can be accepted.\n
     *            - 0: otherwise
     */
    int8_t isRateLimitHonored(pid_t clientTID);

    /**
     * @brief Checks if the Global Rate Limit is honored.
     * @details Resource Tuner sets a cap on the number of Active Requests which can be
     *          served concurrently. If the current Count of Concurrent Active Requests
     *          hits this threshold, then any new Requests shall be dropped.
     * @return int8_t:\n
     *            - 1: If the Request can be accepted.\n
     *            - 0: otherwise
     */
    int8_t isGlobalRateLimitHonored();

    static std::shared_ptr<RateLimiter> getInstance() {
        if(mRateLimiterInstance == nullptr) {
            instanceProtectionLock.lock();
            if(mRateLimiterInstance == nullptr) {
                try {
                    mRateLimiterInstance = std::shared_ptr<RateLimiter> (new RateLimiter());
                } catch(const std::bad_alloc& e) {
                    instanceProtectionLock.unlock();
                    return nullptr;
                }
            }
            instanceProtectionLock.unlock();
        }
        return mRateLimiterInstance;
    }
};

#endif

/*! @} */
