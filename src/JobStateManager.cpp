
#include "../include/JobStateManager.h"

#define STAGE_MASK 0x3
#define STAGE_SHIFT 62
#define PROCESSED_SHIFT 31
#define MAX_31_BITS 0x7FFF'FFFF

JobStateManager::JobStateManager(const uint32_t totalKeys) : state(0) {
    state.store(encodeState(UNDEFINED_STAGE, 0, totalKeys), std::memory_order_release);
}

void JobStateManager::updateState(const stage_t stage, const uint32_t processed,
                                  const uint32_t total) {
    state.store(encodeState(stage, processed, total), std::memory_order_release);
}

void JobStateManager::incrementProcessed() {
    uint64_t oldVal = state.load(std::memory_order_acquire);
    while (true) {
        const stage_t stage = decodeStage(oldVal);
        const uint32_t processed = decodeProcessed(oldVal);
        const uint32_t total = decodeTotal(oldVal);

        if (const uint64_t newVal = encodeState(stage, processed + 1, total);
            state.compare_exchange_weak(
                oldVal, newVal,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            return;
                }
        // on failure, oldVal is updated to current state → retry
    }
}

void JobStateManager::setTotal(const uint32_t newTotal) {
    uint64_t oldVal = state.load(std::memory_order_acquire);
    while (true) {
        const stage_t stage = decodeStage(oldVal);

        // newVal is the same as oldVal, but with the new total value and processed set to 0
        if (const uint64_t newVal = encodeState(stage, 0, newTotal); state.compare_exchange_weak(
                oldVal, newVal,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            return;
                }
    }
}

void JobStateManager::setStage(const stage_t newStage) {
    uint64_t oldVal = state.load(std::memory_order_acquire);
    while (true) {
        const uint32_t total = decodeTotal(oldVal);

        if (const uint64_t newVal = encodeState(newStage, 0, total);
            state.compare_exchange_weak(
                oldVal, newVal,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            return;
                }
    }
}

void JobStateManager::getState(stage_t &outStage, uint32_t &outProcessed,
                               uint32_t &outTotal) const {
    const uint64_t packed = state.load(std::memory_order_acquire);
    outStage = decodeStage(packed);
    outProcessed = decodeProcessed(packed);
    outTotal = decodeTotal(packed);
}

uint64_t JobStateManager::encodeState(const stage_t stage, const uint32_t processed,
                                      const uint32_t total) {
    return (static_cast<uint64_t>(stage & STAGE_MASK) << STAGE_SHIFT)
         | (static_cast<uint64_t>(processed & MAX_31_BITS) << PROCESSED_SHIFT)
         |  static_cast<uint64_t>(total & MAX_31_BITS);
}

stage_t JobStateManager::decodeStage(const uint64_t v) {
    return static_cast<stage_t>((v >> STAGE_SHIFT) & STAGE_MASK);
}

uint32_t JobStateManager::decodeProcessed(const uint64_t v) {
    return static_cast<uint32_t>((v >> PROCESSED_SHIFT) & MAX_31_BITS);
}

uint32_t JobStateManager::decodeTotal(const uint64_t v) {
    return static_cast<uint32_t>(v & MAX_31_BITS);
}
