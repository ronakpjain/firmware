#include "common/phal_G4/dma/dma_internal.h"

#include <string.h>

static PHAL_DMA_Handle_t *dma1_owners[PHAL_DMA_CHANNEL_COUNT] = {0};
static PHAL_DMA_Handle_t *dma2_owners[PHAL_DMA_CHANNEL_COUNT] = {0};

static PHAL_DMA_State_t *state_for_handle(const PHAL_DMA_Handle_t *handle) {
    if (handle == NULL) {
        return NULL;
    }

    PHAL_DMA_State_t *state = (PHAL_DMA_State_t *)(uintptr_t)handle->_storage;
    return state->magic == PHAL_DMA_HANDLE_MAGIC ? state : NULL;
}

static PHAL_DMA_Handle_t **owners_for_controller(DMA_TypeDef *controller) {
    if (controller == DMA1) {
        return dma1_owners;
    }
    if (controller == DMA2) {
        return dma2_owners;
    }
    return NULL;
}

static DMA_Channel_TypeDef *channel_registers(DMA_TypeDef *controller, uint8_t channel) {
    if (controller == DMA1) {
        switch (channel) {
            case 1U: return DMA1_Channel1;
            case 2U: return DMA1_Channel2;
            case 3U: return DMA1_Channel3;
            case 4U: return DMA1_Channel4;
            case 5U: return DMA1_Channel5;
            case 6U: return DMA1_Channel6;
            case 7U: return DMA1_Channel7;
            case 8U: return DMA1_Channel8;
            default: return NULL;
        }
    }

    if (controller == DMA2) {
        switch (channel) {
            case 1U: return DMA2_Channel1;
            case 2U: return DMA2_Channel2;
            case 3U: return DMA2_Channel3;
            case 4U: return DMA2_Channel4;
            case 5U: return DMA2_Channel5;
            case 6U: return DMA2_Channel6;
            case 7U: return DMA2_Channel7;
            case 8U: return DMA2_Channel8;
            default: return NULL;
        }
    }

    return NULL;
}

static DMAMUX_Channel_TypeDef *mux_registers(DMA_TypeDef *controller, uint8_t channel) {
    uint8_t mux_channel;

    if (controller == DMA1) {
        mux_channel = (uint8_t)(channel - 1U);
    } else if (controller == DMA2) {
        mux_channel = (uint8_t)(channel + 7U);
    } else {
        return NULL;
    }

    switch (mux_channel) {
        case 0U: return DMAMUX1_Channel0;
        case 1U: return DMAMUX1_Channel1;
        case 2U: return DMAMUX1_Channel2;
        case 3U: return DMAMUX1_Channel3;
        case 4U: return DMAMUX1_Channel4;
        case 5U: return DMAMUX1_Channel5;
        case 6U: return DMAMUX1_Channel6;
        case 7U: return DMAMUX1_Channel7;
        case 8U: return DMAMUX1_Channel8;
        case 9U: return DMAMUX1_Channel9;
        case 10U: return DMAMUX1_Channel10;
        case 11U: return DMAMUX1_Channel11;
        case 12U: return DMAMUX1_Channel12;
        case 13U: return DMAMUX1_Channel13;
        case 14U: return DMAMUX1_Channel14;
        case 15U: return DMAMUX1_Channel15;
        default: return NULL;
    }
}

static uint32_t channel_flags(uint8_t channel) {
    if (channel < 1U || channel > PHAL_DMA_CHANNEL_COUNT) {
        return 0U;
    }

    return (DMA_ISR_GIF1 | DMA_ISR_TCIF1 | DMA_ISR_HTIF1 | DMA_ISR_TEIF1)
        << (4U * (channel - 1U));
}

static uint32_t channel_transfer_complete_flag(uint8_t channel) {
    return channel >= 1U && channel <= PHAL_DMA_CHANNEL_COUNT
        ? DMA_ISR_TCIF1 << (4U * (channel - 1U))
        : 0U;
}

static uint32_t channel_transfer_error_flag(uint8_t channel) {
    return channel >= 1U && channel <= PHAL_DMA_CHANNEL_COUNT
        ? DMA_ISR_TEIF1 << (4U * (channel - 1U))
        : 0U;
}

