/**
 * File: tpcustomtest.cc
 * ---------------------
 * Unit tests *you* write to exercise the ThreadPool in a variety
 * of ways.
 */

#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <functional>
#include <cstring>
#include <mutex>
#include <atomic>
#include <vector>
#include <chrono>
#include <sys/types.h>
#include <unistd.h> 
#include <dirent.h> 
#include <exception>
#include <iomanip>

#include "thread-pool.h"

using namespace std;

void sleep_for(int slp){
    this_thread::sleep_for(chrono::milliseconds(slp));
}

static mutex oslock;

static const size_t kNumThreads = 4;
static const size_t kNumFunctions = 10;

static bool simpleTest() {
    try {
        ThreadPool pool(kNumThreads);
        for (size_t id = 0; id < kNumFunctions; id++) {
            pool.schedule([id] {
                size_t sleepTime = (id % 3) * 10;
                sleep_for(sleepTime);
            });
        }
        pool.wait();
        return true;
    } catch (...) {
        return false;
    }
}

static bool singleThreadNoWaitTest() {
    try {
        ThreadPool pool(4);
        pool.schedule([&] {});
        sleep_for(1000);
        return true;
    } catch (...) {
        return false;
    }
}

static bool singleThreadSingleWaitTest() {
    try {
        ThreadPool pool(4);
        pool.schedule([] { sleep_for(1000); });
        pool.wait();
        return true;
    } catch (...) {
        return false;
    }
}

static bool noThreadsDoubleWaitTest() {
    try {
        ThreadPool pool(4);
        pool.wait();
        pool.wait();
        return true;
    } catch (...) {
        return false;
    }
}

static bool reuseThreadPoolTest() {
    try {
        ThreadPool pool(4);
        for (size_t i = 0; i < 16; i++) {
            pool.schedule([] { sleep_for(50); });
        }
        pool.wait();
        pool.schedule([] { sleep_for(1000); });
        pool.wait();
        return true;
    } catch (...) {
        return false;
    }
}

static bool stressTest() {
    try {
        const size_t numTasks = 1000;
        const size_t numThreads = 8;
        atomic<int> counter(0);
        ThreadPool pool(numThreads);
        for (size_t i = 0; i < numTasks; i++) {
            pool.schedule([&counter, i] {
                counter++;
                sleep_for(i % 5);
            });
        }
        pool.wait();
        return counter.load() == (int)numTasks;
    } catch (...) {
        return false;
    }
}

static bool multipleWaitTest() {
    try {
        ThreadPool pool(3);
        atomic<int> phase1(0), phase2(0);
        for (int i = 0; i < 5; i++) {
            pool.schedule([&phase1, i] {
                sleep_for(100);
                phase1++;
            });
        }
        pool.wait();
        for (int i = 0; i < 3; i++) {
            pool.schedule([&phase2, i] {
                sleep_for(50);
                phase2++;
            });
        }
        pool.wait();
        return phase1.load() == 5 && phase2.load() == 3;
    } catch (...) {
        return false;
    }
}

static bool concurrentScheduleTest() {
    try {
        ThreadPool pool(4);
        atomic<int> counter(0);
        vector<thread> schedulers;
        for (int i = 0; i < 3; i++) {
            schedulers.emplace_back([&pool, &counter, i] {
                for (int j = 0; j < 20; j++) {
                    pool.schedule([&counter, i, j] {
                        counter++;
                        sleep_for(10);
                    });
                    sleep_for(5);
                }
            });
        }
        for (auto& scheduler : schedulers) {
            scheduler.join();
        }
        pool.wait();
        return counter.load() == 60;
    } catch (...) {
        return false;
    }
}

static bool loadBalanceTest() {
    try {
        const size_t numThreads = 4;
        ThreadPool pool(numThreads);
        vector<atomic<int>> threadWorkCount(numThreads);
        for (auto& count : threadWorkCount) count = 0;
        for (int i = 0; i < 100; i++) {
            pool.schedule([&threadWorkCount, i] {
                size_t threadId = hash<thread::id>{}(this_thread::get_id()) % threadWorkCount.size();
                threadWorkCount[threadId]++;
                sleep_for(1);
            });
        }
        pool.wait();
        int total = 0;
        for (size_t i = 0; i < threadWorkCount.size(); i++) {
            total += threadWorkCount[i].load();
        }
        return total == 100;
    } catch (...) {
        return false;
    }
}

static bool exceptionHandleTest() {
    try {
        ThreadPool pool(2);
        atomic<int> successCount(0);
        for (int i = 0; i < 10; i++) {
            pool.schedule([&successCount, i] {
                try {
                    if (i == 3 || i == 7) {
                        throw runtime_error("Simulated error in task");
                    }
                    successCount++;
                } catch (...) {}
            });
        }
        pool.wait();
        return successCount.load() == 8;
    } catch (...) {
        return false;
    }
}

static bool rapidScheduleWaitTest() {
    try {
        ThreadPool pool(3);
        for (int round = 0; round < 5; round++) {
            atomic<int> roundCounter(0);
            for (int i = 0; i < 5; i++) {
                pool.schedule([&roundCounter, round, i] {
                    roundCounter++;
                    sleep_for(50);
                });
            }
            pool.wait();
            if (roundCounter.load() != 5) return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

static bool memoryUsageTest() {
    try {
        for (int i = 0; i < 5; i++) {
            ThreadPool pool(2);
            atomic<int> taskCount(0);
            for (int j = 0; j < 10; j++) {
                pool.schedule([&taskCount, i, j] {
                    taskCount++;
                    sleep_for(10);
                });
            }
            pool.wait();
            if (taskCount.load() != 10) return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

// Estructura para asociar nombre y función
struct testEntry {
    string name;
    bool (*fn)();
};

int main() {
    vector<testEntry> tests = {
        {"single-thread-no-wait", singleThreadNoWaitTest},
        {"single-thread-single-wait", singleThreadSingleWaitTest},
        {"no-threads-double-wait", noThreadsDoubleWaitTest},
        {"reuse-thread-pool", reuseThreadPoolTest},
        {"simple", simpleTest},
        {"stress", stressTest},
        {"multiple-wait", multipleWaitTest},
        {"concurrent-schedule", concurrentScheduleTest},
        {"load-balance", loadBalanceTest},
        {"exception-handle", exceptionHandleTest},
        {"rapid-schedule-wait", rapidScheduleWaitTest},
        {"memory-usage", memoryUsageTest},
    };

    int failed = 0;
    for (const auto& test : tests) {
        bool ok = false;
        try {
            ok = test.fn();
        } catch (...) {
            ok = false;
        }
        cout << left << setw(30) << test.name << (ok ? "OK" : "FAIL") << endl;
        if (!ok) failed++;
    }
    cout << "----------------------------------------" << endl;
    cout << "Resumen: " << failed << " test(s) fallaron de " << tests.size() << endl;
    return failed;
}