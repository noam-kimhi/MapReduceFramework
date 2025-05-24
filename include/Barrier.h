#ifndef BARRIER_H
#define BARRIER_H

#include <mutex>
#include <condition_variable>

/**
 * @class Barrier
 * @brief A synchronization primitive that allows a group of threads to wait for each other
 *        at a certain point before proceeding.
 *
 * The Barrier class provides a way to synchronize threads such that no thread can proceed
 * past a certain point in their execution until all threads have reached that point. Once
 * all threads have reached the barrier, they are allowed to proceed.
 *
 * This implementation uses an `std::mutex` and `std::condition_variable` for thread coordination.
 */
class Barrier {
public:
    /**
     * @brief Constructs a Barrier object.
     * @param numThreads The number of threads that need to reach
     *                   the barrier before any can proceed.
     */
    explicit Barrier(int numThreads);

    /**
     * @brief Blocks the calling thread until all threads have reached the barrier.
     *
     * This function will block the calling thread until all threads that are part of the barrier
     * have called this function. Once all threads have called this function, they will be allowed
     * to proceed.
     */
    void barrier();

    ~Barrier() = default;

private:
    std::mutex mutex;  // Mutex for synchronizing access to the barrier
    std::condition_variable cv;  // Condition variable for blocking threads
    int count;  // Number of threads that have reached the barrier
    int generation;  // Generation number to track the state of the barrier
    const int numThreads;  // Total number of threads that need to reach the barrier
};

#endif // BARRIER_H
