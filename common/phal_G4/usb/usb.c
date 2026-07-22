/**
 * @file usb.c
 * @brief STM32G4 USB public-layer implementation.
 *
 * This layer validates application requests, allocates packet buffers, and
 * translates simple USB operations into private register-level operations.
 */

#include "common/phal_G4/usb/usb.h"

#include "common/phal_G4/usb/usb_priv.h"

// Eight endpoint descriptors occupy the first 64 bytes of the 1 KiB PMA.
static constexpr uint16_t USB_PMA_BUFFER_START_BYTES = PHAL_USB_ENDPOINT_COUNT * 8U;
static constexpr uint16_t USB_PMA_SIZE_BYTES = 1'024U;

/** State retained by the public layer; PMA addresses never leave this file. */
typedef struct {
    uint16_t transmit_address; /**< Private PMA address for IN packets. */
    uint16_t receive_address; /**< Private PMA address for OUT packets. */
    uint16_t transmit_capacity_bytes; /**< Allocated IN capacity. */
    uint16_t receive_capacity_bytes; /**< Allocated OUT capacity. */
    bool transmit_allocated;
    bool receive_allocated;
    bool configured;
    bool transmit_enabled;
    bool receive_enabled;
} USB_PRIV_EndpointState_t;

static bool g_usb_initialized = false;
static uint16_t g_usb_next_pma_address = USB_PMA_BUFFER_START_BYTES;
static USB_PRIV_EndpointState_t g_usb_endpoints[PHAL_USB_ENDPOINT_COUNT] = {0};

/** Validate an endpoint number before indexing the per-endpoint state. */
static bool usb_endpoint_is_valid(uint8_t endpoint) {
    return endpoint < PHAL_USB_ENDPOINT_COUNT;
}

static bool usb_endpoint_type_is_valid(PHAL_USB_EndpointType_t type) {
    return type <= PHAL_USB_ENDPOINT_TYPE_INTERRUPT;
}

static bool usb_endpoint_direction_is_valid(PHAL_USB_EndpointDirection_t direction) {
    return direction <= PHAL_USB_ENDPOINT_DIRECTION_IN;
}

/**
 * Validate the packet sizes supported by full-speed USB endpoint types.
 *
 * Control and bulk endpoints have discrete legal sizes. Interrupt and
 * isochronous sizes are bounded by the full-speed maximum.
 */
static bool usb_packet_size_is_valid(PHAL_USB_EndpointType_t type, uint16_t size_bytes) {
    if (size_bytes == 0U || size_bytes > PHAL_USB_MAX_PACKET_SIZE_BYTES) {
        return false;
    }

    if (type == PHAL_USB_ENDPOINT_TYPE_ISOCHRONOUS
        || type == PHAL_USB_ENDPOINT_TYPE_INTERRUPT) {
        return true;
    }

    return size_bytes == 8U || size_bytes == 16U || size_bytes == 32U || size_bytes == 64U;
}

/** PMA buffers must start on an even byte address. */
static uint16_t usb_align_buffer_size(uint16_t size_bytes) {
    return (uint16_t)((size_bytes + 1U) & ~1U);
}

/** Allocate the next sequential PMA range without exposing its address. */
static bool usb_allocate_buffer(uint16_t size_bytes, uint16_t *address) {
    uint16_t aligned_size = usb_align_buffer_size(size_bytes);
    if (address == nullptr || aligned_size > USB_PMA_SIZE_BYTES - g_usb_next_pma_address) {
        return false;
    }

    *address = g_usb_next_pma_address;
    g_usb_next_pma_address = (uint16_t)(g_usb_next_pma_address + aligned_size);
    return true;
}

/**
 * Reuse an existing endpoint buffer or reserve a new one.
 *
 * Allocation is staged until the complete endpoint configuration succeeds so
 * a failed transmit/receive pair does not consume PMA space.
 */
