/**
 * @file usb_priv.c
 * @brief STM32G4 USB register-level implementation.
 *
 * Hardware-specific register access stays here. The public layer supplies
 * validated endpoint and packet requests and receives simple status results.
 */

#include "common/phal_G4/phal_G4.h"
#include "common/phal_G4/usb/usb_priv.h"

// The first 64 PMA bytes contain eight 8-byte buffer descriptors.
static constexpr uint16_t USB_PRIV_BUFFER_TABLE_OFFSET = 0U;

// HSI48 startup and USB analog delays are bounded busy-waits during init.
static constexpr uint32_t USB_PRIV_HSI48_TIMEOUT = 1'000'000U;
static constexpr uint32_t USB_PRIV_PHY_STARTUP_DELAY = 1'000U;
static constexpr uint32_t USB_PRIV_DISCONNECT_DELAY = 500'000U;

// Endpoint status values are two-bit toggle encodings from the STM32 USB
// endpoint register. These values must stay private to this implementation.
static constexpr uint8_t USB_PRIV_ENDPOINT_STATUS_DISABLED = 0U;
static constexpr uint8_t USB_PRIV_ENDPOINT_STATUS_STALL = 1U;
static constexpr uint8_t USB_PRIV_ENDPOINT_STATUS_NAK = 2U;
static constexpr uint8_t USB_PRIV_ENDPOINT_STATUS_VALID = 3U;

// USB D-/D+ are dedicated USB pins on PA11/PA12. Type-C CC1/CC2 are PA8/PA9.
static constexpr uint8_t USB_PRIV_DM_PIN = 11U;
static constexpr uint8_t USB_PRIV_DP_PIN = 12U;

/**
 * Return an endpoint register pointer.
 *
 * EP0R through EP7R are 32-bit spaced in the CMSIS register layout even
 * though each register is only 16 bits wide.
 */
static volatile uint16_t *usb_endpoint_register(uint8_t endpoint) {
    return &USB->EP0R + (endpoint * 2U);
}

/**
 * Return a word in an endpoint's four-word PMA buffer descriptor.
 *
 * Each endpoint descriptor occupies 8 bytes: transmit address/count followed
 * by receive address/count. The PMA address is a CPU byte offset.
 */
static volatile uint16_t *usb_buffer_descriptor(uint8_t endpoint, uint8_t word_index) {
    uint32_t offset = USB_PRIV_BUFFER_TABLE_OFFSET + (endpoint * 8U) + (word_index * 2U);
    return (volatile uint16_t *)(USB_PMAADDR + offset);
}

/** Return a CPU pointer for a byte offset in USB packet memory. */
static volatile uint16_t *usb_pma_address(uint16_t address) {
    return (volatile uint16_t *)(USB_PMAADDR + address);
}

/** Busy-wait for short hardware startup intervals without a timer dependency. */
static void usb_delay(uint32_t iterations) {
    while (iterations > 0U) {
        __NOP();
        iterations--;
    }
}

/**
 * Configure the G474 as a USB-C sink.
 *
 * UCPD provides the Rd termination on CC1/CC2. USB is not attached until the
 * public layer later enables D+ pull-up with USB_PRIV_set_connected().
 */
static void usb_configure_type_c_sink(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    RCC->APB1ENR2 |= RCC_APB1ENR2_UCPD1EN;
    __DSB();

    // UCPDEN enables UCPD; ANAMODE selects sink behavior; CCENABLE enables CC1/CC2.
    UCPD1->CFG1 = UCPD_CFG1_UCPDEN;
    UCPD1->CR = UCPD_CR_ANAMODE | UCPD_CR_CCENABLE;
    // Disable the reset-time dead-battery pull-down before using programmed Rd.
    PWR->CR3 |= PWR_CR3_UCPD_DBDIS;
}

/** Enable the dedicated 48 MHz USB clock and wait until it is ready. */
static bool usb_enable_clock(void) {
    RCC->CRRCR |= RCC_CRRCR_HSI48ON;
    for (uint32_t timeout = 0U; timeout < USB_PRIV_HSI48_TIMEOUT; timeout++) {
        if ((RCC->CRRCR & RCC_CRRCR_HSI48RDY) != 0U) {
            return true;
        }
    }

    return false;
}

/** Configure USB and USB-C pins for their dedicated analog paths. */
static void usb_configure_pins(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    __DSB();

    constexpr uint32_t type_c_pin_mask = (0b11U << (8U * 2U)) | (0b11U << (9U * 2U));
    GPIOA->MODER = (GPIOA->MODER & ~type_c_pin_mask) | type_c_pin_mask;
    GPIOA->PUPDR &= ~type_c_pin_mask;

    constexpr uint32_t usb_pin_mask = (0b11U << (USB_PRIV_DM_PIN * 2U))
        | (0b11U << (USB_PRIV_DP_PIN * 2U));
    GPIOA->MODER = (GPIOA->MODER & ~usb_pin_mask) | usb_pin_mask;
    GPIOA->PUPDR &= ~usb_pin_mask;
}

