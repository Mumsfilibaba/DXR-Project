#pragma once
#include "Core/Math/Color.h"
#include "Application/Layout/LayoutTypes.h"

struct IFontFace;

enum class EInteractionState : uint8
{
    Normal,
    Hovered,
    Pressed,
    Disabled,
};

struct FUIStyleColors
{
    /** @brief The fill behind a window's whole client area. */
    FFloatColor WindowBackground = FFloatColor(0.09f, 0.09f, 0.11f, 1.0f);

    /** @brief The fill behind a panel, which reads a step lighter than the window it is docked in. */
    FFloatColor PanelBackground = FFloatColor(0.13f, 0.13f, 0.16f, 1.0f);

    /** @brief The fill of a control that is neither hovered nor pressed. */
    FFloatColor ControlNormal = FFloatColor(0.20f, 0.21f, 0.25f, 1.0f);

    /** @brief The fill a control lifts to while the cursor rests on it. */
    FFloatColor ControlHovered = FFloatColor(0.27f, 0.29f, 0.34f, 1.0f);

    /** @brief The fill a control sinks to while it is held down. */
    FFloatColor ControlPressed = FFloatColor(0.16f, 0.17f, 0.21f, 1.0f);

    /** @brief The fill of a control that cannot be interacted with. */
    FFloatColor ControlDisabled = FFloatColor(0.16f, 0.16f, 0.18f, 1.0f);

    /** @brief The fill of a button at rest, which runs lighter than a control's. */
    FFloatColor ButtonNormal = FFloatColor(0.22f, 0.22f, 0.22f, 1.0f);

    /** @brief The fill a button lifts to while the cursor rests on it. */
    FFloatColor ButtonHovered = FFloatColor(0.34f, 0.34f, 0.34f, 1.0f);

    /** @brief The fill a button takes while it is held down. */
    FFloatColor ButtonPressed = FFloatColor(0.34f, 0.34f, 0.34f, 1.0f);

    /** @brief The fill behind a menu bar entry the cursor rests on. */
    FFloatColor MenuBarItemHovered = FFloatColor(0.27f, 0.29f, 0.34f, 1.0f);

    /** @brief The fill behind the menu bar entry whose menu is open. */
    FFloatColor MenuBarItemActive = FFloatColor(0.25f, 0.55f, 0.95f, 1.0f);

    /** @brief The fill of an open menu. */
    FFloatColor MenuBackground = FFloatColor(0.13f, 0.13f, 0.16f, 1.0f);

    /** @brief The stroke around an open menu. */
    FFloatColor MenuBorder = FFloatColor(0.32f, 0.33f, 0.38f, 1.0f);

    /** @brief The second stroke drawn a pixel inside the first, which is what gives a menu its bevel. */
    FFloatColor MenuInnerBorder = FFloatColor(0.20f, 0.21f, 0.24f, 1.0f);

    /** @brief The fill behind the menu entry the cursor rests on. */
    FFloatColor MenuItemHovered = FFloatColor(0.25f, 0.55f, 0.95f, 1.0f);

    /** @brief The color of the key combination shown at an entry's right edge. */
    FFloatColor MenuItemShortcut = FFloatColor(0.45f, 0.46f, 0.50f, 1.0f);

    /** @brief The rule between two runs of menu entries. */
    FFloatColor MenuSeparator = FFloatColor(106.0f / 255.0f, 106.0f / 255.0f, 106.0f / 255.0f, 1.0f);

    /** @brief The color of the heading naming a run of menu entries. */
    FFloatColor MenuSectionText = FFloatColor(160.0f / 255.0f, 160.0f / 255.0f, 160.0f / 255.0f, 1.0f);

    /** @brief The fill behind an editable line of text, which runs darker than anything around it. */
    FFloatColor InputFieldFill = FFloatColor(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);

    /** @brief The stroke around such a field at rest. */
    FFloatColor InputFieldBorder = FFloatColor(51.0f / 255.0f, 51.0f / 255.0f, 51.0f / 255.0f, 1.0f);

    /** @brief The stroke the cursor resting over such a field replaces the normal one with. */
    FFloatColor InputFieldBorderHovered = FFloatColor(74.0f / 255.0f, 74.0f / 255.0f, 74.0f / 255.0f, 1.0f);

