#include <gtest/gtest.h>

extern "C" {
#include "source/a_box/adbms/commands.h"
}

TEST(AdbmsCommands, CoreRegisterCommandsMatchProtocolValues) {
    EXPECT_EQ(WRCFGA[0], 0x00U);
    EXPECT_EQ(WRCFGA[1], 0x01U);
    EXPECT_EQ(RDCFGA[0], 0x00U);
    EXPECT_EQ(RDCFGA[1], 0x02U);
    EXPECT_EQ(RDCVA[0], 0x00U);
    EXPECT_EQ(RDCVA[1], 0x04U);
    EXPECT_EQ(RDSID[0], 0x00U);
    EXPECT_EQ(RDSID[1], 0x2CU);
}

TEST(AdbmsCommands, ClearPollAndMuteCommandsMatchProtocolValues) {
    EXPECT_EQ(CLRCELL[0], 0x07U);
    EXPECT_EQ(CLRCELL[1], 0x11U);
    EXPECT_EQ(PLADC[0], 0x07U);
    EXPECT_EQ(PLADC[1], 0x18U);
    EXPECT_EQ(MUTE[0], 0x00U);
    EXPECT_EQ(MUTE[1], 0x28U);
    EXPECT_EQ(UNMUTE[0], 0x00U);
    EXPECT_EQ(UNMUTE[1], 0x29U);
}

TEST(AdbmsCommands, StartCommunicationFrameIncludesExpectedPecAndPadding) {
    EXPECT_EQ(STCOMM[0], 0x07U);
    EXPECT_EQ(STCOMM[1], 0x23U);
    EXPECT_EQ(STCOMM[2], 0xB9U);
    EXPECT_EQ(STCOMM[3], 0xE4U);
    for (size_t index = 4; index < 13; ++index) {
        EXPECT_EQ(STCOMM[index], 0U);
    }
}
