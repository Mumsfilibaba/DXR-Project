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
    FFloatColor WindowBackground = FFloatColor(0.09f, 0.09f, 0.09f, 1.0f);

    /**
     * @brief The fill behind a panel.
     *
     * This matches the window background so a shell reads as one backdrop with panels laid on it.
     * A panel's own surface is FUIPanelChromeStyle::Fill, which sits a step off this.
     */
    FFloatColor PanelBackground = FFloatColor(0.09f, 0.09f, 0.09f, 1.0f);

    /** @brief The fill of a control that is neither hovered nor pressed. */
    FFloatColor ControlNormal = FFloatColor(0.20f, 0.20f, 0.20f, 1.0f);

    /** @brief The fill a control lifts to while the cursor rests on it. */
    FFloatColor ControlHovered = FFloatColor(0.27f, 0.27f, 0.27f, 1.0f);

    /** @brief The fill a control takes while it is held down. */
    FFloatColor ControlPressed = FFloatColor(0.16f, 0.16f, 0.16f, 1.0f);

    /** @brief The fill of a control that cannot be interacted with. */
    FFloatColor ControlDisabled = FFloatColor(0.16f, 0.16f, 0.16f, 1.0f);

    /** @brief The fill of a button at rest, which runs lighter than a control's. */
    FFloatColor ButtonNormal = FFloatColor(0.22f, 0.22f, 0.22f, 1.0f);

    /** @brief The fill a button lifts to while the cursor rests on it. */
    FFloatColor ButtonHovered = FFloatColor(0.34f, 0.34f, 0.34f, 1.0f);

    /** @brief The fill a button takes while it is held down. */
    FFloatColor ButtonPressed = FFloatColor(0.34f, 0.34f, 0.34f, 1.0f);

    /** @brief The fill behind an editable line of text, which runs darker than anything around it. */
    FFloatColor InputFieldFill = FFloatColor(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);

    /** @brief The stroke around such a field at rest. */
    FFloatColor InputFieldBorder = FFloatColor(51.0f / 255.0f, 51.0f / 255.0f, 51.0f / 255.0f, 1.0f);

    /** @brief The stroke the cursor resting over such a field replaces the normal one with. */
    FFloatColor InputFieldBorderHovered = FFloatColor(74.0f / 255.0f, 74.0f / 255.0f, 74.0f / 255.0f, 1.0f);

    /** @brief The stroke around anything the other border colors do not name. */
    FFloatColor Border = FFloatColor(0.32f, 0.33f, 0.38f, 1.0f);

    /** @brief The fill a splitter handle takes once the cursor is over it, which is what answers the grab. */
    FFloatColor SeparatorHovered = FFloatColor(0.40f, 0.41f, 0.46f, 1.0f);

    /** @brief The color nearly all text is drawn in. */
    FFloatColor Text = FFloatColor(0.90f, 0.91f, 0.94f, 1.0f);

    /** @brief The color text dims to when what carries it cannot be interacted with. */
    FFloatColor TextDisabled = FFloatColor(0.45f, 0.46f, 0.50f, 1.0f);

    /** @brief The fill behind selected text. */
    FFloatColor TextSelectionBackground = FFloatColor(0.20f, 0.42f, 0.78f, 1.0f);

    /**
     * @brief The fill behind a run of text a search matched.
     *
     * A match can sit inside a selection, and the text keeps whatever color it was written in, so this is
     * held off the selection's hue and carries an alpha rather than covering what is under it.
     */
    FFloatColor SearchTextHighlight = FFloatColor(0.84f, 0.60f, 0.10f, 0.55f);

    /** @brief The color marking what the editor is pointed at, which is the focus ring and the chosen row. */
    FFloatColor Accent = FFloatColor(0.25f, 0.55f, 0.95f, 1.0f);

    /** @brief The accent lifted for a hover, so a selected control still answers the cursor. */
    FFloatColor AccentHovered = FFloatColor(0.35f, 0.63f, 0.98f, 1.0f);
};

struct FUIHeaderStyle
{
    /** @brief The fill of the whole nested panel, header and open body together. */
    FFloatColor Fill = FFloatColor(0.12f, 0.12f, 0.12f, 1.0f);

