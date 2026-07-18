/*
 * These wrappers intentionally compile the header-only utilities as C23.
 * Several utilities use C-only _Generic dispatch, so including them directly
 * in a C++ GoogleTest source would test different language rules or fail to
 * compile rather than exercising the production interface.
 */
#include "common_utils_test_adapter.h"

#include "common/utils/abs.h"
#include "common/utils/clamp.h"
#include "common/utils/countof.h"
#include "common/utils/linear_algebra.h"
#include "common/utils/max.h"
#include "common/utils/min.h"
#include "common/utils/rescale.h"
#include "common/utils/units.h"

int utils_abs_int(int value) { return ABS(value); }
float utils_abs_float(float value) { return ABS(value); }
int utils_clamp_int(int value, int lower, int upper) { return CLAMP(value, lower, upper); }
float utils_clamp_float(float value, float lower, float upper) { return CLAMP(value, lower, upper); }
int utils_max4(int a, int b, int c, int d) { return MAXOF(a, b, c, d); }
float utils_min3(float a, float b, float c) { return MINOF(a, b, c); }

float utils_rescale_float(float value, float in_min, float in_max, float out_min, float out_max) {
    return RESCALE(value, in_min, in_max, out_min, out_max);
}

int utils_rescale_int(int value, int in_min, int in_max, int out_min, int out_max) {
    return RESCALE(value, in_min, in_max, out_min, out_max);
}

size_t utils_array_count(void) {
    const int values[] = {1, 2, 3, 4, 5};
    return countof(values);
}

float utils_vector_magnitude(float x, float y, float z) {
    return vector3_magnitude((vector3_t){x, y, z});
}

void utils_normalize(float x, float y, float z, float output[3]) {
    vector3_t result = vector3_normalize((vector3_t){x, y, z});
    output[0] = result.x;
    output[1] = result.y;
    output[2] = result.z;
}

void utils_matrix_vector(const float matrix[9], const float input[3], float output[3]) {
    matrix3x3_t mat = {.data = {
        {matrix[0], matrix[1], matrix[2]},
        {matrix[3], matrix[4], matrix[5]},
        {matrix[6], matrix[7], matrix[8]},
    }};
    vector3_t in = {input[0], input[1], input[2]};
    vector3_t result = matrix_multiply_vector3(&mat, &in);
    output[0] = result.x;
    output[1] = result.y;
    output[2] = result.z;
}

float utils_fahrenheit_from_celsius(float value) {
    return fahrenheit_from((celsius_t){value}).value;
}
float utils_meters_from_inches(float value) { return meters_from((inches_t){value}).value; }
float utils_seconds_from_hours(float value) { return seconds_from((hours_t){value}).value; }
float utils_radians_from_degrees(float value) { return radians_from((degrees_t){value}).value; }
float utils_kilograms_from_pounds(float value) { return kilograms_from((pounds_t){value}).value; }
