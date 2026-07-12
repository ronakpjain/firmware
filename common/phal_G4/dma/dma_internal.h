#ifndef PHAL_G4_DMA_INTERNAL_H
#define PHAL_G4_DMA_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/phal_G4/dma/dma.h"
#include "common/phal_G4/phal_G4.h"

#define PHAL_DMA_CHANNEL_COUNT 8U
#define PHAL_DMA_TIMEOUT       100000U

typedef enum {
    PHAL_DMA_DIRECTION_PERIPHERAL_TO_MEMORY,
    PHAL_DMA_DIRECTION_MEMORY_TO_PERIPHERAL
} PHAL_DMA_Direction_t;

typedef enum {
    PHAL_DMA_WIDTH_8_BIT,
    PHAL_DMA_WIDTH_16_BIT,
    PHAL_DMA_WIDTH_32_BIT
} PHAL_DMA_Width_t;

typedef void (*PHAL_DMA_CompletionCallback_t)(void *context, bool success);

typedef struct PHAL_DMA_Route {
    DMA_TypeDef *controller;
    uint8_t channel;
    uint8_t request;
    PHAL_DMA_Direction_t direction;
    PHAL_DMA_Width_t peripheral_width;
    PHAL_DMA_Width_t memory_width;
    uint8_t priority;
} PHAL_DMA_Route_t;

bool PHAL_DMA_internalInit(
    PHAL_DMA_Handle_t *handle,
    const PHAL_DMA_Route_t *route
);

bool PHAL_DMA_internalStart(
    PHAL_DMA_Handle_t *handle,
    volatile void *peripheral_address,
    void *memory_address,
    size_t count,
    bool memory_increment,
    bool circular,
    PHAL_DMA_CompletionCallback_t callback,
    void *context
);

bool PHAL_DMA_internalDisable(PHAL_DMA_Handle_t *handle);
void PHAL_DMA_internalClearTransfer(PHAL_DMA_Handle_t *handle);
bool PHAL_DMA_internalRelease(PHAL_DMA_Handle_t *handle);
void PHAL_DMA_internalHandleIRQ(DMA_TypeDef *controller, uint8_t channel);

size_t PHAL_DMA_internalRemaining(const PHAL_DMA_Handle_t *handle);

const PHAL_DMA_Route_t *PHAL_DMA_internalAdcRoute(ADC_TypeDef *instance);
const PHAL_DMA_Route_t *PHAL_DMA_internalSpiRxRoute(SPI_TypeDef *instance);
const PHAL_DMA_Route_t *PHAL_DMA_internalSpiTxRoute(SPI_TypeDef *instance);
const PHAL_DMA_Route_t *PHAL_DMA_internalUsartRxRoute(USART_TypeDef *instance);
const PHAL_DMA_Route_t *PHAL_DMA_internalUsartTxRoute(USART_TypeDef *instance);

#endif
