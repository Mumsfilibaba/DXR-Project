#include <Core/Math/Math.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/ScrollBox.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Style/UIStyle.h>

#include "DrawCanvas.h"
#include "PlaygroundScene.h"
#include "ScenePanel.h"

static constexpr int32 GSwatchSize    = 72;
static constexpr int32 GSwatchSpacing = 12;

static FRectangle SwatchAt(const FRectangle& Bounds, int32 Column, int32 Size = GSwatchSize)
{
    FRectangle Swatch;
    Swatch.Position.X = Bounds.Position.X + (Column * (GSwatchSize + GSwatchSpacing));
    Swatch.Position.Y = Bounds.Position.Y;
    Swatch.Width      = Size;
    Swatch.Height     = Size;
    return Swatch;
}

static int32 DrawCornerRadii(const FDrawGeometry& Geometry, FDrawCommandList& CommandList, int32 LayerId)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    const FCornerRadii Radii[] =
    {
        FCornerRadii(0.0f),
        FCornerRadii(4.0f),
        FCornerRadii(12.0f),
        FCornerRadii(GSwatchSize * 0.5f),
        FCornerRadii::Top(16.0f),
        FCornerRadii::Bottom(16.0f),
        FCornerRadii(20.0f, 0.0f, 20.0f, 0.0f),
    };

    for (int32 Index = 0; Index < static_cast<int32>(ARRAY_COUNT(Radii)); ++Index)
    {
        CommandList.AddBox(LayerId, SwatchAt(Geometry.Bounds, Index), Style.Colors.ControlNormal, Radii[Index]);
    }

    return LayerId;
}

static int32 DrawOutlines(const FDrawGeometry& Geometry, FDrawCommandList& CommandList, int32 LayerId)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    for (int32 Index = 0; Index < 4; ++Index)
    {
        const float Thickness = static_cast<float>(Index + 1);
        CommandList.AddBoxOutline(LayerId, SwatchAt(Geometry.Bounds, Index), Style.Colors.Accent, Thickness);
    }

    for (int32 Index = 0; Index < 3; ++Index)
    {
        const float Thickness = static_cast<float>((Index * 2) + 1);
        CommandList.AddBoxOutline(LayerId, SwatchAt(Geometry.Bounds, Index + 4), Style.Colors.Accent, Thickness, 14.0f);
    }

    return LayerId;
}

static int32 DrawLayers(const FDrawGeometry& Geometry, FDrawCommandList& CommandList, int32 LayerId)
{
    const FFloatColor Colors[] =
    {
        FFloatColor(0.85f, 0.28f, 0.28f, 1.0f),
        FFloatColor(0.28f, 0.72f, 0.38f, 1.0f),
        FFloatColor(0.30f, 0.52f, 0.92f, 1.0f),
        FFloatColor(0.92f, 0.78f, 0.26f, 1.0f),
    };

    const int32 NumColors = static_cast<int32>(ARRAY_COUNT(Colors));
    for (int32 Index = 0; Index < NumColors; ++Index)
    {
        FRectangle Box = SwatchAt(Geometry.Bounds, 0);
        Box.Position.X += Index * (GSwatchSize / 3);
        Box.Position.Y += Index * (GSwatchSize / 6);

        CommandList.AddBox(LayerId + Index, Box, Colors[Index], 6.0f);
    }

    return LayerId + NumColors;
}

static int32 DrawClipping(const FDrawGeometry& Geometry, FDrawCommandList& CommandList, int32 LayerId)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    FRectangle ClipRegion = Geometry.Bounds;
    ClipRegion.Width      = Math::Min(ClipRegion.Width, 220);
    ClipRegion.Height     = Math::Min(ClipRegion.Height, GSwatchSize + 24);

    CommandList.AddBox(LayerId, ClipRegion, Style.Colors.ControlNormal, 8.0f);
    CommandList.PushClip(LayerId, ClipRegion);

    const Vector2 Center(
        static_cast<float>(ClipRegion.Position.X + ClipRegion.Width),
        static_cast<float>(ClipRegion.Position.Y + (ClipRegion.Height / 2)));

    CommandList.AddCircleFilled(LayerId + 1, Center, 64.0f, Style.Colors.Accent);

    for (int32 Index = 0; Index < 9; ++Index)
    {
        const float Angle = static_cast<float>(Index) * (Math::Constants::PI / 8.0f);
        const Vector2 Direction(Math::Cos(Angle), Math::Sin(Angle));
        const Vector2 Start = Center - (Direction * 220.0f);
        const Vector2 End   = Center + (Direction * 220.0f);

        CommandList.AddLine(LayerId + 2, Start, End, Style.Colors.Text, 1.0f);
    }

    CommandList.PopClip(LayerId + 2);
    return LayerId + 2;
}

