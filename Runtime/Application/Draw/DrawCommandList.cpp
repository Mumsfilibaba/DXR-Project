#include "Application/Draw/DrawCommandList.h"
#include "Core/Math/Math.h"

FDrawCommandList::FDrawCommandList()
    : Commands()
    , Points()
    , ScratchPoints()
    , ClipStack()
    , EmptyClipRectangle()
    , UnmatchedPopCount(0)
{
}

FDrawCommandList::~FDrawCommandList() = default;

FDrawCommand& FDrawCommandList::EmplaceCommand(EDrawCommandType Type, int32 LayerId)
{
    FDrawCommand& Command = Commands.Emplace();
    Command.Type       = Type;
    Command.LayerId    = LayerId;
    Command.bIsClipped = !ClipStack.IsEmpty();

    if (Command.bIsClipped)
    {
        Command.ClipRectangle = ClipStack.Last();
    }

    return Command;
}

void FDrawCommandList::AddBox(int32 LayerId, const FRectangle& Bounds, const FFloatColor& Tint, const FCornerRadii& CornerRadius)
{
    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::Box, LayerId);
    Command.Bounds       = Bounds;
    Command.Tint         = Tint;
    Command.CornerRadius = CornerRadius;
}

void FDrawCommandList::AddBoxOutline(int32 LayerId, const FRectangle& Bounds, const FFloatColor& Tint, float Thickness, const FCornerRadii& CornerRadius)
{
    if (Thickness <= 0.0f)
    {
        return;
    }

    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::BoxOutline, LayerId);
    Command.Bounds       = Bounds;
    Command.Tint         = Tint;
    Command.CornerRadius = CornerRadius;
    Command.Thickness    = Thickness;
}

void FDrawCommandList::AddText(int32 LayerId, const FRectangle& Bounds, const String& InText, const IFontFace* Font, const FFloatColor& Tint)
{
    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::Text, LayerId);
    Command.Bounds = Bounds;
    Command.Tint   = Tint;
    Command.Text   = InText;
    Command.Font   = Font;
}

void FDrawCommandList::AddLine(int32 LayerId, const FRectangle& Bounds, const FFloatColor& Tint)
{
    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::Line, LayerId);
    Command.Bounds = Bounds;
    Command.Tint   = Tint;
}

void FDrawCommandList::AddLine(int32 LayerId, const Vector2& Start, const Vector2& End, const FFloatColor& Tint, float Thickness)
{
    const Vector2 LinePoints[2] = { Start, End };
    AddPolyline(LayerId, TArrayView<const Vector2>(LinePoints, 2), Tint, Thickness, false);
}

void FDrawCommandList::AddPolyline(int32 LayerId, TArrayView<const Vector2> InPoints, const FFloatColor& Tint, float Thickness, bool bClosed)
{
    if (InPoints.Size() < 2 || Thickness <= 0.0f)
    {
        return;
    }

    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::Polyline, LayerId);
    Command.Tint      = Tint;
    Command.Thickness = Thickness;
    Command.bIsClosed = bClosed;

    StorePoints(Command, InPoints);
}

void FDrawCommandList::AddConvexPolygon(int32 LayerId, TArrayView<const Vector2> InPoints, const FFloatColor& Tint)
{
    if (InPoints.Size() < 3)
    {
        return;
    }

    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::ConvexPolygon, LayerId);
    Command.Tint = Tint;

    StorePoints(Command, InPoints);
}

