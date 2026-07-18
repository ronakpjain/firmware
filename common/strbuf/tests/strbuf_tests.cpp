#include <gtest/gtest.h>

#include <array>
#include <cstring>

extern "C" {
#include "common/strbuf/strbuf.h"
}

TEST(Strbuf, AppendsRawBytesUpToCapacity) {
    ALLOCATE_STRBUF(buffer, 4);
    const std::array<uint8_t, 4> bytes{1, 2, 3, 4};

    EXPECT_EQ(strbuf_append(&buffer, bytes.data(), bytes.size()), bytes.size());
    EXPECT_EQ(buffer.length, bytes.size());
    EXPECT_EQ(std::memcmp(buffer.data, bytes.data(), bytes.size()), 0);
}

TEST(Strbuf, RejectsRawAppendThatWouldOverflowWithoutMutation) {
    ALLOCATE_STRBUF(buffer, 4);
    const uint8_t initial[] = {1, 2, 3};
    const uint8_t extra[] = {4, 5};
    ASSERT_EQ(strbuf_append(&buffer, initial, sizeof(initial)), sizeof(initial));

    EXPECT_EQ(strbuf_append(&buffer, extra, sizeof(extra)), 0U);
    EXPECT_EQ(buffer.length, sizeof(initial));
    EXPECT_EQ(std::memcmp(buffer.data, initial, sizeof(initial)), 0);
}

TEST(Strbuf, FormatsAndAppendsMultipleValues) {
    ALLOCATE_STRBUF(buffer, 16);

    EXPECT_EQ(strbuf_printf(&buffer, "%s", "rpm="), 4U);
    EXPECT_EQ(strbuf_printf(&buffer, "%d", 1234), 4U);
    EXPECT_EQ(buffer.length, 8U);
    EXPECT_STREQ(reinterpret_cast<char *>(buffer.data), "rpm=1234");
}

TEST(Strbuf, RejectsExactFitBecauseFormattedOutputNeedsATerminator) {
    struct GuardedBuffer {
        uint8_t data[4];
        uint8_t guard;
    } storage{{0xAA, 0xAA, 0xAA, 0xAA}, 0x5A};
    strbuf_t buffer{storage.data, 0, sizeof(storage.data)};

    EXPECT_EQ(strbuf_printf(&buffer, "%s", "1234"), 0U);
    EXPECT_EQ(buffer.length, 0U);
    EXPECT_EQ(storage.guard, 0x5A);
}

TEST(Strbuf, ClearResetsLengthAndAllowsReuse) {
    ALLOCATE_STRBUF(buffer, 8);
    ASSERT_EQ(strbuf_printf(&buffer, "%s", "first"), 5U);

    strbuf_clear(&buffer);

    EXPECT_EQ(buffer.length, 0U);
    EXPECT_EQ(strbuf_printf(&buffer, "%s", "new"), 3U);
    EXPECT_STREQ(reinterpret_cast<char *>(buffer.data), "new");
}
