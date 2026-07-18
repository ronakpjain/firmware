#ifndef NEXTION_FAKE_H
#define NEXTION_FAKE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned value_calls;
    unsigned background_calls;
    unsigned border_calls;
    unsigned text_calls;
    char last_object[32];
    char last_text[64];
    uint16_t last_value;
    uint16_t last_background;
    uint16_t last_border;
} nextion_fake_state_t;

extern nextion_fake_state_t nextion_fake;
void nextion_fake_reset(void);

#ifdef __cplusplus
}
#endif

#endif
