// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


// MemoryPoolTests.cpp
#include "TestUtils.h"
#include "MemoryPool.h"
#include "TestAggregator.h"

#include <vector>
#include <string>
#include <cstdint>
#include <cstdlib>  // malloc
#include <new>      // placement new

#define MTEST_NO_MAIN
#include "../framework/mini.hpp"

#include "mini.hpp"   // your minimal test framework

using namespace mtest;

// -------------------------
// Helper test types
// -------------------------
struct TestBuffer {
    int32_t testId;
    double  score;
    int8_t  isDuplicate;
};

struct ListNode {
    int32_t  val;
    ListNode* next;
};

struct CustomRequest {
    int32_t requestID;
    int64_t requestTimestamp;
};

class DataHub {
private:
    int32_t     mFolderCount;
    int32_t     mUserCount;
    std::string mOrgName;
public:
    DataHub(int32_t folderCount, int32_t userCount, std::string orgName)
        : mFolderCount(folderCount), mUserCount(userCount), mOrgName(std::move(orgName)) {}
};

class CustomDataType {
private:
    int8_t* mDestructorCalled;
public:
    explicit CustomDataType(int8_t* destructorCalled)
        : mDestructorCalled(destructorCalled) {}

    ~CustomDataType() {
        *mDestructorCalled = true;
    }
};

// -------------------------
// Suite: MemoryPoolTests
// -------------------------

MT_TEST(MemoryPoolTests, BasicAllocation1, "unit") {
    MakeAlloc<TestBuffer>(1);

    void* block = GetBlock<TestBuffer>();
    MT_REQUIRE(ctx, block != nullptr);

    FreeBlock<TestBuffer>(static_cast<void*>(block));
}

MT_TEST(MemoryPoolTests, BasicAllocation2, "unit") {
    MakeAlloc<char[250]>(2);

    void* firstBlock = GetBlock<char[250]>();
    MT_REQUIRE(ctx, firstBlock != nullptr);

    void* secondBlock = GetBlock<char[250]>();
    MT_REQUIRE(ctx, secondBlock != nullptr);

    FreeBlock<char[250]>(firstBlock);
    FreeBlock<char[250]>(secondBlock);
}

MT_TEST(MemoryPoolTests, BasicAllocation3, "unit") {
    MakeAlloc<ListNode>(10);
    ListNode* head = nullptr;
    ListNode* cur  = nullptr;

    for (int32_t i = 0; i < 10; ++i) {
        ListNode* node = static_cast<ListNode*>(GetBlock<ListNode>());
        MT_REQUIRE(ctx, node != nullptr);

        node->val  = i + 1;
        node->next = nullptr;

        if (head == nullptr) {
            head = node;
            cur  = node;
        } else {
            cur->next = node;
            cur       = cur->next;
        }
    }

    cur = head;
    int32_t counter = 1;
    while (cur != nullptr) {
        MT_REQUIRE_EQ(ctx, cur->val, counter);
        ListNode* next = cur->next;
        FreeBlock<ListNode>(static_cast<void*>(cur));
        cur = next;
        ++counter;
    }
}

MT_TEST(MemoryPoolTests, BasicAllocation4, "unit") {
    MakeAlloc<std::vector<CustomRequest*>>(1);
    MakeAlloc<CustomRequest>(20);

    auto* requests =
        new (GetBlock<std::vector<CustomRequest*>>()) std::vector<CustomRequest*>;
    MT_REQUIRE(ctx, requests != nullptr);

    for (int32_t i = 0; i < 15; ++i) {
        auto* request = static_cast<CustomRequest*>(GetBlock<CustomRequest>());
        MT_REQUIRE(ctx, request != nullptr);

        request->requestID        = i + 1;
        request->requestTimestamp = 100 * (i + 3);
        requests->push_back(request);
    }

    for (int32_t i = 0; i < static_cast<int32_t>(requests->size()); ++i) {
        MT_REQUIRE_EQ(ctx, (*requests)[i]->requestID,        i + 1);
        MT_REQUIRE_EQ(ctx, (*requests)[i]->requestTimestamp, 100 * (i + 3));
        FreeBlock<CustomRequest>(static_cast<void*>((*requests)[i]));
    }

    FreeBlock<std::vector<CustomRequest*>>(static_cast<void*>(requests));
}