static bool usb_prepare_buffer(USB_PRIV_EndpointState_t *state,
                               bool transmit,
                               uint16_t requested_size,
                               uint16_t *next_address) {
    bool allocated = transmit ? state->transmit_allocated : state->receive_allocated;
    uint16_t capacity = transmit ? state->transmit_capacity_bytes : state->receive_capacity_bytes;
    if (allocated) {
        return requested_size <= capacity;
    }

    if (!usb_allocate_buffer(requested_size, next_address)) {
        return false;
    }
    return true;
}

/** Commit a staged PMA allocation to the endpoint's private state. */
static void usb_commit_buffer(USB_PRIV_EndpointState_t *state,
                              bool transmit,
                              uint16_t requested_size,
                              uint16_t address) {
    if (transmit) {
        state->transmit_address = address;
        state->transmit_capacity_bytes = requested_size;
        state->transmit_allocated = true;
        return;
    }

    state->receive_address = address;
    state->receive_capacity_bytes = requested_size;
    state->receive_allocated = true;
}

/**
 * Initialize the hardware layer first, then publish initialized state.
 *
 * Publishing the state only after hardware setup succeeds prevents callbacks
 * or application calls from observing a partially initialized peripheral.
 */
bool PHAL_USB_init(void) {
    if (g_usb_initialized || !USB_PRIV_initialize_hardware()) {
        return false;
    }

    g_usb_next_pma_address = USB_PMA_BUFFER_START_BYTES;
    for (uint8_t endpoint = 0U; endpoint < PHAL_USB_ENDPOINT_COUNT; endpoint++) {
        g_usb_endpoints[endpoint] = (USB_PRIV_EndpointState_t){0};
    }

    g_usb_initialized = true;
    USB_PRIV_enable_interrupts();
    return true;
}

bool PHAL_USB_deinit(void) {
    if (!g_usb_initialized) {
        return false;
    }

    USB_PRIV_set_connected(false);
    USB_PRIV_disable_interrupts();
    USB_PRIV_deinitialize_hardware();
    g_usb_initialized = false;
    return true;
}

bool PHAL_USB_set_connected(bool connected) {
    if (!g_usb_initialized) {
        return false;
    }

    USB_PRIV_set_connected(connected);
    return true;
}

bool PHAL_USB_configure_endpoint(const PHAL_USB_EndpointConfig_t *config) {
    // Validate before allocating so invalid application configuration has no
    // side effects on the endpoint state or PMA allocator.
    if (!g_usb_initialized || config == nullptr || !usb_endpoint_is_valid(config->endpoint)
        || !usb_endpoint_type_is_valid(config->type)
        || (!config->transmit_enabled && !config->receive_enabled)
        || !usb_packet_size_is_valid(config->type, config->max_packet_size_bytes)) {
        return false;
    }

    USB_PRIV_EndpointState_t *state = &g_usb_endpoints[config->endpoint];
    uint16_t next_transmit_address = state->transmit_address;
    uint16_t next_receive_address = state->receive_address;
    uint16_t saved_next_address = g_usb_next_pma_address;

    if (config->transmit_enabled
        && !usb_prepare_buffer(state,
                               true,
                               config->max_packet_size_bytes,
                               &next_transmit_address)) {
        g_usb_next_pma_address = saved_next_address;
        return false;
    }
    if (config->receive_enabled
        && !usb_prepare_buffer(state,
                               false,
                               config->max_packet_size_bytes,
                               &next_receive_address)) {
        g_usb_next_pma_address = saved_next_address;
        return false;
    }

    if (!USB_PRIV_configure_buffer_descriptors(config->endpoint,
                                               config->transmit_enabled,
                                               next_transmit_address,
                                               config->receive_enabled,
                                               next_receive_address,
                                               config->max_packet_size_bytes)
        || !USB_PRIV_configure_endpoint(config->endpoint,
                                        config->type,
                                        config->transmit_enabled,
                                        config->receive_enabled)) {
        g_usb_next_pma_address = saved_next_address;
        return false;
    }

    if (config->transmit_enabled && !state->transmit_allocated) {
        usb_commit_buffer(state, true, config->max_packet_size_bytes, next_transmit_address);
    }
    if (config->receive_enabled && !state->receive_allocated) {
        usb_commit_buffer(state, false, config->max_packet_size_bytes, next_receive_address);
    }
    state->transmit_enabled = config->transmit_enabled;
    state->receive_enabled = config->receive_enabled;
    state->configured = true;
    return true;
}

