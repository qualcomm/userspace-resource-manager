// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <iostream>

#include "TestUtils.h"
#include "ThreadPool.h"
#include "URMTests.h"

#define TEST_CLASS "COMPONENT"

static std::mutex taskLock;
static std::condition_variable taskCV;

static int32_t sharedVariable = 0;
static std::string sharedString = "";
static int8_t taskCondition = false;

static void threadPoolTask(void* arg) {
	if(arg != nullptr) {
		*(int32_t*)arg = 64;
	}
}

static void threadPoolLongDurationTask(void* arg) {
	std::this_thread::sleep_for(std::chrono::seconds(*(int32_t*)arg));
}

URM_TEST(TestThreadPoolTaskPickup1, {
	ThreadPool* threadPool = new ThreadPool(1, 1);
	std::this_thread::sleep_for(std::chrono::seconds(1));

	int32_t* ptr = (int32_t*) malloc(sizeof(int32_t));
	*ptr = 49;

	int8_t status = threadPool->enqueueTask(threadPoolTask, (void*)ptr);
	E_ASSERT((status == true));
	std::this_thread::sleep_for(std::chrono::seconds(1));

	E_ASSERT((*ptr == 64));

	free(ptr);
	delete threadPool;
	threadPool = nullptr;
})

URM_TEST(TestThreadPoolEnqueueStatus1, {
	ThreadPool* threadPool = new ThreadPool(2, 2);
	std::this_thread::sleep_for(std::chrono::seconds(1));

	int32_t* ptr = (int32_t*) malloc(sizeof(int32_t));
	*ptr = 14;

	int8_t ret = threadPool->enqueueTask(threadPoolTask, (void*)ptr);
	E_ASSERT((ret == true));
	std::this_thread::sleep_for(std::chrono::seconds(1));

	E_ASSERT((*ptr == 64));

	free(ptr);
	delete threadPool;
	threadPool = nullptr;
})

URM_TEST(TestThreadPoolEnqueueStatus2_1, {
	ThreadPool* threadPool = new ThreadPool(1, 1);
	std::this_thread::sleep_for(std::chrono::seconds(1));

	int32_t* ptr = (int32_t*) malloc(sizeof(int32_t));
	*ptr = 2;

	int8_t ret1 = (threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr));
	int8_t ret2 = (threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr));

	E_ASSERT((ret1 == true));
	E_ASSERT((ret2 == true));

	std::this_thread::sleep_for(std::chrono::seconds(8));
	free(ptr);
	delete threadPool;
})

URM_TEST(TestThreadPoolEnqueueStatus2_2, {
	ThreadPool* threadPool = new ThreadPool(2, 2);
	std::this_thread::sleep_for(std::chrono::seconds(1));

	int32_t* ptr = (int32_t*) malloc(sizeof(int32_t));
	*ptr = 2;

	int8_t ret1 = (threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr));
	int8_t ret2 = (threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr));
	int8_t ret3 = (threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr));

	E_ASSERT((ret1 == true));
	E_ASSERT((ret2 == true));
	E_ASSERT((ret3 == true));

	std::this_thread::sleep_for(std::chrono::seconds(8));
	free(ptr);
	delete threadPool;
})

static void helperFunction(void* arg) {
    (void)arg;
    for(int32_t i = 0; i < 1e7; i++) {
        taskLock.lock();
        sharedVariable++;
        taskLock.unlock();
    }
}

URM_TEST(TestThreadPoolTaskProcessing1, {
	sharedVariable = 0;

	ThreadPool* threadPool = new ThreadPool(2, 2);
	std::this_thread::sleep_for(std::chrono::seconds(1));

	int8_t ret1 = threadPool->enqueueTask(helperFunction, nullptr);
	int8_t ret2 = threadPool->enqueueTask(helperFunction, nullptr);

	E_ASSERT((ret1 == true));
	E_ASSERT((ret2 == true));

	// Wait for both tasks to complete
	std::this_thread::sleep_for(std::chrono::seconds(3));

	delete threadPool;

	std::cout<<"sharedVariable value = "<<sharedVariable<<std::endl;
	E_ASSERT((sharedVariable == 2e7));
})

