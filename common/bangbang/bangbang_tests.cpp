#include <gtest/gtest.h>

extern "C" {
#include "common/bangbang/bangbang.h"
}

namespace {
int on_calls;
int off_calls;
void on() { ++on_calls; }
void off() { ++off_calls; }

bangbang_t controller(bool is_on = false) {
    on_calls = 0;
    off_calls = 0;
    return bangbang_t{10.0F, 5.0F, on, off, 100U, 0U, is_on};
}
}  // namespace

TEST(BangBang, SwitchesOnOnlyAfterUpperBoundAndMinimumInterval) {
    auto bang = controller();

    bangbang_update(&bang, 11.0F, 99U);
    EXPECT_FALSE(bang.is_on);
    EXPECT_EQ(on_calls, 0);

    bangbang_update(&bang, 10.0F, 100U);
    EXPECT_TRUE(bang.is_on);
    EXPECT_EQ(bang.last_switch_ms, 100U);
    EXPECT_EQ(on_calls, 1);
}

TEST(BangBang, SwitchesOffOnlyAfterLowerBoundAndMinimumInterval) {
    auto bang = controller(true);

    bangbang_update(&bang, 4.0F, 99U);
    EXPECT_TRUE(bang.is_on);
    bangbang_update(&bang, 5.0F, 100U);

    EXPECT_FALSE(bang.is_on);
    EXPECT_EQ(off_calls, 1);
}

TEST(BangBang, DoesNotRepeatCallbacksWhileStateIsUnchanged) {
    auto bang = controller();

    bangbang_update(&bang, 12.0F, 100U);
    bangbang_update(&bang, 12.0F, 200U);
    bangbang_update(&bang, 7.0F, 300U);

    EXPECT_EQ(on_calls, 1);
    EXPECT_EQ(off_calls, 0);
}

TEST(BangBang, ElapsedTimeCalculationHandlesTimestampWraparound) {
    auto bang = controller();
    bang.last_switch_ms = UINT32_MAX - 49U;

    bangbang_update(&bang, 12.0F, 50U);

    EXPECT_TRUE(bang.is_on);
    EXPECT_EQ(on_calls, 1);
}

TEST(BangBang, SupportsNullCallbacks) {
    auto bang = controller();
    bang.on_func = nullptr;
    bang.off_func = nullptr;

    bangbang_update(&bang, 12.0F, 100U);
    bangbang_update(&bang, 4.0F, 200U);

    EXPECT_FALSE(bang.is_on);
}
