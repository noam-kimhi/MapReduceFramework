
#include "../include/MapReduceFramework.h"
#include "../include/JobStateManager.h"
#include "../include/Barrier.h"

#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>
#include <algorithm>

#define SYS_ERR "system error: %s\n"
#define THREAD_ZERO 0

static std::mutex jobCreationMutex; // Global mutex to synchronize access to startMapReduceJob

/**
 * A struct which includes all the parameters which are relevant to the job.
 */
struct JobContext {
	// Job state
	JobStateManager stateManager;

	// Worker threads
	std::vector<std::thread> threads;

	// Barrier to sync all threads before shuffle
	std::unique_ptr<Barrier> barrier;

	// Input and output vectors
	const InputVec& inputVec;
	OutputVec& outputVec;

	std::mutex outMutex; // Mutex for output vector

	std::vector<bool> joined; // Vector to track if threads have joined
	std::mutex joinMutex; // Mutex for joining threads

	// Thread-safe intermediate data per thread
	std::vector<IntermediateVec> intermediateVecs;

	// Shuffled intermediate data: key → list of values
	std::vector<IntermediateVec> shuffledData;

	// A counter for the shuffled data. After the shuffle phase, this counter will represent
	// the number of intermediate vectors in the shuffled data.
	std::atomic<uint64_t> shuffleCounter;

	std::atomic<uint32_t> nextInputIndex;  // For dynamic map scheduling
	std::atomic<uint32_t> nextReduceIndex; // For dynamic reduce scheduling

	JobContext(const InputVec& input, OutputVec& output, const int nThreads)
		: stateManager(input.size()), barrier(std::make_unique<Barrier>(nThreads)), inputVec(input),
		  outputVec(output), joined(nThreads, false), intermediateVecs(nThreads), shuffleCounter(0),
		  nextInputIndex(0), nextReduceIndex(0) {}
};

/**
 * A struct which contains a thread's context, including its id, the job context,
 * and the thread's intermediate vector (for the map and sort phases).
 */
struct ThreadContext {
	int threadId;
	JobContext* context;
	std::vector<IntermediatePair>* intermediateVec; // Thread-local intermediate data
};

/**
 * This function is the map phase of the MapReduce algorithm.
 * @param client The implementation of MapReduceClient, where the map function is defined.
 * @param tc The thread context, which contains the thread ID and the job context.
 */
void mapPhase(const MapReduceClient& client, ThreadContext *tc) {
	if (tc->threadId == THREAD_ZERO) {
		// Set the job state to MAP_STAGE
		// This is done only by thread 0, to avoid multiple calls to setStage (overhead)
		tc->context->stateManager.setStage(MAP_STAGE);
	}
	while (true) {
		// Atomically fetch and increment the next input index
		const uint32_t oldValue = tc->context->nextInputIndex.fetch_add(1, std::memory_order_relaxed);

		// Check if the index is within bounds
		if (oldValue >= tc->context->inputVec.size()) {
			break; // All input pairs have been processed
		}

		// Safely process the input pair at the fetched index
		// No need to synchronize access to inputVec,
		// since only the current thread has the value of oldValue
		const auto&[fst, snd] = tc->context->inputVec[oldValue];
		client.map(fst, snd, tc);
		// Since we have mapped (processed) a pair,
		// increment the processed count in the job context. This is done atomically.
		tc->context->stateManager.incrementProcessed();
	}
}

void emit2 (K2* key, V2* value, void* context) {
	const auto *tc = static_cast<ThreadContext*>(context);
	// There is no need to synchronize access to intermediateVec,
	// as each thread has its own intermediate vector
	// Add the key-value pair to the thread's intermediate vector
	tc->intermediateVec->emplace_back(key, value);
}

/**
 * This function is the sort phase of the MapReduce algorithm.
 * It sorts the thread's intermediate vector based on the keys.
 * @param tc The thread context, containing the thread's intermediate vector.
 */
void sortPhase(const ThreadContext *tc) {
	// There is no need to synchronize access to intermediateVec,
	// as each thread has its own intermediate vector
	// Sort the intermediate vector based on the keys
	std::sort(tc->intermediateVec->begin(), tc->intermediateVec->end(),
			[](const IntermediatePair& a, const IntermediatePair& b) {
				return *a.first < *b.first;
	});
}

