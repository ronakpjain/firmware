#ifndef TEST_FAKE_MCAN_H
#define TEST_FAKE_MCAN_H

#include <stdint.h>

#include "can_library/generated/can_types.h"

typedef struct {
    bool AMK_Control_bInverterOn;
    bool AMK_Control_bDcOn;
    bool AMK_Control_bEnable;
    bool AMK_Control_bErrorReset;
    int16_t AMK_TorqueSetpoint;
    int16_t AMK_PositiveTorqueLimit;
    int16_t AMK_NegativeTorqueLimit;
} INVA_SET_data_t;

typedef struct {
    uint8_t unused;
} INVA_CRIT_data_t;

typedef struct {
    bool AMK_Status_bSystemReady;
    bool AMK_Status_bError;
    bool AMK_Status_bQuitDcOn;
    bool AMK_Status_bQuitInverterOn;
} INVA_INFO_data_t;

typedef struct {
    uint8_t unused;
} INVA_TEMPS_data_t;

typedef struct {
    uint32_t AMK_DiagnosticNumber;
} INVA_ERR_1_data_t;

typedef struct {
    uint8_t unused;
} INVA_ERR_2_data_t;

#endif // TEST_FAKE_MCAN_H
