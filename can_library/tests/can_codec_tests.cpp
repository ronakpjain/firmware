#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>

#include "can_library/tests/can_codec_test_adapter.h"

TEST(CanCodecPayload, StoresAndLoadsRequestedBytes) {
    std::array<uint8_t, 8> bytes;
    bytes.fill(0xAA);

    can_codec_store_payload_u64(bytes.data(), UINT64_C(0x1122334455667788), 0);
    EXPECT_EQ(bytes, (std::array<uint8_t, 8>{
                         0xAA,
                         0xAA,
                         0xAA,
                         0xAA,
                         0xAA,
                         0xAA,
                         0xAA,
                         0xAA,
                     }));

    can_codec_store_payload_u64(bytes.data(), UINT64_C(0x1122334455667788), 3);
    EXPECT_EQ(bytes, (std::array<uint8_t, 8>{
                         0x88,
                         0x77,
                         0x66,
                         0xAA,
                         0xAA,
                         0xAA,
                         0xAA,
                         0xAA,
                     }));
    EXPECT_EQ(can_codec_load_payload_u64(bytes.data(), 3), UINT64_C(0x667788));

    can_codec_store_payload_u64(bytes.data(), UINT64_C(0x1122334455667788), 8);
    EXPECT_EQ(can_codec_load_payload_u64(bytes.data(), 8), UINT64_C(0x1122334455667788));
    EXPECT_EQ(can_codec_load_payload_u64(bytes.data(), 0), 0U);
}

TEST(CanCodecByteSwap, AppliesRequestedWidth) {
    EXPECT_EQ(
        can_codec_apply_bswap(UINT64_C(0x123456789ABCDEF0), CAN_CODEC_BSWAP_NONE),
        UINT64_C(0x123456789ABCDEF0)
    );
    EXPECT_EQ(
        can_codec_apply_bswap(UINT64_C(0x1234), CAN_CODEC_BSWAP_16),
        UINT64_C(0x3412)
    );
    EXPECT_EQ(
        can_codec_apply_bswap(UINT64_C(0x12345678), CAN_CODEC_BSWAP_32),
        UINT64_C(0x78563412)
    );
    EXPECT_EQ(
        can_codec_apply_bswap(UINT64_C(0x0123456789ABCDEF), CAN_CODEC_BSWAP_64),
        UINT64_C(0xEFCDAB8967452301)
    );
    EXPECT_EQ(
        can_codec_apply_bswap(UINT64_C(0x123456789ABCDEF0), 7),
        UINT64_C(0x123456789ABCDEF0)
    );
}

