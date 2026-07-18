#include "main_module_sdc_test_runtime.h"

#include "source/main_module/sdc/sdc.h"

static uint32_t delay_call_count;
static uint32_t delay_total_ticks;

void test_main_module_reset(void) {
    test_board_phal_reset();
    test_fault_reset();
    delay_call_count = 0;
    delay_total_ticks = 0;
}

void test_main_module_run_sdc(void) { SDC_task_periodic(); }

uint32_t osDelay(uint32_t ticks) {
    delay_call_count++;
    delay_total_ticks += ticks;
    return 0;
}

uint32_t test_os_delay_call_count(void) { return delay_call_count; }
uint32_t test_os_delay_total_ticks(void) { return delay_total_ticks; }
