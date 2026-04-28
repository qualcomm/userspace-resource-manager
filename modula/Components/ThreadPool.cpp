// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ThreadPool.h"

TaskNode::TaskNode() {
    this->taskCallback = nullptr;
    this->args = nullptr;
    this->next = nullptr;
}

TaskNode::~TaskNode() {
    if(this->taskCallback != nullptr) {
        FreeBlock<std::function<void(void*)>> (this->taskCallback);
        this->taskCallback = nullptr;
    }
    this->next = nullptr;
}

TaskQueue::TaskQueue() {
    this->size = 0;
    this->head = this->tail = nullptr;
}

void TaskQueue::add(TaskNode* taskNode) {
    if(taskNode == nullptr) return;

    if(this->head == nullptr) {
        this->head = taskNode;
        this->tail = taskNode;
    } else {
        this->tail->next = taskNode;
        this->tail = this->tail->next;
    }
    this->size++;
}

TaskNode* TaskQueue::poll() {
    TaskNode* taskNode = nullptr;
    if(this->size > 0) {
        if(this->head != nullptr) {
            taskNode = this->head;
            this->head = this->head->next;

            if(this->head == nullptr) {
                this->tail = nullptr;
            }
            this->size--;
        }
    }
    return taskNode;
}

int8_t TaskQueue::isEmpty() {
    return this->size == 0;
}

int32_t TaskQueue::getSize() {
    return this->size;
}

TaskQueue::~TaskQueue() {
    try {
        while(!this->isEmpty()) {
            TaskNode* taskNode = this->poll();
            if(taskNode != nullptr) {
                FreeBlock<TaskNode>(SafeStaticCast(taskNode, void*));
            }
        }
    } catch(const std::exception& e) {}
}

int8_t ThreadPool::threadRoutineHelper(int8_t isCoreThread) {
    try {
        std::unique_lock<std::mutex> threadPoolUniqueLock(this->mThreadPoolMutex);

        // Check for any Pending tasks
        if(!this->mTerminatePool && this->mCurrentTasks->getSize() == 0) {
            // Update the Count of threads in Use
            if(isCoreThread) {
                this->mThreadPoolCond.wait(threadPoolUniqueLock,
                                           [this]{return this->mCurrentTasks->getSize() > 0 ||
                                                         this->mTerminatePool;});
            } else {
                // Expandable Thread
                int8_t awakeStatus = this->mThreadPoolCond.wait_for(
                                            threadPoolUniqueLock,
                                            std::chrono::seconds(10 * 60),
                                            [this]{return this->mCurrentTasks->getSize() > 0 ||
                                                          this->mTerminatePool;});
                if(!awakeStatus) {
                    // If the Thread was woken up due to the Time Interval expiring, then
                    // Proceed with Thread Termination. As it indicates the Thread has been
                    // idle for the last 10 mins.
                    this->mCurrentThreadsCount--;
                    return true;
                }
            }
        }

        // Check if the pool has been terminated. If so, exit the thread.
        if(this->mTerminatePool == true) {
            return true;
        }

        TaskNode* taskNode = nullptr;
        if(this->mCurrentTasks->getSize() > 0) {
            taskNode = this->mCurrentTasks->poll();
        }

        if(taskNode != nullptr && taskNode->taskCallback != nullptr) {
            std::function<void(void*)> task = *taskNode->taskCallback;
            void* args = taskNode->args;

            threadPoolUniqueLock.unlock();

            if(task != nullptr) {
                task(args);
            }

            // Free the TaskNode, before proceeding to the next task
            try {
                FreeBlock<TaskNode>(SafeStaticCast(taskNode, void*));
            } catch(const std::invalid_argument& e) {}
        }

        return false;

    } catch(const std::system_error& e) {
        TYPELOGV(THREAD_POOL_THREAD_TERMINATED, e.what());
        return true;

    } catch(const std::exception& e) {
        TYPELOGV(THREAD_POOL_THREAD_TERMINATED, e.what());
        return true;
    }
}