    /** @brief The tint of the arrow that shows which way the section is folded. */
    FFloatColor ArrowTint = FFloatColor(101.0f / 255.0f, 101.0f / 255.0f, 101.0f / 255.0f, 1.0f);

    /** @brief The one thin stroke around the nested panel. */
    FFloatColor Border = FFloatColor(0.19f, 0.19f, 0.19f, 1.0f);

    /** @brief Unused once the nested panel carries its own outline. Kept so older callers still compile. */
    FFloatColor BottomBorder = FFloatColor(0.19f, 0.19f, 0.19f, 1.0f);

    /** @brief The space between the header's edges and its label. */
    FMargin FramePadding = FMargin(10, 8, 10, 8);

    /** @brief How far the nested panel's corners are rounded, in pixels. */
    float CornerRadius = 8.0f;

    /** @brief How thick the nested panel's outline is, in pixels. */
    float BorderThickness = 1.0f;

    /** @brief How long an open or close takes, in seconds. Zero snaps. */
    float ExpandDuration = 0.15f;
};

struct FUIInnerFrameStyle
{
    /** @brief The fill of a primary view framed inside a docked panel. */
    FFloatColor Fill = FFloatColor(0.09f, 0.09f, 0.09f, 1.0f);

    /** @brief The thin stroke around that view. */
    FFloatColor Border = FFloatColor(0.19f, 0.19f, 0.19f, 1.0f);

    /** @brief How far the inner frame's corners are rounded, in pixels. */
    float CornerRadius = 6.0f;

    /** @brief How thick that stroke is, in pixels. */
    float BorderThickness = 1.0f;

    /** @brief The gutter between the stroke and the view. */
    FMargin Padding = FMargin(6);
};

struct FUIPropertyTableStyle
{
    /** @brief The fill behind a row. */
    FFloatColor RowFill = FFloatColor(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);

    /** @brief The fill behind every second row, which matches RowFill unless a caller wants banding. */
    FFloatColor AlternateRowFill = FFloatColor(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);

    /** @brief The fill behind the row the cursor rests on. */
    FFloatColor HoveredRowFill = FFloatColor(47.0f / 255.0f, 47.0f / 255.0f, 47.0f / 255.0f, 1.0f);

    /** @brief The rules between rows and between columns. */
    FFloatColor GridLine = FFloatColor(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);

    /** @brief The revert arrow on a row the cursor is away from. */
    FFloatColor RevertGlyph = FFloatColor(220.0f / 255.0f, 220.0f / 255.0f, 220.0f / 255.0f, 1.0f);

    /** @brief The revert arrow the cursor is over. */
    FFloatColor RevertGlyphHovered = FFloatColor(160.0f / 255.0f, 160.0f / 255.0f, 160.0f / 255.0f, 1.0f);

    /** @brief The revert arrow being pressed. */
    FFloatColor RevertGlyphPressed = FFloatColor(130.0f / 255.0f, 130.0f / 255.0f, 130.0f / 255.0f, 1.0f);

    /** @brief The space between a cell's edges and its contents. */
    FMargin CellPadding = FMargin(6, 4, 6, 4);

    /** @brief How far the leftmost label sits from the table's left edge, in pixels. */
    int32 LabelIndent = 6;

    /** @brief How wide the column holding the revert arrow is, in pixels. */
    int32 RevertColumnWidth = 40;
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

struct FUICheckBoxStyle
{
    /** @brief The fill inside the box, which stays dark whether or not the box is ticked. */
    FFloatColor Fill = FFloatColor(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);

    /** @brief The tick, and the dash an undetermined box carries in its place. */
    FFloatColor CheckMark = FFloatColor(166.0f / 255.0f, 166.0f / 255.0f, 166.0f / 255.0f, 1.0f);

    /** @brief The border of a box the cursor is away from. */
    FFloatColor Border = FFloatColor(60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f, 1.0f);

    /** @brief The border of a box the cursor is over. */
    FFloatColor BorderHovered = FFloatColor(100.0f / 255.0f, 100.0f / 255.0f, 100.0f / 255.0f, 1.0f);

    /** @brief The border of a box being pressed, which lights rather than lifting so the box reads as armed. */
    FFloatColor BorderPressed = FFloatColor(0.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);