static bool disable_channel(PHAL_DMA_State_t *state) {
    state->registers->CCR &= ~DMA_CCR_EN;
    for (uint32_t timeout = PHAL_DMA_TIMEOUT; timeout != 0U; --timeout) {
        if ((state->registers->CCR & DMA_CCR_EN) == 0U) {
            return true;
        }
    }
    return false;
}

static void enable_irq(DMA_TypeDef *controller, uint8_t channel) {
    if (controller == DMA1) {
        switch (channel) {
            case 1U: NVIC_EnableIRQ(DMA1_Channel1_IRQn); break;
            case 2U: NVIC_EnableIRQ(DMA1_Channel2_IRQn); break;
            case 3U: NVIC_EnableIRQ(DMA1_Channel3_IRQn); break;
            case 4U: NVIC_EnableIRQ(DMA1_Channel4_IRQn); break;
            case 5U: NVIC_EnableIRQ(DMA1_Channel5_IRQn); break;
            case 6U: NVIC_EnableIRQ(DMA1_Channel6_IRQn); break;
            case 7U: NVIC_EnableIRQ(DMA1_Channel7_IRQn); break;
            case 8U: NVIC_EnableIRQ(DMA1_Channel8_IRQn); break;
            default: break;
        }
    } else if (controller == DMA2) {
        switch (channel) {
            case 1U: NVIC_EnableIRQ(DMA2_Channel1_IRQn); break;
            case 2U: NVIC_EnableIRQ(DMA2_Channel2_IRQn); break;
            case 3U: NVIC_EnableIRQ(DMA2_Channel3_IRQn); break;
            case 4U: NVIC_EnableIRQ(DMA2_Channel4_IRQn); break;
            case 5U: NVIC_EnableIRQ(DMA2_Channel5_IRQn); break;
            case 6U: NVIC_EnableIRQ(DMA2_Channel6_IRQn); break;
            case 7U: NVIC_EnableIRQ(DMA2_Channel7_IRQn); break;
            case 8U: NVIC_EnableIRQ(DMA2_Channel8_IRQn); break;
            default: break;
        }
    }
}

static void remove_owner(PHAL_DMA_Handle_t *handle) {
    for (size_t i = 0U; i < PHAL_DMA_CHANNEL_COUNT; ++i) {
        if (dma1_owners[i] == handle) {
            dma1_owners[i] = NULL;
        }
        if (dma2_owners[i] == handle) {
            dma2_owners[i] = NULL;
        }
    }
}

bool PHAL_DMA_internalInit(
    PHAL_DMA_Handle_t *handle,
    const PHAL_DMA_Route_t *route
) {
    if (handle == NULL || route == NULL || route->controller == NULL
        || route->channel < 1U || route->channel > PHAL_DMA_CHANNEL_COUNT
        || route->request > (DMAMUX_CxCR_DMAREQ_ID_Msk >> DMAMUX_CxCR_DMAREQ_ID_Pos)
        || route->direction > PHAL_DMA_DIRECTION_MEMORY_TO_PERIPHERAL
        || route->peripheral_width > PHAL_DMA_WIDTH_32_BIT
        || route->memory_width > PHAL_DMA_WIDTH_32_BIT
        || route->priority > 3U) {
        return false;
    }

    DMA_Channel_TypeDef *registers = channel_registers(route->controller, route->channel);
    DMAMUX_Channel_TypeDef *mux = mux_registers(route->controller, route->channel);
    PHAL_DMA_Handle_t **owners = owners_for_controller(route->controller);
    if (registers == NULL || mux == NULL || owners == NULL) {
        return false;
    }

    PHAL_DMA_State_t *state = state_for_handle(handle);
    if (state != NULL && state->busy) {
        return false;
    }

    if (owners[route->channel - 1U] != NULL && owners[route->channel - 1U] != handle) {
        return false;
    }

    remove_owner(handle);

    if (route->controller == DMA1) {
        RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN;
    } else {
        RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN | RCC_AHB1ENR_DMAMUX1EN;
    }
    (void)RCC->AHB1ENR;

    state = (PHAL_DMA_State_t *)(uintptr_t)handle->_storage;
    memset(state, 0, sizeof(*state));
    state->magic = PHAL_DMA_HANDLE_MAGIC;
    state->route = route;
    state->registers = registers;
    state->initialized = true;

    owners[route->channel - 1U] = handle;

    if (!disable_channel(state)) {
        remove_owner(handle);
        memset(state, 0, sizeof(*state));
        return false;
    }

    route->controller->IFCR = channel_flags(route->channel);
    mux->CCR = (mux->CCR & ~DMAMUX_CxCR_DMAREQ_ID_Msk)
        | (((uint32_t)route->request << DMAMUX_CxCR_DMAREQ_ID_Pos) & DMAMUX_CxCR_DMAREQ_ID_Msk);
    return true;
}

