#include <gtest/gtest.h>

#include "common_utils_test_adapter.h"

TEST(Utils, GenericNumericOperationsDispatchForIntsAndFloats) {
    EXPECT_EQ(utils_abs_int(-7), 7);
    EXPECT_FLOAT_EQ(utils_abs_float(-2.5F), 2.5F);
    EXPECT_EQ(utils_clamp_int(20, -5, 10), 10);
    EXPECT_FLOAT_EQ(utils_clamp_float(-2.0F, -1.0F, 1.0F), -1.0F);
    EXPECT_EQ(utils_max4(-4, 8, 3, 2), 8);
    EXPECT_FLOAT_EQ(utils_min3(4.0F, -2.0F, 8.0F), -2.0F);
}

TEST(Utils, RescaleSupportsNormalReversedAndZeroInputRanges) {
    EXPECT_FLOAT_EQ(utils_rescale_float(5.0F, 0.0F, 10.0F, 0.0F, 100.0F), 50.0F);
    EXPECT_FLOAT_EQ(utils_rescale_float(2.0F, 0.0F, 4.0F, 10.0F, 0.0F), 5.0F);
    EXPECT_FLOAT_EQ(utils_rescale_float(7.0F, 2.0F, 2.0F, 30.0F, 80.0F), 30.0F);
    EXPECT_EQ(utils_rescale_int(5, 0, 10, 0, 20), 10);
}

TEST(Utils, CountofReportsArrayElements) {
    EXPECT_EQ(utils_array_count(), 5U);
}

TEST(Utils, VectorAndMatrixOperationsProduceExpectedResults) {
    EXPECT_FLOAT_EQ(utils_vector_magnitude(3.0F, 4.0F, 12.0F), 13.0F);

    float normalized[3]{};
    utils_normalize(0.0F, 3.0F, 4.0F, normalized);
    EXPECT_FLOAT_EQ(normalized[0], 0.0F);
    EXPECT_FLOAT_EQ(normalized[1], 0.6F);
    EXPECT_FLOAT_EQ(normalized[2], 0.8F);

    const float matrix[9] = {1, 2, 3, 0, 1, 4, 5, 6, 0};
    const float input[3] = {1, 2, 3};
    float output[3]{};
    utils_matrix_vector(matrix, input, output);
    EXPECT_FLOAT_EQ(output[0], 14.0F);
    EXPECT_FLOAT_EQ(output[1], 14.0F);
    EXPECT_FLOAT_EQ(output[2], 17.0F);
}

TEST(Utils, NormalizingNearZeroVectorReturnsZero) {
    float output[3]{};
    utils_normalize(0.00001F, 0.0F, 0.0F, output);
    EXPECT_FLOAT_EQ(output[0], 0.0F);
    EXPECT_FLOAT_EQ(output[1], 0.0F);
    EXPECT_FLOAT_EQ(output[2], 0.0F);
}

TEST(Utils, UnitConversionsMatchReferenceValues) {
    EXPECT_FLOAT_EQ(utils_fahrenheit_from_celsius(100.0F), 212.0F);
    EXPECT_NEAR(utils_meters_from_inches(12.0F), 0.3048F, 1e-6F);
    EXPECT_FLOAT_EQ(utils_seconds_from_hours(2.0F), 7200.0F);
    EXPECT_NEAR(utils_radians_from_degrees(180.0F), 3.14159265F, 1e-6F);
    EXPECT_NEAR(utils_kilograms_from_pounds(10.0F), 4.53592F, 1e-5F);
}
