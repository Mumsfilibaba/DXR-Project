#include <Core/Containers/Array.h>
#include <Core/Math/Math.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Style/UIStyle.h>

#include "DrawCanvas.h"
#include "PlaygroundScene.h"
#include "ScenePanel.h"

static Vector2 TopLeftOf(const FRectangle& Bounds)
{
    return Vector2(static_cast<float>(Bounds.Position.X), static_cast<float>(Bounds.Position.Y));
}

static Vector2 PolarAround(const Vector2& Center, float Angle, float Radius)
{
    return Center + Vector2(Math::Cos(Angle) * Radius, Math::Sin(Angle) * Radius);
}

static int32 DrawAngleFan(const FDrawGeometry& Geometry, FDrawCommandList& CommandList, int32 LayerId)
{
    const FUIStyle& Style  = FUIStyle::GetDefault();
    const Vector2   Origin = TopLeftOf(Geometry.Bounds);

    constexpr int32 NumSpokes = 48;

    const float   Radius = static_cast<float>(Math::Min(Geometry.Bounds.Width, Geometry.Bounds.Height * 2)) * 0.5f;
    const Vector2 Center = Origin + Vector2(Radius, static_cast<float>(Geometry.Bounds.Height));

    for (int32 Index = 0; Index < NumSpokes; ++Index)
    {
        const float Angle = -Math::Constants::PI * (static_cast<float>(Index) / static_cast<float>(NumSpokes - 1));
        CommandList.AddLine(LayerId, Center, PolarAround(Center, Angle, Radius), Style.Colors.Text, 1.0f);
    }

    const Vector2 ThickCenter = Center + Vector2((Radius * 2.0f) + 32.0f, 0.0f);
    for (int32 Index = 0; Index < NumSpokes / 4; ++Index)
    {
        const float Angle = -Math::Constants::PI * (static_cast<float>(Index) / static_cast<float>((NumSpokes / 4) - 1));
        CommandList.AddLine(LayerId, ThickCenter, PolarAround(ThickCenter, Angle, Radius), Style.Colors.Accent, 3.0f);
    }

    return LayerId;
}

static int32 DrawThicknessRamp(const FDrawGeometry& Geometry, FDrawCommandList& CommandList, int32 LayerId)
{
    const FUIStyle& Style  = FUIStyle::GetDefault();
    const Vector2   Origin = TopLeftOf(Geometry.Bounds);

    constexpr int32 NumSteps = 8;

    const float Span = static_cast<float>(Geometry.Bounds.Width) * 0.45f;
    for (int32 Index = 0; Index < NumSteps; ++Index)
    {
        const float Thickness = static_cast<float>(Index + 1);
        const float Y         = 8.0f + (static_cast<float>(Index) * 14.0f);

        CommandList.AddLine(LayerId, Origin + Vector2(0.0f, Y), Origin + Vector2(Span, Y), Style.Colors.Text, Thickness);

        const Vector2 DiagonalStart = Origin + Vector2(Span + 24.0f + (static_cast<float>(Index) * 22.0f), 4.0f);
        CommandList.AddLine(LayerId, DiagonalStart, DiagonalStart + Vector2(18.0f, 108.0f), Style.Colors.Accent, Thickness);
    }

    return LayerId;
}