bool PHAL_USB_set_address(uint8_t address) {
    return g_usb_initialized && USB_PRIV_set_address(address);
}

bool PHAL_USB_stall(uint8_t endpoint, PHAL_USB_EndpointDirection_t direction) {
    if (!g_usb_initialized || !usb_endpoint_is_valid(endpoint)
        || !usb_endpoint_direction_is_valid(direction)) {
        return false;
    }

    USB_PRIV_EndpointState_t *state = &g_usb_endpoints[endpoint];
    bool enabled = direction == PHAL_USB_ENDPOINT_DIRECTION_IN
        ? state->transmit_enabled : state->receive_enabled;
    if (!state->configured || !enabled) {
        return false;
    }

    return USB_PRIV_stall_endpoint(endpoint, direction);
}

bool PHAL_USB_write(uint8_t endpoint, const void *source, uint16_t length_bytes) {
    // IN transfers are accepted only while the endpoint is NAK/ready. This
    // prevents overwriting a packet that is still owned by USB hardware.
    if (!g_usb_initialized || !usb_endpoint_is_valid(endpoint)
        || (source == nullptr && length_bytes != 0U)) {
        return false;
    }

    USB_PRIV_EndpointState_t *state = &g_usb_endpoints[endpoint];
    if (!state->configured || !state->transmit_enabled
        || length_bytes > state->transmit_capacity_bytes
        || !USB_PRIV_endpoint_is_ready(endpoint, PHAL_USB_ENDPOINT_DIRECTION_IN)) {
        return false;
    }

    USB_PRIV_clear_endpoint_interrupt(endpoint, PHAL_USB_ENDPOINT_DIRECTION_IN);
    USB_PRIV_write_pma(state->transmit_address, source, length_bytes);
    USB_PRIV_set_transmit_length(endpoint, length_bytes);
    return USB_PRIV_set_endpoint_valid(endpoint, PHAL_USB_ENDPOINT_DIRECTION_IN);
}

bool PHAL_USB_read(uint8_t endpoint,
                   void *destination,
                   uint16_t destination_capacity,
                   uint16_t *received_length) {
    // Leave an oversized packet pending so the application can retry safely.
    if (!g_usb_initialized || !usb_endpoint_is_valid(endpoint) || received_length == nullptr
        || (destination == nullptr && destination_capacity != 0U)) {
        return false;
    }

    USB_PRIV_EndpointState_t *state = &g_usb_endpoints[endpoint];
    if (!state->configured || !state->receive_enabled) {
        return false;
    }

    uint16_t length_bytes = USB_PRIV_read_received_length(endpoint);
    *received_length = length_bytes;
    if (length_bytes > destination_capacity) {
        return false;
    }

    USB_PRIV_read_pma(state->receive_address, destination, length_bytes);
    if (!USB_PRIV_clear_endpoint_interrupt(endpoint, PHAL_USB_ENDPOINT_DIRECTION_OUT)) {
        return false;
    }
    USB_PRIV_rearm_receive_buffer(endpoint);
    return USB_PRIV_set_endpoint_valid(endpoint, PHAL_USB_ENDPOINT_DIRECTION_OUT);
}

bool PHAL_USB_get_frame_number(uint16_t *frame_number) {
    if (!g_usb_initialized || frame_number == nullptr) {
        return false;
    }

    *frame_number = USB_PRIV_get_frame_number();
    return true;
}

[[gnu::weak]]
void PHAL_USB_callback(const PHAL_USB_Event_t *event) {
    (void)event;
}

void USB_LP_IRQHandler(void) {
    USB_PRIV_handle_interrupt();
}

void USB_HP_IRQHandler(void) {
    USB_PRIV_handle_interrupt();
}
