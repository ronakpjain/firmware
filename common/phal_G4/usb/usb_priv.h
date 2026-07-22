/**
 * @file usb_priv.h
 * @brief STM32G4 USB register-level implementation interface.
 *
 * This interface is for usb.c and usb_priv.c only. Public code must use
 * usb.h; PMA addresses, endpoint status encodings, and USB register values do
 * not cross the public/private boundary.
 */

#ifndef PHAL_G4_USB_PRIV_H
#define PHAL_G4_USB_PRIV_H

#include <stdbool.h>
#include <stdint.h>

#include "common/phal_G4/usb/usb.h"

/**
 * @brief Initialize USB clocks, pins, analog power, registers, and PMA state.
 * @return true when HSI48 starts and hardware initialization completes.
 */
bool USB_PRIV_initialize_hardware(void);

/** @brief Put the USB peripheral into its powered-down state. */
void USB_PRIV_deinitialize_hardware(void);

/** @brief Enable the USB interrupt sources and low-priority NVIC line. */
void USB_PRIV_enable_interrupts(void);

/** @brief Disable the USB interrupt sources and low-priority NVIC line. */
void USB_PRIV_disable_interrupts(void);

/**
 * @brief Set the USB D+ pull-up state.
 * @param connected true to attach to the host, false to detach.
 */
void USB_PRIV_set_connected(bool connected);

/**
 * @brief Write a validated device address to the USB address register.
 * @param address USB address in the range accepted by the peripheral.
 * @return true when written; false when address is out of range.
 */
bool USB_PRIV_set_address(uint8_t address);

/**
 * @brief Decode and service one USB interrupt.
 *
 * This function acknowledges IN transfers before invoking PHAL_USB_callback so
 * the application can queue the next packet from its callback. OUT and SETUP
 * packets remain pending until PHAL_USB_read() consumes them.
 */
void USB_PRIV_handle_interrupt(void);

/**
 * @brief Program an endpoint's type and enabled directions.
 *
 * The arguments are already validated by the public layer. Endpoint STAT
 * values are hardware encodings and must not be used outside this file.
 *
 * @param endpoint Endpoint number.
 * @param type USB transfer type.
 * @param transmit_enabled Enable the IN direction.
 * @param receive_enabled Enable the OUT direction.
 * @return true when the endpoint register is programmed.
 */
bool USB_PRIV_configure_endpoint(uint8_t endpoint,
                                 PHAL_USB_EndpointType_t type,
                                 bool transmit_enabled,
                                 bool receive_enabled);

/**
 * @brief Program one endpoint's PMA buffer descriptor table.
 *
 * The public layer allocates and validates the addresses. This function only
 * translates the receive capacity into the STM32 COUNT_RX encoding and writes
 * the four descriptor words.
 *
 * @param endpoint Endpoint number.
 * @param transmit_enabled Enable the IN descriptor.
 * @param transmit_buffer_address PMA address for the IN buffer.
 * @param receive_enabled Enable the OUT descriptor.
 * @param receive_buffer_address PMA address for the OUT buffer.
 * @param receive_buffer_size OUT buffer capacity in bytes.
 * @return true when the descriptor encoding is valid and written.
 */
bool USB_PRIV_configure_buffer_descriptors(uint8_t endpoint,
                                           bool transmit_enabled,
                                           uint16_t transmit_buffer_address,
                                           bool receive_enabled,
                                           uint16_t receive_buffer_address,
                                           uint16_t receive_buffer_size);

/**
 * @brief Check whether a direction is ready for a new transfer.
 * @param endpoint Endpoint number.
 * @param direction IN or OUT direction.
 * @return true when the direction is in the internal ready/NAK state.
 */
bool USB_PRIV_endpoint_is_ready(uint8_t endpoint, PHAL_USB_EndpointDirection_t direction);

/**
 * @brief Stall one validated endpoint direction.
 * @param endpoint Endpoint number.
 * @param direction IN or OUT direction.
 * @return true when the status encoding is written.
 */
bool USB_PRIV_stall_endpoint(uint8_t endpoint, PHAL_USB_EndpointDirection_t direction);

/**
 * @brief Mark one endpoint direction valid for the next transfer.
 * @param endpoint Endpoint number.
 * @param direction IN or OUT direction.
 * @return true when the status encoding is written.
 */
bool USB_PRIV_set_endpoint_valid(uint8_t endpoint, PHAL_USB_EndpointDirection_t direction);

/**
 * @brief Clear an endpoint correct-transfer flag.
 * @param endpoint Endpoint number.
 * @param direction IN or OUT direction.
 * @return true after the register write.
 */
bool USB_PRIV_clear_endpoint_interrupt(uint8_t endpoint,
                                       PHAL_USB_EndpointDirection_t direction);

/**
 * @brief Check the endpoint SETUP bit.
 * @param endpoint Endpoint number.
 * @return true when the pending OUT transaction is a SETUP transaction.
 */
bool USB_PRIV_endpoint_is_setup(uint8_t endpoint);

/**
 * @brief Read the received byte count from an endpoint's PMA descriptor.
 * @param endpoint Endpoint number.
 * @return Number of bytes in the pending OUT packet.
 */
uint16_t USB_PRIV_read_received_length(uint8_t endpoint);

/**
 * @brief Set the transmit byte count in an endpoint's PMA descriptor.
 * @param endpoint Endpoint number.
 * @param length_bytes Number of bytes to transmit.
 */
void USB_PRIV_set_transmit_length(uint8_t endpoint, uint16_t length_bytes);

/**
 * @brief Restore an OUT buffer's configured receive capacity.
 * @param endpoint Endpoint number.
 */
void USB_PRIV_rearm_receive_buffer(uint8_t endpoint);

/**
 * @brief Copy bytes from a CPU buffer into USB packet memory.
 * @param address PMA byte address.
 * @param source Source buffer.
 * @param length_bytes Number of bytes to copy.
 */
void USB_PRIV_write_pma(uint16_t address, const void *source, uint16_t length_bytes);

/**
 * @brief Copy bytes from USB packet memory into a CPU buffer.
 * @param address PMA byte address.
 * @param destination Destination buffer.
 * @param length_bytes Number of bytes to copy.
 */
void USB_PRIV_read_pma(uint16_t address, void *destination, uint16_t length_bytes);

/**
 * @brief Read the current USB frame counter.
 * @return Current 11-bit frame number.
 */
uint16_t USB_PRIV_get_frame_number(void);

#endif // PHAL_G4_USB_PRIV_H
