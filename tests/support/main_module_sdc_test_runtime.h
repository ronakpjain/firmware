#ifndef MAIN_MODULE_SDC_TEST_RUNTIME_H
#define MAIN_MODULE_SDC_TEST_RUNTIME_H

#include <stdint.h>

#include "board_phal_fake.h"
#include "dashboard_pedals_test_runtime.h"
#include "stm32g474xx.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_main_module_reset(void);
void test_main_module_run_sdc(void);
uint32_t test_os_delay_call_count(void);
uint32_t test_os_delay_total_ticks(void);

#ifdef __cplusplus
}
#endif

#endif
