#ifndef DASHBOARD_PEDALS_TEST_RUNTIME_H
#define DASHBOARD_PEDALS_TEST_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#include "can_library/generated/DASHBOARD.h"
#include "can_library/generated/can_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t throttle;
    uint8_t regen;
    uint8_t brake;
    uint32_t send_count;
} test_pedal_tx_t;

void test_dashboard_reset(void);
void test_fault_set_latched(fault_id_t fault_id, bool is_latched);
float test_fault_last_value(fault_id_t fault_id);
uint32_t test_fault_update_count(fault_id_t fault_id);
void test_dashboard_set_raw_adc(
    uint16_t throttle1,
    uint16_t throttle2,
    uint16_t regen1,
    uint16_t regen2,
    uint16_t brake1_pressure,
    uint16_t brake2_pressure
);
void test_dashboard_run_pedals(void);
pedals_data_t test_dashboard_get_pedal_values(void);
test_pedal_tx_t test_dashboard_get_last_pedal_tx(void);

#ifdef __cplusplus
}
#endif

#endif