bool USB_PRIV_initialize_hardware(void) {
    // USB FS requires the dedicated, factory-trimmed 48 MHz oscillator.
    if (!usb_enable_clock()) {
        return false;
    }

    // CLK48SEL = 0 selects HSI48 as the USB clock source.
    RCC->CCIPR &= ~RCC_CCIPR_CLK48SEL_Msk;
    RCC->APB1ENR1 |= RCC_APB1ENR1_USBEN;
    __DSB();
    usb_configure_pins();
    usb_configure_type_c_sink();

    // Reset the USB peripheral after its clock and pins are available.
    RCC->APB1RSTR1 |= RCC_APB1RSTR1_USBRST;
    __DSB();
    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_USBRST;
    __DSB();

    // Power up the analog transceiver before releasing the forced reset.
    USB->CNTR = USB_CNTR_FRES | USB_CNTR_PDWN;
    usb_delay(USB_PRIV_PHY_STARTUP_DELAY);
    USB->CNTR = USB_CNTR_FRES;
    usb_delay(USB_PRIV_PHY_STARTUP_DELAY);

    // Clear pending events, endpoint registers, and the device address.
    USB->ISTR = 0U;
    for (uint8_t endpoint = 0U; endpoint < PHAL_USB_ENDPOINT_COUNT; endpoint++) {
        *usb_endpoint_register(endpoint) = 0U;
    }
    USB->BTABLE = USB_PRIV_BUFFER_TABLE_OFFSET;
    USB->DADDR = 0U;
    USB->LPMCSR = 0U;
    USB->BCDR = 0U;
    USB->CNTR = 0U;
    __DSB();
    usb_delay(USB_PRIV_DISCONNECT_DELAY);
    return true;
}

void USB_PRIV_deinitialize_hardware(void) {
    USB->BCDR = 0U;
    USB->CNTR = USB_CNTR_FRES | USB_CNTR_PDWN;
    USB->DADDR = 0U;
    RCC->APB1ENR1 &= ~RCC_APB1ENR1_USBEN;
    __DSB();
}

void USB_PRIV_enable_interrupts(void) {
    // Enable correct-transfer, error, wakeup, suspend, and reset interrupts.
    USB->CNTR = USB_CNTR_CTRM | USB_CNTR_PMAOVRM | USB_CNTR_ERRM | USB_CNTR_WKUPM
        | USB_CNTR_SUSPM | USB_CNTR_RESETM;
    USB->DADDR = USB_DADDR_EF;
    NVIC_EnableIRQ(USB_LP_IRQn);
}

void USB_PRIV_disable_interrupts(void) {
    USB->CNTR = 0U;
    NVIC_DisableIRQ(USB_LP_IRQn);
}

void USB_PRIV_set_connected(bool connected) {
    if (connected) {
        USB->BCDR |= USB_BCDR_DPPU;
        return;
    }

    USB->BCDR &= ~USB_BCDR_DPPU;
}

bool USB_PRIV_set_address(uint8_t address) {
    if (address > USB_DADDR_ADD) {
        return false;
    }

    USB->DADDR = USB_DADDR_EF | address;
    return true;
}

/** Translate the public endpoint type to the STM32 EP_TYPE field. */
static bool usb_get_type_bits(PHAL_USB_EndpointType_t type, uint16_t *type_bits) {
    if (type_bits == nullptr) {
        return false;
    }

    switch (type) {
        case PHAL_USB_ENDPOINT_TYPE_CONTROL:
            *type_bits = USB_EP_CONTROL;
            return true;
        case PHAL_USB_ENDPOINT_TYPE_BULK:
            *type_bits = USB_EP_BULK;
            return true;
        case PHAL_USB_ENDPOINT_TYPE_ISOCHRONOUS:
            *type_bits = USB_EP_ISOCHRONOUS;
            return true;
        case PHAL_USB_ENDPOINT_TYPE_INTERRUPT:
            *type_bits = USB_EP_INTERRUPT;
            return true;
        default:
            return false;
    }
}

