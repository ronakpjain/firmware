#ifndef CAN_CODEC_TEST_ADAPTER_H
#define CAN_CODEC_TEST_ADAPTER_H

/**
 * GoogleTest suites are C++, but the codec helpers are header-only C23 code.
 * An extern "C" block changes linkage only; it does not make a C++ translation
 * unit use C semantics. These adapter functions are therefore implemented in
 * can_codec_test_adapter.c so the helpers under test are compiled as C23, just
 * as they are in the firmware.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CAN_CODEC_BSWAP_NONE = 0,
    CAN_CODEC_BSWAP_16   = 16,
    CAN_CODEC_BSWAP_32   = 32,
    CAN_CODEC_BSWAP_64   = 64,
};

uint64_t can_codec_apply_bswap(uint64_t raw, uint8_t bswap_width);
uint64_t can_codec_load_payload_u64(const uint8_t *data, uint8_t len);
void can_codec_store_payload_u64(uint8_t *dest, uint64_t payload, uint8_t len);
uint64_t can_codec_pack_raw_signal(
    uint64_t payload,
    uint64_t raw,
    uint64_t mask,
    uint8_t bit_shift,
    uint8_t bswap_width
);
uint64_t can_codec_unpack_raw_signal(
    uint64_t payload,
    uint64_t mask,
    uint8_t bit_shift,
    uint8_t bswap_width
);
int64_t can_codec_sign_extend_raw(uint64_t raw, uint8_t bit_length);
uint64_t can_codec_float32_to_raw(float value);
float can_codec_raw_to_float32(uint64_t raw);

#ifdef __cplusplus
}
#endif

#endif // CAN_CODEC_TEST_ADAPTER_H
