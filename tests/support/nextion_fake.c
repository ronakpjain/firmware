#include "nextion_fake.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

nextion_fake_state_t nextion_fake = {0};

static void remember_object(const char *object) {
    (void)snprintf(nextion_fake.last_object, sizeof(nextion_fake.last_object), "%s", object);
}

void nextion_fake_reset(void) { memset(&nextion_fake, 0, sizeof(nextion_fake)); }

void NXT_setValue(char *obj_name, uint16_t val) {
    ++nextion_fake.value_calls;
    remember_object(obj_name);
    nextion_fake.last_value = val;
}

void NXT_setBackground(char *obj_name, uint16_t val) {
    ++nextion_fake.background_calls;
    remember_object(obj_name);
    nextion_fake.last_background = val;
}

void NXT_setBorderWidth(char *obj_name, uint16_t val) {
    ++nextion_fake.border_calls;
    remember_object(obj_name);
    nextion_fake.last_border = val;
}

void NXT_setText(char *obj_name, char *text) {
    ++nextion_fake.text_calls;
    remember_object(obj_name);
    (void)snprintf(nextion_fake.last_text, sizeof(nextion_fake.last_text), "%s", text);
}

void NXT_setTextFormatted(char *obj_name, const char *format, ...) {
    ++nextion_fake.text_calls;
    remember_object(obj_name);
    va_list args;
    va_start(args, format);
    (void)vsnprintf(nextion_fake.last_text, sizeof(nextion_fake.last_text), format, args);
    va_end(args);
}
