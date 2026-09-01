#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/Paths.h"
#include "Application/Text/TrueTypeFontFace.h"

static FEditorFonts GFonts;
static FUIStyle     GStyle;

static constexpr int32 GBodyPixelHeight      = 18;
static constexpr int32 GTitlePixelHeight     = 18;
static constexpr int32 GMonospacePixelHeight = 14;

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
    GStyle.Colors.Accent                  = FromBytes(94, 94, 204);

    GStyle.Metrics.ControlPadding         = FMargin(10, 6, 10, 6);
    GStyle.Metrics.CornerRadius           = 4.0f;
    GStyle.Metrics.BorderThickness        = 1.0f;
    GStyle.Metrics.RowHeight              = RowHeight;
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
