#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include "main_module_sdc_test_runtime.h"

namespace {

constexpr uint8_t unreadable = 0xFF;

constexpr std::array<uint8_t, 17> mux_addresses = {
    11, 10, 9, 8, 7, 6, 5, unreadable, 4, 12, 3, 2, 13, unreadable, 1, 14, 0,
};

constexpr std::array<fault_id_t, 17> fault_ids = {
    FAULT_ID_SDC1_IMD,
    FAULT_ID_SDC2_BMS,
    FAULT_ID_SDC3_BSPD,
    FAULT_ID_SDC4_MAIN_OK,
    FAULT_ID_SDC5_BOTS,
    FAULT_ID_SDC6_INERTIA,
    FAULT_ID_SDC7_COCKPIT_ESTOP,
    TEST_FAULT_ID_COUNT,
    FAULT_ID_SDC9_FRONT_INTERLOCK,
    FAULT_ID_SDC10_RIGHT_ESTOP,
    FAULT_ID_SDC11_LEFT_ESTOP,
    FAULT_ID_SDC12_MSD,
    FAULT_ID_SDC13_E_METER,
    TEST_FAULT_ID_COUNT,
    FAULT_ID_SDC15_REAR_INTERLOCK,
    FAULT_ID_SDC16_TSMS,
    FAULT_ID_SDC17_AIR_M,
};

TEST(MainModuleSdc, PollsEveryReadableMuxNodeAndWrapsTheSequence) {
    test_main_module_reset();

    constexpr size_t sequence_repetitions = 2;
    for (size_t cycle = 0; cycle < mux_addresses.size() * sequence_repetitions; ++cycle) {
        const size_t index = cycle % mux_addresses.size();
        const uint8_t address = mux_addresses[index];
        const uint32_t delay_count_before = test_os_delay_call_count();
        const uint32_t gpio_write_count_before = test_board_gpio_write_count();

        GPIOB->IDR = UINT32_C(1) << 9; // Closed SDC node reads high.
        test_main_module_run_sdc();

        if (address == unreadable) {
            EXPECT_EQ(test_board_gpio_write_count(), gpio_write_count_before)
                << "SDC index " << index;
            EXPECT_EQ(test_os_delay_call_count(), delay_count_before) << "SDC index " << index;
            continue;
        }

        ASSERT_EQ(test_board_gpio_write_count(), gpio_write_count_before + 4)
            << "SDC index " << index;
        for (uint8_t bit = 0; bit < 4; ++bit) {
            const test_gpio_write_t write = test_board_gpio_write_at(gpio_write_count_before + bit);
            EXPECT_EQ(write.bank, GPIOB) << "SDC index " << index << ", mux bit " << bit;
            EXPECT_EQ(write.pin, 4 + bit) << "SDC index " << index << ", mux bit " << bit;
            EXPECT_EQ(write.value, ((address >> bit) & 0x1U) != 0)
                << "SDC index " << index << ", mux bit " << bit;
        }
        EXPECT_EQ(test_os_delay_call_count(), delay_count_before + 1) << "SDC index " << index;
        EXPECT_EQ(test_fault_update_count(fault_ids[index]), cycle / mux_addresses.size() + 1)
            << "SDC index " << index;
        EXPECT_FLOAT_EQ(test_fault_last_value(fault_ids[index]), 0.0F) << "SDC index " << index;
    }

    EXPECT_EQ(test_os_delay_call_count(), 30U);
    EXPECT_EQ(test_os_delay_total_ticks(), 30U);
}

} // namespace