// Lambda Function
URM_TEST(TestThreadPoolTaskProcessing2, {
	ThreadPool* threadPool = new ThreadPool(1, 1);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	sharedVariable = 0;

	int8_t ret = threadPool->enqueueTask([&](void* arg) {
        (void)arg;
		for(int32_t i = 0; i < 1e7; i++) {
			sharedVariable++;
		}
	}, nullptr);

	E_ASSERT((ret == true));

	// Wait for both tasks to complete
	std::this_thread::sleep_for(std::chrono::seconds(3));

	E_ASSERT((sharedVariable == 1e7));

	delete threadPool;
})

URM_TEST(TestThreadPoolTaskProcessing3, {
	ThreadPool* threadPool = new ThreadPool(1, 1);
	std::this_thread::sleep_for(std::chrono::seconds(1));

	sharedVariable = 0;
	int32_t* callID = (int32_t*) malloc(sizeof(int32_t));
	*callID = 56;

	int8_t ret = threadPool->enqueueTask([&](void* arg) {
		E_ASSERT((arg != nullptr));
		sharedVariable = *(int32_t*)arg;
	}, (void*)callID);

	E_ASSERT((ret == true));

	// Wait for both tasks to complete
	std::this_thread::sleep_for(std::chrono::seconds(3));

	E_ASSERT((sharedVariable == *callID));

	free(callID);
	delete threadPool;
})

static void taskAFunc(void* arg) {
    (void)arg;
    std::unique_lock<std::mutex> uniqueLock(taskLock);
    sharedString.push_back('A');
    taskCondition = true;
    taskCV.notify_one();
}

static void taskBFunc(void* arg) {
    (void)arg;
    std::unique_lock<std::mutex> uniqueLock(taskLock);
    while(!taskCondition) {
        taskCV.wait(uniqueLock);
    }
    sharedString.push_back('B');
}

URM_TEST(TestThreadPoolTaskProcessing4, {
	ThreadPool* threadPool = new ThreadPool(2, 2);
	std::this_thread::sleep_for(std::chrono::seconds(1));

	int8_t ret1 = threadPool->enqueueTask(taskAFunc, nullptr);
	int8_t ret2 = threadPool->enqueueTask(taskBFunc, nullptr);

	E_ASSERT((ret1 == true));
	E_ASSERT((ret2 == true));

	// Wait for both tasks to complete
	std::this_thread::sleep_for(std::chrono::seconds(1));

	E_ASSERT((sharedString == "AB"));

	delete threadPool;
})

// Tests For Thread Pool Scaling
URM_TEST(TestThreadPoolEnqueueStatusWithExpansion1, {
	ThreadPool* threadPool = new ThreadPool(2, 3);
	std::this_thread::sleep_for(std::chrono::seconds(1));

	int32_t* ptr = (int32_t*) malloc(sizeof(int32_t));
	*ptr = 3;

	int8_t ret1 = (threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr));
	int8_t ret2 = (threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr));
	int8_t ret3 = (threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr));

	E_ASSERT((ret1 == true));
	E_ASSERT((ret2 == true));
	E_ASSERT((ret3 == true));

	std::this_thread::sleep_for(std::chrono::seconds(8));
	free(ptr);
	delete threadPool;
})

URM_TEST(TestThreadPoolEnqueueStatusWithExpansion2, {
	ThreadPool* threadPool = new ThreadPool(2, 4);
	std::this_thread::sleep_for(std::chrono::seconds(1));

	int32_t* ptr = (int32_t*) malloc(sizeof(int32_t));
	*ptr = 3;

	int8_t ret1 = (threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr));
	int8_t ret2 = (threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr));
	int8_t ret3 = (threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr));
	int8_t ret4 = (threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr));

	E_ASSERT((ret1 == true));
	E_ASSERT((ret2 == true));
	E_ASSERT((ret3 == true));
	E_ASSERT((ret4 == true));

	std::this_thread::sleep_for(std::chrono::seconds(8));
	free(ptr);
	delete threadPool;
})