    /** @brief How thick that border is drawn, in pixels. */
    float BorderThickness = 2.0f;
};

struct FUINumericEntryStyle
{
    /** @brief The fill from the field's left edge to the value, which is what makes a ranged field read as a slider. */
    FFloatColor TrackFill = FFloatColor(133.0f / 255.0f, 133.0f / 255.0f, 133.0f / 255.0f, 1.0f);
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
    FFloatColor AncestorFill = FFloatColor(47.0f / 255.0f, 47.0f / 255.0f, 47.0f / 255.0f, 1.0f);

    /** @brief The color of a row's own name. */
    FFloatColor LabelText = FFloatColor(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);

    /** @brief The color of the trailing column, which runs dimmer than the name. */
    FFloatColor SecondaryText = FFloatColor(122.0f / 255.0f, 122.0f / 255.0f, 122.0f / 255.0f, 1.0f);

    /**
     * @brief The name's color on a selected row, which has to carry against the selection fill rather
     * than against the row. The resting colors are dark greys picked to sit on a near-black row, and a
     * saturated fill under them leaves the text with almost no contrast left.
     */
    FFloatColor SelectedLabelText = FFloatColor(1.0f, 1.0f, 1.0f, 1.0f);

    /** @brief The trailing column's color on a selected row, dimmer than the name as it is at rest. */
    FFloatColor SelectedSecondaryText = FFloatColor(222.0f / 255.0f, 230.0f / 255.0f, 240.0f / 255.0f, 1.0f);

    /** @brief The fill behind the run of a name a search matched. */
    FFloatColor SearchHighlight = FFloatColor(139.0f / 255.0f, 194.0f / 255.0f, 74.0f / 255.0f, 1.0f);

    /** @brief The color that run is drawn in, which has to carry against the highlight rather than the row. */
    FFloatColor SearchHighlightText = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);

    /** @brief The tint of the disclosure arrow on a row that has children. */
    FFloatColor ArrowTint = FFloatColor(101.0f / 255.0f, 101.0f / 255.0f, 101.0f / 255.0f, 1.0f);

    /** @brief The radius used by hover and selection highlights. */
    float CornerRadius = 4.0f;

    /** @brief The fill behind the column captions, matching the shared header fill. */
    FFloatColor HeaderFill = FFloatColor(47.0f / 255.0f, 47.0f / 255.0f, 47.0f / 255.0f, 1.0f);

    /** @brief The rule under the column captions. */
    FFloatColor HeaderSeparator = FFloatColor(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);

    /** @brief How far one level of depth moves a row's contents right, in pixels. */
    int32 IndentPerLevel = 18;

    /** @brief How far the disclosure arrow sits in from the row's left edge, in pixels. */
    int32 ContentInset = 8;
};

struct FUIScrollBarStyle
{
    /** @brief The fill behind the whole bar. */
    FFloatColor Track = FFloatColor(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);

    /** @brief The fill of the thumb at rest. */
    FFloatColor Grab = FFloatColor(87.0f / 255.0f, 87.0f / 255.0f, 87.0f / 255.0f, 1.0f);

    /** @brief The fill of the thumb the cursor is over or dragging, which does not sink further on the press. */
    FFloatColor GrabActive = FFloatColor(127.0f / 255.0f, 127.0f / 255.0f, 127.0f / 255.0f, 1.0f);

    /** @brief How far the track's and the thumb's corners are rounded, in pixels. */
    float CornerRadius = 12.0f;

    /** @brief How long a bar that hides itself takes to come in once the cursor is over its view, in seconds. */
    float FadeInDuration = 0.1f;

    /** @brief How long it takes to fade away again once the cursor leaves, deliberately the slower of the two. */
    float FadeOutDuration = 0.4f;
};

struct FUIMenuBarStyle
{
    /** @brief The fill behind an entry the cursor rests on. */
    FFloatColor ItemHovered = FFloatColor(0.27f, 0.27f, 0.27f, 1.0f);

    /** @brief The fill behind the entry whose menu is open. */
    FFloatColor ItemActive = FFloatColor(0.34f, 0.34f, 0.34f, 1.0f);

    /** @brief The least height the strip takes, in pixels. */
    int32 Height = 38;

    /** @brief The space between an entry's bounds and its label. */
    FMargin ItemPadding = FMargin(10, 4, 10, 4);

