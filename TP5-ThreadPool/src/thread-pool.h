/**
 * File: thread-pool.h
 * -------------------
 * This class defines the ThreadPool class, which accepts a collection
 * of thunks (which are zero-argument functions that don't return a value)
 * and schedules them in a FIFO manner to be executed by a constant number
 * of child threads that exist solely to invoke previously scheduled thunks.
 */

#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>     // for size_t
#include <functional>  // for the function template used in the schedule signature
#include <thread>      // for thread
#include <vector>      // for vector
#include <queue>       // for queue
#include <mutex>       // for mutex
#include <condition_variable> // for condition_variable
#include "Semaphore.h" // for Semaphore

using namespace std;

/**
 * @brief Represents a worker in the thread pool.
 * 
 * The `worker_t` struct contains information about a worker 
 * thread in the thread pool. Includes the thread object, 
 * availability status, the task to be executed, and a semaphore 
 * to signal when work is ready for the worker to process.
 */
typedef struct worker {
    thread ts;                          // thread handle
    function<void(void)> thunk;         // task to execute
    bool available;                     // worker availability status
    Semaphore workReady;                // semaphore to signal work is ready
    Semaphore workDone;                 // semaphore to signal work is completed
    
    worker() : available(true), workReady(0), workDone(0) {}
} worker_t;

class ThreadPool {
  public:

  /**
  * Constructs a ThreadPool configured to spawn up to the specified
  * number of threads.
  */
    ThreadPool(size_t numThreads);

  /**
  * Schedules the provided thunk (which is something that can
  * be invoked as a zero-argument function without a return value)
  * to be executed by one of the ThreadPool's threads as soon as
  * all previously scheduled thunks have been handled.
  */
    void schedule(const function<void(void)>& thunk);

  /**
  * Blocks and waits until all previously scheduled thunks
  * have been executed in full.
  */
    void wait();

  /**
  * Waits for all previously scheduled thunks to execute, and then
  * properly brings down the ThreadPool and any resources tapped
  * over the course of its lifetime.
  */
    ~ThreadPool();
    
  private:

    void worker(int id);
    void dispatcher();
    
    thread dt;                              // dispatcher thread handle
    vector<worker_t> wts;                   // worker thread handles
    bool done;                              // flag to indicate the pool is being destroyed
    
    // Task queue management
    queue<function<void(void)>> taskQueue;  // queue of tasks to execute
    mutex queueLock;                        // mutex to protect the queue of tasks
    condition_variable taskAvailable;       // notify dispatcher when tasks are available
    
    // Worker availability management
    queue<int> availableWorkers;            // queue of available worker IDs
    mutex workerLock;                       // mutex to protect worker availability
    condition_variable workerAvailable;     // notify dispatcher when workers are available
    
    // Wait functionality
    int activeTasks;                        // number of tasks currently being executed
    mutex waitLock;                         // mutex to protect wait state
    condition_variable allTasksDone;        // notify when all tasks are completed
    
    /* ThreadPools are the type of thing that shouldn't be cloneable, since it's
    * not clear what it means to clone a ThreadPool (should copies of all outstanding
    * functions to be executed be copied?).
    *
    * In order to prevent cloning, we remove the copy constructor and the
    * assignment operator.  By doing so, the compiler will ensure we never clone
    * a ThreadPool. */
    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};
#endif