TEST(CanCodecSignals, PacksAndUnpacksRawValues) {
    uint64_t payload = 0;
    payload = can_codec_pack_raw_signal(payload, 0xAB, 0xFF, 0, CAN_CODEC_BSWAP_NONE);
    EXPECT_EQ(payload, UINT64_C(0xAB));
    EXPECT_EQ(
        can_codec_unpack_raw_signal(payload, 0xFF, 0, CAN_CODEC_BSWAP_NONE),
        UINT64_C(0xAB)
    );

    payload = can_codec_pack_raw_signal(0, 0x5, 0xF, 12, CAN_CODEC_BSWAP_NONE);
    EXPECT_EQ(payload, UINT64_C(0x5000));
    EXPECT_EQ(
        can_codec_unpack_raw_signal(payload, 0xF, 12, CAN_CODEC_BSWAP_NONE),
        UINT64_C(0x5)
    );

    payload = can_codec_pack_raw_signal(payload, 0xFFFF, 0x3, 0, CAN_CODEC_BSWAP_NONE);
    EXPECT_EQ(payload & UINT64_C(0x3), UINT64_C(0x3));
    EXPECT_EQ(payload & ~UINT64_C(0x3), UINT64_C(0x5000));
    EXPECT_EQ(
        can_codec_unpack_raw_signal(payload, 0x3, 0, CAN_CODEC_BSWAP_NONE),
        UINT64_C(0x3)
    );

    payload = UINT64_C(0xF000000000000000);
    payload = can_codec_pack_raw_signal(payload, 0x12, 0xFF, 8, CAN_CODEC_BSWAP_NONE);
    EXPECT_EQ(payload & UINT64_C(0xF000000000000000), UINT64_C(0xF000000000000000));
    EXPECT_EQ(
        can_codec_unpack_raw_signal(payload, 0xFF, 8, CAN_CODEC_BSWAP_NONE),
        UINT64_C(0x12)
    );

    payload = can_codec_pack_raw_signal(0, 0x1234, 0xFFFF, 0, CAN_CODEC_BSWAP_16);
    EXPECT_EQ(payload, UINT64_C(0x3412));
    EXPECT_EQ(
        can_codec_unpack_raw_signal(payload, 0xFFFF, 0, CAN_CODEC_BSWAP_16),
        UINT64_C(0x1234)
    );

    payload = can_codec_pack_raw_signal(
        0,
        0x12345678,
        UINT64_C(0xFFFFFFFF),
        8,
        CAN_CODEC_BSWAP_32
    );
    EXPECT_EQ(payload, UINT64_C(0x7856341200));
    EXPECT_EQ(
        can_codec_unpack_raw_signal(
            payload,
            UINT64_C(0xFFFFFFFF),
            8,
            CAN_CODEC_BSWAP_32
        ),
        UINT64_C(0x12345678)
    );

    payload = can_codec_pack_raw_signal(
        0,
        UINT64_C(0x0123456789ABCDEF),
        std::numeric_limits<uint64_t>::max(),
        0,
        CAN_CODEC_BSWAP_64
    );
    EXPECT_EQ(payload, UINT64_C(0xEFCDAB8967452301));
    EXPECT_EQ(
        can_codec_unpack_raw_signal(
            payload,
            std::numeric_limits<uint64_t>::max(),
            0,
            CAN_CODEC_BSWAP_64
        ),
        UINT64_C(0x0123456789ABCDEF)
    );
}

TEST(CanCodecSignals, SignExtendsRawValues) {
    EXPECT_EQ(can_codec_sign_extend_raw(0, 0), 0);

    EXPECT_EQ(can_codec_sign_extend_raw(0x7F, 8), 127);
    EXPECT_EQ(can_codec_sign_extend_raw(0x80, 8), -128);
    EXPECT_EQ(can_codec_sign_extend_raw(0xFF, 8), -1);

    EXPECT_EQ(can_codec_sign_extend_raw(0x7FF, 12), 2047);
    EXPECT_EQ(can_codec_sign_extend_raw(0x800, 12), -2048);
    EXPECT_EQ(can_codec_sign_extend_raw(0xFFF, 12), -1);

    EXPECT_EQ(can_codec_sign_extend_raw(0x7FFF, 16), 32767);
    EXPECT_EQ(can_codec_sign_extend_raw(0x8000, 16), -32768);
    EXPECT_EQ(can_codec_sign_extend_raw(0xFFFF, 16), -1);

    EXPECT_EQ(
        can_codec_sign_extend_raw(UINT64_C(0x7FFFFFFFFFFFFFFF), 64),
        std::numeric_limits<int64_t>::max()
    );
    EXPECT_EQ(
        can_codec_sign_extend_raw(UINT64_C(0x8000000000000000), 64),
        std::numeric_limits<int64_t>::min()
    );
}

TEST(CanCodecFloat, PreservesFloatBitPatterns) {
    EXPECT_EQ(can_codec_float32_to_raw(1.0F), UINT64_C(0x3F800000));

    const uint64_t raw = can_codec_float32_to_raw(-12.5F);
    EXPECT_EQ(can_codec_float32_to_raw(can_codec_raw_to_float32(raw)), raw);
    EXPECT_EQ(
        can_codec_float32_to_raw(can_codec_raw_to_float32(UINT64_C(0x40490FDB))),
        UINT64_C(0x40490FDB)
    );
}
