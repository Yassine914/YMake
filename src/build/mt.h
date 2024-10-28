#pragma once

#include "../core/defines.h"
#include "../core/error.h"

#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <queue>
#include <condition_variable>

using std::condition_variable;
using std::function;
using std::mutex;
using std::queue;
using std::thread;
using std::unique_lock;
using std::vector;

// get max number of threads based on hardware concurrency
inline usize GetMaxThreads()
{
    usize maxThreads = std::thread::hardware_concurrency();
    if(maxThreads == 0) // returns 0 if not well defined.
    {
        maxThreads = 2; // Fallback to 2 threads if hardware concurrency is not defined
    }
    return maxThreads;
}

namespace Y {

class ThreadPool
{
    private:
    // vector of worker threads
    vector<thread> workers;

    // task queue (handles lambdas with arguments).
    queue<function<void()>> tasks;

    // synchronization
    mutex qMutex;
    condition_variable cv;
    bool stop;

    public:
    ThreadPool() : stop(false)
    {
        const usize MAX_THREADS = GetMaxThreads();
        for(size_t i = 0; i < MAX_THREADS; ++i)
        {
            workers.emplace_back([this] {
                while(true)
                {
                    function<void()> task;

                    {
                        unique_lock<mutex> lock(this->qMutex);
                        this->cv.wait(lock, [this] { return this->stop || !this->tasks.empty(); });

                        if(this->stop && this->tasks.empty())
                            return;

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            });
        }
    }

    void AddTask(function<void()> task)
    {
        {
            unique_lock<mutex> lock(qMutex);
            tasks.push(std::move(task));
        }
        cv.notify_one();
    }

    void JoinAll()
    {
        { // scope for mutex.
            unique_lock<mutex> lock(qMutex);
            stop = true;
        }
        cv.notify_all();

        for(thread &worker : workers)
        {
            if(worker.joinable())
                worker.join();
        }
    }

    void Lock() { qMutex.lock(); }

    void Unlock() { qMutex.unlock(); }

    ~ThreadPool() { JoinAll(); }
};

} // namespace Y
