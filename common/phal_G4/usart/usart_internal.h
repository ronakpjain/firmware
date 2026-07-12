#ifndef PHAL_G4_USART_INTERNAL_H
#define PHAL_G4_USART_INTERNAL_H

#include "common/phal_G4/usart/usart.h"
#include "common/phal_G4/dma/dma_internal.h"

#define PHAL_USART_INTERNAL_TIMEOUT 100000U
#define PHAL_USART_SLOT_COUNT       3U

typedef struct {
    PHAL_USART_Handle_t *handle;
    uint8_t *rx_buffer;
    size_t rx_capacity;
    size_t rx_length;
    bool rx_idle;
    bool tx_dma_complete;
    bool registered;
} PHAL_USART_State_t;

PHAL_USART_State_t *PHAL_USART_internalStateForInstance(USART_TypeDef *instance);

bool PHAL_USART_internalValidateConfig(const PHAL_USART_Config_t *config);
bool PHAL_USART_internalPrepareInstance(
    const PHAL_USART_Config_t *config,
    uint32_t *baud_register
);
bool PHAL_USART_internalInitializeStateAndDma(
    PHAL_USART_Handle_t *handle,
    const PHAL_USART_Config_t *config
);
void PHAL_USART_internalConfigureRegisters(
    PHAL_USART_Handle_t *handle,
    const PHAL_USART_Config_t *config,
    uint32_t baud_register
);

bool PHAL_USART_internalValidateTransmit(
    const PHAL_USART_Handle_t *handle,
    const uint8_t *data,
    size_t length
);
bool PHAL_USART_internalArmTransmit(
    PHAL_USART_Handle_t *handle,
    const uint8_t *data,
    size_t length
);

bool PHAL_USART_internalValidateReceive(
    const PHAL_USART_Handle_t *handle,
    const uint8_t *data,
    size_t length
);
bool PHAL_USART_internalArmReceive(
    PHAL_USART_Handle_t *handle,
    uint8_t *data,
    size_t length,
    bool idle
);

bool PHAL_USART_internalValidateHandle(const PHAL_USART_Handle_t *handle);
void PHAL_USART_internalDisableReceiveRequests(PHAL_USART_Handle_t *handle);
void PHAL_USART_internalCompleteStopReceive(PHAL_USART_Handle_t *handle);
bool PHAL_USART_internalCompleteTransmit(PHAL_USART_Handle_t *handle, bool success);
void PHAL_USART_internalHandleIRQ(USART_TypeDef *instance);

#endif