int8_t ThreadPool::addNewThread(int8_t isCoreThread) {
    // First Create a ThreadNode for this thread
    ThreadNode* thNode = nullptr;
    try {
        thNode = (ThreadNode*)(GetBlock<ThreadNode>());

    } catch(const std::bad_alloc& e) {
        return false;
    }

    thNode->next = nullptr;
    thNode->th = nullptr;
    thNode->pthreadHandle = 0;
    thNode->usePthread = 0;

    try {
        auto threadStartRoutine = ([this](int8_t isCoreThread) {
            while(true) {
                if(threadRoutineHelper(isCoreThread)) {
                    return;
                }
            }
        });

        try {
            // If custom stack size is specified, use pthread directly
            if(this->mThreadStackSize > 0) {
                pthread_attr_t attr;
                if(pthread_attr_init(&attr) != 0) {
                    FreeBlock<ThreadNode>(static_cast<void*>(thNode));
                    throw std::system_error(errno, std::system_category(), "pthread_attr_init failed");
                }

                if(pthread_attr_setstacksize(&attr, this->mThreadStackSize) != 0) {
                    pthread_attr_destroy(&attr);
                    FreeBlock<ThreadNode>(static_cast<void*>(thNode));
                    throw std::system_error(errno, std::system_category(), "pthread_attr_setstacksize failed");
                }

                // Create a wrapper structure to pass both this pointer and isCoreThread
                struct ThreadArgs {
                    ThreadPool* pool;
                    int8_t isCoreThread;
                };

                ThreadArgs* args = new ThreadArgs{this, isCoreThread};

                int result = pthread_create(&thNode->pthreadHandle, &attr,
                    [](void* arg) -> void* {
                        ThreadArgs* targs = static_cast<ThreadArgs*>(arg);
                        ThreadPool* pool = targs->pool;
                        int8_t isCoreThread = targs->isCoreThread;
                        delete targs;

                        while(true) {
                            if(pool->threadRoutineHelper(isCoreThread)) {
                                return nullptr;
                            }
                        }
                    }, args);

                pthread_attr_destroy(&attr);

                if(result != 0) {
                    delete args;
                    FreeBlock<ThreadNode>(static_cast<void*>(thNode));
                    throw std::system_error(result, std::system_category(), "pthread_create failed");
                }

                thNode->usePthread = 1;
            } else {
                // Use default std::thread creation
                thNode->th = new std::thread(threadStartRoutine, isCoreThread);
                thNode->usePthread = 0;
            }

        } catch(const std::system_error& e) {
            FreeBlock<ThreadNode>(static_cast<void*>(thNode));
            throw;

        } catch(const std::bad_alloc& e) {
            FreeBlock<ThreadNode>(static_cast<void*>(thNode));
            throw;
        }

        // Add this ThreadNode to the ThreadList
        if(this->mThreadQueueHead == nullptr) {
            this->mThreadQueueHead = thNode;
            this->mThreadQueueTail = thNode;
        } else {
            this->mThreadQueueTail->next = thNode;
            this->mThreadQueueTail = this->mThreadQueueTail->next;
        }

        return true;

    } catch(const std::exception& e) {
        TYPELOGV(THREAD_POOL_THREAD_CREATION_FAILURE, e.what());
    }

    return false;
}

ThreadPool::ThreadPool(int32_t desiredCapacity, int32_t maxCapacity, size_t stackSize) {
    this->mThreadQueueHead = this->mThreadQueueTail = nullptr;

    this->mDesiredPoolCapacity = desiredCapacity;
    this->mCurrentThreadsCount = 0;
    this->mThreadStackSize = stackSize;

    if(maxCapacity < desiredCapacity) {
        maxCapacity = desiredCapacity;
    }
    this->mMaxPoolCapacity = maxCapacity;

    this->mTotalTasksCount = 0;
    this->mTerminatePool = false;

    try {
        this->mCurrentTasks = new TaskQueue;

    } catch(const std::bad_alloc& e) {
        TYPELOGV(THREAD_POOL_INIT_FAILURE, e.what());

        if(this->mCurrentTasks != nullptr) {
            delete this->mCurrentTasks;
        }
    }

    MakeAlloc<ThreadNode>(this->mMaxPoolCapacity + 1);
    MakeAlloc<TaskNode>((this->mMaxPoolCapacity + 1) * maxLoadPerThread);
    MakeAlloc<std::function<void(void*)>>((this->mMaxPoolCapacity + 1) * maxLoadPerThread);

    // Add desired number of Threads to the Pool
    for(int32_t i = 0; i < this->mDesiredPoolCapacity; i++) {
        if(this->addNewThread(true)) {
            this->mCurrentThreadsCount++;
        }
    }

    LOGI("RESTUNE_THREAD_POOL",
         "Requested Thread Count = " + std::to_string(this->mDesiredPoolCapacity) + ", "  \
         "Allocated Thread Count = " + std::to_string(this->mCurrentThreadsCount) + ", " \
         "Stack Size = " + std::to_string(this->mThreadStackSize) + " bytes");

    this->mDesiredPoolCapacity = this->mCurrentThreadsCount;
}