static TSharedPtr<FVisualElement> MakeCanvas(int32 Height, const FOnCanvasDraw& OnDraw)
{
    FDrawCanvas::FDesc CanvasDesc;
    CanvasDesc.DesiredSize  = IntVector2(0, Height);
    CanvasDesc.OnCanvasDraw = OnDraw;
    return FDrawCanvas::Create(CanvasDesc);
}

static TSharedPtr<FVisualElement> MakeNestedBorders(const FPlaygroundFonts& Fonts)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    FTextBlock::FDesc LabelDesc;
    LabelDesc.Text            = "three borders deep";
    LabelDesc.Font            = Fonts.Body;
    LabelDesc.ColorAndOpacity = Style.Colors.Text;

    FBorder::FDesc InnerDesc;
    InnerDesc.BackgroundColor = Style.Colors.ControlPressed;
    InnerDesc.Padding         = FMargin(10, 6, 10, 6);
    InnerDesc.CornerRadius    = 4.0f;
    InnerDesc.Content         = FTextBlock::Create(LabelDesc);

    FBorder::FDesc MiddleDesc;
    MiddleDesc.BackgroundColor = Style.Colors.ControlNormal;
    MiddleDesc.Padding         = FMargin(10);
    MiddleDesc.CornerRadius    = 8.0f;
    MiddleDesc.Content         = FBorder::Create(InnerDesc);

    FBorder::FDesc OuterDesc;
    OuterDesc.BackgroundColor = Style.Colors.Accent;
    OuterDesc.Padding         = FMargin(10);
    OuterDesc.CornerRadius    = 12.0f;
    OuterDesc.Content         = FBorder::Create(MiddleDesc);

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(FBorder::Create(OuterDesc)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Row;
}

static TSharedPtr<FVisualElement> MakeScrollDemo(const FPlaygroundFonts& Fonts)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    TSharedPtr<FVerticalBox> Rows = FVerticalBox::Create();
    for (int32 RowIndex = 0; RowIndex < 24; ++RowIndex)
    {
        FTextBlock::FDesc RowDesc;
        RowDesc.Text            = String::Printf("Row %02d - scroll with the wheel, the clip is the scroll box's own", RowIndex);
        RowDesc.Font            = Fonts.Monospace;
        RowDesc.ColorAndOpacity = (RowIndex % 2) == 0 ? Style.Colors.Text : Style.Colors.TextDisabled;

        Rows->AddSlot(FTextBlock::Create(RowDesc))
            .SetPadding(FMargin(8, 2, 8, 2))
            .SetHorizontalAlignment(EHorizontalAlignment::Left);
    }

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    ScrollBox->SetContent(Rows);

    FBorder::FDesc ViewportDesc;
    ViewportDesc.BackgroundColor = Style.Colors.WindowBackground;
    ViewportDesc.Padding         = FMargin(4);
    ViewportDesc.CornerRadius    = 4.0f;
    ViewportDesc.MinHeight       = 150;
    ViewportDesc.Content         = ScrollBox;

    return FBorder::Create(ViewportDesc);
}

FPlaygroundScene CreateFoundationsScene(const FPlaygroundFonts& Fonts)
{
    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Rounded boxes";
        Desc.Description = "Uniform radii, then the top-only and bottom-only factories, then two opposite corners.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeCanvas(GSwatchSize, FOnCanvasDraw::CreateStatic(&DrawCornerRadii));
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Outlines";
        Desc.Description = "Stroked inward from the edge at one to four pixels, square then rounded.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeCanvas(GSwatchSize, FOnCanvasDraw::CreateStatic(&DrawOutlines));
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Borders and padding";
        Desc.Description = "Real elements rather than raw commands, so this is the measure and arrange path.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeNestedBorders(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Layers";
        Desc.Description = "Four boxes, each one layer above the last. Yellow on top means the ordering held.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeCanvas(GSwatchSize + 40, FOnCanvasDraw::CreateStatic(&DrawLayers));
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Clipping";
        Desc.Description = "A circle and nine lines that all overrun the region they are clipped to.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeCanvas(GSwatchSize + 24, FOnCanvasDraw::CreateStatic(&DrawClipping));
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Scrolling";
        Desc.Description = "A scroll box clipping a column taller than the space it was given.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeScrollDemo(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene("Foundations", MakeSceneColumn("Foundations", Fonts, Panels));
}
