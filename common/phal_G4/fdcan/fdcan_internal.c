#include "common/phal_G4/fdcan/fdcan_internal.h"

#include <string.h>

#include "common/phal_G4/rcc/rcc.h"

static PHAL_CAN_State_t states[] = {
    {.handle = &PHAL_CAN1},
    {.handle = &PHAL_CAN2},
    {.handle = &PHAL_CAN3},
};

PHAL_CAN_State_t *PHAL_CAN_internalState(PHAL_CAN_Handle_t *handle) {
    if (handle == NULL) {
        return NULL;
    }
    for (size_t i = 0U; i < sizeof(states) / sizeof(states[0]); ++i) {
        if (states[i].handle == handle) {
            return &states[i];
        }
    }
    return NULL;
}

PHAL_CAN_Handle_t *PHAL_CAN_internalHandle(FDCAN_GlobalTypeDef *instance) {
    for (size_t i = 0U; i < sizeof(states) / sizeof(states[0]); ++i) {
        if (states[i].handle->instance == instance && states[i].handle->initialized) {
            return states[i].handle;
        }
    }
    return NULL;
}

FDCAN_GlobalTypeDef *PHAL_CAN_internalExpectedInstance(const PHAL_CAN_Handle_t *handle) {
    if (handle == &PHAL_CAN1) return FDCAN1;
    if (handle == &PHAL_CAN2) return FDCAN2;
    if (handle == &PHAL_CAN3) return FDCAN3;
    return NULL;
}

bool PHAL_CAN_internalEnterInit(FDCAN_GlobalTypeDef *instance) {
    if (instance == NULL) {
        return false;
    }
    instance->CCCR &= ~FDCAN_CCCR_CSR;
    for (uint32_t timeout = PHAL_CAN_TIMEOUT; timeout != 0U; --timeout) {
        if ((instance->CCCR & FDCAN_CCCR_CSA) == 0U) {
            break;
        }
        if (timeout == 1U) {
            return false;
        }
    }
    instance->CCCR |= FDCAN_CCCR_INIT;
    for (uint32_t timeout = PHAL_CAN_TIMEOUT; timeout != 0U; --timeout) {
        if ((instance->CCCR & FDCAN_CCCR_INIT) != 0U) {
            instance->CCCR |= FDCAN_CCCR_CCE;
            return true;
        }
        if (timeout == 1U) {
            return false;
        }
    }
    return false;
}

bool PHAL_CAN_internalExitInit(FDCAN_GlobalTypeDef *instance) {
    if (instance == NULL) {
        return false;
    }
    instance->CCCR &= ~FDCAN_CCCR_INIT;
    for (uint32_t timeout = PHAL_CAN_TIMEOUT; timeout != 0U; --timeout) {
        if ((instance->CCCR & FDCAN_CCCR_INIT) == 0U) {
            return true;
        }
        if (timeout == 1U) {
            return false;
        }
    }
    return false;
}

bool PHAL_CAN_internalMakeNBTP(uint32_t kernel_hz, uint32_t bit_rate, uint32_t *nbtp) {
    if (kernel_hz == 0U || bit_rate == 0U || nbtp == NULL) {
        return false;
    }

    uint32_t time_quanta;
    uint32_t segment1;
    uint32_t segment2;
    uint32_t sjw;
    switch (bit_rate) {
        case 125000U:
        case 250000U:
        case 500000U:
        case 1000000U:
            time_quanta = 16U;
            segment1 = 13U;
            segment2 = 2U;
            sjw = 2U;
            break;
        case 2000000U:
            time_quanta = 8U;
            segment1 = 4U;
            segment2 = 3U;
            sjw = 2U;
            break;
        default:
            return false;
    }

    if (sjw > segment2) {
        sjw = segment2;
    }
    const uint64_t denominator = (uint64_t)bit_rate * time_quanta;
    const uint32_t prescaler = (uint32_t)(kernel_hz / denominator);
    if (prescaler < 1U || prescaler > 512U
        || (uint64_t)prescaler * denominator != kernel_hz
        || segment1 < 1U || segment1 > 256U
        || segment2 < 1U || segment2 > 128U
        || sjw < 1U || sjw > 128U) {
        return false;
    }

    *nbtp = (((prescaler - 1U) << FDCAN_NBTP_NBRP_Pos) & FDCAN_NBTP_NBRP_Msk)
        | (((segment1 - 1U) << FDCAN_NBTP_NTSEG1_Pos) & FDCAN_NBTP_NTSEG1_Msk)
        | (((segment2 - 1U) << FDCAN_NBTP_NTSEG2_Pos) & FDCAN_NBTP_NTSEG2_Msk)
        | (((sjw - 1U) << FDCAN_NBTP_NSJW_Pos) & FDCAN_NBTP_NSJW_Msk);
    return true;
}

