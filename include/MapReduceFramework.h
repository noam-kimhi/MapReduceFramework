#ifndef MAPREDUCEFRAMEWORK_H
#define MAPREDUCEFRAMEWORK_H

#include "MapReduceClient.h"

#define MAX_PERCENTAGE 100.0f

/**
 * An identifier of a running job.
 * Returned when starting a job and used by other framework functions to identify the job.
 */
typedef void* JobHandle;

enum stage_t {UNDEFINED_STAGE=0, MAP_STAGE=1, SHUFFLE_STAGE=2, REDUCE_STAGE=3};

/**
 * A struct which quantizes the state of a job.
 *
 * stage_t stage: an enum which indicates the stage of the job
 *				  (UNDEFINED_STAGE, MAP_STAGE, SHUFFLE_STAGE, REDUCE_STAGE) <br>
 *                We will save the job stage using the enum.
 *                The job starts at the undefined stage until the first thread starts the map phase.
 *
 * float percentage: A float which indicates the percentage of the job that has been completed.
 */
typedef struct {
	stage_t stage;
	float percentage;
} JobState;

/**
 * This function saves the intermediary elements (K2*, V2*) in the context's data structures.
 * @param key The key of an intermediary input element.
 * @param value The value of an intermediary input element.
 * @param context Contains the thread's context.
 * @note This function is called by the client's map function,
 *		 and the context is passed from the framework to the client's map function as a parameter.
 */
void emit2 (K2* key, V2* value, void* context);

/**
 * This function saves the output element in the context data structures (output vector).
 * @param key The key of an output element.
 * @param value The value of an output element.
 * @param context Contains the thread's context.
 * @note This function is called from the client's reduce function,
 *		 and the context is passed from the framework to the client's map function as a parameter.
 */
void emit3 (K3* key, V3* value, void* context);

/**
 * This function starts running the MapReduce algorithm and returns a handle to the job.
 * @param client The implementation of MapReduceClient, or in other words,
 *				 the task that the framework should run.
 * @param inputVec A vector of pairs (K1*, V1*) that is the input. We assume that it is valid.
 * @param outputVec A vector to which output elements will be added before returning.
 *					We assume that it is empty.
 * @param multiThreadLevel The number of worker threads to be used for running the algorithm.
 *						   We assume that it is greater-than or equal-to 1.
 * @return The JobHandle that will be used for monitoring the job.
 */
JobHandle startMapReduceJob(const MapReduceClient& client,
	const InputVec& inputVec, OutputVec& outputVec,
	int multiThreadLevel);

/**
 * This function gets a JobHandle returned by startMapReduceFramework and waits until it is finished.
 * @param job The JobHandle returned by startMapReduceFramework.
 */
void waitForJob(JobHandle job);

/**
 * This function gets a JobHandle and updates the state of the job into the given JobState struct.
 * @param job The JobHandle returned by startMapReduceFramework.
 * @param state A pointer to a JobState struct that will be updated with the current state of the job.
 */
void getJobState(JobHandle job, JobState* state);

/**
 * This function releases all resources of a job.
 *
 * After this function is called, the job handle will be invalid.
 *
 * In case that the function is called and the job is not finished yet,
 * it waits until it is finished to close it.
 * @param job The JobHandle to be closed.
 */
void closeJobHandle(JobHandle job);
	
	
#endif //MAPREDUCEFRAMEWORK_H