void FDrawCommandList::AddPanelChrome(
    int32               LayerId,
    const FRectangle&   Bounds,
    const FCornerRadii& CornerRadius,
    float               Thickness,
    const FFloatColor&  BorderTint,
    const FFloatColor&  BackdropTint)
{
    if (Bounds.IsEmpty())
    {
        return;
    }

    const FCornerRadii Clamped = CornerRadius.ClampToBounds(Bounds);

    const float Left   = static_cast<float>(Bounds.Position.X);
    const float Top    = static_cast<float>(Bounds.Position.Y);
    const float Right  = static_cast<float>(Bounds.GetRight());
    const float Bottom = static_cast<float>(Bounds.GetBottom());

    struct FCorner
    {
        Vector2 Point;
        Vector2 ArcCenter;
        float   Radius;
        float   StartAngle;
        float   EndAngle;
    };

    const FCorner Corners[4] =
    {
        { Vector2(Left,  Top),    Vector2(Left  + Clamped.TopLeft,     Top    + Clamped.TopLeft),     Clamped.TopLeft,     -Math::Constants::HalfPI, -Math::Constants::PI     },
        { Vector2(Right, Top),    Vector2(Right - Clamped.TopRight,    Top    + Clamped.TopRight),    Clamped.TopRight,    -Math::Constants::HalfPI,  0.0f          },
        { Vector2(Right, Bottom), Vector2(Right - Clamped.BottomRight, Bottom - Clamped.BottomRight), Clamped.BottomRight,  0.0f,           Math::Constants::HalfPI },
        { Vector2(Left,  Bottom), Vector2(Left  + Clamped.BottomLeft,  Bottom - Clamped.BottomLeft),  Clamped.BottomLeft,   Math::Constants::HalfPI,  Math::Constants::PI     },
    };

    for (const FCorner& Corner : Corners)
    {
        if (Corner.Radius <= 0.0f)
        {
            continue;
        }

        const int32 Segments = ResolveCircleSegments(Corner.Radius, Math::Abs(Corner.EndAngle - Corner.StartAngle), 0);

        BuildArcPoints(Corner.ArcCenter, Corner.Radius, Corner.StartAngle, Corner.EndAngle, Segments);
        ScratchPoints.Insert(0, Corner.Point);

        AddConvexPolygon(LayerId, ScratchPoints, BackdropTint);
    }

    AddBoxOutline(LayerId, Bounds, BorderTint, Thickness, Clamped);
}

void FDrawCommandList::AddTriangle(int32 LayerId, const Vector2& A, const Vector2& B, const Vector2& C, const FFloatColor& Tint)
{
    const Vector2 Corners[3] = { A, B, C };
    AddConvexPolygon(LayerId, TArrayView<const Vector2>(Corners, 3), Tint);
}

void FDrawCommandList::AddCircle(int32 LayerId, const Vector2& Center, float Radius, const FFloatColor& Tint, float Thickness, int32 Segments)
{
    if (Radius <= 0.0f)
    {
        return;
    }

    const int32 SegmentCount = ResolveCircleSegments(Radius, Math::Constants::TwoPI, Segments);

    BuildArcPoints(Center, Radius, 0.0f, Math::Constants::TwoPI - (Math::Constants::TwoPI / static_cast<float>(SegmentCount)), SegmentCount - 1);
    AddPolyline(LayerId, ScratchPoints, Tint, Thickness, true);
}

void FDrawCommandList::AddCircleFilled(int32 LayerId, const Vector2& Center, float Radius, const FFloatColor& Tint, int32 Segments)
{
    if (Radius <= 0.0f)
    {
        return;
    }

    const int32 SegmentCount = ResolveCircleSegments(Radius, Math::Constants::TwoPI, Segments);

    BuildArcPoints(Center, Radius, 0.0f, Math::Constants::TwoPI - (Math::Constants::TwoPI / static_cast<float>(SegmentCount)), SegmentCount - 1);
    AddConvexPolygon(LayerId, ScratchPoints, Tint);
}

void FDrawCommandList::AddArc(int32 LayerId, const Vector2& Center, float Radius, float StartAngle, float EndAngle, const FFloatColor& Tint, float Thickness, int32 Segments)
{
    if (Radius <= 0.0f)
    {
        return;
    }

    const int32 SegmentCount = ResolveCircleSegments(Radius, Math::Abs(EndAngle - StartAngle), Segments);

    BuildArcPoints(Center, Radius, StartAngle, EndAngle, SegmentCount);
    AddPolyline(LayerId, ScratchPoints, Tint, Thickness, false);
}