    /** @brief The stroke around anything the other border colors do not name. */
    FFloatColor Border = FFloatColor(0.32f, 0.33f, 0.38f, 1.0f);

    /** @brief The color nearly all text is drawn in. */
    FFloatColor Text = FFloatColor(0.90f, 0.91f, 0.94f, 1.0f);

    /** @brief The color text dims to when what carries it cannot be interacted with. */
    FFloatColor TextDisabled = FFloatColor(0.45f, 0.46f, 0.50f, 1.0f);

    /** @brief The fill behind selected text. */
    FFloatColor TextSelectionBackground = FFloatColor(0.20f, 0.42f, 0.78f, 1.0f);

    /** @brief The color marking what the editor is pointed at, which is the focus ring and the chosen row. */
    FFloatColor Accent = FFloatColor(0.25f, 0.55f, 0.95f, 1.0f);

    /** @brief The accent lifted for a hover, so a selected control still answers the cursor. */
    FFloatColor AccentHovered = FFloatColor(0.35f, 0.63f, 0.98f, 1.0f);
};

struct FUIHeaderStyle
{
    /** @brief The fill behind the header, which does not lift on hover. */
    FFloatColor Fill = FFloatColor(47.0f / 255.0f, 47.0f / 255.0f, 47.0f / 255.0f, 1.0f);

    /** @brief The tint of the arrow that shows which way the section is folded. */
    FFloatColor ArrowTint = FFloatColor(101.0f / 255.0f, 101.0f / 255.0f, 101.0f / 255.0f, 1.0f);

    /** @brief The rule below a closed header, which is what separates one closed section from the next. */
    FFloatColor BottomBorder = FFloatColor(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);

    /** @brief The space between the header's edges and its label. */
    FMargin FramePadding = FMargin(10, 8, 10, 8);

    /** @brief How far the header's corners are rounded, in pixels. */
    float CornerRadius = 2.0f;

    /** @brief The width of the rule below a closed header, in pixels. */
    float BorderThickness = 2.0f;
};

struct FUIPropertyTableStyle
{
    /** @brief The fill behind a row. */
    FFloatColor RowFill = FFloatColor(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);

    /** @brief The fill behind every second row, which matches RowFill unless a caller wants banding. */
    FFloatColor AlternateRowFill = FFloatColor(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);

    /** @brief The fill behind the row the cursor rests on. */
    FFloatColor HoveredRowFill = FFloatColor(48.0f / 255.0f, 48.0f / 255.0f, 48.0f / 255.0f, 1.0f);

    /** @brief The rules between rows and between columns. */
    FFloatColor GridLine = FFloatColor(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);

    /** @brief The space between a cell's edges and its contents. */
    FMargin CellPadding = FMargin(6, 4, 6, 4);

    /** @brief How far the leftmost label sits from the table's left edge, in pixels. */
    int32 LabelIndent = 6;

    /** @brief How wide the column holding the revert arrow is, in pixels. */
    int32 RevertColumnWidth = 28;
};

struct FUIComboBoxStyle
{
    /** @brief The fill behind the closed field, which runs darker than a button. */
    FFloatColor Fill = FFloatColor(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);

    /** @brief The color of the selected value shown while the field is closed. */
    FFloatColor Text = FFloatColor(170.0f / 255.0f, 170.0f / 255.0f, 170.0f / 255.0f, 1.0f);

    /** @brief The color that value takes once the cursor rests on the field. */
    FFloatColor TextHovered = FFloatColor(1.0f, 1.0f, 1.0f, 1.0f);

    /** @brief The tint of the triangle at the field's right edge. */
    FFloatColor Arrow = FFloatColor(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);

    /** @brief The fill of the list the field opens. */
    FFloatColor PopupFill = FFloatColor(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);

    /** @brief The stroke around that list. */
    FFloatColor PopupBorder = FFloatColor(0.0f, 90.0f / 255.0f, 173.0f / 255.0f, 1.0f);

    /** @brief The fill behind the row of the list that is currently chosen. */
    FFloatColor SelectionFill = FFloatColor(0.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);
};

