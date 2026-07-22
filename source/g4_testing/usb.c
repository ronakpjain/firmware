#include "g4_testing.h"
#if (G4_TESTING_CHOSEN == TEST_USB)

#include <stdint.h>

#include "common/phal/gpio.h"
#include "common/phal/rcc.h"
#include "common/phal/usb.h"
#include "main.h"

static constexpr uint32_t kTargetCoreClockrateHz = 16'000'000;
static constexpr uint8_t kUsbConfigurationValue = 1U;
static constexpr uint8_t kUsbRequestGetStatus = 0U;
static constexpr uint8_t kUsbRequestSetAddress = 5U;
static constexpr uint8_t kUsbRequestGetDescriptor = 6U;
static constexpr uint8_t kUsbRequestGetConfiguration = 8U;
static constexpr uint8_t kUsbRequestSetConfiguration = 9U;
static constexpr uint8_t kUsbDescriptorDevice = 1U;
static constexpr uint8_t kUsbDescriptorConfiguration = 2U;
static constexpr uint8_t kUsbRequestDirectionIn = 0x80U;
static constexpr uint8_t kUsbRequestTypeMask = 0x60U;
static constexpr uint8_t kUsbRequestStandard = 0U;
static constexpr uint16_t kUsbPacketBufferSize = 64U;

static ClockRateConfig_t g_clock_config = {
    .clock_source = CLOCK_SOURCE_HSI,
    .use_pll = false,
    .system_clock_target_hz = kTargetCoreClockrateHz,
    .ahb_clock_target_hz = kTargetCoreClockrateHz,
    .apb1_clock_target_hz = kTargetCoreClockrateHz,
    .apb2_clock_target_hz = kTargetCoreClockrateHz,
};

static const uint8_t g_device_descriptor[] = {
    18U, 1U, 0x00U, 0x02U, 0xFFU, 0x00U, 0x00U, 64U,
    0x09U, 0x12U, 0x03U, 0x00U, 0x01U, 0x00U, 1U, 2U, 0U, 1U,
};

static const uint8_t g_language_descriptor[] = {4U, 3U, 0x09U, 0x04U};
static const uint8_t g_manufacturer_descriptor[] = {
    8U, 3U, 'P', 0U, 'E', 0U, 'R', 0U,
};
static const uint8_t g_product_descriptor[] = {
    34U, 3U, 'S', 0U, 'T', 0U, 'M', 0U, '3', 0U, '2', 0U, 'G', 0U,
    '4', 0U, ' ', 0U, 'U', 0U, 'S', 0U, 'B', 0U, ' ', 0U, 'T', 0U,
    'e', 0U, 's', 0U, 't', 0U,
};

static const uint8_t g_configuration_descriptor[] = {
    9U, 2U, 32U, 0U, 1U, 1U, 0U, 0x80U, 50U,
    9U, 4U, 0U, 0U, 2U, 0xFFU, 0U, 0U, 0U,
    7U, 5U, 0x01U, 2U, 64U, 0U, 0U,
    7U, 5U, 0x81U, 2U, 64U, 0U, 0U,
};

static uint8_t g_setup_packet[8] = {0};
static uint8_t g_bulk_buffer[kUsbPacketBufferSize] = {0};
static uint8_t g_pending_address = 0U;
static bool g_address_pending = false;
static bool g_configured = false;

static void usb_stall_control_endpoint(void) {
    PHAL_USB_stall(0U, PHAL_USB_ENDPOINT_DIRECTION_IN);
    PHAL_USB_stall(0U, PHAL_USB_ENDPOINT_DIRECTION_OUT);
}

static void usb_configure_control_endpoint(void) {
    static const PHAL_USB_EndpointConfig_t endpoint0 = {
        .max_packet_size_bytes = kUsbPacketBufferSize,
        .endpoint = 0U,
        .type = PHAL_USB_ENDPOINT_TYPE_CONTROL,
        .transmit_enabled = true,
        .receive_enabled = true,
    };
    PHAL_USB_configure_endpoint(&endpoint0);
}

static void usb_configure_bulk_endpoint(void) {
    static const PHAL_USB_EndpointConfig_t endpoint1 = {
        .max_packet_size_bytes = kUsbPacketBufferSize,
        .endpoint = 1U,
        .type = PHAL_USB_ENDPOINT_TYPE_BULK,
        .transmit_enabled = true,
        .receive_enabled = true,
    };
    PHAL_USB_configure_endpoint(&endpoint1);
}

static uint16_t usb_minimum_length(uint16_t requested_length, uint16_t available_length) {
    return requested_length < available_length ? requested_length : available_length;
}

static void usb_send_control_data(const uint8_t *data, uint16_t data_length, uint16_t requested_length) {
    uint16_t length = usb_minimum_length(data_length, requested_length);
    PHAL_USB_write(0U, data, length);
}

static void usb_send_control_status(void) {
    PHAL_USB_write(0U, nullptr, 0U);
}