    /** @brief How much shorter than the strip an entry stands, top and bottom, in pixels. */
    int32 ItemInset = 4;

    /** @brief The gap held between two adjacent entries, in pixels. */
    int32 ItemSpacing = 4;

    /** @brief How far the highlight's corners are rounded, in pixels. */
    float ItemCornerRadius = 6.0f;
};

struct FUIMenuStyle
{
    /** @brief The fill of an open menu. */
    FFloatColor Background = FFloatColor(0.13f, 0.13f, 0.16f, 1.0f);

    /** @brief The single stroke around an open menu. */
    FFloatColor Border = FFloatColor(0.32f, 0.33f, 0.38f, 1.0f);

    /** @brief The fill behind the entry the cursor rests on. */
    FFloatColor ItemHovered = FFloatColor(0.25f, 0.55f, 0.95f, 1.0f);

    /** @brief The color of the key combination shown at an entry's right edge. */
    FFloatColor ItemShortcut = FFloatColor(0.45f, 0.46f, 0.50f, 1.0f);

    /** @brief The rule between two runs of entries. */
    FFloatColor Separator = FFloatColor(106.0f / 255.0f, 106.0f / 255.0f, 106.0f / 255.0f, 1.0f);

    /** @brief The color of the heading naming a run of entries. */
    FFloatColor SectionText = FFloatColor(160.0f / 255.0f, 160.0f / 255.0f, 160.0f / 255.0f, 1.0f);

    /** @brief The height every row takes unless its face is taller than that, in pixels. */
    int32 RowHeight = 30;

    /** @brief The width a menu is held out to even when every row in it is narrower, in pixels. */
    int32 MinWidth = 226;

    /** @brief How far an entry's highlight is held back from either edge of the row, in pixels. */
    int32 ItemHighlightInset = 4;

    /** @brief How far the highlight's corners are rounded, in pixels. */
    float ItemCornerRadius = 4.0f;

    /** @brief How far the corners of the menu itself are rounded, in pixels. */
    float CornerRadius = 6.0f;
};