struct FUIAxisColors
{
    /**
     * @brief Picks one axis by index, so a loop over a vector's components needs no switch.
     *
     * @param Index The axis, where 0 is X and anything above 2 is clamped to Z.
     * @return That axis's color.
     */
    NODISCARD const FFloatColor& operator[](int32 Index) const
    {
        return Index <= 0 ? X : (Index == 1 ? Y : Z);
    }

    /** @brief The color of the first component. */
    FFloatColor X = FFloatColor(204.0f / 255.0f, 26.0f / 255.0f, 38.0f / 255.0f, 1.0f);

    /** @brief The color of the second component. */
    FFloatColor Y = FFloatColor(51.0f / 255.0f, 179.0f / 255.0f, 51.0f / 255.0f, 1.0f);

    /** @brief The color of the third component. */
    FFloatColor Z = FFloatColor(26.0f / 255.0f, 64.0f / 255.0f, 204.0f / 255.0f, 1.0f);
};

struct FUITreeRowStyle
{
    /** @brief The fill behind a row. */
    FFloatColor Fill = FFloatColor(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f);

    /** @brief The fill behind every second row, which is what makes a long list readable. */
    FFloatColor AlternateFill = FFloatColor(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);

    /** @brief The fill behind the row the cursor rests on. */
    FFloatColor HoveredFill = FFloatColor(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);

    /** @brief The fill behind a selected row while its view holds the focus. */
    FFloatColor SelectedFill = FFloatColor(0.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);

    /** @brief The fill that selection fades to once the focus moves elsewhere. */
    FFloatColor InactiveSelectedFill = FFloatColor(64.0f / 255.0f, 87.0f / 255.0f, 111.0f / 255.0f, 1.0f);

    /** @brief The fill marking a row that stands on the path to the selected one. */
    FFloatColor AncestorFill = FFloatColor(44.0f / 255.0f, 50.0f / 255.0f, 58.0f / 255.0f, 1.0f);

    /** @brief The color of a row's own name. */
    FFloatColor LabelText = FFloatColor(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);

    /** @brief The color of the trailing column, which runs dimmer than the name. */
    FFloatColor SecondaryText = FFloatColor(122.0f / 255.0f, 122.0f / 255.0f, 122.0f / 255.0f, 1.0f);

    /** @brief The fill behind the run of a name a search matched. */
    FFloatColor SearchHighlight = FFloatColor(13.0f / 255.0f, 59.0f / 255.0f, 105.0f / 255.0f, 1.0f);

    /** @brief The tint of the disclosure arrow on a row that has children. */
    FFloatColor ArrowTint = FFloatColor(101.0f / 255.0f, 101.0f / 255.0f, 101.0f / 255.0f, 1.0f);

    /** @brief How far one level of depth moves a row's contents right, in pixels. */
    int32 IndentPerLevel = 18;
};

struct FUIStyleMetrics
{
    /** @brief The space between a control's bounds and its contents. */
    FMargin ControlPadding = FMargin(8, 4);

    /** @brief The space between a button's bounds and its label, which runs wider than a control's. */
    FMargin ButtonPadding = FMargin(12, 4);

    /** @brief How far a control's corners are rounded, in pixels. */
    float CornerRadius = 3.0f;

    /** @brief How far a button's corners are rounded, which is rounder than a control's. */
    float ButtonCornerRadius = 3.0f;

    /** @brief The width of a stroke around a control, in pixels. */
    float BorderThickness = 1.0f;

    /** @brief How tall one row in a tree, a list or a table is, in pixels. */
    int32 RowHeight = 24;

    /** @brief How tall a button is, which is taller than a row so a strip of them does not read as a list. */
    int32 ButtonHeight = 24;

    /** @brief How wide a scroll bar is across its short axis, in pixels. */
    int32 ScrollBarThickness = 12;

    /** @brief The width of the rule a separator draws, in pixels. */
    int32 SeparatorThickness = 1;

    /** @brief The width of the rule a menu separator draws, which runs heavier than an ordinary one. */
    int32 MenuSeparatorThickness = 2;
};

struct APPLICATION_API FUIStyle
{
    /**
     * @brief Gets the style every widget reads when it was not handed one of its own.
     *
     * @return The process-wide default, which is the shipped theme until SetDefault replaces it.
     */
    NODISCARD static const FUIStyle& GetDefault();