uintptr_t PHAL_CAN_internalRamBase(FDCAN_GlobalTypeDef *instance) {
    if (instance == FDCAN1) {
        return (uintptr_t)SRAMCAN_BASE;
    }
    if (instance == FDCAN2) {
        return (uintptr_t)SRAMCAN_BASE + SRAMCAN_SIZE;
    }
    if (instance == FDCAN3) {
        return (uintptr_t)SRAMCAN_BASE + (2U * SRAMCAN_SIZE);
    }
    return 0U;
}

_Static_assert(SRAMCAN_SIZE == 848U, "Unexpected G4 fixed FDCAN message-RAM layout");
_Static_assert(SRAMCAN_TOTAL_SIZE == (3U * SRAMCAN_SIZE), "FDCAN partition count changed");
_Static_assert(SRAMCAN_TOTAL_SIZE == 2544U, "FDCAN partitions exceed audited layout");

PHAL_CAN_Handle_t PHAL_CAN1;
PHAL_CAN_Handle_t PHAL_CAN2;
PHAL_CAN_Handle_t PHAL_CAN3;

static bool supported_instance(FDCAN_GlobalTypeDef *instance) {
    return instance == FDCAN1 || instance == FDCAN2 || instance == FDCAN3;
}

static uint32_t fdcan_ir_all(void) {
    return FDCAN_IR_RF0N | FDCAN_IR_RF0F | FDCAN_IR_RF0L
        | FDCAN_IR_RF1N | FDCAN_IR_RF1F | FDCAN_IR_RF1L
        | FDCAN_IR_HPM | FDCAN_IR_TC | FDCAN_IR_TCF | FDCAN_IR_TFE
        | FDCAN_IR_TEFN | FDCAN_IR_TEFF | FDCAN_IR_TEFL | FDCAN_IR_TSW
        | FDCAN_IR_MRAF | FDCAN_IR_TOO | FDCAN_IR_ELO | FDCAN_IR_EP
        | FDCAN_IR_EW | FDCAN_IR_BO | FDCAN_IR_WDI | FDCAN_IR_PEA
        | FDCAN_IR_PED | FDCAN_IR_ARA;
}

static void clear_message_ram(FDCAN_GlobalTypeDef *instance) {
    volatile uint32_t *ram = (volatile uint32_t *)PHAL_CAN_internalRamBase(instance);
    for (size_t offset = 0U; offset < SRAMCAN_SIZE / sizeof(uint32_t); ++offset) {
        ram[offset] = 0U;
    }
}

static void configure_default_filters(FDCAN_GlobalTypeDef *instance) {
    clear_message_ram(instance);
    instance->RXGFC = FDCAN_RXGFC_RRFE | FDCAN_RXGFC_RRFS;
    instance->XIDAM = FDCAN_XIDAM_EIDM_Msk;
}

