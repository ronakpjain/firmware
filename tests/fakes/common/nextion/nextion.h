#ifndef TEST_FAKE_NEXTION_H
#define TEST_FAKE_NEXTION_H

#include <stddef.h>
#include <stdint.h>

void NXT_setValue(char *obj_name, uint16_t val);
void NXT_setBackground(char *obj_name, uint16_t val);
void NXT_setBorderWidth(char *obj_name, uint16_t val);
void NXT_setText(char *obj_name, char *text);
void NXT_setTextFormatted(char *obj_name, const char *format, ...);

#endif