    /**
     * @brief Replaces the process-wide default style.
     *
     * @param InStyle The style to install.
     */
    static void SetDefault(const FUIStyle& InStyle);

    /** @brief Puts the process-wide default back to the shipped theme. */
    static void ResetDefault();

    /**
     * @brief Picks the control fill matching an interaction state.
     *
     * @param State The state the control is in.
     * @return The fill color for that state.
     */
    NODISCARD const FFloatColor& GetControlColor(EInteractionState State) const;

    /**
     * @brief Picks the button fill matching an interaction state, which runs lighter than a control's and
     * switches to the accent pair for a button that is showing itself as the chosen one of a set.
     *
     * @param State       The state the button is in.
     * @param bIsSelected True for the lit fill, which a toggle that is on and an open dropdown both want.
     * @return The fill color for that state.
     */
    NODISCARD const FFloatColor& GetButtonColor(EInteractionState State, bool bIsSelected) const;

    /**
     * @brief Picks the text color matching an interaction state, which only dims when disabled.
     *
     * @param State The state the control is in.
     * @return The text color for that state.
     */
    NODISCARD const FFloatColor& GetTextColor(EInteractionState State) const;

    /** @brief The palette every widget draws from. */
    FUIStyleColors Colors;

    /** @brief The sizes and spacings every widget lays out to. */
    FUIStyleMetrics Metrics;

    /** @brief The look of a collapsible section's header bar. */
    FUIHeaderStyle Header;

    /** @brief The look of a table of labels and their editors. */
    FUIPropertyTableStyle PropertyTable;

    /** @brief The look of a field that opens a list of values. */
    FUIComboBoxStyle ComboBox;

    /** @brief The look of one row in a tree or a list. */
    FUITreeRowStyle TreeRow;

    /** @brief The colors the three spatial axes are drawn in. */
    FUIAxisColors AxisColors;

    /** @brief The face nearly all text is drawn with, which is null until something installs one. */
    const IFontFace* NormalFont = nullptr;

    /** @brief The fixed-width face a console, a log or a number column is drawn with, or null for none. */
    const IFontFace* MonospaceFont = nullptr;
};

struct FInputFrameStyle
{
    /** @brief The fill behind the line of text. */
    FFloatColor Fill = FUIStyle::GetDefault().Colors.InputFieldFill;

    /** @brief The stroke around a field that is neither hovered nor focused. */
    FFloatColor BorderNormal = FUIStyle::GetDefault().Colors.InputFieldBorder;

    /** @brief The stroke the cursor resting over the field replaces the normal one with. */
    FFloatColor BorderHovered = FUIStyle::GetDefault().Colors.InputFieldBorderHovered;

    /** @brief The stroke a focused field carries, which is what marks where typing lands. */
    FFloatColor BorderFocused = FUIStyle::GetDefault().Colors.Accent;

    /** @brief The color of the text being edited. */
    FFloatColor Text = FUIStyle::GetDefault().Colors.Text;

    /** @brief The color of the hint shown in place of the text while the field is empty. */
    FFloatColor HintNormal = FUIStyle::GetDefault().Colors.TextDisabled;

    /** @brief The hint color of a focused field, which ImGui brightens slightly. */
    FFloatColor HintFocused = FUIStyle::GetDefault().Colors.TextDisabled;

    /** @brief The fill behind selected text. */
    FFloatColor Selection = FUIStyle::GetDefault().Colors.TextSelectionBackground;

    /** @brief The tint of an icon sitting inside a field that is neither hovered nor focused. */
    FFloatColor IconNormal = FUIStyle::GetDefault().Colors.TextDisabled;

    /** @brief The tint an icon takes once the field is focused or the cursor is resting over the icon. */
    FFloatColor IconFocused = FUIStyle::GetDefault().Colors.Text;

    /** @brief The width of the stroke in pixels, where zero draws no stroke. */
    float BorderThickness = FUIStyle::GetDefault().Metrics.BorderThickness;

    /** @brief How far the corners are rounded, in pixels, a large value giving ImGui's pill. */
    float CornerRadius = FUIStyle::GetDefault().Metrics.CornerRadius;
};