bool PHAL_DMA_internalStart(
    PHAL_DMA_Handle_t *handle,
    volatile void *peripheral_address,
    void *memory_address,
    size_t count,
    bool memory_increment,
    bool circular,
    PHAL_DMA_CompletionCallback_t callback,
    void *context
) {
    PHAL_DMA_State_t *state = state_for_handle(handle);
    if (state == NULL || !state->initialized || peripheral_address == NULL
        || memory_address == NULL || count == 0U || count > UINT16_MAX
        || callback == NULL || state->busy) {
        return false;
    }

    if (!disable_channel(state)) {
        return false;
    }

    state->route->controller->IFCR = channel_flags(state->route->channel);
    state->registers->CPAR = (uint32_t)(uintptr_t)peripheral_address;
    state->registers->CMAR = (uint32_t)(uintptr_t)memory_address;
    state->registers->CNDTR = (uint32_t)count;

    uint32_t ccr = 0U;
    if (state->route->direction == PHAL_DMA_DIRECTION_MEMORY_TO_PERIPHERAL) {
        ccr |= DMA_CCR_DIR;
    }
    ccr |= ((uint32_t)state->route->peripheral_width << DMA_CCR_PSIZE_Pos) & DMA_CCR_PSIZE_Msk;
    ccr |= ((uint32_t)state->route->memory_width << DMA_CCR_MSIZE_Pos) & DMA_CCR_MSIZE_Msk;
    ccr |= ((uint32_t)state->route->priority << DMA_CCR_PL_Pos) & DMA_CCR_PL_Msk;
    if (memory_increment) {
        ccr |= DMA_CCR_MINC;
    }
    if (circular) {
        ccr |= DMA_CCR_CIRC;
    }
    ccr |= DMA_CCR_TCIE | DMA_CCR_TEIE;

    state->registers->CCR = ccr;
    state->callback = callback;
    state->callback_context = context;
    state->circular = circular;
    state->busy = true;

    enable_irq(state->route->controller, state->route->channel);
    state->registers->CCR = ccr | DMA_CCR_EN;
    return true;
}

bool PHAL_DMA_internalAbort(PHAL_DMA_State_t *state) {
    if (state == NULL || !state->initialized) {
        return false;
    }

    bool disabled = disable_channel(state);
    state->route->controller->IFCR = channel_flags(state->route->channel);
    state->busy = false;
    state->circular = false;
    state->callback = NULL;
    state->callback_context = NULL;
    return disabled;
}

void PHAL_DMA_internalHandleIRQ(DMA_TypeDef *controller, uint8_t channel) {
    PHAL_DMA_Handle_t **owners = owners_for_controller(controller);
    uint32_t flags = channel_flags(channel);
    if (owners == NULL || flags == 0U) {
        return;
    }

    uint32_t status = controller->ISR & flags;
    if (status == 0U) {
        return;
    }

    PHAL_DMA_Handle_t *handle = owners[channel - 1U];
    PHAL_DMA_State_t *state = state_for_handle(handle);
    if (state == NULL || !state->initialized) {
        controller->IFCR = flags;
        return;
    }

    const bool error = (status & channel_transfer_error_flag(channel)) != 0U;
    const bool complete = (status & channel_transfer_complete_flag(channel)) != 0U;
    if (!error && !complete) {
        controller->IFCR = flags;
        return;
    }

    const bool keep_circular = state->circular && !error;
    bool disabled = true;
    if (!keep_circular) {
        disabled = disable_channel(state);
    }
    controller->IFCR = flags;

    PHAL_DMA_CompletionCallback_t callback = state->callback;
    void *context = state->callback_context;
    const bool success = complete && !error && disabled;

    if (!keep_circular) {
        state->busy = false;
        state->callback = NULL;
        state->callback_context = NULL;
        state->circular = false;
    }

    if (callback != NULL) {
        callback(context, success);
    }
}

