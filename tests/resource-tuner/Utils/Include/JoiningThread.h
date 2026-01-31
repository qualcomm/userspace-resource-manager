#pragma once
#include <thread>

struct joining_thread {
    std::thread t;
    explicit joining_thread(std::thread&& tt) : t(std::move(tt)) {}
    ~joining_thread() { if (t.joinable()) t.join(); }

    joining_thread(const joining_thread&) = delete;
    joining_thread& operator=(const joining_thread&) = delete;
    joining_thread(joining_thread&&) = delete;
    joining_thread& operator=(joining_thread&&) = delete;
};