static void usb_handle_setup_packet(void) {
    uint8_t request_type = g_setup_packet[0];
    uint8_t request = g_setup_packet[1];
    uint16_t value = (uint16_t)g_setup_packet[2] | ((uint16_t)g_setup_packet[3] << 8U);
    uint16_t requested_length = (uint16_t)g_setup_packet[6]
        | ((uint16_t)g_setup_packet[7] << 8U);

    if ((request_type & kUsbRequestTypeMask) != kUsbRequestStandard) {
        usb_stall_control_endpoint();
        return;
    }

    switch (request) {
        case kUsbRequestGetStatus: {
            static const uint8_t status[] = {0U, 0U};
            usb_send_control_data(status, sizeof(status), requested_length);
            break;
        }
        case kUsbRequestGetDescriptor: {
            uint8_t descriptor_type = (uint8_t)(value >> 8U);
            if ((request_type & kUsbRequestDirectionIn) == 0U) {
                usb_stall_control_endpoint();
                break;
            }
            if (descriptor_type == kUsbDescriptorDevice) {
                usb_send_control_data(g_device_descriptor, sizeof(g_device_descriptor), requested_length);
            } else if (descriptor_type == kUsbDescriptorConfiguration) {
                usb_send_control_data(g_configuration_descriptor,
                                      sizeof(g_configuration_descriptor),
                                      requested_length);
            } else if (descriptor_type == 3U) {
                const uint8_t *descriptor = nullptr;
                uint16_t descriptor_length = 0U;
                switch ((uint8_t)value) {
                    case 0U:
                        descriptor = g_language_descriptor;
                        descriptor_length = sizeof(g_language_descriptor);
                        break;
                    case 1U:
                        descriptor = g_manufacturer_descriptor;
                        descriptor_length = sizeof(g_manufacturer_descriptor);
                        break;
                    case 2U:
                        descriptor = g_product_descriptor;
                        descriptor_length = sizeof(g_product_descriptor);
                        break;
                    default:
                        break;
                }
                if (descriptor == nullptr) {
                    usb_stall_control_endpoint();
                } else {
                    usb_send_control_data(descriptor, descriptor_length, requested_length);
                }
            } else {
                usb_stall_control_endpoint();
            }
            break;
        }
        case kUsbRequestSetAddress:
            g_pending_address = (uint8_t)value;
            g_address_pending = true;
            usb_send_control_status();
            break;
        case kUsbRequestGetConfiguration: {
            static uint8_t configuration = 0U;
            configuration = g_configured ? kUsbConfigurationValue : 0U;
            usb_send_control_data(&configuration, sizeof(configuration), requested_length);
            break;
        }
        case kUsbRequestSetConfiguration:
            if ((uint8_t)value == kUsbConfigurationValue) {
                g_configured = true;
                usb_configure_bulk_endpoint();
                usb_send_control_status();
            } else if (value == 0U) {
                g_configured = false;
                usb_send_control_status();
            } else {
                usb_stall_control_endpoint();
            }
            break;
        default:
            usb_stall_control_endpoint();
            break;
    }
}

void PHAL_USB_callback(const PHAL_USB_Event_t *event) {
    if (event->reset) {
        g_configured = false;
        g_address_pending = false;
        PHAL_USB_set_address(0U);
        usb_configure_control_endpoint();
    }

    if (!event->transfer) {
        return;
    }

    if (event->endpoint == 0U && event->direction == PHAL_USB_ENDPOINT_DIRECTION_OUT) {
        uint16_t received_length = 0U;
        if (!event->setup) {
            /* A control-IN transfer ends with a zero-length OUT status packet. */
            PHAL_USB_read(0U, nullptr, 0U, &received_length);
            return;
        }
        if (!PHAL_USB_read(0U, g_setup_packet, sizeof(g_setup_packet), &received_length)
            || received_length != sizeof(g_setup_packet)) {
            usb_stall_control_endpoint();
            return;
        }
        usb_handle_setup_packet();
        return;
    }

    if (event->endpoint == 0U && event->direction == PHAL_USB_ENDPOINT_DIRECTION_IN) {
        if (g_address_pending) {
            PHAL_USB_set_address(g_pending_address);
            g_address_pending = false;
        }
        return;
    }

    if (event->endpoint == 1U && g_configured
        && event->direction == PHAL_USB_ENDPOINT_DIRECTION_OUT) {
        uint16_t received_length = 0U;
        if (PHAL_USB_read(1U, g_bulk_buffer, sizeof(g_bulk_buffer), &received_length)) {
            PHAL_USB_write(1U, g_bulk_buffer, received_length);
        }
    }
}

void HardFault_Handler(void);

int main(void) {
    GPIOInitConfig_t gpio_config[] = {
        GPIO_INIT_OUTPUT(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_OUTPUT_LOW_SPEED),
        GPIO_INIT_OUTPUT(LED_RED_PORT, LED_RED_PIN, GPIO_OUTPUT_LOW_SPEED),
    };

    if (PHAL_configureClockRates(&g_clock_config)
        || !PHAL_initGPIO(gpio_config, 2U)
        || !PHAL_USB_init()) {
        PHAL_writeGPIO(LED_GREEN_PORT, LED_GREEN_PIN, false);
        PHAL_writeGPIO(LED_RED_PORT, LED_RED_PIN, true);
        HardFault_Handler();
    }

    usb_configure_control_endpoint();
    PHAL_USB_set_connected(true);

    PHAL_writeGPIO(LED_GREEN_PORT, LED_GREEN_PIN, true);
    PHAL_writeGPIO(LED_RED_PORT, LED_RED_PIN, false);

    while (true) {
        __WFI();
    }
}

void HardFault_Handler(void) {
    while (true) {
        __NOP();
    }
}

#endif // G4_TESTING_CHOSEN == TEST_USB