bool USB_PRIV_configure_endpoint(uint8_t endpoint,
                                 PHAL_USB_EndpointType_t type,
                                 bool transmit_enabled,
                                 bool receive_enabled) {
    // Endpoint STAT fields are toggle bits: XOR changes only the requested
    // direction while preserving the endpoint address and transfer type.
    uint16_t type_bits = 0U;
    if (!usb_get_type_bits(type, &type_bits)) {
        return false;
    }

    uint16_t current = *usb_endpoint_register(endpoint);
    uint16_t value = (uint16_t)(endpoint & USB_EPADDR_FIELD) | type_bits;
    value |= (current & USB_EPTX_STAT)
        ^ (transmit_enabled ? (USB_PRIV_ENDPOINT_STATUS_NAK << 4U)
                             : (USB_PRIV_ENDPOINT_STATUS_DISABLED << 4U));
    value |= (current & USB_EPRX_STAT)
        ^ (receive_enabled ? (USB_PRIV_ENDPOINT_STATUS_VALID << 12U)
                           : (USB_PRIV_ENDPOINT_STATUS_DISABLED << 12U));
    *usb_endpoint_register(endpoint) = value;
    return true;
}

bool USB_PRIV_configure_buffer_descriptors(uint8_t endpoint,
                                           bool transmit_enabled,
                                           uint16_t transmit_buffer_address,
                                           bool receive_enabled,
                                           uint16_t receive_buffer_address,
                                           uint16_t receive_buffer_size) {
    // COUNT_RX uses 2-byte blocks through 62 bytes, then 32-byte blocks.
    // The public layer has already checked the endpoint size and PMA range.
    uint16_t receive_count_encoding = 0U;
    if (receive_enabled && receive_buffer_size == 0U) {
        return false;
    }
    if (receive_enabled && receive_buffer_size <= 62U) {
        uint16_t block_count = (receive_buffer_size + 1U) / 2U;
        receive_count_encoding = (uint16_t)(block_count << 10U);
    } else if (receive_enabled) {
        uint16_t block_count = (receive_buffer_size + 31U) / 32U;
        receive_count_encoding = USB_COUNT0_RX_BLSIZE | (uint16_t)((block_count - 1U) << 10U);
    }

    *usb_buffer_descriptor(endpoint, 0U) = transmit_enabled ? transmit_buffer_address : 0U;
    *usb_buffer_descriptor(endpoint, 1U) = 0U;
    *usb_buffer_descriptor(endpoint, 2U) = receive_enabled ? receive_buffer_address : 0U;
    *usb_buffer_descriptor(endpoint, 3U) = receive_enabled ? receive_count_encoding : 0U;
    return true;
}

/** Read the two-bit STAT field for an IN or OUT endpoint direction. */
static uint16_t usb_endpoint_status(uint8_t endpoint, PHAL_USB_EndpointDirection_t direction) {
    uint16_t register_value = *usb_endpoint_register(endpoint);
    uint16_t status_mask = direction == PHAL_USB_ENDPOINT_DIRECTION_IN ? USB_EPTX_STAT : USB_EPRX_STAT;
    uint8_t shift = direction == PHAL_USB_ENDPOINT_DIRECTION_IN ? 4U : 12U;
    return (uint16_t)((register_value & status_mask) >> shift);
}

bool USB_PRIV_endpoint_is_ready(uint8_t endpoint, PHAL_USB_EndpointDirection_t direction) {
    return usb_endpoint_status(endpoint, direction) == USB_PRIV_ENDPOINT_STATUS_NAK;
}

/**
 * Write an endpoint STAT field using the STM32 toggle-register convention.
 *
 * Endpoint registers must be written with the XOR of the current and desired
 * status; writing the desired value directly would produce the wrong state.
 */
static bool usb_set_endpoint_status(uint8_t endpoint,
                                    PHAL_USB_EndpointDirection_t direction,
                                    uint8_t status) {
    if (status > USB_PRIV_ENDPOINT_STATUS_VALID) {
        return false;
    }

    uint16_t current = *usb_endpoint_register(endpoint);
    uint16_t mask = direction == PHAL_USB_ENDPOINT_DIRECTION_IN ? USB_EPTX_STAT : USB_EPRX_STAT;
    uint16_t desired = (uint16_t)status << (direction == PHAL_USB_ENDPOINT_DIRECTION_IN ? 4U : 12U);
    uint16_t value = current & (USB_EPREG_MASK & ~USB_EP_SETUP);
    value |= (current & mask) ^ desired;
    *usb_endpoint_register(endpoint) = value;
    return true;
}

bool USB_PRIV_stall_endpoint(uint8_t endpoint, PHAL_USB_EndpointDirection_t direction) {
    return usb_set_endpoint_status(endpoint, direction, USB_PRIV_ENDPOINT_STATUS_STALL);
}

bool USB_PRIV_set_endpoint_valid(uint8_t endpoint, PHAL_USB_EndpointDirection_t direction) {
    return usb_set_endpoint_status(endpoint, direction, USB_PRIV_ENDPOINT_STATUS_VALID);
}

