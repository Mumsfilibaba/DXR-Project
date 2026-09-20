#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Application/Elements/Expander.h"
#include "Application/Elements/PropertyTable.h"
#include "Application/Elements/SearchBox.h"
#include "Application/Elements/TreeView.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

class FTextBlock;

struct FEditorFonts
{
    /** @brief The face nearly everything is drawn with. */
    TSharedPtr<IFontFace> Body;

    /** @brief The body face at the same size but bold, for a section header or an active tab. */
    TSharedPtr<IFontFace> BodyBold;

    /** @brief The larger face the window caption is drawn with. */
    TSharedPtr<IFontFace> Title;

    /** @brief The fixed-width face the console, the log and any number column are drawn with. */
    TSharedPtr<IFontFace> Monospace;
};

struct ENGINE_API FEditorStyle
{
    /** @brief The height of the window caption, matching what the ImGui title bar reserves. */
    static constexpr int32 TitleBarHeight = 36;

    /** @brief The height of the console strip along the bottom of the window. */
    static constexpr int32 FooterHeight = 40;

    /** @brief The height of one row in a tree, a list or a property table. */
    static constexpr int32 RowHeight = 24;

    /** @brief The height of an input field's frame, which is the body font's 20px plus ImGui's 6px above and below. */
    static constexpr int32 FrameHeight = 32;

    /** @brief The height of a button, which is the body font's 20px plus ImGui's 4px above and below. */
    static constexpr int32 ButtonHeight = 28;

    /** @brief The edge length of an icon square in a tool bar or a tree row. */
    static constexpr int32 IconSize = 16;

    /** @brief The inset held around a docked panel's content, in pixels. */
    static constexpr int32 PanelPadding = 8;

    /** @brief The gap between a panel's header band and the body under it, in pixels. */
    static constexpr int32 ItemSpacing = 6;

    /** @brief How wide a rich tool tip is allowed to grow, in pixels. */
    static constexpr int32 ToolTipMaxWidth = 420;

    /** @brief The width text wraps at inside a rich tool tip, which is its width less the frame around it. */
    static constexpr int32 ToolTipTextWidth = 400;

    /**
     * @brief Loads the faces and installs the editor palette as the process-wide default style.
     *
     * @return True when every face loaded.
     */
    static bool Initialize();

    /** @brief Drops the faces and puts the default style back. */
    static void Release();

    /** @return The faces, which are null until Initialize has succeeded. */
    NODISCARD static const FEditorFonts& GetFonts();

    /** @return The palette and metrics the editor installed, which is the process-wide default. */
    NODISCARD static const FUIStyle& GetStyle();

    /** @return The fill of the console strip along the bottom of the window. */
    NODISCARD static FFloatColor GetFooterColor();

    /** @return The fill of the completion list the console field opens above itself. */
    NODISCARD static FFloatColor GetCandidateListColor();

    /** @return The stroke around the completion list, which runs lighter than its fill. */
    NODISCARD static FFloatColor GetCandidateListBorderColor();

    /** @return The color of a completion row that is not the selected one, which reads white. */
    NODISCARD static FFloatColor GetCandidateTextColor();

    /** @return The fill of the completion row the arrow keys landed on. */
    NODISCARD static FFloatColor GetCandidateSelectionColor();

    /** @return The fill behind the run of a completion name the typed word matched. */
    NODISCARD static FFloatColor GetCandidateHighlightColor();

    /** @return The fill of a rich tool tip, which is the fill an open menu takes so the two read alike. */
    NODISCARD static FFloatColor GetToolTipColor();

    /** @return The stroke around a rich tool tip, which is the stroke an open menu takes. */
    NODISCARD static FFloatColor GetToolTipBorderColor();

    /**
     * @brief Wraps content in the frame every rich tool tip draws, so the fill, the rounding, the stroke
     * and the padding are set here rather than repeated at every call site.
     *
     * @param Content The element to frame.
     * @return The frame, ready to hand to FToolTipService::RequestToolTip.
     */
    NODISCARD static TSharedPtr<FVisualElement> MakeToolTipFrame(const TSharedPtr<FVisualElement>& Content);

    /**
     * @brief Wraps a primary view in the thin rounded inner frame used inside docked panels.
     *
     * @param Content The view to frame.
     * @return The frame, ready to slot under a toolbar.
     */
    NODISCARD static TSharedPtr<FVisualElement> MakeInnerFrame(const TSharedPtr<FVisualElement>& Content);

