#ifndef PHAL_G4_FDCAN_INTERNAL_H
#define PHAL_G4_FDCAN_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/phal_G4/fdcan/fdcan.h"
#include "common/phal_G4/fdcan/fdcan_priv.h"

#define PHAL_CAN_TIMEOUT 100000U

typedef struct {
    PHAL_CAN_Handle_t *handle;
    size_t standard_filter_count;
    size_t extended_filter_count;
} PHAL_CAN_State_t;

PHAL_CAN_State_t *PHAL_CAN_internalState(PHAL_CAN_Handle_t *handle);
PHAL_CAN_Handle_t *PHAL_CAN_internalHandle(FDCAN_GlobalTypeDef *instance);
FDCAN_GlobalTypeDef *PHAL_CAN_internalExpectedInstance(const PHAL_CAN_Handle_t *handle);

bool PHAL_CAN_internalEnterInit(FDCAN_GlobalTypeDef *instance);
bool PHAL_CAN_internalExitInit(FDCAN_GlobalTypeDef *instance);
bool PHAL_CAN_internalMakeNBTP(uint32_t kernel_hz, uint32_t bit_rate, uint32_t *nbtp);
uintptr_t PHAL_CAN_internalRamBase(FDCAN_GlobalTypeDef *instance);

bool PHAL_CAN_internalConfigure(
    PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_Config_t *config
);
bool PHAL_CAN_internalSetFilters(
    PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_FilterConfig_t *filters
);
bool PHAL_CAN_internalSend(
    PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_Message_t *message
);
bool PHAL_CAN_internalTxAvailable(const PHAL_CAN_Handle_t *handle);

#endif
