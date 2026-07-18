#include <gtest/gtest.h>

#include <array>

extern "C" {
#include "source/a_box/adbms/pec.h"
}

TEST(AdbmsPec15, MatchesDatasheetCommandVectors) {
    const std::array<uint8_t, 2> write_config_a{0x00, 0x01};
    const std::array<uint8_t, 2> read_cell_a{0x00, 0x04};

    EXPECT_EQ(adbms_pec_get_pec15(write_config_a.size(), write_config_a.data()), 0x3D6EU);
    EXPECT_EQ(adbms_pec_get_pec15(read_cell_a.size(), read_cell_a.data()), 0x07C2U);
}

TEST(AdbmsPec15, EmptyInputReturnsShiftedSeed) {
    EXPECT_EQ(adbms_pec_get_pec15(0U, nullptr), 0x0020U);
}

TEST(AdbmsPec10, ReceiveCommandCounterAffectsChecksum) {
    const std::array<uint8_t, 4> data{0x12, 0x34, 0x56, 0xA8};

    const uint16_t without_counter = adbms_pec_get_pec10(false, 3U, data.data());
    const uint16_t with_counter = adbms_pec_get_pec10(true, 3U, data.data());

    EXPECT_NE(with_counter, without_counter);
    EXPECT_LE(with_counter, 0x03FFU);
    EXPECT_LE(without_counter, 0x03FFU);
}

TEST(AdbmsPec10, IgnoresLowCounterBitsReservedForPec) {
    std::array<uint8_t, 3> first{0xAA, 0x55, 0xA8};
    std::array<uint8_t, 3> second{0xAA, 0x55, 0xAB};

    EXPECT_EQ(adbms_pec_get_pec10(true, 2U, first.data()),
              adbms_pec_get_pec10(true, 2U, second.data()));
}