struct FUITabStyle
{
    /** @brief The fill of a resting tab, which is clear so the strip shows straight through it. */
    FFloatColor Fill = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);

    /** @brief The fill of a tab the cursor is over, which stays under the active tab's so that one still leads. */
    FFloatColor FillHovered = FFloatColor(37.0f / 255.0f, 37.0f / 255.0f, 37.0f / 255.0f, 1.0f);

    /**
     * @brief The fill of the active tab.
     *
     * No rule is drawn over it, so this is the whole of what marks the active tab out and has to stand
     * clear of the panel the strip is laid on.
     */
    FFloatColor FillActive = FFloatColor(52.0f / 255.0f, 52.0f / 255.0f, 52.0f / 255.0f, 1.0f);

    /**
     * @brief The fill of the strip behind the tabs, which shows wherever the tabs run out.
     *
     * Transparent by default so a strip at the top of a docked panel lets the panel's own rounded
     * fill through instead of squaring off its top corners.
     */
    FFloatColor StripFill = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);

    /**
     * @brief The rule along the top of the active tab, which is off by default.
     *
     * Setting ActiveStripThickness strokes this round the pill, bright on the top edge and fading over the
     * corners to ActiveStripTrailAlpha for the rest of it.
     */
    FFloatColor ActiveStrip = FFloatColor(0.0f, 122.0f / 255.0f, 204.0f / 255.0f, 1.0f);

    /** @brief The rule on a tab's trailing edge, which is off by default while the pills carry the separation. */
    FFloatColor Separator = FFloatColor(43.0f / 255.0f, 43.0f / 255.0f, 43.0f / 255.0f, 1.0f);

    /** @brief The fill behind the close cross while the cursor is on the cross itself. */
    FFloatColor CloseHovered = FFloatColor(70.0f / 255.0f, 70.0f / 255.0f, 70.0f / 255.0f, 1.0f);

    /** @brief How thick that rule is, in pixels, zero leaving the active tab to carry its fill alone. */
    int32 ActiveStripThickness = 0;

    /**
     * @brief How much of each of the pill's top corners that rule spends fading back, as a share of the arc.
     *
     * Half runs the accent at full strength until the middle of the corner and has it down to its trail by
     * the side edge.
     */
    float ActiveStripFadeFraction = 0.5f;

    /**
     * @brief What the accent keeps of its color once round the corner, as a share of it.
     *
     * The remainder rings the rest of the pill, so the active tab reads as a bright rule on top over a faint
     * border rather than as a rule that stops dead. Zero leaves the rule on its own.
     */
    float ActiveStripTrailAlpha = 0.28f;

    /** @brief The gap either side of a tab, in pixels, which is what parts one pill from the next. */
    int32 Spacing = 4;

    /** @brief How far a tab is inset from the top of the strip, in pixels. */
    int32 TopInset = 4;

    /** @brief How far a tab is inset from the bottom of the strip, in pixels. */
    int32 BottomInset = 4;

    /** @brief How far a tab's corners are rounded, in pixels, which is what makes it read as a pill. */
    float CornerRadius = 5.0f;

    /** @brief How far the close button's corners are rounded, matching the tab's own. */
    float CloseCornerRadius = 5.0f;

    /** @brief The height of the strip, in pixels, which the pill fills bar its two insets. */
    int32 StripHeight = 40;

    /** @brief The width a tab is held out to even when its label is shorter than that, in pixels. */
    int32 MinWidth = 140;

    /** @brief The space either side of a tab's label, in pixels. */
    int32 HorizontalPadding = 12;

    /** @brief How far the label is nudged off the tab's centre line, in pixels, negative being up. */
    int32 LabelOffsetY = 0;

    /** @brief The gap between a tab's label and its close button, in pixels. */
    int32 LabelCloseGap = 6;

    /** @brief The side of the square the close button fills, in pixels, which stays inside the pill. */
    int32 CloseSize = 22;

    /** @brief How far the close button is held off the tab's trailing edge, in pixels. */
    int32 CloseInset = 4;

    /** @brief The side of the glyph centred in that square, in pixels. */
    int32 CloseIconSize = 16;

    /** @brief The width of the rule between two tabs, which is off while the pills carry the separation. */
    int32 SeparatorThickness = 0;

    /** @brief How thick the scroll bar under the tabs is, in pixels. */
    int32 ScrollBarThickness = 3;

    /**
     * @brief How much clear space is kept between a pill and that bar, in pixels.
     *
     * The bottom inset covers what it can of the bar and this gap, and a strip that overflows takes the
     * rest off its pills. A strip everything fits in keeps them at full height, since it shows no bar.
     */
    int32 ScrollBarGap = 4;

    /** @brief How long the scroll bar takes to appear once the cursor is over the strip, in seconds. */
    float ScrollBarFadeInDuration = 0.1f;

    /** @brief How long it takes to fade away again once the cursor leaves, which is deliberately the slower of the two. */
    float ScrollBarFadeOutDuration = 0.6f;
};

struct FUIPanelChromeStyle
{
    /**
     * @brief The fill of a docked panel, which sits a single step off the window background
     * so a panel reads as a card laid on the backdrop rather than as a different surface.
     */
    FFloatColor Fill = FFloatColor(0.12f, 0.12f, 0.12f, 1.0f);

    /** @brief The one thin stroke around that card. */
    FFloatColor Border = FFloatColor(0.19f, 0.19f, 0.19f, 1.0f);

    /** @brief The stroke the card carries while it holds the focus. */
    FFloatColor BorderFocused = FFloatColor(0.25f, 0.55f, 0.95f, 1.0f);

    /** @brief How far the card's corners are rounded, in pixels. */
    float CornerRadius = 8.0f;

    /** @brief How thick that stroke is drawn, in pixels. */
    float BorderThickness = 1.0f;

    /**
     * @brief How much backdrop is left between two neighbouring cards, in pixels. This is also
     * the width of the splitter handle, since the gap is what a drag grabs.
     */
    int32 Gap = 6;
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

    /** @brief How far a fill drawn behind a run of text, a search match or a selection, is rounded, in pixels. */
    float TextHighlightCornerRadius = 3.0f;

    /**
     * @brief The gutter that fill is grown past the run it sits behind, in pixels. The rounding takes its
     * bite out of this gutter rather than out of the glyphs, which is why it is held above zero. It stays
     * narrow because a highlight sits flush against the text either side of it: a wider gutter would draw
     * the fill under the neighbouring glyphs, and a taller one would push it past the row that carries it.
     */
    FMargin TextHighlightPadding = FMargin(2, 1);