static int32 DrawPolylines(const FDrawGeometry& Geometry, FDrawCommandList& CommandList, int32 LayerId)
{
    const FUIStyle& Style  = FUIStyle::GetDefault();
    const Vector2   Origin = TopLeftOf(Geometry.Bounds);

    TArray<Vector2> Zigzag;
    for (int32 Index = 0; Index < 10; ++Index)
    {
        const float X = static_cast<float>(Index) * 26.0f;
        const float Y = ((Index % 2) == 0) ? 0.0f : 52.0f - (static_cast<float>(Index) * 4.0f);
        Zigzag.Add(Origin + Vector2(X, 8.0f + Y));
    }

    CommandList.AddPolyline(LayerId, TArrayView<const Vector2>(Zigzag.Data(), Zigzag.Size()), Style.Colors.Accent, 6.0f);

    TArray<Vector2> Ring;
    const Vector2   RingCenter = Origin + Vector2(340.0f, 44.0f);

    for (int32 Index = 0; Index < 5; ++Index)
    {
        const float Angle = (Math::Constants::TwoPI * static_cast<float>(Index) / 5.0f) - Math::Constants::HalfPI;
        Ring.Add(PolarAround(RingCenter, Angle, 38.0f));
    }

    CommandList.AddPolyline(LayerId, TArrayView<const Vector2>(Ring.Data(), Ring.Size()), Style.Colors.Text, 4.0f, true);

    TArray<Vector2> Star;
    const Vector2   StarCenter = Origin + Vector2(460.0f, 44.0f);

    for (int32 Index = 0; Index < 10; ++Index)
    {
        const float Angle  = (Math::Constants::TwoPI * static_cast<float>(Index) / 10.0f) - Math::Constants::HalfPI;
        const float Radius = ((Index % 2) == 0) ? 40.0f : 16.0f;
        Star.Add(PolarAround(StarCenter, Angle, Radius));
    }

    CommandList.AddPolyline(LayerId, TArrayView<const Vector2>(Star.Data(), Star.Size()), Style.Colors.Accent, 3.0f, true);
    return LayerId;
}

static int32 DrawCirclesAndArcs(const FDrawGeometry& Geometry, FDrawCommandList& CommandList, int32 LayerId)
{
    const FUIStyle& Style  = FUIStyle::GetDefault();
    const Vector2   Origin = TopLeftOf(Geometry.Bounds);

    float NextX = 0.0f;
    for (int32 Index = 0; Index < 5; ++Index)
    {
        const float Radius = 4.0f * static_cast<float>(1 << Index);
        NextX += Radius;
        CommandList.AddCircle(LayerId, Origin + Vector2(NextX, 66.0f), Radius, Style.Colors.Text, 1.0f);
        NextX += Radius + 14.0f;
    }

    const Vector2 PieCenter = Origin + Vector2(NextX + 60.0f, 66.0f);
    for (int32 Index = 0; Index < 5; ++Index)
    {
        const float StartAngle = Math::Constants::TwoPI * (static_cast<float>(Index) / 5.0f);
        const float EndAngle   = StartAngle + (Math::Constants::TwoPI / 6.0f);
        const float Shade      = 0.35f + (0.13f * static_cast<float>(Index));

        CommandList.AddArcFilled(LayerId, PieCenter, 54.0f, StartAngle, EndAngle, FFloatColor(Shade, Shade * 0.6f, 0.85f, 1.0f));
    }

    const Vector2 ArcCenter = Origin + Vector2(NextX + 200.0f, 76.0f);
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const float Radius    = 20.0f + (static_cast<float>(Index) * 14.0f);
        const float Thickness = static_cast<float>(Index + 1) * 1.5f;

        CommandList.AddArc(LayerId, ArcCenter, Radius, -Math::Constants::PI * 0.9f, -Math::Constants::PI * 0.1f, Style.Colors.Accent, Thickness);
    }

    CommandList.AddCircleFilled(LayerId, Origin + Vector2(NextX + 300.0f, 66.0f), 34.0f, Style.Colors.ControlHovered);
    return LayerId;
}

static int32 DrawFilledPolygons(const FDrawGeometry& Geometry, FDrawCommandList& CommandList, int32 LayerId)
{
    const FUIStyle& Style  = FUIStyle::GetDefault();
    const Vector2   Origin = TopLeftOf(Geometry.Bounds);

    CommandList.AddTriangle(
        LayerId,
        Origin + Vector2(0.0f, 84.0f),
        Origin + Vector2(44.0f, 0.0f),
        Origin + Vector2(88.0f, 84.0f),
        Style.Colors.Accent);

    float NextX = 116.0f;
    for (int32 Sides = 3; Sides <= 8; ++Sides)
    {
        TArray<Vector2> Polygon;
        const Vector2   Center = Origin + Vector2(NextX + 40.0f, 42.0f);

        for (int32 Index = 0; Index < Sides; ++Index)
        {
            const float Angle = (Math::Constants::TwoPI * static_cast<float>(Index) / static_cast<float>(Sides)) - Math::Constants::HalfPI;
            Polygon.Add(PolarAround(Center, Angle, 38.0f));
        }

        const float Shade = 0.30f + (0.09f * static_cast<float>(Sides - 3));
        CommandList.AddConvexPolygon(LayerId, TArrayView<const Vector2>(Polygon.Data(), Polygon.Size()), FFloatColor(Shade, 0.72f, Shade + 0.2f, 1.0f));

        NextX += 92.0f;
    }

    return LayerId;
}