size_t PHAL_DMA_internalRemaining(const PHAL_DMA_Handle_t *handle) {
    const PHAL_DMA_State_t *state = state_for_handle(handle);
    if (state == NULL || !state->initialized) {
        return 0U;
    }
    return state->registers->CNDTR;
}

#define PHAL_DMA_ROUTE(controller_, channel_, request_, direction_, peripheral_width_, memory_width_) \
    { \
        .controller = (controller_), \
        .channel = (channel_), \
        .request = (request_), \
        .direction = (direction_), \
        .peripheral_width = (peripheral_width_), \
        .memory_width = (memory_width_), \
        .priority = 1U \
    }

static const PHAL_DMA_Route_t adc_routes[] = {
    PHAL_DMA_ROUTE(DMA1, 1U, 5U, PHAL_DMA_DIRECTION_PERIPHERAL_TO_MEMORY,
                   PHAL_DMA_WIDTH_16_BIT, PHAL_DMA_WIDTH_16_BIT),
    PHAL_DMA_ROUTE(DMA2, 1U, 36U, PHAL_DMA_DIRECTION_PERIPHERAL_TO_MEMORY,
                   PHAL_DMA_WIDTH_16_BIT, PHAL_DMA_WIDTH_16_BIT),
    PHAL_DMA_ROUTE(DMA2, 2U, 37U, PHAL_DMA_DIRECTION_PERIPHERAL_TO_MEMORY,
                   PHAL_DMA_WIDTH_16_BIT, PHAL_DMA_WIDTH_16_BIT),
    PHAL_DMA_ROUTE(DMA2, 3U, 38U, PHAL_DMA_DIRECTION_PERIPHERAL_TO_MEMORY,
                   PHAL_DMA_WIDTH_16_BIT, PHAL_DMA_WIDTH_16_BIT),
};

static const PHAL_DMA_Route_t spi_rx_routes[] = {
    PHAL_DMA_ROUTE(DMA1, 2U, 10U, PHAL_DMA_DIRECTION_PERIPHERAL_TO_MEMORY,
                   PHAL_DMA_WIDTH_8_BIT, PHAL_DMA_WIDTH_8_BIT),
    PHAL_DMA_ROUTE(DMA1, 4U, 12U, PHAL_DMA_DIRECTION_PERIPHERAL_TO_MEMORY,
                   PHAL_DMA_WIDTH_8_BIT, PHAL_DMA_WIDTH_8_BIT),
    PHAL_DMA_ROUTE(DMA2, 2U, 14U, PHAL_DMA_DIRECTION_PERIPHERAL_TO_MEMORY,
                   PHAL_DMA_WIDTH_8_BIT, PHAL_DMA_WIDTH_8_BIT),
};

static const PHAL_DMA_Route_t spi_tx_routes[] = {
    PHAL_DMA_ROUTE(DMA1, 3U, 11U, PHAL_DMA_DIRECTION_MEMORY_TO_PERIPHERAL,
                   PHAL_DMA_WIDTH_8_BIT, PHAL_DMA_WIDTH_8_BIT),
    PHAL_DMA_ROUTE(DMA1, 5U, 13U, PHAL_DMA_DIRECTION_MEMORY_TO_PERIPHERAL,
                   PHAL_DMA_WIDTH_8_BIT, PHAL_DMA_WIDTH_8_BIT),
    PHAL_DMA_ROUTE(DMA2, 3U, 15U, PHAL_DMA_DIRECTION_MEMORY_TO_PERIPHERAL,
                   PHAL_DMA_WIDTH_8_BIT, PHAL_DMA_WIDTH_8_BIT),
};