static bool valid_filters(const PHAL_CAN_FilterConfig_t *filters) {
    if (filters == NULL || filters->standard_id_count > MAX_NUM_SID_FILTER
        || filters->extended_id_count > MAX_NUM_XID_FILTER
        || (filters->standard_id_count != 0U && filters->standard_ids == NULL)
        || (filters->extended_id_count != 0U && filters->extended_ids == NULL)) {
        return false;
    }
    for (size_t i = 0U; i < filters->standard_id_count; ++i) {
        if (filters->standard_ids[i] > 0x7FFU) {
            return false;
        }
    }
    for (size_t i = 0U; i < filters->extended_id_count; ++i) {
        if (filters->extended_ids[i] > 0x1FFFFFFFU) {
            return false;
        }
    }
    return true;
}

bool PHAL_CAN_internalValidateInit(
    PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_Config_t *config
) {
    return PHAL_CAN_internalState(handle) != NULL && config != NULL
        && supported_instance(PHAL_CAN_internalExpectedInstance(handle))
        && config->bit_rate != 0U;
}

bool PHAL_CAN_internalBuildTiming(uint32_t bit_rate, uint32_t *nbtp) {
    const uint32_t kernel_hz = PHAL_RCC_fdcanClockHz();
    return kernel_hz != 0U && PHAL_CAN_internalMakeNBTP(kernel_hz, bit_rate, nbtp);
}

void PHAL_CAN_internalEnableClock(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_FDCANEN;
    (void)RCC->APB1ENR1;
}

bool PHAL_CAN_internalConfigureController(
    PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_Config_t *config,
    uint32_t nbtp
) {
    PHAL_CAN_State_t *state = PHAL_CAN_internalState(handle);
    FDCAN_GlobalTypeDef *instance = PHAL_CAN_internalExpectedInstance(handle);
    if (state == NULL || instance == NULL || !PHAL_CAN_internalEnterInit(instance)) {
        return false;
    }

    uint32_t cccr = instance->CCCR;
    cccr &= ~(FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE | FDCAN_CCCR_DAR
              | FDCAN_CCCR_TXP | FDCAN_CCCR_MON | FDCAN_CCCR_ASM
              | FDCAN_CCCR_TEST | FDCAN_CCCR_PXHD);
    instance->CCCR = cccr;
    instance->NBTP = nbtp;
    instance->TXBC &= ~FDCAN_TXBC_TFQM;
    configure_default_filters(instance);

    instance->ILS = FDCAN_ILS_SMSG;
    instance->IR = fdcan_ir_all();
    instance->IE = FDCAN_IE_RF0NE | FDCAN_IE_RF0LE | FDCAN_IE_TCE
        | FDCAN_IE_MRAFE;
    instance->TXBTIE = (1UL << SRAMCAN_TFQ_NBR) - 1UL;
    instance->ILE = FDCAN_ILE_EINT0 | FDCAN_ILE_EINT1;

    instance->TEST &= ~FDCAN_TEST_LBCK;
    if (config->loopback) {
        instance->CCCR |= FDCAN_CCCR_TEST | FDCAN_CCCR_MON;
        instance->TEST |= FDCAN_TEST_LBCK;
    }

    if (!PHAL_CAN_internalExitInit(instance)) {
        handle->initialized = false;
        return false;
    }

    handle->instance = instance;
    handle->initialized = true;
    state->standard_filter_count = 0U;
    state->extended_filter_count = 0U;
    return true;
}

bool PHAL_CAN_internalValidateFilters(
    const PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_FilterConfig_t *filters
) {
    return PHAL_CAN_internalValidateHandle(handle) && valid_filters(filters);
}

