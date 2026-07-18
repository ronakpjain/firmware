#ifndef COMMON_UTILS_TEST_ADAPTER_H
#define COMMON_UTILS_TEST_ADAPTER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int utils_abs_int(int value);
float utils_abs_float(float value);
int utils_clamp_int(int value, int lower, int upper);
float utils_clamp_float(float value, float lower, float upper);
int utils_max4(int a, int b, int c, int d);
float utils_min3(float a, float b, float c);
float utils_rescale_float(float value, float in_min, float in_max, float out_min, float out_max);
int utils_rescale_int(int value, int in_min, int in_max, int out_min, int out_max);
size_t utils_array_count(void);
float utils_vector_magnitude(float x, float y, float z);
void utils_normalize(float x, float y, float z, float output[3]);
void utils_matrix_vector(const float matrix[9], const float input[3], float output[3]);
float utils_fahrenheit_from_celsius(float value);
float utils_meters_from_inches(float value);
float utils_seconds_from_hours(float value);
float utils_radians_from_degrees(float value);
float utils_kilograms_from_pounds(float value);

#ifdef __cplusplus
}
#endif

#endif