void FDrawCommandList::AddArcFilled(int32 LayerId, const Vector2& Center, float Radius, float StartAngle, float EndAngle, const FFloatColor& Tint, int32 Segments)
{
    if (Radius <= 0.0f)
    {
        return;
    }

    const int32 SegmentCount = ResolveCircleSegments(Radius, Math::Abs(EndAngle - StartAngle), Segments);

    BuildArcPoints(Center, Radius, StartAngle, EndAngle, SegmentCount);

    ScratchPoints.Insert(0, Center);
    AddConvexPolygon(LayerId, ScratchPoints, Tint);
}

void FDrawCommandList::AddBezier(int32 LayerId, const Vector2& P0, const Vector2& P1, const Vector2& P2, const Vector2& P3, const FFloatColor& Tint, float Thickness, int32 Segments)
{
    int32 SegmentCount = Segments;
    if (SegmentCount <= 0)
    {
        const float ControlLength = (P1 - P0).GetLength() + (P2 - P1).GetLength() + (P3 - P2).GetLength();
        SegmentCount = Math::Clamp(static_cast<int32>(ControlLength * 0.25f), MinBezierSegments, MaxBezierSegments);
    }

    ScratchPoints.Clear();
    ScratchPoints.Reserve(SegmentCount + 1);

    for (int32 Index = 0; Index <= SegmentCount; ++Index)
    {
        const float T        = static_cast<float>(Index) / static_cast<float>(SegmentCount);
        const float OneMinus = 1.0f - T;

        const float W0 = OneMinus * OneMinus * OneMinus;
        const float W1 = 3.0f * OneMinus * OneMinus * T;
        const float W2 = 3.0f * OneMinus * T * T;
        const float W3 = T * T * T;

        ScratchPoints.Add(Vector2((P0.X * W0) + (P1.X * W1) + (P2.X * W2) + (P3.X * W3), (P0.Y * W0) + (P1.Y * W1) + (P2.Y * W2) + (P3.Y * W3)));
    }

    AddPolyline(LayerId, ScratchPoints, Tint, Thickness, false);
}

void FDrawCommandList::AddImage(int32 LayerId, const FRectangle& Bounds, const FUIBrush& Brush, const FFloatColor& Tint, const FCornerRadii& CornerRadius)
{
    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::Image, LayerId);
    Command.Bounds       = Bounds;
    Command.Tint         = Tint;
    Command.Brush        = Brush;
    Command.CornerRadius = CornerRadius;
}

void FDrawCommandList::AddRoundedBottomBar(int32 LayerId, const FRectangle& Bounds, const FCornerRadii& CornerRadius, float Thickness, const FFloatColor& Tint, float FadeWidth)
{
    if (Thickness <= 0.0f)
    {
        return;
    }

    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::RoundedBottomBar, LayerId);
    Command.Bounds       = Bounds;
    Command.Tint         = Tint;
    Command.CornerRadius = CornerRadius;
    Command.Thickness    = Thickness;
    Command.FadeWidth    = Math::Max(FadeWidth, 0.0f);
}

void FDrawCommandList::AddRoundedAccentRing(int32 LayerId, const FRectangle& Bounds, const FCornerRadii& CornerRadius, float Thickness,
    const FFloatColor& Tint, float FadeFraction, float TrailAlpha)
{
    if (Thickness <= 0.0f)
    {
        return;
    }

    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::RoundedAccentRing, LayerId);
    Command.Bounds       = Bounds;
    Command.Tint         = Tint;
    Command.CornerRadius = CornerRadius;
    Command.Thickness    = Thickness;
    Command.FadeFraction = Math::Clamp(FadeFraction, 0.0f, 1.0f);
    Command.TrailAlpha   = Math::Clamp(TrailAlpha, 0.0f, 1.0f);
}