/**
 * This function creates a new sequence of (K2, V2) pairs,
 * where in each sequence all keys are identical, and all
 * elements with a given key are in a single sequence.
 * @param tc The context of thread 0, which is responsible for the shuffle phase.
 */
void shufflePhase(const ThreadContext *tc) {
	JobContext* context = tc->context;

	// Count the total number of intermediate pairs
	size_t totalPairs = 0;
	for (const auto& vec : context->intermediateVecs) {
		totalPairs += vec.size();
	}

	// Reset the processed count and change the total since we are starting the shuffle phase
	context->stateManager.setTotal(totalPairs);

	while (true) {
		const K2* maxKey = nullptr;

		// Find max key at the back of any non-empty vector
		for (const auto& vec : context->intermediateVecs) {
			if (!vec.empty()) {
				if (const K2* candidate = vec.back().first;
					maxKey == nullptr || *maxKey < *candidate) {
					maxKey = candidate;
				}
			}
		}

		if (maxKey == nullptr) {
			break; // All vectors empty
		}

		// Collect all pairs with maxKey
		context->shuffledData.emplace_back(); // Create a new IntermediateVec
		auto& currentGroup = context->shuffledData.back(); // Reference to the new vector

		for (auto& vec : context->intermediateVecs) {
			while (!vec.empty() && !(*vec.back().first < *maxKey) &&
				   !(*maxKey < *vec.back().first)) {
				// If we get here, it means that the key is equal to maxKey
				// Move the pair from the intermediate vector to the shuffled data
				currentGroup.emplace_back(vec.back().first, vec.back().second);
				vec.pop_back(); // Remove the pair from the intermediate vector
				context->stateManager.incrementProcessed(); // Increment the processed count
			}
		}

		// Since we have added a new vector to shuffledData,
		// we need to increment the shuffle counter
		context->shuffleCounter.fetch_add(1, std::memory_order_relaxed);
	}
}

/**
 * This function is the reduce phase of the MapReduce algorithm.
 * It processes the shuffled data and applies the reduce function defined in the client.
 * @param client The implementation of MapReduceClient, where the reduce function is defined.
 * @param tc The thread context, which contains the thread ID and the job context.
 */
void reducePhase(const MapReduceClient& client, ThreadContext *tc) {
	while (true) {
		// Atomically fetch and increment the next reduce index
		const uint32_t oldValue = tc->context->nextReduceIndex.fetch_add(1, std::memory_order_relaxed);

		// Check if the index is within bounds (number of vectors in the shuffled data)
		if (oldValue >= tc->context->shuffleCounter.load(std::memory_order_relaxed)) {
			break; // All input pairs have been processed
		}

		// Safely process the shuffled data at the fetched index
		// No need to synchronize access to shuffledData,
		// since only the current thread has the value of oldValue
		client.reduce(&tc->context->shuffledData[oldValue], tc);
		// Since we have reduced (processed) a vector,
		// Increment the processed count in the job context. This is done atomically.
		tc->context->stateManager.incrementProcessed();
	}
}

void emit3 (K3* key, V3* value, void* context) {
	const auto *tc = static_cast<ThreadContext*>(context);
	// Lock the output vector for thread safety
	std::lock_guard<std::mutex> lock(tc->context->outMutex);
	// Add the key-value pair to the output vector
	tc->context->outputVec.emplace_back(key, value);
	// There is no need to unlock the mutex explicitly,
	// as it will be released when going out of scope
}

/**
 * This function is the main thread function for each worker thread.
 *
 * Each thread does the following:
 * 1. Running the map phase
 * 2. Sorting the intermediate data
 * 3. Waiting for all threads to finish the sort phase
 * 4. Thread 0 then shuffles the whole intermediate data
 * 5. All threads wait for thread 0 to finish the shuffle phase
 * 6. Finally, all threads run the reduce phase
 * @param context The job context, which contains the job state and other relevant data.
 * @param client The implementation of MapReduceClient, where the map and reduce functions are defined.
 * @param threadId The ID of the thread executing this function.
 * @param intermediateVec The thread's intermediate vector, which stores the intermediate key-value pairs.
 */