TaskNode* ThreadPool::createTaskNode(std::function<void(void*)> callback, void* args) {
    // Create a task Node
    TaskNode* taskNode = nullptr;
    try {
        taskNode = (TaskNode*)(GetBlock<TaskNode>());

    } catch(const std::bad_alloc& e) {
        return nullptr;
    }

    try {
        taskNode->taskCallback = new(GetBlock<std::function<void(void*)>>())
                                     std::function<void(void*)>;

        *taskNode->taskCallback = std::move(callback);

    } catch(const std::bad_alloc& e) {
        FreeBlock<TaskNode>(static_cast<void*>(taskNode));
        return nullptr;
    }

    taskNode->args = args;
    taskNode->next = nullptr;

    return taskNode;
}

int8_t ThreadPool::enqueueTask(std::function<void(void*)> taskCallback, void* args) {
    try {
        if(taskCallback == nullptr) return false;

        std::unique_lock<std::mutex> threadPoolUniqueLock(this->mThreadPoolMutex);
        int8_t taskAccepted = false;

        // First Check if any Thread in the Core Group is Available
        // If it is, assign the Task to that Thread.
        if(this->mCurrentTasks->getSize() <= maxLoadPerThread * this->mCurrentThreadsCount) {
            // Add the task to the Current List
            TaskNode* taskNode = createTaskNode(taskCallback, args);
            if(taskNode == nullptr) {
                throw std::bad_alloc();
            }

            this->mCurrentTasks->add(taskNode);
            taskAccepted = true;
        }

        // Check if the Pool can be expanded to accomodate this Request
        if(!taskAccepted && this->mCurrentThreadsCount < this->mMaxPoolCapacity) {
            // Add the task to the current List
            TaskNode* taskNode = createTaskNode(taskCallback, args);
            if(taskNode == nullptr) {
                throw std::bad_alloc();
            }

            if(this->addNewThread(false)) {
                this->mCurrentThreadsCount++;
            }

            this->mCurrentTasks->add(taskNode);
            taskAccepted = true;
        }

        if(taskAccepted) {
            this->mThreadPoolCond.notify_one();
            return true;
        }

        TYPELOGD(THREAD_POOL_FULL_ALERT);
        return false;

    } catch(const std::bad_alloc& e) {
        TYPELOGV(THREAD_POOL_ENQUEUE_TASK_FAILURE, e.what());
        return false;

    } catch(const std::system_error& e) {
        TYPELOGV(THREAD_POOL_ENQUEUE_TASK_FAILURE, e.what());
        return false;
    }

    TYPELOGD(THREAD_POOL_FULL_ALERT);
    return false;
}

ThreadPool::~ThreadPool() {
    try {
        // Terminate all the threads
        this->mThreadPoolMutex.lock();
        this->mTerminatePool = true;
        this->mThreadPoolCond.notify_all();
        this->mThreadPoolMutex.unlock();

        ThreadNode* thNode = this->mThreadQueueHead;
        while(thNode != nullptr) {
            ThreadNode* nextNode = thNode->next;

            try {
                if(thNode->usePthread) {
                    // Join pthread
                    if(thNode->pthreadHandle != 0) {
                        pthread_join(thNode->pthreadHandle, nullptr);
                    }
                } else {
                    // Join std::thread
                    if(thNode->th != nullptr && thNode->th->joinable()) {
                        thNode->th->join();
                    }
                }
            } catch(const std::exception& e) {}

            if(thNode->th != nullptr) {
                delete thNode->th;
                thNode->th = nullptr;
            }

            thNode = nextNode;
        }

        thNode = this->mThreadQueueHead;
        while(thNode != nullptr) {
            ThreadNode* nextNode = thNode->next;
            FreeBlock<ThreadNode>(static_cast<void*>(thNode));
            thNode = nextNode;
        }

        delete this->mCurrentTasks;

    } catch(const std::exception& e) {}
}
