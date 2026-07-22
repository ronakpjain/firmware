# STM32G4 USB HAL usage

This HAL is for a USB full-speed device. The application supplies descriptors,
enumeration, control requests, and class handling. Include the architecture
wrapper:

```cpp
#include "common/phal/usb.h"
```

## 1. Define the USB event handler

Provide a strong definition of `PHAL_USB_callback()`. It runs in interrupt
context, so keep it short and do not block. `event->reset` reports a bus reset.
For transfers, use `endpoint`, `direction`, and `setup` to select the work:

```cpp
void PHAL_USB_callback(const PHAL_USB_Event_t *event) {
    if (event->reset) {
        g_configured = false;
        PHAL_USB_set_address(0U);
        configure_control_endpoint();
    }

    if (!event->transfer) {
        return;
    }

    if (event->endpoint == 0U
        && event->direction == PHAL_USB_ENDPOINT_DIRECTION_OUT) {
        if (event->setup) {
            handle_setup_packet();
        } else {
            handle_control_status_packet();
        }
        return;
    }

    if (event->endpoint == 1U
        && event->direction == PHAL_USB_ENDPOINT_DIRECTION_OUT) {
        handle_bulk_out_packet();
    }
}
```

`PHAL_USB_ENDPOINT_DIRECTION_OUT` means host-to-device and
`PHAL_USB_ENDPOINT_DIRECTION_IN` means device-to-host. An IN event has already
finished, so another IN packet may be queued from the callback.

## 2. Initialize and configure endpoints

Initialize while disconnected. Configure endpoint zero before attaching:

```cpp
static const PHAL_USB_EndpointConfig_t endpoint0 = {
    .max_packet_size_bytes = 64U,
    .endpoint = 0U,
    .type = PHAL_USB_ENDPOINT_TYPE_CONTROL,
    .transmit_enabled = true,
    .receive_enabled = true,
};

if (!PHAL_USB_init()
    || !PHAL_USB_configure_endpoint(&endpoint0)
    || !PHAL_USB_set_connected(true)) {
    handle_usb_initialization_error();
}
```

Configure additional endpoints when handling `SET_CONFIGURATION`. Endpoint
numbers are bidirectional: endpoint 1 is `0x01` for OUT and `0x81` for IN.

```cpp
static const PHAL_USB_EndpointConfig_t endpoint1 = {
    .max_packet_size_bytes = 64U,
    .endpoint = 1U,
    .type = PHAL_USB_ENDPOINT_TYPE_BULK,
    .transmit_enabled = true,
    .receive_enabled = true,
};

PHAL_USB_configure_endpoint(&endpoint1);
```

Use 64 bytes for control, bulk, and interrupt endpoints. Isochronous endpoints
may use a larger full-speed packet size when the application needs it.

## 3. Receive an OUT packet

Call `PHAL_USB_read()` from the OUT event. The packet is consumed only when the
call succeeds. A zero-length packet uses a null destination and zero capacity:

```cpp
static uint8_t receive_buffer[64];

static void handle_bulk_out_packet(void) {
    uint16_t received_length = 0U;
    if (PHAL_USB_read(1U,
                      receive_buffer,
                      sizeof(receive_buffer),
                      &received_length)) {
        process_packet(receive_buffer, received_length);
    }
}
```

If the destination is too small, the packet remains pending. Increase the
buffer or handle the error before returning from the callback.

## 4. Send an IN packet

Call `PHAL_USB_write()` with the endpoint number and one packet of data. Use a
null source only for a zero-length packet:

```cpp
static const uint8_t response[] = {0x01U, 0x02U, 0x03U};

if (!PHAL_USB_write(1U, response, sizeof(response))) {
    handle_usb_transmit_error();
}
```

Queue the next packet after the corresponding IN event. Do not manually clear
endpoint interrupts or change endpoint status for normal transfers.

## 5. Handle control requests

Use endpoint zero for enumeration and control transfers. Read the eight-byte
SETUP packet in the endpoint-zero OUT event, then send a response with
`PHAL_USB_write(0U, ...)`. Send a control status packet with:

```cpp
PHAL_USB_write(0U, nullptr, 0U);
```

For unsupported requests, stall the affected direction:

```cpp
PHAL_USB_stall(0U, PHAL_USB_ENDPOINT_DIRECTION_IN);
PHAL_USB_stall(0U, PHAL_USB_ENDPOINT_DIRECTION_OUT);
```

Apply a pending `SET_ADDRESS` after its status-stage IN event:

```cpp
if (pending_address) {
    PHAL_USB_set_address(address);
    pending_address = false;
}
```

## 6. Disconnect and shut down

```cpp
PHAL_USB_set_connected(false);
PHAL_USB_deinit();
```

Check every boolean return from initialization, endpoint configuration, and
transfers. `PHAL_USB_get_frame_number(&frame_number)` can be used when a USB
frame counter is needed.
