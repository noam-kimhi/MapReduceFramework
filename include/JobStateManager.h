
#ifndef JOBSTATEMANAGER_H
#define JOBSTATEMANAGER_H

#include <atomic>
#include "MapReduceFramework.h"

/**
 * JobStateManager is a thread-safe class that manages the state of a MapReduce job.
 * It keeps track of the current stage (UNDEFINED, MAP, SHUFFLE, REDUCE), the number of processed
 * elements, and the total number of elements.
 */
class JobStateManager {
public:

    /**
     * Constructor for JobStateManager.
     * Creates an instance with the initial state set to UNDEFINED_STAGE, processed=0,
     * and total=totalKeys.
     * @param totalKeys The total number of keys in the input vector.
     */
    explicit JobStateManager(uint32_t totalKeys);

    /**
     * Updates the state of the job.
     * @param stage The current stage of the job (UNDEFINED_STAGE, MAP_STAGE, SHUFFLE_STAGE,
     *              REDUCE_STAGE).
     * @param processed The number of processed elements.
     * @param total The total number of elements.
     */
    void updateState(stage_t stage, uint32_t processed, uint32_t total);

    /**
     * Thread-safe increment of the processed count.
     */
    void incrementProcessed();

    /**
     * Sets the total number of elements.
     * Also resets the processed count to 0.
     * @param newTotal The new total number of elements.
     */
    void setTotal(uint32_t newTotal);

    /**
     * Sets the stage of the job.
     * Also resets the processed count to 0.
     * @param newStage The new stage of the job (UNDEFINED_STAGE, MAP_STAGE, SHUFFLE_STAGE,
     *                 REDUCE_STAGE).
     */
    void setStage(stage_t newStage);

    /**
     * Retrieves the current state of the job.
     * @param outStage Where to store the current stage of the job.
     * @param outProcessed Where to store the number of processed elements.
     * @param outTotal Where to store the total number of elements.
     */
    void getState(stage_t &outStage, uint32_t &outProcessed, uint32_t &outTotal) const;

private:
    std::atomic<uint64_t> state;  // [63..62]=stage | [61..31]=processed | [30..0]=total

    /**
     * Encodes the state of the job into a packed 64-bit integer.
     * @param stage The current stage of the job (UNDEFINED_STAGE, MAP_STAGE, SHUFFLE_STAGE,
     *              REDUCE_STAGE).
     * @param processed The number of processed elements.
     * @param total The total number of elements.
     * @return A packed 64-bit integer representing the state of the job.
     */
    static uint64_t encodeState(stage_t stage, uint32_t processed, uint32_t total);

    /**
     * Decodes the packed 64-bit integer to retrieve the stage of the job.
     * @param v The packed 64-bit integer representing the state of the job.
     * @return The stage of the job (UNDEFINED_STAGE, MAP_STAGE, SHUFFLE_STAGE, REDUCE_STAGE).
     */
    static stage_t decodeStage(uint64_t v);

    /**
     * Decodes the packed 64-bit integer to retrieve the number of processed elements.
     * @param v The packed 64-bit integer representing the state of the job.
     * @return The number of processed elements.
     */
    static uint32_t decodeProcessed(uint64_t v);

    /**
     * Decodes the packed 64-bit integer to retrieve the total number of elements.
     * @param v The packed 64-bit integer representing the state of the job.
     * @return The total number of elements.
     */
    static uint32_t decodeTotal(uint64_t v);
};


#endif //JOBSTATEMANAGER_H
