#ifndef FAULT_TEST_RUNTIME_H
#define FAULT_TEST_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#include "can_library/generated/can_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_fault_reset(void);
void test_fault_set_latched(fault_id_t fault_id, bool is_latched);
float test_fault_last_value(fault_id_t fault_id);
uint32_t test_fault_update_count(fault_id_t fault_id);

#ifdef __cplusplus
}
#endif

#endif
