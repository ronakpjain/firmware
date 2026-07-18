#include <gtest/gtest.h>

extern "C" {
#include "source/driveline/oil_temps/oil_temps_table.h"
}

TEST(OilTemperature, ReturnsKnownCalibrationPoints) {
    EXPECT_FLOAT_EQ(oil_temps_R_to_T(120.0F), 100.0F);
    EXPECT_FLOAT_EQ(oil_temps_R_to_T(522.0F), 50.0F);
    EXPECT_FLOAT_EQ(oil_temps_R_to_T(3400.0F), 2.3F);
}

TEST(OilTemperature, InterpolatesBetweenCalibrationPoints) {
    EXPECT_NEAR(oil_temps_R_to_T(548.0F), 49.0F, 1e-5F);
}

TEST(OilTemperature, ClampsOutsideCalibrationRange) {
    EXPECT_FLOAT_EQ(oil_temps_R_to_T(0.0F), 110.0F);
    EXPECT_FLOAT_EQ(oil_temps_R_to_T(10000.0F), -17.5F);
}
