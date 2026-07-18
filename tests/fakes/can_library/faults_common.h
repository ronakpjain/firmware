#ifndef TEST_FAKE_FAULTS_COMMON_H
#define TEST_FAKE_FAULTS_COMMON_H

#include <stdbool.h>

#include "can_library/generated/can_types.h"

void update_fault(fault_id_t fault_id, float value);
bool is_latched(fault_id_t fault_id);
bool is_clear(fault_id_t fault_id);

#endif // TEST_FAKE_FAULTS_COMMON_H