void PHAL_CAN_internalProgramFilters(
    PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_FilterConfig_t *filters
) {
    FDCAN_GlobalTypeDef *instance = handle->instance;
    if (instance == NULL) {
        return;
    }
    const uintptr_t base = PHAL_CAN_internalRamBase(instance);
    volatile uint32_t *standard = (volatile uint32_t *)(base + SRAMCAN_FLSSA);
    volatile uint32_t *extended = (volatile uint32_t *)(base + SRAMCAN_FLESA);
    for (size_t i = 0U; i < SRAMCAN_FLS_NBR; ++i) {
        standard[i] = 0U;
    }
    for (size_t i = 0U; i < 2U * SRAMCAN_FLE_NBR; ++i) {
        extended[i] = 0U;
    }

    for (size_t i = 0U; i < filters->standard_id_count; ++i) {
        const uint32_t id = filters->standard_ids[i];
        /* SFT=classic mask, SFEC=FIFO0, SFID1=id, SFID2=all bits. */
        standard[i] = (2U << 30) | (1U << 27) | (id << 16) | 0x7FFU;
    }
    for (size_t i = 0U; i < filters->extended_id_count; ++i) {
        const uint32_t id = filters->extended_ids[i];
        extended[i * 2U] = (1U << 29) | id;
        extended[i * 2U + 1U] = (2U << 30) | 0x1FFFFFFFU;
    }

    instance->RXGFC = FDCAN_RXGFC_RRFE | FDCAN_RXGFC_RRFS
        | (FDCAN_REJECT << FDCAN_RXGFC_ANFE_Pos)
        | (FDCAN_REJECT << FDCAN_RXGFC_ANFS_Pos)
        | (((uint32_t)filters->standard_id_count << FDCAN_RXGFC_LSS_Pos)
           & FDCAN_RXGFC_LSS_Msk)
        | (((uint32_t)filters->extended_id_count << FDCAN_RXGFC_LSE_Pos)
           & FDCAN_RXGFC_LSE_Msk);
    instance->XIDAM = FDCAN_XIDAM_EIDM_Msk;
}

void PHAL_CAN_internalStoreFilterCounts(
    PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_FilterConfig_t *filters
) {
    PHAL_CAN_State_t *state = PHAL_CAN_internalState(handle);
    if (state == NULL) {
        return;
    }
    state->standard_filter_count = filters->standard_id_count;
    state->extended_filter_count = filters->extended_id_count;
}

bool PHAL_CAN_internalValidateHandle(const PHAL_CAN_Handle_t *handle) {
    return handle != NULL && handle->initialized && handle->instance != NULL;
}

bool PHAL_CAN_internalTxAvailable(const PHAL_CAN_Handle_t *handle) {
    return (handle->instance->TXFQS & FDCAN_TXFQS_TFQF) == 0U;
}

bool PHAL_CAN_internalValidateMessage(
    const PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_Message_t *message
) {
    return PHAL_CAN_internalValidateHandle(handle) && message != NULL && message->length <= 8U
        && (message->extended || message->id <= 0x7FFU)
        && (!message->extended || message->id <= 0x1FFFFFFFU)
        && PHAL_CAN_internalTxAvailable(handle);
}

bool PHAL_CAN_internalWriteTx(PHAL_CAN_Handle_t *handle, const PHAL_CAN_Message_t *message) {
    FDCAN_GlobalTypeDef *instance = handle->instance;
    if (instance == NULL) {
        return false;
    }
    const uint32_t status = instance->TXFQS;
    const uint32_t put = (status & FDCAN_TXFQS_TFQPI_Msk) >> FDCAN_TXFQS_TFQPI_Pos;
    if (put >= SRAMCAN_TFQ_NBR) {
        return false;
    }

    const uintptr_t base = PHAL_CAN_internalRamBase(instance);
    volatile uint32_t *tx = (volatile uint32_t *)(base + SRAMCAN_TFQSA
        + (put * SRAMCAN_TFQ_SIZE));
    tx[0] = message->extended
        ? ((message->id & 0x1FFFFFFFU) | (1U << 30))
        : ((message->id & 0x7FFU) << 18);
    tx[1] = ((uint32_t)message->length & 0xFU) << 16;
    uint32_t data0 = 0U;
    uint32_t data1 = 0U;
    memcpy(&data0, &message->data[0], sizeof(data0));
    memcpy(&data1, &message->data[4], sizeof(data1));
    tx[2] = data0;
    tx[3] = data1;
    instance->TXBAR = 1UL << put;
    return true;
}

