#include <gtest/gtest.h>

#include "dashboard_pedals_test_runtime.h"

namespace {

class DashboardPedalsTest : public testing::Test {
protected:
    void SetUp() override {
        test_dashboard_reset();
    }
};

TEST_F(DashboardPedalsTest, RescalesCalibratedInputsAndTransmitsThem) {
    test_dashboard_set_raw_adc(455, 700, 875, 0, 2775, 0);

    test_dashboard_run_pedals();

    const pedals_data_t values = test_dashboard_get_pedal_values();
    EXPECT_EQ(values.throttle, 50);
    EXPECT_EQ(values.regen, 50);
    EXPECT_EQ(values.brake, 50);

    const test_pedal_tx_t tx = test_dashboard_get_last_pedal_tx();
    EXPECT_EQ(tx.throttle, 50);
    EXPECT_EQ(tx.regen, 50);
    EXPECT_EQ(tx.brake, 50);
    EXPECT_EQ(tx.send_count, 1U);

    EXPECT_FLOAT_EQ(test_fault_last_value(FAULT_ID_APPS_WIRING_T1), 455.0F);
    EXPECT_FLOAT_EQ(test_fault_last_value(FAULT_ID_APPS_WIRING_T2), 3395.0F);
    EXPECT_FLOAT_EQ(test_fault_last_value(FAULT_ID_BSE), 875.0F);
    EXPECT_FLOAT_EQ(test_fault_last_value(FAULT_ID_APPS_IMPLAUSIBLE), 0.0F);
    EXPECT_FLOAT_EQ(test_fault_last_value(FAULT_ID_APPS_BRAKE), 1.0F);
}

TEST_F(DashboardPedalsTest, ClampsInputsOutsideCalibrationRanges) {
    test_dashboard_set_raw_adc(0, 0, 0, 0, 4000, 0);

    test_dashboard_run_pedals();

    const pedals_data_t values = test_dashboard_get_pedal_values();
    EXPECT_EQ(values.throttle, 0);
    EXPECT_EQ(values.regen, 100);
    EXPECT_EQ(values.brake, 0);
    EXPECT_FLOAT_EQ(test_fault_last_value(FAULT_ID_APPS_IMPLAUSIBLE), 100.0F);
}

TEST_F(DashboardPedalsTest, SuppressesThrottleWhenAnySafetyFaultIsLatched) {
    test_dashboard_set_raw_adc(455, 700, 875, 0, 2775, 0);
    test_fault_set_latched(FAULT_ID_APPS_IMPLAUSIBLE, true);

    test_dashboard_run_pedals();

    EXPECT_EQ(test_dashboard_get_pedal_values().throttle, 50);
    EXPECT_EQ(test_dashboard_get_last_pedal_tx().throttle, 0);
}

TEST_F(DashboardPedalsTest, RequestsBrakeOverlapRecoveryUntilThrottleIsReleased) {
    test_dashboard_set_raw_adc(200, 775, 875, 0, 2775, 0);
    test_fault_set_latched(FAULT_ID_APPS_BRAKE, true);

    test_dashboard_run_pedals();

    EXPECT_EQ(test_fault_update_count(FAULT_ID_APPS_BRAKE), 1U);
    EXPECT_FLOAT_EQ(test_fault_last_value(FAULT_ID_APPS_BRAKE), 0.0F);
    EXPECT_EQ(test_dashboard_get_last_pedal_tx().throttle, 0);
}

} // namespace
