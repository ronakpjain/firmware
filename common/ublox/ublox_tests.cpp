#include <gtest/gtest.h>

#include <array>
#include <cstring>

extern "C" {
#include "common/ublox/nav_pvt.h"
#include "common/ublox/nav_relposned.h"
}

TEST(UbloxNavPvt, DecodesPayloadWhenHeaderMatches) {
    NAV_PVT_data_t expected{};
    expected.iTOW = 123456U;
    expected.year = 2026U;
    expected.month = 7U;
    expected.day = 17U;
    expected.fixType = GPS_FIX_TYPE_GNSS_3D;
    expected.numSatellites = 14U;
    expected.longitude = -861234567;
    expected.latitude = 401234567;
    expected.groundSpeed = 2718;

    std::array<uint8_t, NAV_PVT_TOTAL_LENGTH> packet{};
    packet[0] = NAV_PVT_HEADER_B0;
    packet[1] = NAV_PVT_HEADER_B1;
    packet[2] = NAV_PVT_CLASS;
    packet[3] = NAV_PVT_MSG_ID;
    std::memcpy(packet.data() + 6, &expected, sizeof(expected));

    NAV_PVT_data_t actual{};
    NAV_PVT_decode(&actual, packet.data());

    EXPECT_EQ(std::memcmp(&actual, &expected, sizeof(expected)), 0);
}

TEST(UbloxNavPvt, InvalidHeaderLeavesDestinationUnchanged) {
    std::array<uint8_t, NAV_PVT_TOTAL_LENGTH> packet{};
    packet[0] = 0x00;
    NAV_PVT_data_t actual{};
    actual.iTOW = 0xDEADBEEFU;

    NAV_PVT_decode(&actual, packet.data());

    EXPECT_EQ(actual.iTOW, 0xDEADBEEFU);
}

TEST(UbloxRelPosNed, DecodesSignedPositionsAccuracyAndFlags) {
    NAV_RELPOSNED_data_t expected{};
    expected.version = 1U;
    expected.refStationId = 42U;
    expected.iTOW = 9000U;
    expected.relPosN = -120;
    expected.relPosE = 345;
    expected.relPosD = -67;
    expected.accN = 10U;
    expected.flags = static_cast<NAV_RELPOSNED_flags_t>(
        NAV_RELPOSNED_FLAGS_GNSS_FIX_OK | NAV_RELPOSNED_FLAGS_CARR_SOLN_FIXED);

    std::array<uint8_t, NAV_RELPOSNED_TOTAL_LENGTH> packet{};
    packet[0] = NAV_RELPOSNED_HEADER_B0;
    packet[1] = NAV_RELPOSNED_HEADER_B1;
    packet[2] = NAV_RELPOSNED_CLASS;
    packet[3] = NAV_RELPOSNED_MSG_ID;
    std::memcpy(packet.data() + 6, &expected, sizeof(expected));

    NAV_RELPOSNED_data_t actual{};
    NAV_RELPOSNED_decode(&actual, packet.data());

    EXPECT_EQ(std::memcmp(&actual, &expected, sizeof(expected)), 0);
}

TEST(UbloxRelPosNed, WrongMessageIdLeavesDestinationUnchanged) {
    std::array<uint8_t, NAV_RELPOSNED_TOTAL_LENGTH> packet{};
    packet[0] = NAV_RELPOSNED_HEADER_B0;
    packet[1] = NAV_RELPOSNED_HEADER_B1;
    packet[2] = NAV_RELPOSNED_CLASS;
    packet[3] = 0xFF;
    NAV_RELPOSNED_data_t actual{};
    actual.refStationId = 99U;

    NAV_RELPOSNED_decode(&actual, packet.data());

    EXPECT_EQ(actual.refStationId, 99U);
}
