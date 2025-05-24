#include "../include/Barrier.h"

Barrier::Barrier(const int numThreads): count(0), generation(0), numThreads(numThreads) {}

void Barrier::barrier() {
    std::unique_lock<std::mutex> lock(mutex); // Lock the mutex to ensure thread safety
    int gen = generation;

    if (++count < numThreads) {
        // If not all threads have reached the barrier, wait for others to arrive.
        // The predicate ensures that we only wake up when the generation has changed,
        // indicating that all threads have reached the barrier.
        cv.wait(lock, [this, gen] { return gen != generation; });
    } else {
        // If all threads have reached the barrier, reset the count and generation.
        count = 0;
        generation++;
        cv.notify_all(); // Notify all waiting threads that the barrier has been crossed.
    }
}
