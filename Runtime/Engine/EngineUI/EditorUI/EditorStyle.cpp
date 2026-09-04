#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Engine/EngineUI/EditorUI/EditorIcons.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/Paths.h"
#include "Application/Elements/Border.h"
#include "Application/Text/TrueTypeFontFace.h"

static FEditorFonts GFonts;
static FUIStyle     GStyle;

static constexpr int32 GBodyPixelHeight      = 18;
static constexpr int32 GTitlePixelHeight     = 18;
static constexpr int32 GMonospacePixelHeight = 14;

/** @brief The width of the stroke around an input field, in pixels, which runs heavier than a panel's. */
static constexpr float GInputFieldBorderThickness = 2.0f;

/** @brief How far a search field's corners are rounded, in pixels, which is far enough to close into a pill. */
static constexpr float GSearchFieldCornerRadius = 16.0f;

/** @brief How far the console field's corners are rounded, in pixels, which stays square enough to read as a line. */
static constexpr float GConsoleFieldCornerRadius = 4.0f;

/** @brief The space between an input field's left or right edge and its contents, in pixels. */
static constexpr int32 GInputFieldPaddingX = 12;

/** @brief The space between an input field's top or bottom edge and its contents, in pixels. */
static constexpr int32 GInputFieldPaddingY = 6;

/** @brief The width of the stroke around a rich tool tip, in pixels. */
static constexpr float GToolTipBorderThickness = 2.0f;

/** @brief The space between a rich tool tip's edges and its contents, in pixels, matching ImGui's window padding. */
static constexpr int32 GToolTipPadding = 4;

static FFloatColor FromBytes(int32 R, int32 G, int32 B)
{
    return FFloatColor(R / 255.0f, G / 255.0f, B / 255.0f, 1.0f);
}

static TSharedPtr<IFontFace> LoadFace(const CHAR* Filename, int32 PixelHeight)
{
    const String Path = Paths::GetAssetDir() + "/Editor/Fonts/" + Filename;

    TSharedPtr<FTrueTypeFontFace> Face = FTrueTypeFontFace::CreateFromFile(Path, PixelHeight);
    if (!Face)
    {
        LOG_ERROR("[FEditorStyle]: Failed to load '%s'", *Path);
        return nullptr;
    }

    return Face;
}

bool FEditorStyle::Initialize()
{
    GFonts.Body      = LoadFace("segoeui.ttf", GBodyPixelHeight);
    GFonts.BodyBold  = LoadFace("segoeuib.ttf", GBodyPixelHeight);
    GFonts.Title     = LoadFace("segoeui.ttf", GTitlePixelHeight);
    GFonts.Monospace = LoadFace("consola.ttf", GMonospacePixelHeight);

    if (!GFonts.Body || !GFonts.BodyBold || !GFonts.Title || !GFonts.Monospace)
    {
        Release();
        return false;
    }

    GStyle.Colors.WindowBackground        = FromBytes(36, 36, 36);
    GStyle.Colors.PanelBackground         = FromBytes(30, 30, 31);
    GStyle.Colors.ControlNormal           = FromBytes(51, 51, 55);
    GStyle.Colors.ControlHovered          = FromBytes(66, 66, 72);
    GStyle.Colors.ControlPressed          = FromBytes(43, 43, 48);
    GStyle.Colors.ControlDisabled         = FromBytes(41, 41, 44);
    GStyle.Colors.ButtonNormal            = FromBytes(56, 56, 56);
    GStyle.Colors.ButtonHovered           = FromBytes(87, 87, 87);
    GStyle.Colors.ButtonPressed           = FromBytes(87, 87, 87);
    GStyle.Colors.MenuBarItemHovered      = FromBytes(87, 87, 87);
    GStyle.Colors.MenuBarItemActive       = FromBytes(9, 92, 176);
    GStyle.Colors.MenuBackground          = FromBytes(56, 56, 56);
    GStyle.Colors.MenuBorder              = FromBytes(63, 63, 63);
    GStyle.Colors.MenuInnerBorder         = FromBytes(50, 50, 50);
    GStyle.Colors.MenuItemHovered         = FromBytes(0, 112, 224);
    GStyle.Colors.MenuItemShortcut        = FromBytes(175, 175, 175);
    GStyle.Colors.MenuSeparator           = FromBytes(106, 106, 106);
    GStyle.Colors.MenuSectionText         = FromBytes(160, 160, 160);
    GStyle.Colors.Border                  = FromBytes(21, 21, 21);
    GStyle.Colors.Text                    = FromBytes(230, 230, 232);
    GStyle.Colors.TextDisabled            = FromBytes(115, 117, 122);
    GStyle.Colors.TextSelectionBackground = FromBytes(51, 107, 199);
    GStyle.Colors.Accent                  = FromBytes(9, 92, 176);
    GStyle.Colors.AccentHovered           = FromBytes(15, 110, 205);

    GStyle.Metrics.ControlPadding         = FMargin(10, 6, 10, 6);
    GStyle.Metrics.ButtonPadding          = FMargin(12, 4, 12, 4);
    GStyle.Metrics.CornerRadius           = 4.0f;
    GStyle.Metrics.ButtonCornerRadius     = 6.0f;
    GStyle.Metrics.BorderThickness        = 1.0f;
    GStyle.Metrics.RowHeight              = RowHeight;
    GStyle.Metrics.ButtonHeight           = ButtonHeight;
    GStyle.Metrics.ScrollBarThickness     = 16;
    GStyle.Metrics.SeparatorThickness     = 1;
    GStyle.Metrics.MenuSeparatorThickness = 2;

    GStyle.NormalFont    = GFonts.Body.Get();
    GStyle.MonospaceFont = GFonts.Monospace.Get();

    FUIStyle::SetDefault(GStyle);
    return true;
}

