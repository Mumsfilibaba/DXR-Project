#include "StyleTests.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Application/Menus/PopupWindow.h>
#include <Application/Style/UIStyle.h>
#include <CoreApplication/Platform/PlatformApplicationMisc.h>
#include <RHI/RHI.h>

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

    TEST_SECTION("A scroll bar's thumb carries against its track, and grabbing it lightens it further");
    TEST_EXPECT(Style.ScrollBar.Grab.R > Style.ScrollBar.Track.R);
    TEST_EXPECT(Style.ScrollBar.GrabActive.R > Style.ScrollBar.Grab.R);
    TEST_EXPECT(Style.ScrollBar.CornerRadius > 0.0f);

    TEST_SECTION("A dock tab paints nothing at rest, and with no rule over the active one its fill has to carry it");
    TEST_EXPECT_EQ(Style.Tab.Fill.A, 0.0f);
    TEST_EXPECT(Style.Tab.FillHovered.R > Style.Tab.StripFill.R);
    TEST_EXPECT(Style.Tab.FillActive.R > Style.Panel.Fill.R);
    TEST_EXPECT(Style.Tab.FillActive.R > Style.Tab.FillHovered.R);

    TEST_SECTION("A tab is held inside the strip and rounded, so a hover or active fill reads as a pill");
    TEST_EXPECT(Style.Tab.CornerRadius > 0.0f);
    TEST_EXPECT(Style.Tab.Spacing > 0);
    TEST_EXPECT(Style.Tab.TopInset > 0);
    TEST_EXPECT(Style.Tab.BottomInset > 0);
    TEST_EXPECT_EQ(Style.Tab.SeparatorThickness, 0);

    TEST_SECTION("Every tab opens at the same width at least, wide enough that the floor is the label's room rather than the chrome's");
    TEST_EXPECT(Style.Tab.MinWidth > 0);
    TEST_EXPECT(Style.Tab.MinWidth > 2 * (Style.Tab.HorizontalPadding + Style.Tab.CloseSize));

    TEST_SECTION("A close button fits inside the tab with room to spare, and its glyph centres inside itself");
    TEST_EXPECT(Style.Tab.CloseSize < Style.Tab.StripHeight - Style.Tab.TopInset - Style.Tab.BottomInset);
    TEST_EXPECT(Style.Tab.CloseIconSize <= Style.Tab.CloseSize - 4);
    TEST_EXPECT_EQ((Style.Tab.CloseSize - Style.Tab.CloseIconSize) % 2, 0);

    TEST_SECTION("The label sits on the tab's centre line");
    TEST_EXPECT_EQ(Style.Tab.LabelOffsetY, 0);

    TEST_SECTION("It sits nearer the tab's edge than the label does, at the same remove a menu row's highlight keeps");
    TEST_EXPECT(Style.Tab.CloseInset > 0);
    TEST_EXPECT(Style.Tab.CloseInset < Style.Tab.HorizontalPadding);
    TEST_EXPECT_EQ(Style.Tab.CloseInset, Style.Menu.ItemHighlightInset);

    TEST_SECTION("No rule is drawn over the active pill, though the tokens a style would stroke one with are still to hand");
    TEST_EXPECT_EQ(Style.Tab.ActiveStripThickness, 0);
    TEST_EXPECT(Style.Tab.ActiveStrip.B > Style.Tab.ActiveStrip.R);
    TEST_EXPECT(Style.Tab.ActiveStripFadeFraction > 0.0f);
    TEST_EXPECT(Style.Tab.ActiveStripFadeFraction <= 1.0f);
    TEST_EXPECT(Style.Tab.ActiveStripTrailAlpha > 0.0f);
    TEST_EXPECT(Style.Tab.ActiveStripTrailAlpha < 0.5f);

    TEST_SECTION("The bar that scrolls an overflowing strip is a hairline, and comes in faster than it goes out");
    TEST_EXPECT(Style.Tab.ScrollBarThickness > 0);
    TEST_EXPECT(Style.Tab.ScrollBarThickness < Style.Tab.StripHeight);

    TEST_SECTION("It stands clear of the pills, and the room the two take still leaves a pill worth having");
    TEST_EXPECT(Style.Tab.ScrollBarGap > 0);
    TEST_EXPECT(Style.Tab.ScrollBarThickness + Style.Tab.ScrollBarGap < Style.Tab.StripHeight - Style.Tab.TopInset);
    TEST_EXPECT(Style.Tab.ScrollBarFadeInDuration > 0.0f);
    TEST_EXPECT(Style.Tab.ScrollBarFadeOutDuration > Style.Tab.ScrollBarFadeInDuration);

    TEST_SECTION("A menu bar entry lifts on hover and lifts further while its menu is open");
    TEST_EXPECT(Style.MenuBar.ItemActive.R > Style.MenuBar.ItemHovered.R);
    TEST_EXPECT(Style.MenuBar.Height > 0);

    TEST_SECTION("Its highlight is held inside the strip and rounded, so it reads as a pill rather than a flat fill");
    TEST_EXPECT(Style.MenuBar.ItemInset > 0);
    TEST_EXPECT(Style.MenuBar.ItemCornerRadius > 0.0f);
    TEST_EXPECT(Style.MenuBar.ItemPadding.GetTotalVertical() < Style.MenuBar.Height);

    TEST_SECTION("Its entries are parted by a gap of their own, independent of the flush tabs");
    TEST_EXPECT(Style.MenuBar.ItemSpacing > 0);

    TEST_SECTION("A menu row matches the entry it drops from");
    TEST_EXPECT_EQ(Style.Menu.RowHeight, Style.MenuBar.Height - (2 * Style.MenuBar.ItemInset));

    TEST_SECTION("Every menu opens at the same width at least, however narrow the rows in it measure");
    TEST_EXPECT(Style.Menu.MinWidth > 0);
    TEST_EXPECT(Style.Menu.MinWidth > Style.MenuBar.Height);

    TEST_SECTION("An open menu reads against the surface behind it, and a shortcut reads quieter than the entry it belongs to");
    TEST_EXPECT(Style.Menu.Border.R > Style.Menu.Background.R);
    TEST_EXPECT(Style.Menu.ItemShortcut.R < Style.Colors.Text.R);
    TEST_EXPECT(Style.Menu.SectionText.R < Style.Colors.Text.R);

    TEST_SECTION("Its rows carry the same inset pill the strip's entries do, inside a panel rounded wider than they are");
    TEST_EXPECT(Style.Menu.ItemHighlightInset > 0);
    TEST_EXPECT(Style.Menu.ItemCornerRadius > 0.0f);
    TEST_EXPECT(Style.Menu.CornerRadius > Style.Menu.ItemCornerRadius);

    TEST_SECTION("The metrics leave room for a line of text inside a control");
    TEST_EXPECT(Style.Metrics.RowHeight > 16);
    TEST_EXPECT(Style.Metrics.FrameHeight >= Style.Metrics.RowHeight);
    TEST_EXPECT(Style.Metrics.ButtonHeight > 16);
    TEST_EXPECT(Style.Metrics.BorderThickness > 0.0f);
    TEST_EXPECT(Style.Metrics.ScrollBarThickness > 0);
    TEST_EXPECT(Style.Metrics.ControlPadding.GetTotalHorizontal() > 0);
    TEST_EXPECT(Style.Metrics.ButtonPadding.GetTotalHorizontal() > Style.Metrics.ControlPadding.GetTotalHorizontal());

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

    TEST_SECTION("A button reads lighter than a control, since a control is a surface and a button is a thing to press");
    TEST_EXPECT(Style.GetButtonColor(EInteractionState::Normal, false) == Style.Colors.ButtonNormal);
    TEST_EXPECT(Style.GetButtonColor(EInteractionState::Hovered, false) == Style.Colors.ButtonHovered);
    TEST_EXPECT(Style.GetButtonColor(EInteractionState::Pressed, false) == Style.Colors.ButtonPressed);
    TEST_EXPECT(Style.Colors.ButtonHovered.R > Style.Colors.ButtonNormal.R);

    TEST_SECTION("A selected button takes the accent, and hovering it brightens that rather than greying it");
    TEST_EXPECT(Style.GetButtonColor(EInteractionState::Normal, true) == Style.Colors.Accent);
    TEST_EXPECT(Style.GetButtonColor(EInteractionState::Hovered, true) == Style.Colors.AccentHovered);
    TEST_EXPECT(Style.GetButtonColor(EInteractionState::Pressed, true) == Style.Colors.AccentHovered);

    TEST_SECTION("A disabled button reads as disabled whether or not it is the selected one of its set");
    TEST_EXPECT(Style.GetButtonColor(EInteractionState::Disabled, false) == Style.Colors.ControlDisabled);
    TEST_EXPECT(Style.GetButtonColor(EInteractionState::Disabled, true) == Style.Colors.ControlDisabled);

    TEST_SECTION("Only the disabled state dims the text");
    TEST_EXPECT(Style.GetTextColor(EInteractionState::Normal) == Style.Colors.Text);
    TEST_EXPECT(Style.GetTextColor(EInteractionState::Hovered) == Style.Colors.Text);
    TEST_EXPECT(Style.GetTextColor(EInteractionState::Pressed) == Style.Colors.Text);
    TEST_EXPECT(Style.GetTextColor(EInteractionState::Disabled) == Style.Colors.TextDisabled);

    TEST_SECTION("A nested expander is a rounded panel, not a flat bar with an underline");
    TEST_EXPECT(Style.Header.CornerRadius >= Style.Panel.CornerRadius - 0.01f);
    TEST_EXPECT(Style.Header.BorderThickness <= 1.0f + 0.01f);
    TEST_EXPECT(Style.Header.ExpandDuration > 0.0f);

    TEST_SECTION("An inner view is framed thinner than the docked panel around it");
    TEST_EXPECT(Style.InnerFrame.CornerRadius > 0.0f);
    TEST_EXPECT(Style.InnerFrame.CornerRadius <= Style.Panel.CornerRadius);
    TEST_EXPECT(Style.InnerFrame.BorderThickness > 0.0f);
    TEST_EXPECT(Style.InnerFrame.Padding.Left > Style.InnerFrame.BorderThickness);
    TEST_EXPECT(Style.InnerFrame.Padding.Top > Style.InnerFrame.BorderThickness);

    TEST_SECTION("Tree interaction highlights round inside their framed view");
    TEST_EXPECT(Style.TreeRow.CornerRadius > 0.0f);

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

