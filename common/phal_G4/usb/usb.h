/**
 * @file usb.h
 * @brief STM32G4 USB full-speed device public API.
 *
 * Application-facing endpoint, packet, and USB event operations. Register and
 * packet-memory details are private to usb_priv.h.
 */

#ifndef PHAL_G4_USB_H
#define PHAL_G4_USB_H

#include <stdbool.h>
#include <stdint.h>

/** Number of bidirectional USB endpoint numbers implemented by the STM32G4. */
static constexpr uint8_t PHAL_USB_ENDPOINT_COUNT = 8U;

/** Largest packet size supported by a full-speed STM32G4 USB endpoint. */
static constexpr uint16_t PHAL_USB_MAX_PACKET_SIZE_BYTES = 1'023U;

/** USB endpoint transfer type. */
typedef enum : uint8_t {
    PHAL_USB_ENDPOINT_TYPE_CONTROL     = 0U, /**< Control transfer endpoint. */
    PHAL_USB_ENDPOINT_TYPE_BULK        = 1U, /**< Reliable, non-periodic endpoint. */
    PHAL_USB_ENDPOINT_TYPE_ISOCHRONOUS = 2U, /**< Time-sensitive endpoint. */
    PHAL_USB_ENDPOINT_TYPE_INTERRUPT   = 3U, /**< Periodic endpoint. */
} PHAL_USB_EndpointType_t;

/** USB packet direction as seen by the device. */
typedef enum : uint8_t {
    PHAL_USB_ENDPOINT_DIRECTION_OUT = 0U, /**< Host to device. */
    PHAL_USB_ENDPOINT_DIRECTION_IN  = 1U, /**< Device to host. */
} PHAL_USB_EndpointDirection_t;

/**
 * @brief One USB event delivered to the application.
 *
 * The callback runs in interrupt context. A reset event and a transfer event
 * may be reported together. For a transfer, endpoint identifies the endpoint
 * number, direction identifies host-to-device (OUT) or device-to-host (IN),
 * and setup identifies a SETUP packet on an OUT endpoint.
 */
typedef struct {
    uint8_t endpoint; /**< Endpoint number; meaningful when transfer is true. */
    PHAL_USB_EndpointDirection_t direction; /**< Transfer direction. */
    bool reset; /**< Bus reset is pending. */
    bool transfer; /**< An endpoint transfer is pending. */
    bool setup; /**< The OUT transfer is a SETUP packet. */
} PHAL_USB_Event_t;

/**
 * @brief Handle USB events in application code.
 *
 * This weak default does nothing. Define a strong function with this exact
 * signature in the application to receive USB events; no callback registration
 * is required.
 *
 * @param event Event description. The pointer is valid only during the call.
 */
void PHAL_USB_callback(const PHAL_USB_Event_t *event);

/**
 * @brief Endpoint configuration with automatic packet-memory allocation.
 *
 * The HAL allocates one packet-memory buffer for each enabled direction. The
 * requested size is both the buffer capacity and the endpoint's maximum
 * packet size. USB full-speed control, bulk, and interrupt endpoints accept
 * at most 64 bytes; isochronous endpoints may use the full device limit.
 * Endpoint zero must be configured before attaching the device.
 *
 * Configuring an endpoint again reuses its existing buffers. A later request
 * for a larger buffer is rejected rather than moving another endpoint's
 * buffer. Configure endpoints in increasing packet-memory order when using
 * different buffer sizes.
 */
typedef struct {
    uint16_t max_packet_size_bytes; /**< Maximum packet and buffer size. */
    uint8_t endpoint; /**< Endpoint number in the range [0, PHAL_USB_ENDPOINT_COUNT). */
    PHAL_USB_EndpointType_t type; /**< USB transfer type. */
    bool transmit_enabled; /**< Enable device-to-host (IN) packets. */
    bool receive_enabled; /**< Enable host-to-device (OUT) packets. */
} PHAL_USB_EndpointConfig_t;

