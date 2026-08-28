#include "StyleTests.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Application/Style/UIStyle.h>

bool StyleDefaults_Test()
{
    TEST_BEGIN();

    const FUIStyle Style;

    TEST_SECTION("Every color is opaque, so nothing draws invisible by accident");
    TEST_EXPECT_EQ(Style.Colors.WindowBackground.A, 1.0f);
    TEST_EXPECT_EQ(Style.Colors.PanelBackground.A, 1.0f);
    TEST_EXPECT_EQ(Style.Colors.ControlNormal.A, 1.0f);
    TEST_EXPECT_EQ(Style.Colors.Text.A, 1.0f);

    TEST_SECTION("The theme is dark, so text reads lighter than the surface behind it");
    TEST_EXPECT(Style.Colors.Text.R > Style.Colors.PanelBackground.R);
    TEST_EXPECT(Style.Colors.TextDisabled.R < Style.Colors.Text.R);

    TEST_SECTION("Hover lifts the control and press sinks it, which is what makes a click read");
    TEST_EXPECT(Style.Colors.ControlHovered.R > Style.Colors.ControlNormal.R);
    TEST_EXPECT(Style.Colors.ControlPressed.R < Style.Colors.ControlNormal.R);

    TEST_SECTION("The metrics leave room for a line of text inside a control");
    TEST_EXPECT(Style.Metrics.RowHeight > 16);
    TEST_EXPECT(Style.Metrics.BorderThickness > 0.0f);
    TEST_EXPECT(Style.Metrics.ScrollBarThickness > 0);
    TEST_EXPECT(Style.Metrics.ControlPadding.GetTotalHorizontal() > 0);

    TEST_SECTION("A fresh style names no faces, so a caller has to supply them");
    TEST_EXPECT(Style.NormalFont == nullptr);
    TEST_EXPECT(Style.MonospaceFont == nullptr);

    TEST_END();
}

bool StyleControlColor_Test()
{
    TEST_BEGIN();

    const FUIStyle Style;

    TEST_SECTION("Each interaction state selects its own fill");
    TEST_EXPECT(Style.GetControlColor(EInteractionState::Normal) == Style.Colors.ControlNormal);
    TEST_EXPECT(Style.GetControlColor(EInteractionState::Hovered) == Style.Colors.ControlHovered);
    TEST_EXPECT(Style.GetControlColor(EInteractionState::Pressed) == Style.Colors.ControlPressed);
    TEST_EXPECT(Style.GetControlColor(EInteractionState::Disabled) == Style.Colors.ControlDisabled);

    TEST_SECTION("Only the disabled state dims the text");
    TEST_EXPECT(Style.GetTextColor(EInteractionState::Normal) == Style.Colors.Text);
    TEST_EXPECT(Style.GetTextColor(EInteractionState::Hovered) == Style.Colors.Text);
    TEST_EXPECT(Style.GetTextColor(EInteractionState::Pressed) == Style.Colors.Text);
    TEST_EXPECT(Style.GetTextColor(EInteractionState::Disabled) == Style.Colors.TextDisabled);

    TEST_END();
}

bool StyleOverride_Test()
{
    TEST_BEGIN();

    FUIStyle::ResetDefault();

    TEST_SECTION("The process default starts as the shipped theme");
    TEST_EXPECT(FUIStyle::GetDefault().Colors.Accent == FUIStyle().Colors.Accent);

    TEST_SECTION("Installing a theme is one assignment rather than a sweep");
    FUIStyle Custom;
    Custom.Colors.Accent     = FFloatColor(1.0f, 0.0f, 0.0f, 1.0f);
    Custom.Metrics.RowHeight = 40;

    FUIStyle::SetDefault(Custom);

    TEST_EXPECT(FUIStyle::GetDefault().Colors.Accent == FFloatColor(1.0f, 0.0f, 0.0f, 1.0f));
    TEST_EXPECT_EQ(FUIStyle::GetDefault().Metrics.RowHeight, 40);

    TEST_SECTION("Resetting puts the shipped theme back, so one test cannot colour the next");
    FUIStyle::ResetDefault();
    TEST_EXPECT(FUIStyle::GetDefault().Colors.Accent == FUIStyle().Colors.Accent);
    TEST_EXPECT_EQ(FUIStyle::GetDefault().Metrics.RowHeight, FUIStyle().Metrics.RowHeight);

    TEST_END();
}
