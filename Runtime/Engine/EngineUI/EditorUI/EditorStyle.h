#pragma once
#include "Core/Containers/SharedPtr.h"
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
    static constexpr int32 FooterHeight = 32;

    /** @brief The height of one row in a tree, a list or a property table. */
    static constexpr int32 RowHeight = 22;

    /** @brief The edge length of an icon square in a tool bar or a tree row. */
    static constexpr int32 IconSize = 16;

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
};