MT_TEST(MemoryPoolTests, BasicAllocation5, "unit") {
    MakeAlloc<DataHub>(1);

    auto* dataHubObj = new (GetBlock<DataHub>()) DataHub(30, 17, "XYZ-co");
    MT_REQUIRE(ctx, dataHubObj != nullptr);

    FreeBlock<DataHub>(static_cast<void*>(dataHubObj));
}

MT_TEST(MemoryPoolTests, BasicAllocation6, "unit") {
    int8_t allocationFailed = false;
    void* block = nullptr;

    try {
        block = GetBlock<char[120]>();
    } catch (const std::bad_alloc&) {
        allocationFailed = true;
    }

    MT_REQUIRE(ctx, block == nullptr);
    MT_REQUIRE_EQ(ctx, allocationFailed, true);
}

MT_TEST(MemoryPoolTests, BasicAllocation7, "unit") {
    MakeAlloc<int64_t>(1);

    void* block = nullptr;
    try {
        block = GetBlock<int64_t>();
    } catch (const std::bad_alloc&) {
        // no-op
    }
    MT_REQUIRE(ctx, block != nullptr);

    block = nullptr;
    try {
        block = GetBlock<char[8]>();
    } catch (const std::bad_alloc&) {
        // expected
    }
    MT_REQUIRE(ctx, block == nullptr);
}

MT_TEST(MemoryPoolTests, FreeingMemory1, "unit") {
    MakeAlloc<char[125]>(2);

    void* firstBlock  = GetBlock<char[125]>();
    MT_REQUIRE(ctx, firstBlock != nullptr);

    void* secondBlock = GetBlock<char[125]>();
    MT_REQUIRE(ctx, secondBlock != nullptr);

    FreeBlock<char[125]>(static_cast<void*>(firstBlock));

    void* thirdBlock  = GetBlock<char[125]>();
    MT_REQUIRE(ctx, thirdBlock != nullptr);
}

MT_TEST(MemoryPoolTests, FreeingMemory2, "unit") {
    MakeAlloc<char[200]>(5);

    std::vector<void*> allocatedBlocks;

    for (int32_t i = 0; i < 5; ++i) {
        allocatedBlocks.push_back(GetBlock<char[200]>());
        MT_REQUIRE(ctx, allocatedBlocks.back() != nullptr);
    }

    for (int32_t i = 0; i < 5; ++i) {
        void* block = nullptr;
        int8_t allocationFailed = false;

        try {
            block = GetBlock<char[200]>();
        } catch (const std::bad_alloc&) {
            allocationFailed = true;
        }

        MT_REQUIRE(ctx, block == nullptr);
        MT_REQUIRE_EQ(ctx, allocationFailed, true);
    }

    for (int32_t i = 0; i < 5; ++i) {
        FreeBlock<char[200]>(static_cast<void*>(allocatedBlocks[i]));
    }

    for (int32_t i = 0; i < 5; ++i) {
        allocatedBlocks[i] = GetBlock<char[200]>();
        MT_REQUIRE(ctx, allocatedBlocks[i] != nullptr);
    }

    for (int32_t i = 0; i < 5; ++i) {
        FreeBlock<char[200]>(static_cast<void*>(allocatedBlocks[i]));
    }
}

MT_TEST(MemoryPoolTests, FreeingMemory3_DestructorCalled, "unit") {
    MakeAlloc<CustomDataType>(1);

    int8_t* destructorCalled = static_cast<int8_t*>(std::malloc(sizeof(int8_t)));
    *destructorCalled = false;

    auto* customDTObject =
        new (GetBlock<CustomDataType>()) CustomDataType(destructorCalled);

    FreeBlock<CustomDataType>(static_cast<void*>(customDTObject));
    MT_REQUIRE_EQ(ctx, *destructorCalled, true);

    // Optional: free test heap flag to avoid leak in test harness
    std::free(destructorCalled);
}