static uint8_t dlc_to_length(uint8_t dlc) {
    static const uint8_t lengths[16] = {
        0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U,
        8U, 8U, 8U, 8U, 8U, 8U, 8U, 8U,
    };
    return lengths[dlc & 0xFU];
}

static bool receive_one(PHAL_CAN_Handle_t *handle, PHAL_CAN_Message_t *message) {
    FDCAN_GlobalTypeDef *instance = handle->instance;
    if (instance == NULL) {
        return false;
    }
    const uint32_t status = instance->RXF0S;
    if ((status & FDCAN_RXF0S_F0FL_Msk) == 0U || message == NULL) {
        return false;
    }

    const uint32_t get = (status & FDCAN_RXF0S_F0GI_Msk) >> FDCAN_RXF0S_F0GI_Pos;
    if (get >= SRAMCAN_RF0_NBR) {
        return false;
    }
    const uintptr_t base = PHAL_CAN_internalRamBase(instance);
    volatile uint32_t *rx = (volatile uint32_t *)(base + SRAMCAN_RF0SA
        + (get * SRAMCAN_RF0_SIZE));
    const uint32_t word0 = rx[0];
    const uint32_t word1 = rx[1];

    message->extended = (word0 & (1UL << 30)) != 0U;
    message->id = message->extended ? (word0 & 0x1FFFFFFFU) : ((word0 >> 18) & 0x7FFU);
    message->length = dlc_to_length((uint8_t)((word1 >> 16) & 0xFU));
    memcpy(message->data, (const void *)&rx[2], sizeof(message->data));
    instance->RXF0A = get;
    return true;
}

static void handle_rx_irq(FDCAN_GlobalTypeDef *instance) {
    PHAL_CAN_Handle_t *handle = PHAL_CAN_internalHandle(instance);
    if (handle == NULL) {
        instance->IR = FDCAN_IR_RF0N | FDCAN_IR_RF0F | FDCAN_IR_RF0L;
        return;
    }

    const uint32_t pending = instance->IR & instance->IE;
    const uint32_t rx_flags = pending & (FDCAN_IR_RF0N | FDCAN_IR_RF0F | FDCAN_IR_RF0L);
    if (rx_flags != 0U) {
        instance->IR = rx_flags;
        PHAL_CAN_Message_t message;
        while (receive_one(handle, &message)) {
            PHAL_CAN_rxCallback(handle, &message);
        }
    }
    const uint32_t errors = pending & (FDCAN_IR_MRAF | FDCAN_IR_RF0L);
    if (errors != 0U) {
        instance->IR = errors;
    }
}

static void handle_tx_irq(FDCAN_GlobalTypeDef *instance) {
    PHAL_CAN_Handle_t *handle = PHAL_CAN_internalHandle(instance);
    const uint32_t pending = instance->IR & instance->IE;
    if ((pending & FDCAN_IR_TC) != 0U) {
        instance->IR = FDCAN_IR_TC;
        if (handle != NULL) {
            PHAL_CAN_txCallback(handle);
        }
    }
}

__attribute__((weak)) void PHAL_CAN_rxCallback(
    PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_Message_t *message
) {
    (void)handle;
    (void)message;
}

__attribute__((weak)) void PHAL_CAN_txCallback(PHAL_CAN_Handle_t *handle) {
    (void)handle;
}

void FDCAN1_IT0_IRQHandler(void) { handle_rx_irq(FDCAN1); }
void FDCAN1_IT1_IRQHandler(void) { handle_tx_irq(FDCAN1); }
void FDCAN2_IT0_IRQHandler(void) { handle_rx_irq(FDCAN2); }
void FDCAN2_IT1_IRQHandler(void) { handle_tx_irq(FDCAN2); }
void FDCAN3_IT0_IRQHandler(void) { handle_rx_irq(FDCAN3); }
void FDCAN3_IT1_IRQHandler(void) { handle_tx_irq(FDCAN3); }
