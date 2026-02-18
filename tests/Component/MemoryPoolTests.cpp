// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "TestUtils.h"
#include "MemoryPool.h"
#include "URMTests.h"

#define TEST_CLASS "COMPONENT"

// Test structure, used for allocation testing
struct TestBuffer {
    int32_t testId;
    double score;
    int8_t isDuplicate;
};

URM_TEST(TestMemoryPoolBasicAllocation1, {
    MakeAlloc<TestBuffer>(1);

    void* block = GetBlock<TestBuffer>();
    E_ASSERT((block != nullptr));

    FreeBlock<TestBuffer>(static_cast<void*>(block));
})

URM_TEST(TestMemoryPoolBasicAllocation2, {
    MakeAlloc<char[250]>(2);

    void* firstBlock = GetBlock<char[250]>();
    E_ASSERT((firstBlock != nullptr));

    void* secondBlock = GetBlock<char[250]>();
    E_ASSERT((secondBlock != nullptr));

    FreeBlock<char[250]>(firstBlock);
    FreeBlock<char[250]>(secondBlock);
})

struct ListNode {
    int32_t val;
    ListNode* next;
};

URM_TEST(TestMemoryPoolBasicAllocation3, {
    MakeAlloc<ListNode>(10);
    ListNode* head = nullptr;
    ListNode* cur = nullptr;

    for(int32_t i = 0; i < 10; i++) {
        ListNode* node = (ListNode*)GetBlock<ListNode>();
        E_ASSERT((node != nullptr));

        node->val = i + 1;
        node->next = nullptr;

        if(head == nullptr) {
            head = node;
            cur = node;
        } else {
            cur->next = node;
            cur = cur->next;
        }
    }

    cur = head;
    int32_t counter = 1;
    while(cur != nullptr) {
        E_ASSERT((cur->val == counter));
        ListNode* next = cur->next;
        FreeBlock<ListNode>(static_cast<void*>(cur));

        cur = next;
        counter++;
    }
})

struct CustomRequest {
    int32_t requestID;
    int64_t requestTimestamp;
};

URM_TEST(TestMemoryPoolBasicAllocation4, {
    MakeAlloc<std::vector<CustomRequest*>>(1);
    MakeAlloc<CustomRequest>(20);

    std::vector<CustomRequest*>* requests =
        new (GetBlock<std::vector<CustomRequest*>>()) std::vector<CustomRequest*>;

    E_ASSERT((requests != nullptr));

    for(int32_t i = 0; i < 15; i++) {
        CustomRequest* request = (CustomRequest*) GetBlock<CustomRequest>();
        E_ASSERT((request != nullptr));

        request->requestID = i + 1;
        request->requestTimestamp = 100 * (i + 3);
        requests->push_back(request);
    }

    for(int32_t i = 0; i < (int32_t)requests->size(); i++) {
        E_ASSERT(((*requests)[i]->requestID == i + 1));
        E_ASSERT(((*requests)[i]->requestTimestamp == 100 * (i + 3)));

        FreeBlock<CustomRequest>(static_cast<void*>((*requests)[i]));
    }

    FreeBlock<std::vector<CustomRequest*>>(static_cast<void*>(requests));
})

class DataHub {
private:
    int32_t mFolderCount;
    int32_t mUserCount;
    std::string mOrgName;
public:
    DataHub(int32_t mFolderCount, int32_t mUserCount, std::string mOrgName) {
        this->mFolderCount = mFolderCount;
        this->mUserCount = mUserCount;
        this->mOrgName = mOrgName;
    }
};

URM_TEST(TestMemoryPoolBasicAllocation5, {
    MakeAlloc<DataHub>(1);

    DataHub* dataHubObj = new(GetBlock<DataHub>()) DataHub(30, 17, "XYZ-co");

    E_ASSERT((dataHubObj != nullptr));

    FreeBlock<DataHub>(static_cast<void*>(dataHubObj));
})

URM_TEST(TestMemoryPoolBasicAllocation6, {
    int8_t allocationFailed = false;
    void* block = nullptr;

    try {
        block = GetBlock<char[120]>();
    } catch(const std::bad_alloc& e) {
        allocationFailed = true;
    }

    E_ASSERT((block == nullptr));
    E_ASSERT((allocationFailed == true));
})

URM_TEST(TestMemoryPoolBasicAllocation7, {
    MakeAlloc<int64_t>(1);

    void* block = nullptr;
    try {
        block = GetBlock<int64_t>();
    } catch(const std::bad_alloc& e) {
    }

    E_ASSERT((block != nullptr));

    block = nullptr;
    try {
        block = GetBlock<char[8]>();
    } catch(const std::bad_alloc& e) {}

    E_ASSERT((block == nullptr));
})

URM_TEST(TestMemoryPoolFreeingMemory1, {
    MakeAlloc<char[125]>(2);

    void* firstBlock = GetBlock<char[125]>();
    E_ASSERT((firstBlock != nullptr));

    void* secondBlock = GetBlock<char[125]>();
    E_ASSERT((secondBlock != nullptr));

    FreeBlock<char[125]>(static_cast<void*>(firstBlock));

    void* thirdBlock = GetBlock<char[125]>();
    E_ASSERT((thirdBlock != nullptr));
})

URM_TEST(TestMemoryPoolFreeingMemory2, {
    MakeAlloc<char[200]>(5);

    std::vector<void*> allocatedBlocks;

    for(int32_t i = 0; i < 5; i++) {
        allocatedBlocks.push_back(GetBlock<char[200]>());
        E_ASSERT((allocatedBlocks.back() != nullptr));
    }

    for(int32_t i = 0; i < 5; i++) {
        void* block = nullptr;
        int8_t allocationFailed = false;

        try {
            block = GetBlock<char[200]>();
        } catch(const std::bad_alloc& e) {
            allocationFailed = true;
        }

        E_ASSERT((block == nullptr));
        E_ASSERT((allocationFailed == true));
    }

    for(int32_t i = 0; i < 5; i++) {
        FreeBlock<char[200]>(static_cast<void*>(allocatedBlocks[i]));
    }

    for(int32_t i = 0; i < 5; i++) {
        allocatedBlocks[i] = GetBlock<char[200]>();
        E_ASSERT((allocatedBlocks[i] != nullptr));
    }

    for(int32_t i = 0; i < 5; i++) {
        FreeBlock<char[200]>(static_cast<void*>(allocatedBlocks[i]));
    }
})

class CustomDataType {
private:
    int8_t* mDestructorCalled;

public:
    CustomDataType(int8_t* mDestructorCalled) {
        this->mDestructorCalled = mDestructorCalled;
    }

    ~CustomDataType() {
        *this->mDestructorCalled = true;
    }
};

URM_TEST(TestMemoryPoolFreeingMemory, {
    MakeAlloc<CustomDataType>(1);

    int8_t* destructorCalled = (int8_t*) malloc(sizeof(int8_t));
    *destructorCalled = false;

    CustomDataType* customDTObject =
        new(GetBlock<CustomDataType>()) CustomDataType(destructorCalled);

    FreeBlock<CustomDataType>(static_cast<void*>(customDTObject));
    E_ASSERT((*destructorCalled == true));
})