bool USB_PRIV_clear_endpoint_interrupt(uint8_t endpoint,
                                       PHAL_USB_EndpointDirection_t direction) {
    // CTR_RX/CTR_TX are cleared by writing zero while preserving other fields.
    uint16_t current = *usb_endpoint_register(endpoint);
    uint16_t value = current & (USB_EPREG_MASK & ~USB_EP_SETUP);
    value &= (uint16_t)~(direction == PHAL_USB_ENDPOINT_DIRECTION_IN
                              ? USB_EP_CTR_TX : USB_EP_CTR_RX);
    *usb_endpoint_register(endpoint) = value;
    return true;
}

bool USB_PRIV_endpoint_is_setup(uint8_t endpoint) {
    return (*usb_endpoint_register(endpoint) & USB_EP_SETUP) != 0U;
}

uint16_t USB_PRIV_read_received_length(uint8_t endpoint) {
    return *usb_buffer_descriptor(endpoint, 3U) & USB_COUNT0_TX_COUNT0_TX_Msk;
}

void USB_PRIV_set_transmit_length(uint8_t endpoint, uint16_t length_bytes) {
    *usb_buffer_descriptor(endpoint, 1U) = length_bytes;
}

void USB_PRIV_rearm_receive_buffer(uint8_t endpoint) {
    // Preserve NUM_BLOCK/BLSIZE and discard the received COUNT_RX value.
    volatile uint16_t *descriptor = usb_buffer_descriptor(endpoint, 3U);
    uint16_t size_encoding = *descriptor & (uint16_t)~USB_COUNT0_TX_COUNT0_TX_Msk;
    *descriptor = size_encoding;
}

void USB_PRIV_write_pma(uint16_t address, const void *source, uint16_t length_bytes) {
    // PMA is accessed as 16-bit words; pad the final odd byte with zero.
    const uint8_t *input = source;
    volatile uint16_t *destination = usb_pma_address(address);
    for (uint16_t offset = 0U; offset < length_bytes; offset += 2U) {
        uint16_t word = input[offset];
        if ((offset + 1U) < length_bytes) {
            word |= (uint16_t)input[offset + 1U] << 8U;
        }
        *destination = word;
        destination++;
    }
}

void USB_PRIV_read_pma(uint16_t address, void *destination, uint16_t length_bytes) {
    // Read words from PMA and unpack only the requested number of bytes.
    uint8_t *output = destination;
    volatile const uint16_t *source = usb_pma_address(address);
    for (uint16_t offset = 0U; offset < length_bytes; offset += 2U) {
        uint16_t word = *source;
        output[offset] = (uint8_t)word;
        if ((offset + 1U) < length_bytes) {
            output[offset + 1U] = (uint8_t)(word >> 8U);
        }
        source++;
    }
}

static void usb_clear_interrupts(uint16_t interrupt_flags) {
    // USB_ISTR event bits are clear-on-zero; CTR is handled per endpoint.
    constexpr uint16_t clearable_flags = USB_ISTR_PMAOVR | USB_ISTR_ERR | USB_ISTR_WKUP
        | USB_ISTR_SUSP | USB_ISTR_RESET | USB_ISTR_SOF | USB_ISTR_ESOF | USB_ISTR_L1REQ;
    uint16_t flags_to_clear = interrupt_flags & clearable_flags;
    if (flags_to_clear != 0U) {
        USB->ISTR = (uint16_t)~flags_to_clear;
    }
}

void USB_PRIV_handle_interrupt(void) {
    // Capture ISTR once so the callback receives a self-consistent event.
    uint16_t interrupt_status = USB->ISTR;
    PHAL_USB_Event_t event = {0};
    event.reset = (interrupt_status & USB_ISTR_RESET) != 0U;
    event.transfer = (interrupt_status & USB_ISTR_CTR) != 0U;

    if (event.transfer) {
        event.endpoint = (uint8_t)(interrupt_status & USB_ISTR_EP_ID);
        event.direction = (interrupt_status & USB_ISTR_DIR) != 0U
            ? PHAL_USB_ENDPOINT_DIRECTION_OUT : PHAL_USB_ENDPOINT_DIRECTION_IN;
        event.setup = event.direction == PHAL_USB_ENDPOINT_DIRECTION_OUT
            && USB_PRIV_endpoint_is_setup(event.endpoint);

        if (event.direction == PHAL_USB_ENDPOINT_DIRECTION_IN) {
            USB_PRIV_clear_endpoint_interrupt(event.endpoint, event.direction);
            usb_set_endpoint_status(event.endpoint,
                                    event.direction,
                                    USB_PRIV_ENDPOINT_STATUS_NAK);
        }
    }

    PHAL_USB_callback(&event);
    usb_clear_interrupts(interrupt_status);
}

uint16_t USB_PRIV_get_frame_number(void) {
    return USB->FNR & USB_FNR_FN;
}
