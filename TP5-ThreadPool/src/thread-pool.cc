/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
using namespace std;

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), done(false), activeTasks(0) {
    // Initialize available workers queue with all worker IDs
    for (size_t i = 0; i < numThreads; i++) {
        availableWorkers.push(i);
    }
    
    // Create and start worker threads
    for (size_t i = 0; i < numThreads; i++) {
        wts[i].ts = thread([this, i] { worker(i); });
    }
    
    // Create and start dispatcher thread
    dt = thread([this] { dispatcher(); });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    {
        lock_guard<mutex> lg(queueLock);
        taskQueue.push(thunk);
    }
    
    // Notify dispatcher that a new task is available
    taskAvailable.notify_one();
}

void ThreadPool::wait() {
    unique_lock<mutex> ul(waitLock);
    allTasksDone.wait(ul, [this] { 
        lock_guard<mutex> lg(queueLock);
        return taskQueue.empty() && activeTasks == 0; 
    });
}

ThreadPool::~ThreadPool() {
    // Signal that we're shutting down
    {
        lock_guard<mutex> lg(queueLock);
        done = true;
    }
    
    // Notify dispatcher to wake up and exit
    taskAvailable.notify_all();
    
    // Wait for dispatcher to finish
    if (dt.joinable()) {
        dt.join();
    }
    
    // Signal all workers to wake up and exit
    for (size_t i = 0; i < wts.size(); i++) {
        wts[i].workReady.signal();
    }
    
    // Wait for all workers to finish
    for (size_t i = 0; i < wts.size(); i++) {
        if (wts[i].ts.joinable()) {
            wts[i].ts.join();
        }
    }
}

void ThreadPool::dispatcher() {
    while (true) {
        // Wait for a task to be available
        unique_lock<mutex> taskLock(queueLock);
        taskAvailable.wait(taskLock, [this] { return !taskQueue.empty() || done; });
        
        // Check if we should exit
        if (done && taskQueue.empty()) {
            taskLock.unlock();
            break;
        }
        
        // If no tasks available, continue
        if (taskQueue.empty()) {
            taskLock.unlock();
            continue;
        }
        
        // Get the next task
        function<void(void)> task = taskQueue.front();
        taskQueue.pop();
        taskLock.unlock();
        
        // Wait for an available worker
        unique_lock<mutex> workerLock_ul(workerLock);
        workerAvailable.wait(workerLock_ul, [this] { return !availableWorkers.empty() || done; });
        
        // Check if we should exit
        if (done) {
            workerLock_ul.unlock();
            break;
        }
        
        // Get an available worker
        int workerId = availableWorkers.front();
        availableWorkers.pop();
        workerLock_ul.unlock();
        
        // Mark worker as unavailable and assign task
        wts[workerId].available = false;
        wts[workerId].thunk = task;
        
        // Increment active tasks counter
        {
            lock_guard<mutex> lg(waitLock);
            activeTasks++;
        }
        
        // Signal the worker to start working
        wts[workerId].workReady.signal();
    }
}

void ThreadPool::worker(int id) {
    while (true) {
        // Wait for work to be assigned
        wts[id].workReady.wait();
        
        // Check if we should exit
        if (done) {
            break;
        }
        
        // Execute the assigned task
        wts[id].thunk();
        
        // Mark worker as available again
        {
            lock_guard<mutex> lg(workerLock);
            wts[id].available = true;
            availableWorkers.push(id);
        }
        
        // Decrement active tasks counter and notify if all tasks are done
        {
            lock_guard<mutex> lg(waitLock);
            activeTasks--;
            if (activeTasks == 0) {
                allTasksDone.notify_all();
            }
        }
        
        // Notify dispatcher that this worker is available
        workerAvailable.notify_one();
    }
}