/**
 * @brief Initialize the USB full-speed device peripheral.
 *
 * The peripheral starts disconnected. The HAL enables HSI48, configures the
 * fixed G4 USB pins and USB-C sink termination, resets the peripheral, and
 * enables its interrupt. Call PHAL_USB_configure_endpoint() for endpoint zero
 * before PHAL_USB_set_connected(true).
 *
 * The application receives events through the weak PHAL_USB_callback()
 * function. Override that function with a strong definition when events are
 * needed.
 *
 * @return true when initialized; false when already initialized or hardware
 *         initialization fails.
 */
bool PHAL_USB_init(void);

/**
 * @brief Deinitialize the USB peripheral and disconnect from the host.
 *
 * All endpoint configuration is discarded. PHAL_USB_init() must succeed again
 * before any other USB operation.
 *
 * @return true when deinitialized; false when the peripheral was not active.
 */
bool PHAL_USB_deinit(void);

/**
 * @brief Attach or detach the USB device by controlling D+ pull-up.
 *
 * Configure endpoint zero before attaching. Detaching does not erase endpoint
 * configuration; it remains available until PHAL_USB_deinit().
 *
 * @param connected true to attach, false to detach.
 * @return true when the request was applied; false when uninitialized.
 */
bool PHAL_USB_set_connected(bool connected);

/**
 * @brief Apply an endpoint configuration.
 *
 * @param config Configuration previously described by
 *        PHAL_USB_EndpointConfig_t.
 * @return true when applied; false for invalid configuration or state.
 */
bool PHAL_USB_configure_endpoint(const PHAL_USB_EndpointConfig_t *config);

/**
 * @brief Set the USB address assigned by the host.
 *
 * For SET_ADDRESS, call this after the request's status-stage IN transfer.
 *
 * @param address USB address in the range 0 through 127.
 * @return true when applied; false when uninitialized or out of range.
 */
bool PHAL_USB_set_address(uint8_t address);

/**
 * @brief Stall one endpoint direction.
 *
 * A stalled direction remains stalled until the application changes the USB
 * device state by reconfiguring the endpoint or handling the request. Use this
 * for unsupported control requests; normal packet flow is managed by read and
 * write.
 *
 * @param endpoint Endpoint number.
 * @param direction Direction to stall.
 * @return true when applied; false for an invalid or unconfigured endpoint.
 */
bool PHAL_USB_stall(uint8_t endpoint, PHAL_USB_EndpointDirection_t direction);

/**
 * @brief Queue one packet for transmission on an IN endpoint.
 *
 * The HAL copies the packet into packet memory and makes the endpoint valid.
 * The packet is one USB transaction; queue the next packet from the IN event.
 * A null source is valid only for a zero-length packet.
 *
 * @param endpoint Endpoint number.
 * @param source Packet data, or null for a zero-length packet.
 * @param length_bytes Number of bytes to transmit.
 * @return true when queued; false for invalid arguments, a full packet buffer,
 *         or an endpoint that is not ready.
 */
bool PHAL_USB_write(uint8_t endpoint, const void *source, uint16_t length_bytes);

/**
 * @brief Consume one packet from an OUT endpoint.
 *
 * The HAL copies the packet, clears its interrupt, and re-arms the endpoint on
 * success. If the destination is too small, the packet remains pending. A
 * zero-length packet may use a null destination with zero capacity.
 *
 * @param endpoint Endpoint number.
 * @param destination Destination buffer, or null when capacity is zero.
 * @param destination_capacity Destination capacity in bytes.
 * @param received_length Output packet length; must not be null.
 * @return true when copied and re-armed; false for invalid arguments or state.
 */
bool PHAL_USB_read(uint8_t endpoint,
                   void *destination,
                   uint16_t destination_capacity,
                   uint16_t *received_length);

/**
 * @brief Read the current USB frame number.
 *
 * @param frame_number Output 11-bit frame number; must not be null.
 * @return true when read; false when uninitialized or frame_number is null.
 */
bool PHAL_USB_get_frame_number(uint16_t *frame_number);

#endif // PHAL_G4_USB_H
