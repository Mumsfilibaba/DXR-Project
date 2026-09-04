#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Application/Elements/SearchBox.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

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
    static constexpr int32 RowHeight = 22;

    /** @brief The height of a button, which is the body font's 18px plus ImGui's 4px above and below. */
    static constexpr int32 ButtonHeight = 26;

    /** @brief The edge length of an icon square in a tool bar or a tree row. */
    static constexpr int32 IconSize = 16;

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

    /** @return The fill behind a dock tab strip and the seams between panels. */
    NODISCARD static FFloatColor GetSeamColor();

    /** @return The fill of a tab that is not the active one. */
    NODISCARD static FFloatColor GetInactiveTabColor();

    /** @return The fill of a row a selection covers. */
    NODISCARD static FFloatColor GetSelectionColor();

    /** @return The fill of every second row in a table, which is what makes long lists readable. */
    NODISCARD static FFloatColor GetAlternateRowColor();

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

    /** @return The fill of a rich tool tip, which reads lighter than the list it is raised above. */
    NODISCARD static FFloatColor GetToolTipColor();

    /** @return The stroke around a rich tool tip. */
    NODISCARD static FFloatColor GetToolTipBorderColor();

    /**
     * @brief Wraps content in the frame every rich tool tip draws, so the fill, the stroke and the padding
     * are set here rather than repeated at every call site.
     *
     * @param Content The element to frame.
     * @return The frame, ready to hand to FToolTipService::RequestToolTip.
     */
    NODISCARD static TSharedPtr<FVisualElement> MakeToolTipFrame(const TSharedPtr<FVisualElement>& Content);

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
};
