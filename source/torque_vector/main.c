/**
 * @file main.c
 * @brief "Torque Vector" node source code
 *
 * @author Irving Wang (irvingw@purdue.edu)
 */

#include "main.h"

/* System Includes */
#include "can_library/generated/TORQUE_VECTOR.h"
#include "common/freertos/freertos.h"
#include "common/heartbeat/heartbeat.h"
#include "common/phal/can.h"
#include "common/phal/gpio.h"
#include "common/phal/rcc.h"
#include "common/phal/usart.h"
#include "common/utils/countof.h"
#include "common/watchdog/watchdog.h"

/* Module Includes */
#include "control_loop.h"
#include "sensors.h"
#include "telemetry.h"

/* PER HAL Initialization Structures */
GPIOInitConfig_t gpio_config[] = {
    // Status LEDs
    GPIO_INIT_OUTPUT(HEARTBEAT_LED_PORT, HEARTBEAT_LED_PIN, GPIO_OUTPUT_LOW_SPEED),
    GPIO_INIT_OUTPUT(ERROR_LED_PORT, ERROR_LED_PIN, GPIO_OUTPUT_LOW_SPEED),
    GPIO_INIT_OUTPUT(CONNECTION_LED_PORT, CONNECTION_LED_PIN, GPIO_OUTPUT_LOW_SPEED),

    // VCAN
    GPIO_INIT_FDCAN2TX_PB13, // we fly swapped TX/RX
    GPIO_INIT_FDCAN2RX_PB12,

    // GCAN
    // ! these pin are erroneously swapped on the schematic
    // GPIO_INIT_FDCAN1TX_PA12,
    // GPIO_INIT_FDCAN1RX_PA11,

    // Rover GPS
    GPIO_INIT_USART3RX_PB11,
    GPIO_INIT_USART3TX_PB10,
    GPIO_INIT_OUTPUT(ROVER_RESET_PORT, ROVER_RESET_PIN, GPIO_OUTPUT_LOW_SPEED),

    // Base GPS
    GPIO_INIT_USART1TX_PA9,
    GPIO_INIT_USART1RX_PA10,
    GPIO_INIT_OUTPUT(BASE_RESET_PORT, BASE_RESET_PIN, GPIO_OUTPUT_LOW_SPEED),
};

static constexpr uint32_t TargetCoreClockrateHz = 16'000'000;
PHAL_RCC_Config_t clock_config = {
    .clock_source           = CLOCK_SOURCE_HSE,
    .use_pll                = false,
    .system_clock_target_hz = TargetCoreClockrateHz,
    .ahb_clock_target_hz    = TargetCoreClockrateHz,
    .apb1_clock_target_hz   = TargetCoreClockrateHz,
    .apb2_clock_target_hz   = TargetCoreClockrateHz,
    .hse_bypass             = true,
};

// USART Configuration for GPS
static constexpr uint32_t GPS_BAUD_RATE = 460'800;
PHAL_USART_Handle_t usart3;
static const PHAL_USART_Config_t usart3_config = {
    .instance     = USART3,
    .baud_rate    = GPS_BAUD_RATE,
    .word_length  = 8,
    .parity       = PHAL_USART_PARITY_NONE,
    .stop_bits    = PHAL_USART_STOP_BITS_1,
    .hardware_rts = false,
    .hardware_cts = false,
};

extern void HardFault_Handler(void);

void PHAL_USART_receiveCompleteCallback(
    PHAL_USART_Handle_t *handle,
    bool success,
    size_t received_length
) {
    (void)success;
    (void)received_length;
    if (handle == &usart3
        && !PHAL_USART_startIdleReceive(
            handle,
            (uint8_t *)rover_rx_buffer,
            sizeof(rover_rx_buffer))) {
        HardFault_Handler();
    }
}

// Thread Defines
DEFINE_CAN_TASKS();
DEFINE_TASK(control_loop, CONTROL_LOOP_PERIOD_MS, osPriorityNormal, STACK_4096);
DEFINE_TASK(gps_periodic, GPS_THREAD_PERIOD_MS, osPriorityLow, STACK_1024);
DEFINE_TASK(report_telemetry_100hz, TELEMETRY_100HZ_PERIOD_MS, osPriorityLow, STACK_512);
DEFINE_TASK(report_telemetry_1hz, TELEMETRY_1HZ_PERIOD_MS, osPriorityLow, STACK_512);
DEFINE_WATCHDOG_TASK();
DEFINE_HEARTBEAT_TASK(nullptr);

int main(void) {
    // Hardware Initialization
    if (!PHAL_RCC_configure(&clock_config)) {
        HardFault_Handler();
    }
    WDG_init();
    if (false == PHAL_initGPIO(gpio_config, countof(gpio_config))) {
        HardFault_Handler();
    }
    if (!PHAL_USART_init(&usart3, &usart3_config)) {
        HardFault_Handler();
    }
    if (!PHAL_USART_startIdleReceive(&usart3, (uint8_t *)rover_rx_buffer,
                                     sizeof(rover_rx_buffer))) {
        HardFault_Handler();
    }
    if (!PHAL_CAN_init(&PHAL_CAN2, &(PHAL_CAN_Config_t){
            .bit_rate = VCAN_BAUD_RATE, .loopback = false})) {
        HardFault_Handler();
    }
    if (!CAN_init()) {
        HardFault_Handler();
    }

    initialize_calibration();

    PHAL_GPIO_write(ROVER_RESET_PORT, ROVER_RESET_PIN, 1);
    PHAL_GPIO_write(BASE_RESET_PORT, BASE_RESET_PIN, 1);

    control_init();

    // Software Initialization
    osKernelInitialize();

    START_CAN_TASKS();
    CAN_SEND_tv_init(WDG_get_CSR());
    START_TASK(control_loop);
    START_TASK(gps_periodic);
    START_TASK(report_telemetry_100hz);
    START_TASK(report_telemetry_1hz);
    START_HEARTBEAT_TASK();
    START_WATCHDOG_TASK();

    // no way home
    osKernelStart();

    return 0;
}

void HardFault_Handler() {
    __disable_irq();
    SysTick->CTRL        = 0;
    ERROR_LED_PORT->BSRR = (1 << ERROR_LED_PIN);
    while (1) {
        __asm__("NOP"); // spin
    }
}