    /** @brief How tall one row in a tree, a list or a table is, in pixels. */
    int32 RowHeight = 24;

    /**
     * @brief How tall the frame of an editable control is, in pixels. This runs taller than a row
     * because a row only has to hold a line of text while a frame has to hold one with padding
     * around it.
     */
    int32 FrameHeight = 24;

    /** @brief How tall a button is, which is taller than a row so a strip of them does not read as a list. */
    int32 ButtonHeight = 24;

    /**
     * @brief The label every button drawing a word of its own is given at least the room for. A short
     * word like "Fit" would otherwise come out a chip beside a neighbour twice its width, so the
     * narrowest a labelled button gets is what one carrying this word measures, which is what makes a
     * row of them read as one strip. A button carrying content of its own is sized to that content
     * instead, which is what keeps an icon button square.
     *
     * Held as a word rather than as a pixel count because the room it takes is a property of the face
     * it is drawn with: a count picked against one UI font is wrong the moment that font changes size.
     */
    const CHAR* ButtonMinLabel = "Reset";

    /**
     * @brief Works out how narrow a labelled button may be, which is the room ButtonMinLabel takes at
     * the button's own face with the same padding it puts around a label of its own.
     *
     * @param Face    The face the label is drawn with, or null for a button with none.
     * @param Padding The space between the button's bounds and its label.
     * @return The floor in pixels, which is zero for a button with no face to measure against.
     */
    NODISCARD int32 ResolveButtonMinWidth(const IFontFace* Face, const FMargin& Padding) const;

    /** @brief How wide a scroll bar is across its short axis, in pixels. */
    int32 ScrollBarThickness = 12;

    /** @brief The width of the rule a separator draws, in pixels. */
    int32 SeparatorThickness = 1;

    /** @brief The width of the rule a menu separator draws, in pixels. */
    int32 MenuSeparatorThickness = 1;

    /** @brief How thick the pill hinting at a grabbable splitter handle is, in pixels. */
    int32 SplitterHintThickness = 4;

    /**
     * @brief How far the contents of a title bar are held off its leading edge, in pixels.
     *
     * This is a floor rather than the whole inset, so a platform that reserves more of the leading edge
     * for its own caption buttons still gets what it asked for.
     */
    int32 TitleBarLeadingInset = 8;
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

    /** @brief The look of the card a docked panel is drawn as. */
    FUIPanelChromeStyle Panel;

    /** @brief The look of a primary view framed inside a docked panel. */
    FUIInnerFrameStyle InnerFrame;

    /** @brief The look of a collapsible nested panel. */
    FUIHeaderStyle Header;

    /** @brief The look of a table of labels and their editors. */
    FUIPropertyTableStyle PropertyTable;

    /** @brief The look of a field that opens a list of values. */
    FUIComboBoxStyle ComboBox;

    /** @brief The look of a numeric field, including the track a ranged one fills. */
    FUINumericEntryStyle NumericEntry;

    /** @brief The look of a box that is ticked. */
    FUICheckBoxStyle CheckBox;

    /** @brief The look of one row in a tree or a list. */
    FUITreeRowStyle TreeRow;

    /** @brief The look of the bar a scrollable view puts along its edge. */
    FUIScrollBarStyle ScrollBar;

    /** @brief The look of the tabs a dock node puts along its top. */
    FUITabStyle Tab;

    /** @brief The look of the strip of drop-down titles along the top of the window. */
    FUIMenuBarStyle MenuBar;

    /** @brief The look of an open drop-down or context menu. */
    FUIMenuStyle Menu;

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

    /** @brief The pill drawn behind the clear button while the cursor rests on the button itself. */
    FFloatColor ClearHovered = FUIStyle::GetDefault().Colors.ControlHovered;

    /** @brief The width of the stroke in pixels, where zero draws no stroke. */
    float BorderThickness = FUIStyle::GetDefault().Metrics.BorderThickness;

    /** @brief How far the corners are rounded, in pixels. */
    float CornerRadius = FUIStyle::GetDefault().Metrics.CornerRadius;

    /** @brief How far the clear button's hover pill is rounded, in pixels. */
    float ClearCornerRadius = 4.0f;
};
