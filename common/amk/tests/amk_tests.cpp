#include <gtest/gtest.h>

extern "C" {
#include "common/amk/amk.h"
}

namespace {
int flush_calls = 0;
void flush() { ++flush_calls; }

class AmkTest : public ::testing::Test {
protected:
    void SetUp() override {
        flush_calls = 0;
        AMK_init(&amk, flush, &set, &crit, &info, &temps, &err1, &err2, &precharged);
    }

    void advanceToRunning() {
        precharged = true;
        info.AMK_Status_bSystemReady = true;
        AMK_periodic(&amk);  // OFF schedules STARTING
        info.AMK_Status_bQuitDcOn = true;
        info.AMK_Status_bQuitInverterOn = true;
        AMK_periodic(&amk);  // STARTING schedules RUNNING
        AMK_periodic(&amk);  // enter RUNNING
    }

    AMK_t amk{};
    INVA_SET_data_t set{};
    INVA_CRIT_data_t crit{};
    INVA_INFO_data_t info{};
    INVA_TEMPS_data_t temps{};
    INVA_ERR_1_data_t err1{};
    INVA_ERR_2_data_t err2{};
    bool precharged = false;
};
}  // namespace

TEST_F(AmkTest, InitSetsSafeOutputsAndDefaultTorqueLimits) {
    EXPECT_EQ(amk.state, AMK_STATE_OFF);
    EXPECT_FALSE(set.AMK_Control_bDcOn);
    EXPECT_FALSE(set.AMK_Control_bInverterOn);
    EXPECT_FALSE(set.AMK_Control_bEnable);
    EXPECT_EQ(set.AMK_TorqueSetpoint, 0);
    EXPECT_EQ(set.AMK_PositiveTorqueLimit, 2140);
    EXPECT_EQ(set.AMK_NegativeTorqueLimit, -500);
}

TEST_F(AmkTest, ReadyAndAcknowledgedInverterAdvancesToRunning) {
    advanceToRunning();

    EXPECT_EQ(amk.state, AMK_STATE_RUNNING);
    EXPECT_TRUE(set.AMK_Control_bDcOn);
    EXPECT_TRUE(set.AMK_Control_bInverterOn);
    EXPECT_TRUE(set.AMK_Control_bEnable);
    EXPECT_EQ(set.AMK_PositiveTorqueLimit, 2140);
    EXPECT_EQ(flush_calls, 3);
}

TEST_F(AmkTest, TorqueCommandsRequireRunningStateAndClampPositiveRequest) {
    AMK_set_torque(&amk, 100);
    EXPECT_EQ(set.AMK_TorqueSetpoint, 0);

    advanceToRunning();
    AMK_set_torque(&amk, 250);
    EXPECT_EQ(set.AMK_TorqueSetpoint, 2100);
    AMK_set_torque(&amk, -25);
    EXPECT_EQ(set.AMK_TorqueSetpoint, -250);
}

TEST_F(AmkTest, LossOfReadinessStopsTheInverterOnFollowingCycle) {
    advanceToRunning();
    precharged = false;

    AMK_periodic(&amk);
    EXPECT_EQ(amk.next_state, AMK_STATE_OFF);
    AMK_periodic(&amk);

    EXPECT_EQ(amk.state, AMK_STATE_OFF);
    EXPECT_FALSE(set.AMK_Control_bDcOn);
    EXPECT_FALSE(set.AMK_Control_bEnable);
    EXPECT_EQ(set.AMK_TorqueSetpoint, 0);
}

TEST_F(AmkTest, SimpleDiagnosticErrorRequestsOneCycleReset) {
    info.AMK_Status_bError = true;
    err1.AMK_DiagnosticNumber = 3587U;

    AMK_periodic(&amk);
    EXPECT_EQ(amk.next_state, AMK_STATE_RECOVERING);
    AMK_periodic(&amk);

    EXPECT_EQ(amk.state, AMK_STATE_RECOVERING);
    EXPECT_FALSE(set.AMK_Control_bErrorReset);
    EXPECT_FALSE(set.AMK_Control_bInverterOn);
    EXPECT_EQ(set.AMK_TorqueSetpoint, 0);
}
