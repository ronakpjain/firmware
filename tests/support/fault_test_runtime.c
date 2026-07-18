#include "fault_test_runtime.h"

#include <string.h>

static bool fault_latched[TEST_FAULT_ID_COUNT] = {0};
static float fault_last_values[TEST_FAULT_ID_COUNT] = {0};
static uint32_t fault_update_counts[TEST_FAULT_ID_COUNT] = {0};

static bool is_valid_fault_id(fault_id_t fault_id) {
    return (uint32_t)fault_id < TEST_FAULT_ID_COUNT;
}

void test_fault_reset(void) {
    memset(fault_latched, 0, sizeof(fault_latched));
    memset(fault_last_values, 0, sizeof(fault_last_values));
    memset(fault_update_counts, 0, sizeof(fault_update_counts));
}

void update_fault(fault_id_t fault_id, float value) {
    if (!is_valid_fault_id(fault_id)) {
        return;
    }
    fault_last_values[fault_id] = value;
    fault_update_counts[fault_id]++;
}

bool is_latched(fault_id_t fault_id) {
    return is_valid_fault_id(fault_id) && fault_latched[fault_id];
}

bool is_clear(fault_id_t fault_id) { return !is_latched(fault_id); }

void test_fault_set_latched(fault_id_t fault_id, bool is_fault_latched) {
    if (is_valid_fault_id(fault_id)) {
        fault_latched[fault_id] = is_fault_latched;
    }
}

float test_fault_last_value(fault_id_t fault_id) {
    return is_valid_fault_id(fault_id) ? fault_last_values[fault_id] : 0.0F;
}

uint32_t test_fault_update_count(fault_id_t fault_id) {
    return is_valid_fault_id(fault_id) ? fault_update_counts[fault_id] : 0;
}
