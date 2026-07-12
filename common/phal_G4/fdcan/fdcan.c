#include "common/phal_G4/fdcan/fdcan_internal.h"

bool PHAL_CAN_init(PHAL_CAN_Handle_t *handle, const PHAL_CAN_Config_t *config) {
    uint32_t nbtp;
    if (!PHAL_CAN_internalValidateInit(handle, config)
        || !PHAL_CAN_internalBuildTiming(config->bit_rate, &nbtp)) {
        return false;
    }

    PHAL_CAN_internalEnableClock();
    return PHAL_CAN_internalConfigureController(handle, config, nbtp);
}

bool PHAL_CAN_setFilters(
    PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_FilterConfig_t *filters
) {
    if (!PHAL_CAN_internalValidateFilters(handle, filters)
        || !PHAL_CAN_internalEnterInit(handle->instance)) {
        return false;
    }

    PHAL_CAN_internalProgramFilters(handle, filters);
    if (!PHAL_CAN_internalExitInit(handle->instance)) {
        return false;
    }

    PHAL_CAN_internalStoreFilterCounts(handle, filters);
    return true;
}

bool PHAL_CAN_send(PHAL_CAN_Handle_t *handle, const PHAL_CAN_Message_t *message) {
    if (!PHAL_CAN_internalValidateMessage(handle, message)) {
        return false;
    }
    return PHAL_CAN_internalWriteTx(handle, message);
}

bool PHAL_CAN_txAvailable(const PHAL_CAN_Handle_t *handle) {
    return PHAL_CAN_internalValidateHandle(handle)
        && PHAL_CAN_internalTxAvailable(handle);
}