void threadFunc(JobContext* context, const MapReduceClient& client,
                const int threadId, IntermediateVec* intermediateVec) {
	// Create a thread context for each thread
	ThreadContext tc{threadId, context, intermediateVec};

	mapPhase(client, &tc); // First, the thread runs the map phase

	sortPhase(&tc); // Then, the thread sorts its intermediate data

	// The thread waits for all other threads to finish map phase
	context->barrier->barrier();

	if (threadId == THREAD_ZERO) { // Make sure only thread 0 is calling shuffle
		context->stateManager.setStage(SHUFFLE_STAGE);
		shufflePhase(&tc);
		// Reset the state since we are starting the reduce phase
		context->stateManager.updateState(
			REDUCE_STAGE, 0, context->shuffleCounter.load(std::memory_order_relaxed)
		);
	}

	// All the threads must wait for thread 0 to finish
	// the shuffle phase before continuing to the reduce phase
	context->barrier->barrier();

	reducePhase(client, &tc); // After the shuffle phase, it runs the reduce phase
}

JobHandle startMapReduceJob(const MapReduceClient& client, const InputVec& inputVec,
							OutputVec& outputVec, const int multiThreadLevel) {
	// Lock the mutex to ensure thread-safe execution
	std::lock_guard<std::mutex> lock(jobCreationMutex);

	if (inputVec.empty()) {
		return nullptr; // No input pairs to process
	}

	// Create the job's context
	JobContext *context;
	try {
		context = new JobContext(inputVec, outputVec, multiThreadLevel);
	} catch (const std::bad_alloc& e) {
		printf(SYS_ERR, e.what());
		exit(EXIT_FAILURE);
	}

	context->threads.reserve(multiThreadLevel); // Reserve space for all the threads
	for (int i = 0; i < multiThreadLevel; ++i) {
		try {
			// Create a thread that runs the map-reduce job
			context->threads.emplace_back(
				[=, &client, threadId = i, intermediateVec = &context->intermediateVecs[i]]() {
					threadFunc(context, client, threadId, intermediateVec);
				}
			);
		} catch (const std::system_error& e) {
			printf(SYS_ERR, e.what());
			exit(EXIT_FAILURE);
		}
	}
	return context;
	// The mutex will now be unlocked automatically when going out of scope, ensuring thread safety
}

void getJobState(JobHandle job, JobState *state) {
	if (job == nullptr) {
		state->stage = REDUCE_STAGE; // Last stage
		state->percentage = MAX_PERCENTAGE; // Job is done
		return;
	}
	const auto *context = static_cast<JobContext*>(job);
	uint32_t numProcessed;
	uint32_t numTotal;
	// Get the current state of the job. This is done atomically.
	context->stateManager.getState(state->stage, numProcessed, numTotal);
	// Calculate the percentage of completion, ensuring it does not exceed 100%
	// Since numTotal is the number of total pairs,
	// if it is zero this means that "we have successfully processed all the pairs"
	// is a vacuous truth, so we set the percentage to 100%.
	// Otherwise, we calculate the percentage based on the number of processed pairs
	// and the total number of pairs. We use std::min to ensure it does not exceed 100%.
	state->percentage = numTotal == 0 ? MAX_PERCENTAGE : std::min(
		static_cast<float>(numProcessed) / static_cast<float>(numTotal) * MAX_PERCENTAGE, MAX_PERCENTAGE
	);
}

void waitForJob(JobHandle job) {
	if (job == nullptr) {
		return; // Nothing to do
	}
	auto *context = static_cast<JobContext*>(job);
	for (size_t i = 0; i < context->threads.size(); ++i) {
		std::lock_guard<std::mutex> lock(context->joinMutex);
		// Check if the thread is joinable
		if (!context->joined[i] && context->threads[i].joinable()) {
			context->threads[i].join(); // If it is, join it
			context->joined[i] = true; // After joining, mark it as joined
		}
		// Mutex unlocks automatically when lock goes out of scope
	}
}

void closeJobHandle(JobHandle job) {
	if (job == nullptr) {
		return; // Nothing to do
	}
	waitForJob(job); // First, wait for the job to finish
	const auto *context = static_cast<JobContext*>(job);
	delete context; // After we know that the job is finished, we can safely delete its context
}