    /**
     * @brief Builds an inner frame with an explicit gutter, so a view can sit flush against the stroke.
     *
     * @param Content The element inside the frame.
     * @param Padding The gutter between the stroke and the content.
     */
    NODISCARD static TSharedPtr<FVisualElement> MakeInnerFrame(const TSharedPtr<FVisualElement>& Content, const FMargin& Padding);

    /**
     * @brief Builds the card an information panel names itself with, a bold line over a dimmed one in the
     * header fill, so About, RHI Info and Engine Stats all open the same way.
     *
     * @param Title            The bold line, which is what the panel is showing.
     * @param Subtitle         The dimmed line under it, which qualifies the title.
     * @param OutSubtitleText  Filled with the dimmed line's block when the caller has to rewrite it later.
     * @return The card, ready to slot above the panel's scroll view.
     */
    NODISCARD static TSharedPtr<FVisualElement> MakeHeaderCard(const String& Title, const String& Subtitle, TSharedPtr<FTextBlock>* OutSubtitleText = nullptr);

    /** @return The frame every editor search field draws, rounded far enough to read as a pill. */
    NODISCARD static FInputFrameStyle GetInputFrameStyle();

    /** @return The frame the footer command line draws, which is the search field's at a squarer corner. */
    NODISCARD static FInputFrameStyle GetConsoleInputFrameStyle();

    /**
     * @brief Builds the description an editor search field is created from, so the face, the icons, the icon
     * size, the padding and the frame are set here rather than repeated at every call site.
     *
     * @param Hint      Drawn in place of the text while the field is empty.
     * @param OnChanged Fired whenever the search text changes.
     * @return The description, ready to hand to FSearchBox::Create.
     */
    NODISCARD static FSearchBox::FDesc MakeSearchBoxDesc(const String& Hint, const FOnSearchTextChanged& OnChanged);

    /**
     * @brief The space a collapsible section reserves above and below itself.
     *
     * Every section carries its own fill and outline, so a stack of them needs this gap to keep one
     * header off the next and off the edge of the view holding them.
     *
     * @return The padding, ready to hand to FBoxSlot::SetPadding.
     */
    NODISCARD static FMargin GetSectionSpacing();

    /**
     * @brief The gap held between collapsible sections that stack directly on top of each other.
     *
     * Panels that are nothing but a list of headers read better with a tighter, even rhythm than
     * GetSectionSpacing gives. The gap sits below each section so the first header still lines up
     * with whatever sits above the stack.
     *
     * @return The padding, ready to hand to FBoxSlot::SetPadding.
     */
    NODISCARD static FMargin GetSectionStackSpacing();

    /**
     * @brief Builds the description a collapsible editor section is created from. The arrow brushes come
     * from the icon atlas, which is built after the style is installed and so cannot be part of it.
     *
     * @param Label       The text the header shows.
     * @param Content     The element shown below the header while the section is open.
     * @param bIsExpanded True to start with the content shown.
     * @return The description, ready to hand to FExpander::Create.
     */
    NODISCARD static FExpander::FDesc MakeExpanderDesc(const String& Label, const TSharedPtr<FVisualElement>& Content, bool bIsExpanded = false);

    /**
     * @brief Builds the description an editor property table is created from, so the face, the revert
     * icon and the revert column are set here rather than repeated at every call site.
     *
     * @param LabelColumnFraction The share of the width the label column takes.
     * @param LabelColumnWidth    How wide that column is held instead, in pixels, zero leaving it to the fraction.
     * @return The description, ready to hand to FPropertyTable::Create.
     */
    NODISCARD static FPropertyTable::FDesc MakePropertyTableDesc(float LabelColumnFraction, int32 LabelColumnWidth = 0);

    /**
     * @brief Builds the description a read-only editor table is created from. Nothing in one is editable,
     * so it keeps no revert column and alternates its rows instead, which is what makes a long run of
     * counters readable.
     *
     * @return The description, ready to hand to FPropertyTable::Create.
     */
    NODISCARD static FPropertyTable::FDesc MakeDataTableDesc();

    /**
     * @brief Builds the description the read-only tables in an information panel are created from, which is
     * the data table held to a fixed label column so every value lines up down the panel and across panels.
     *
     * @return The description, ready to hand to FPropertyTable::Create.
     */
    NODISCARD static FPropertyTable::FDesc MakeInfoTableDesc();

    /**
     * @brief Fills in the arrow brushes a tree's rows draw their disclosure with, which the icon atlas
     * holds and the style cannot.
     *
     * @param OutDesc The description to fill, whose other fields are left alone.
     */
    static void ApplyTreeViewArrows(FTreeView::FDesc& OutDesc);
};