static int32 DrawBeziers(const FDrawGeometry& Geometry, FDrawCommandList& CommandList, int32 LayerId)
{
    const FUIStyle& Style  = FUIStyle::GetDefault();
    const Vector2   Origin = TopLeftOf(Geometry.Bounds);

    for (int32 Index = 0; Index < 4; ++Index)
    {
        const float Thickness = static_cast<float>(Index + 1);
        const float Y         = 10.0f + (static_cast<float>(Index) * 26.0f);

        const Vector2 Start = Origin + Vector2(0.0f, Y);
        const Vector2 End   = Origin + Vector2(240.0f, Y + 60.0f);

        CommandList.AddBezier(
            LayerId,
            Start,
            Start + Vector2(110.0f, 0.0f),
            End - Vector2(110.0f, 0.0f),
            End,
            Style.Colors.Accent,
            Thickness);
    }

    const Vector2 CuspStart = Origin + Vector2(300.0f, 130.0f);
    CommandList.AddBezier(
        LayerId,
        CuspStart,
        CuspStart + Vector2(160.0f, -170.0f),
        CuspStart + Vector2(-40.0f, -170.0f),
        CuspStart + Vector2(120.0f, 0.0f),
        Style.Colors.Text,
        2.0f);

    const Vector2 SweepStart = Origin + Vector2(480.0f, 128.0f);
    CommandList.AddBezier(
        LayerId,
        SweepStart,
        SweepStart + Vector2(60.0f, -120.0f),
        SweepStart + Vector2(180.0f, -120.0f),
        SweepStart + Vector2(240.0f, 0.0f),
        Style.Colors.ControlHovered,
        4.0f);

    return LayerId;
}

static TSharedPtr<FVisualElement> MakeCanvas(int32 Height, const FOnCanvasDraw& OnDraw)
{
    FDrawCanvas::FDesc CanvasDesc;
    CanvasDesc.DesiredSize  = IntVector2(0, Height);
    CanvasDesc.OnCanvasDraw = OnDraw;
    return FDrawCanvas::Create(CanvasDesc);
}

FPlaygroundScene CreateVectorsScene(const FPlaygroundFonts& Fonts)
{
    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Lines at every angle";
        Desc.Description = "A half-turn of spokes at one pixel, then the same at three. Watch for thinning off-axis.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeCanvas(130, FOnCanvasDraw::CreateStatic(&DrawAngleFan));
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Thickness";
        Desc.Description = "One to eight pixels, horizontal and diagonal, where a wrong half-width is obvious.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeCanvas(126, FOnCanvasDraw::CreateStatic(&DrawThicknessRamp));
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Polylines and joins";
        Desc.Description = "A tightening zigzag against the miter limit, a closed ring, and a star with sharp inner points.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeCanvas(100, FOnCanvasDraw::CreateStatic(&DrawPolylines));
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Circles and arcs";
        Desc.Description = "Radii an octave apart at one pixel, filled wedges, and nested arcs at growing thickness.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeCanvas(140, FOnCanvasDraw::CreateStatic(&DrawCirclesAndArcs));
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Filled polygons";
        Desc.Description = "A triangle, then a fan for every side count from three to eight.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeCanvas(90, FOnCanvasDraw::CreateStatic(&DrawFilledPolygons));
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Beziers";
        Desc.Description = "Graph links at four thicknesses, a near-cusp, and a long shallow sweep.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeCanvas(140, FOnCanvasDraw::CreateStatic(&DrawBeziers));
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene("Vectors", MakeSceneColumn("Vectors", Fonts, Panels));
}