void FEditorStyle::Release()
{
    FUIStyle::ResetDefault();

    GFonts.Body.Reset();
    GFonts.BodyBold.Reset();
    GFonts.Title.Reset();
    GFonts.Monospace.Reset();

    GStyle = FUIStyle();
}

const FEditorFonts& FEditorStyle::GetFonts()
{
    return GFonts;
}

const FUIStyle& FEditorStyle::GetStyle()
{
    return FUIStyle::GetDefault();
}

FFloatColor FEditorStyle::GetSeamColor()
{
    return FromBytes(21, 21, 21);
}

FFloatColor FEditorStyle::GetInactiveTabColor()
{
    return FromBytes(26, 26, 27);
}

FFloatColor FEditorStyle::GetSelectionColor()
{
    return FromBytes(51, 76, 122);
}

FFloatColor FEditorStyle::GetAlternateRowColor()
{
    return FromBytes(34, 34, 36);
}

FFloatColor FEditorStyle::GetFooterColor()
{
    return FromBytes(36, 36, 36);
}

FFloatColor FEditorStyle::GetCandidateListColor()
{
    return FromBytes(26, 26, 26);
}

FFloatColor FEditorStyle::GetCandidateListBorderColor()
{
    return FromBytes(56, 56, 56);
}

FFloatColor FEditorStyle::GetCandidateTextColor()
{
    return FromBytes(192, 192, 192);
}

FFloatColor FEditorStyle::GetCandidateSelectionColor()
{
    return FromBytes(64, 87, 111);
}

FFloatColor FEditorStyle::GetCandidateHighlightColor()
{
    return FromBytes(139, 194, 74);
}

FFloatColor FEditorStyle::GetToolTipColor()
{
    return FromBytes(56, 56, 56);
}

FFloatColor FEditorStyle::GetToolTipBorderColor()
{
    return FromBytes(71, 71, 71);
}

TSharedPtr<FVisualElement> FEditorStyle::MakeToolTipFrame(const TSharedPtr<FVisualElement>& Content)
{
    FBorder::FDesc FrameDesc;
    FrameDesc.BackgroundColor = GetToolTipColor();
    FrameDesc.BorderColor     = GetToolTipBorderColor();
    FrameDesc.BorderThickness = GToolTipBorderThickness;
    FrameDesc.Padding         = FMargin(GToolTipPadding);
    FrameDesc.Content         = Content;

    return FBorder::Create(FrameDesc);
}

FInputFrameStyle FEditorStyle::GetInputFrameStyle()
{
    FInputFrameStyle FrameStyle;
    FrameStyle.Fill            = FromBytes(15, 15, 15);
    FrameStyle.BorderNormal    = FromBytes(51, 51, 51);
    FrameStyle.BorderHovered   = FromBytes(74, 74, 74);
    FrameStyle.BorderFocused   = FromBytes(9, 92, 176);
    FrameStyle.Text            = FFloatColor::White;
    FrameStyle.HintNormal      = FromBytes(76, 76, 76);
    FrameStyle.HintFocused     = FromBytes(97, 97, 97);
    FrameStyle.Selection       = FromBytes(0, 112, 224);
    FrameStyle.IconNormal      = FromBytes(192, 192, 192);
    FrameStyle.IconFocused     = FFloatColor::White;
    FrameStyle.BorderThickness = GInputFieldBorderThickness;
    FrameStyle.CornerRadius    = GSearchFieldCornerRadius;
    return FrameStyle;
}

FInputFrameStyle FEditorStyle::GetConsoleInputFrameStyle()
{
    FInputFrameStyle FrameStyle = GetInputFrameStyle();
    FrameStyle.CornerRadius     = GConsoleFieldCornerRadius;
    return FrameStyle;
}

FSearchBox::FDesc FEditorStyle::MakeSearchBoxDesc(const String& Hint, const FOnSearchTextChanged& OnChanged)
{
    FSearchBox::FDesc Desc;
    Desc.HintText      = Hint;
    Desc.Font          = GFonts.Body;
    Desc.SearchIcon    = FEditorIcons::Search;
    Desc.ClearIcon     = FEditorIcons::Close;
    Desc.IconSize      = IconSize;
    Desc.Padding       = FMargin(GInputFieldPaddingX, GInputFieldPaddingY);
    Desc.Style         = GetInputFrameStyle();
    Desc.OnTextChanged = OnChanged;
    return Desc;
}