bool PopupCornerRounding_Test()
{
    TEST_BEGIN();

    const bool bWasSupported = RHI::bSupportsTransparentSwapChain;

    TEST_SECTION("The theme keeps the radii it was given, whatever the RHI can do with a surface");
    RHI::bSupportsTransparentSwapChain = false;
    FUIStyle::SetDefault(FUIStyle());

    TEST_EXPECT_EQ(FUIStyle::GetDefault().Menu.CornerRadius, FUIStyle().Menu.CornerRadius);
    TEST_EXPECT_EQ(FUIStyle::GetDefault().Metrics.CornerRadius, FUIStyle().Metrics.CornerRadius);

    TEST_SECTION("Which matters because the same radius rounds check boxes and combo fields, not only popups");
    TEST_EXPECT(FUIStyle::GetDefault().Metrics.CornerRadius > 0.0f);
    TEST_EXPECT_EQ(FUIStyle::GetDefault().Menu.ItemCornerRadius, FUIStyle().Menu.ItemCornerRadius);
    TEST_EXPECT_EQ(FUIStyle::GetDefault().Tab.CornerRadius, FUIStyle().Tab.CornerRadius);
    TEST_EXPECT_EQ(FUIStyle::GetDefault().MenuBar.ItemCornerRadius, FUIStyle().MenuBar.ItemCornerRadius);

    TEST_SECTION("A platform that rounds windows itself wins, because that needs no transparent surface at all");
    if (FPlatformApplicationMisc::SupportsRoundedWindowCorners())
    {
        RHI::bSupportsTransparentSwapChain = true;
        TEST_EXPECT(Popups::ResolveCornerRounding() == EPopupCornerRounding::System);

        RHI::bSupportsTransparentSwapChain = false;
        TEST_EXPECT(Popups::ResolveCornerRounding() == EPopupCornerRounding::System);
    }
    else
    {
        TEST_SECTION("Without one, a popup draws its own corners wherever the RHI can carry alpha to the desktop");
        RHI::bSupportsTransparentSwapChain = true;
        TEST_EXPECT(Popups::ResolveCornerRounding() == EPopupCornerRounding::Content);

        TEST_SECTION("And where it cannot, there is nothing left to round a popup with");
        RHI::bSupportsTransparentSwapChain = false;
        TEST_EXPECT(Popups::ResolveCornerRounding() == EPopupCornerRounding::None);
    }

    RHI::bSupportsTransparentSwapChain = bWasSupported;
    FUIStyle::ResetDefault();

    TEST_END();
}
