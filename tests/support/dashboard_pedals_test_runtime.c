#include "dashboard_pedals_test_runtime.h"

#include <string.h>

#include "source/dashboard/main.h"
#include "source/dashboard/pedals/pedals.h"

volatile raw_adc_values_t raw_adc_values = {0};

static test_pedal_tx_t last_pedal_tx = {0};

void test_dashboard_reset(void) {
    memset((void *)&raw_adc_values, 0, sizeof(raw_adc_values));
    test_fault_reset();
    memset(&last_pedal_tx, 0, sizeof(last_pedal_tx));
    pedal_values = (pedals_data_t){0};
}

void CAN_SEND_pedals(uint8_t throttle, uint8_t regen, uint8_t brake) {
    last_pedal_tx = (test_pedal_tx_t){throttle, regen, brake, last_pedal_tx.send_count + 1};
}

void test_dashboard_set_raw_adc(
    uint16_t throttle1,
    uint16_t throttle2,
    uint16_t regen1,
    uint16_t regen2,
    uint16_t brake1_pressure,
    uint16_t brake2_pressure
) {
    raw_adc_values = (raw_adc_values_t){
        .throttle1 = throttle1,
        .throttle2 = throttle2,
        .regen1 = regen1,
        .regen2 = regen2,
        .brake1_pressure = brake1_pressure,
        .brake2_pressure = brake2_pressure,
    };
}

void test_dashboard_run_pedals(void) { pedals_periodic(); }
pedals_data_t test_dashboard_get_pedal_values(void) { return pedal_values; }
test_pedal_tx_t test_dashboard_get_last_pedal_tx(void) { return last_pedal_tx; }
