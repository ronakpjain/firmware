#ifndef TEST_FAKE_DASHBOARD_H
#define TEST_FAKE_DASHBOARD_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t last_rx;
    bool (*is_stale)(void);
    uint8_t throttle;
    uint8_t regen;
    uint8_t brake;
} pedals_data_t;

void CAN_SEND_pedals(uint8_t throttle, uint8_t regen, uint8_t brake);

#endif // TEST_FAKE_DASHBOARD_H
