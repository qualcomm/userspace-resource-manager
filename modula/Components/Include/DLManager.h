// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef DL_MANAGER_H
#define DL_MANAGER_H

#include "ErrCodes.h"

struct DLRootNode {
public:
    struct {
        struct DLRootNode* next;
        struct DLRootNode* prev;
    } mLinkages[1];

    virtual DLRootNode* getNextPtr(int32_t linker) = 0;
    virtual DLRootNode* getPrevPtr(int32_t linker) = 0;
    virtual void setNextLinkage(int32_t linker, DLRootNode* node) = 0;
    virtual void setPrevLinkage(int32_t linker, DLRootNode* node) = 0;
};

template <typename T>
class Iterable: public DLRootNode {
public:
    T mData;

    virtual DLRootNode* getNextPtr(int32_t linker) override {
        if(linker < 0 || linker >= 1) return nullptr;
        return this->mLinkages[linker].next;
    }

    virtual DLRootNode* getPrevPtr(int32_t linker) override {
        if(linker < 0 || linker >= 1) return nullptr;
        return this->mLinkages[linker].prev;
    }

    virtual void setNextLinkage(int32_t linker, DLRootNode* node) override {
        if(linker < 0 || linker >= 1) return;
        this->mLinkages[linker].next = node;
    }

    virtual void setPrevLinkage(int32_t linker, DLRootNode* node) override {
        if(linker < 0 || linker >= 1) return;
        this->mLinkages[linker].prev = node;
    }

    Iterable () {
        this->mLinkages[0].next = this->mLinkages[0].prev = nullptr;
    }
};

// Creates N + 1 linkers
#define CREATE_N_WAY_ITERABLE(N)                                                        \
    template <typename T>                                                               \
    class ExtIterable##N: public DLRootNode {                                           \
    public:                                                                             \
        T mData;                                                                        \
        struct {                                                                        \
            struct DLRootNode* next;                                                    \
            struct DLRootNode* prev;                                                    \
        } mExtraLinks[N];                                                               \
        virtual DLRootNode* getNextPtr(int32_t linker) override {                       \
            if(linker < 0 || linker >= N + 1) return nullptr;                           \
            if(linker < 1) return this->mLinkages[linker].next;                         \
            if(linker < N + 1) return this->mExtraLinks[linker - 1].next;               \
            return nullptr;                                                             \
        }                                                                               \
        virtual DLRootNode* getPrevPtr(int32_t linker) override {                       \
            if(linker < 0 || linker >= N + 1) return nullptr;                           \
            if(linker < 1) return this->mLinkages[linker].prev;                         \
            if(linker < N + 1) return this->mExtraLinks[linker - 1].prev;               \
            return nullptr;                                                             \
        }                                                                               \
        virtual void setNextLinkage(int32_t linker, DLRootNode* node) override {        \
            if(linker < 0 || linker >= N + 1) return;                                   \
            if(linker < 1) {                                                            \
                this->mLinkages[linker].next = node;                                    \
            } else if(linker < N + 1) {                                                 \
                this->mExtraLinks[linker - 1].next = node;                              \
            }                                                                           \
        }                                                                               \
        virtual void setPrevLinkage(int32_t linker, DLRootNode* node) override {        \
            if(linker < 0 || linker >= N + 1) return;                                   \
            if(linker < 1) {                                                            \
                this->mLinkages[linker].prev = node;                                    \
            } else if(linker < N + 1) {                                                 \
                this->mExtraLinks[linker - 1].prev = node;                              \
            }                                                                           \
        }                                                                               \
        ExtIterable##N() {                                                              \
            this->mLinkages[0].next = this->mLinkages[0].prev = nullptr;                \
            for(int32_t index = 0; index < N; index++) {                                \
                this->mExtraLinks[index].next = this->mExtraLinks[index].prev = nullptr;\
            }                                                                           \
        }                                                                               \
    };

CREATE_N_WAY_ITERABLE(1)

// Comparison Policy Callback
typedef int8_t (*DLPolicy)(DLRootNode* newNode, DLRootNode* targetNode);

typedef struct _policyDir {
    DLPolicy mAscPolicy;
    DLPolicy mDescPolicy;
    DLPolicy mReplacePolicy;

    _policyDir() : mAscPolicy(nullptr), mDescPolicy(nullptr) {};
} PolicyRepo;

enum DLOptions {
    INSERT_START,
    INSERT_END,
    INSERT_N_NODE_START,
};

// Note this class is exclusively for Linked List operations management, i.e. node manipulations
// It does not perform any allocations or deallocations itself
// All nodes Should be allocated and freed by the client itself
// All fields in this class are kept public to allow finer operations wherever needed by any client.
class DLManager {
private:
    ErrCode insertHelper(DLRootNode* node);
    ErrCode insertHelper(DLRootNode* node, DLOptions option, int32_t n = 0);
    ErrCode insertWithPolicyHelper(DLRootNode* node, DLPolicy policy);
    ErrCode insertAscHelper(DLRootNode* node);
    ErrCode insertDescHelper(DLRootNode* node);
    ErrCode deleteNodeHelper(DLRootNode* node);
    void destroyHelper();
    int8_t isNodeNthHelper(int32_t n, DLRootNode* node);
    int8_t matchAgainstHelper(DLManager* target, DLPolicy policy = nullptr);
    int32_t getLenHelper();

public:
    DLRootNode* mHead;
    DLRootNode* mTail;

    int32_t mTotalLinkers;
    int32_t mLinkerInUse; // must be a value b/w 0 to mTotalLinkers - 1
    int32_t mSize;
    int32_t mRank;

    // Declared as public so that the cb's can be selectively set at DLManager creation time.
    PolicyRepo mSavedPolicies;

    DLManager(int32_t linkerInUse = 0);

    ErrCode insert(DLRootNode* node) {
        return this->insertHelper(node);
    }

    ErrCode insert(DLRootNode* node, DLOptions option, int32_t n = 0) {
        return this->insertHelper(node, option, n);
    }

    ErrCode insertWithPolicy(DLRootNode* node, DLPolicy policy) {
        return this->insertWithPolicyHelper(node, policy);
    }

    // Specialized functions which require certain fields in the PolicyRepo to be non-nul
    ErrCode insertAsc(DLRootNode* node) { // .mAscPolicy must be set
        return this->insertAscHelper(node);
    }

    ErrCode insertDesc(DLRootNode* node) { // .mDescPolicy must be set
        return this->insertDescHelper(node);
    }

    ErrCode deleteNode(DLRootNode* node) {
        return this->deleteNodeHelper(node);
    }

    void destroy() {
        this->destroyHelper();
    }

    int8_t isNodeNth(int32_t n, DLRootNode* node) { // 0 based indexing
        return this->isNodeNthHelper(n, node);
    }

    int8_t matchAgainst(DLManager* target, DLPolicy policy = nullptr) {
        return this->matchAgainstHelper(target, policy);
    }

    int32_t getLen() {
        return this->getLenHelper();
    }

    DLRootNode* getNth(int32_t n);
};

#define DL_ITERATE(dlm) \
    for(DLRootNode* iter = dlm->mHead; iter != nullptr; iter = iter->getNextPtr(dlm->mLinkerInUse))

#define DL_BACK(dlm) \
    for(DLRootNode* iter = dlm->mTail; iter != nullptr; iter = iter->getPrevPtr(dlm->mLinkerInUse))

#endif
