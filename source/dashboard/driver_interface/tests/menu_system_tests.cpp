#include <gtest/gtest.h>

extern "C" {
#include "source/dashboard/driver_interface/menu_system.h"
#include "nextion_fake.h"
}

namespace {
int change_calls = 0;
void changed() { ++change_calls; }

menu_element_t element(element_type_t type, char *name, uint16_t value = 0) {
    return menu_element_t{type, STATE_NORMAL, name, changed, nullptr, value, 0, 10, 2};
}

class MenuSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        change_calls = 0;
        nextion_fake_reset();
    }
};
}  // namespace

TEST_F(MenuSystemTest, NavigationWrapsAtBothEnds) {
    char first_name[] = "first";
    char second_name[] = "second";
    menu_element_t elements[] = {
        element(ELEMENT_BUTTON, first_name),
        element(ELEMENT_BUTTON, second_name),
    };
    menu_page_t page{elements, 2, 0, false, false};

    MS_moveUp(&page);
    EXPECT_EQ(page.current_index, 1U);
    MS_moveDown(&page);
    EXPECT_EQ(page.current_index, 0U);
    EXPECT_EQ(nextion_fake.border_calls, 4U);
}

TEST_F(MenuSystemTest, SelectedNumericValueIncrementsDecrementsAndWraps) {
    char name[] = "limit";
    menu_element_t elements[] = {element(ELEMENT_VAL, name, 10)};
    menu_page_t page{elements, 1, 0, true, false};

    MS_moveUp(&page);
    EXPECT_EQ(elements[0].current_value, 0U);
    MS_moveDown(&page);
    EXPECT_EQ(elements[0].current_value, 10U);
    EXPECT_STREQ(nextion_fake.last_text, "10");
}

TEST_F(MenuSystemTest, OptionSelectionTogglesValueAndInvokesCallback) {
    char name[] = "regen";
    menu_element_t elements[] = {element(ELEMENT_OPTION, name)};
    menu_page_t page{elements, 1, 0, false, false};

    MS_select(&page);
    EXPECT_EQ(elements[0].current_value, 1U);
    EXPECT_EQ(nextion_fake.last_value, 1U);
    EXPECT_EQ(change_calls, 1);
}

TEST_F(MenuSystemTest, ListSelectionClearsOtherItems) {
    char first_name[] = "dry";
    char second_name[] = "wet";
    menu_element_t elements[] = {
        element(ELEMENT_LIST, first_name, 1),
        element(ELEMENT_LIST, second_name, 0),
    };
    menu_page_t page{elements, 2, 1, false, false};

    MS_select(&page);

    EXPECT_EQ(elements[0].current_value, 0U);
    EXPECT_EQ(elements[1].current_value, 1U);
    EXPECT_EQ(MS_listGetSelected(&page), 1);
}

TEST_F(MenuSystemTest, DeselectingEditedValueInvokesCallback) {
    char name[] = "torque";
    menu_element_t elements[] = {element(ELEMENT_VAL, name, 4)};
    menu_page_t page{elements, 1, 0, false, false};

    MS_select(&page);
    EXPECT_TRUE(page.is_element_selected);
    MS_select(&page);

    EXPECT_FALSE(page.is_element_selected);
    EXPECT_EQ(change_calls, 1);
}
