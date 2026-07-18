#include <gtest/gtest.h>

#include <iterator>

extern "C" {
#include "lerp_lut.h"
}

namespace {

constexpr lut_entry_t entries[] = {
    {-5.0F, 10.0F},
    {0.0F, 20.0F},
    {10.0F, 40.0F},
    {30.0F, -20.0F},
};

const lerp_lut_t lut = {entries, std::size(entries)};

TEST(LerpLut, ClampsKeysOutsideTableRange) {
    EXPECT_FLOAT_EQ(lut_lookup(&lut, -100.0F), 10.0F);
    EXPECT_FLOAT_EQ(lut_lookup(&lut, -5.0F), 10.0F);
    EXPECT_FLOAT_EQ(lut_lookup(&lut, 30.0F), -20.0F);
    EXPECT_FLOAT_EQ(lut_lookup(&lut, 100.0F), -20.0F);
}

TEST(LerpLut, ReturnsValuesAtExactInteriorKeys) {
    EXPECT_FLOAT_EQ(lut_lookup(&lut, 0.0F), 20.0F);
    EXPECT_FLOAT_EQ(lut_lookup(&lut, 10.0F), 40.0F);
}

TEST(LerpLut, InterpolatesWithinUnevenIntervals) {
    EXPECT_FLOAT_EQ(lut_lookup(&lut, -2.5F), 15.0F);
    EXPECT_FLOAT_EQ(lut_lookup(&lut, 5.0F), 30.0F);
    EXPECT_FLOAT_EQ(lut_lookup(&lut, 20.0F), 10.0F);
}

TEST(LerpLut, InterpolatesTablesWithDecreasingValues) {
    EXPECT_FLOAT_EQ(lut_lookup(&lut, 25.0F), -5.0F);
}

TEST(LerpLut, SupportsMinimumTableSize) {
    constexpr lut_entry_t two_entries[] = {
        {2.0F, 4.0F},
        {6.0F, 12.0F},
    };
    const lerp_lut_t two_entry_lut = {two_entries, std::size(two_entries)};

    EXPECT_FLOAT_EQ(lut_lookup(&two_entry_lut, 1.0F), 4.0F);
    EXPECT_FLOAT_EQ(lut_lookup(&two_entry_lut, 4.0F), 8.0F);
    EXPECT_FLOAT_EQ(lut_lookup(&two_entry_lut, 7.0F), 12.0F);
}

} // namespace