static const PHAL_DMA_Route_t usart_rx_routes[] = {
    PHAL_DMA_ROUTE(DMA1, 5U, 24U, PHAL_DMA_DIRECTION_PERIPHERAL_TO_MEMORY,
                   PHAL_DMA_WIDTH_8_BIT, PHAL_DMA_WIDTH_8_BIT),
    PHAL_DMA_ROUTE(DMA1, 3U, 26U, PHAL_DMA_DIRECTION_PERIPHERAL_TO_MEMORY,
                   PHAL_DMA_WIDTH_8_BIT, PHAL_DMA_WIDTH_8_BIT),
    PHAL_DMA_ROUTE(DMA1, 1U, 28U, PHAL_DMA_DIRECTION_PERIPHERAL_TO_MEMORY,
                   PHAL_DMA_WIDTH_8_BIT, PHAL_DMA_WIDTH_8_BIT),
};

static const PHAL_DMA_Route_t usart_tx_routes[] = {
    PHAL_DMA_ROUTE(DMA1, 7U, 25U, PHAL_DMA_DIRECTION_MEMORY_TO_PERIPHERAL,
                   PHAL_DMA_WIDTH_8_BIT, PHAL_DMA_WIDTH_8_BIT),
    PHAL_DMA_ROUTE(DMA1, 4U, 27U, PHAL_DMA_DIRECTION_MEMORY_TO_PERIPHERAL,
                   PHAL_DMA_WIDTH_8_BIT, PHAL_DMA_WIDTH_8_BIT),
    PHAL_DMA_ROUTE(DMA1, 2U, 29U, PHAL_DMA_DIRECTION_MEMORY_TO_PERIPHERAL,
                   PHAL_DMA_WIDTH_8_BIT, PHAL_DMA_WIDTH_8_BIT),
};

_Static_assert(sizeof(adc_routes) / sizeof(adc_routes[0]) == 4U, "ADC DMA route table changed");
_Static_assert(sizeof(spi_rx_routes) / sizeof(spi_rx_routes[0]) == 3U, "SPI RX DMA route table changed");
_Static_assert(sizeof(spi_tx_routes) / sizeof(spi_tx_routes[0]) == 3U, "SPI TX DMA route table changed");
_Static_assert(sizeof(usart_rx_routes) / sizeof(usart_rx_routes[0]) == 3U, "USART RX DMA route table changed");
_Static_assert(sizeof(usart_tx_routes) / sizeof(usart_tx_routes[0]) == 3U, "USART TX DMA route table changed");
_Static_assert(sizeof(PHAL_DMA_State_t) <= sizeof(PHAL_DMA_Handle_t), "DMA handle storage is too small");

const PHAL_DMA_Route_t *PHAL_DMA_internalAdcRoute(ADC_TypeDef *instance) {
    if (instance == ADC1) return &adc_routes[0];
    if (instance == ADC2) return &adc_routes[1];
    if (instance == ADC3) return &adc_routes[2];
    if (instance == ADC4) return &adc_routes[3];
    return NULL;
}

const PHAL_DMA_Route_t *PHAL_DMA_internalSpiRxRoute(SPI_TypeDef *instance) {
    if (instance == SPI1) return &spi_rx_routes[0];
    if (instance == SPI2) return &spi_rx_routes[1];
    if (instance == SPI3) return &spi_rx_routes[2];
    return NULL;
}

const PHAL_DMA_Route_t *PHAL_DMA_internalSpiTxRoute(SPI_TypeDef *instance) {
    if (instance == SPI1) return &spi_tx_routes[0];
    if (instance == SPI2) return &spi_tx_routes[1];
    if (instance == SPI3) return &spi_tx_routes[2];
    return NULL;
}

const PHAL_DMA_Route_t *PHAL_DMA_internalUsartRxRoute(USART_TypeDef *instance) {
    if (instance == USART1) return &usart_rx_routes[0];
    if (instance == USART2) return &usart_rx_routes[1];
    if (instance == USART3) return &usart_rx_routes[2];
    return NULL;
}

const PHAL_DMA_Route_t *PHAL_DMA_internalUsartTxRoute(USART_TypeDef *instance) {
    if (instance == USART1) return &usart_tx_routes[0];
    if (instance == USART2) return &usart_tx_routes[1];
    if (instance == USART3) return &usart_tx_routes[2];
    return NULL;
}