void FDrawCommandList::PushClip(int32 LayerId, const FRectangle& ClipRectangle)
{
    const FRectangle Resolved = ClipStack.IsEmpty() ? ClipRectangle : ClipStack.Last().Intersect(ClipRectangle);
    ClipStack.Add(Resolved);

    FDrawCommand& Command = Commands.Emplace();
    Command.Type    = EDrawCommandType::ClipPush;
    Command.Bounds  = Resolved;
    Command.LayerId = LayerId;
}

void FDrawCommandList::PopClip(int32 LayerId)
{
    if (ClipStack.IsEmpty())
    {
        UnmatchedPopCount++;
    }
    else
    {
        ClipStack.RemoveAt(ClipStack.LastIndex());
    }

    FDrawCommand& Command = Commands.Emplace();
    Command.Type    = EDrawCommandType::ClipPop;
    Command.LayerId = LayerId;
}

void FDrawCommandList::Reset()
{
    Commands.Clear();
    Points.Clear();
    ScratchPoints.Clear();
    ClipStack.Clear();
    UnmatchedPopCount = 0;
}

TArrayView<const Vector2> FDrawCommandList::GetCommandPoints(const FDrawCommand& Command) const
{
    if (Command.PointCount <= 0)
    {
        return TArrayView<const Vector2>();
    }

    return TArrayView<const Vector2>(Points.Data() + Command.PointOffset, Command.PointCount);
}

int32 FDrawCommandList::CountCommandsOfType(EDrawCommandType Type) const
{
    int32 Count = 0;
    for (const FDrawCommand& Command : Commands)
    {
        if (Command.Type == Type)
        {
            Count++;
        }
    }

    return Count;
}

int32 FDrawCommandList::FindTextCommand(const StringView& InText) const
{
    for (int32 Index = 0; Index < Commands.Size(); Index++)
    {
        const FDrawCommand& Command = Commands[Index];
        if (Command.Type == EDrawCommandType::Text && Command.Text.Equals(InText.Data(), InText.Length()))
        {
            return Index;
        }
    }

    return InvalidIndex;
}

void FDrawCommandList::StorePoints(FDrawCommand& Command, TArrayView<const Vector2> InPoints)
{
    Command.PointOffset = Points.Size();
    Command.PointCount  = InPoints.Size();

    Points.Reserve(Points.Size() + InPoints.Size());
    for (const Vector2& Point : InPoints)
    {
        Points.Add(Point);
    }
}

void FDrawCommandList::BuildArcPoints(const Vector2& Center, float Radius, float StartAngle, float EndAngle, int32 Segments)
{
    ScratchPoints.Clear();

    const int32 PointCount = Math::Max(Segments, 1) + 1;
    ScratchPoints.Reserve(PointCount);

    const float AngleStep = (EndAngle - StartAngle) / static_cast<float>(Math::Max(Segments, 1));

    for (int32 Index = 0; Index < PointCount; ++Index)
    {
        const float Angle = StartAngle + (AngleStep * static_cast<float>(Index));
        ScratchPoints.Add(Vector2(Center.X + (Radius * Math::Cos(Angle)), Center.Y + (Radius * Math::Sin(Angle))));
    }
}

int32 FDrawCommandList::ResolveCircleSegments(float Radius, float AngleSweep, int32 RequestedSegments)
{
    if (RequestedSegments > 0)
    {
        return Math::Clamp(RequestedSegments, MinCircleSegments, MaxCircleSegments);
    }

    const float ArcLength = Radius * Math::Max(Math::Abs(AngleSweep), 0.0001f);
    return Math::Clamp(static_cast<int32>(ArcLength * 0.35f), MinCircleSegments, MaxCircleSegments);
}
