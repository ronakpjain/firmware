#include <gtest/gtest.h>

extern "C" {
#include "source/a_box/thermistor/thermistor.h"
}

TEST(Thermistor, ReturnsKnownDatasheetPoints) {
    EXPECT_FLOAT_EQ(thermistor_R_to_T(10000.0F), 25.0F);
    EXPECT_FLOAT_EQ(thermistor_R_to_T(680.0F), 100.0F);
    EXPECT_FLOAT_EQ(thermistor_R_to_T(32650.0F), 0.0F);
}

TEST(Thermistor, InterpolatesBetweenAdjacentPoints) {
    EXPECT_NEAR(thermistor_R_to_T((10000.0F + 12490.0F) / 2.0F), 22.5F, 1e-4F);
}

TEST(Thermistor, ClampsOutsideTableRange) {
    EXPECT_FLOAT_EQ(thermistor_R_to_T(1.0F), 155.0F);
    EXPECT_FLOAT_EQ(thermistor_R_to_T(2000000.0F), -55.0F);
}
