#include "Application/Gizmo/GizmoMath.h"
#include "Core/Math/Math.h"

bool FGizmoMath::WorldToClient(
    const Matrix4&    ViewProjection, 
    const FRectangle& Viewport, 
    const Vector3&    WorldPosition, 
    Vector2&          OutClientPosition)
{
    const Vector4 ClipPosition = ViewProjection.Transform(Vector4(WorldPosition.X, WorldPosition.Y, WorldPosition.Z, 1.0f));
    if (ClipPosition.W <= Math::Constants::Epsilon)
    {
        return false;
    }

    const float InverseW = 1.0f / ClipPosition.W;
    const float RangeX   = (ClipPosition.X * InverseW) * 0.5f + 0.5f;
    const float RangeY   = 1.0f - ((ClipPosition.Y * InverseW) * 0.5f + 0.5f);

    OutClientPosition = Vector2(
        static_cast<float>(Viewport.Position.X) + (RangeX * static_cast<float>(Viewport.Width)),
        static_cast<float>(Viewport.Position.Y) + (RangeY * static_cast<float>(Viewport.Height)));

    return true;
}

void FGizmoMath::ComputeCameraRay(
    const Matrix4&    ViewProjection, 
    const FRectangle& Viewport, 
    const IntVector2& ClientPosition, 
    Vector3&          OutOrigin, 
    Vector3&          OutDirection)
{
    const Matrix4 InverseViewProjection = ViewProjection.GetInverse();

    const float Width  = Viewport.Width > 0 ? static_cast<float>(Viewport.Width) : 1.0f;
    const float Height = Viewport.Height > 0 ? static_cast<float>(Viewport.Height) : 1.0f;
    const float RangeX = (static_cast<float>(ClientPosition.X - Viewport.Position.X)) / Width;
    const float RangeY = (static_cast<float>(ClientPosition.Y - Viewport.Position.Y)) / Height;
    const float ClipX  = (RangeX * 2.0f) - 1.0f;
    const float ClipY  = 1.0f - (RangeY * 2.0f);

    Vector4 NearPoint = InverseViewProjection.Transform(Vector4(ClipX, ClipY, 0.0f, 1.0f));
    Vector4 FarPoint  = InverseViewProjection.Transform(Vector4(ClipX, ClipY, 1.0f, 1.0f));

    if (Math::Abs(NearPoint.W) > Math::Constants::Epsilon)
    {
        NearPoint *= 1.0f / NearPoint.W;
    }

    if (Math::Abs(FarPoint.W) > Math::Constants::Epsilon)
    {
        FarPoint *= 1.0f / FarPoint.W;
    }

    OutOrigin    = Vector3(NearPoint.X, NearPoint.Y, NearPoint.Z);
    OutDirection = (Vector3(FarPoint.X, FarPoint.Y, FarPoint.Z) - OutOrigin).GetNormalized();
}

bool FGizmoMath::IntersectRayPlane(
    const Vector3& PlanePoint, 
    const Vector3& PlaneNormal, 
    const Vector3& RayOrigin, 
    const Vector3& RayDirection, 
    Vector3&       OutHit)
{
    const float Denominator = PlaneNormal.DotProduct(RayDirection);
    if (Math::Abs(Denominator) < Math::Constants::Epsilon)
    {
        return false;
    }

    const float Distance = (PlanePoint - RayOrigin).DotProduct(PlaneNormal) / Denominator;
    if (Distance < 0.0f)
    {
        return false;
    }

    OutHit = RayOrigin + (RayDirection * Distance);
    return true;
}

Vector3 FGizmoMath::ComputeAxisPlaneNormal(const Vector3& Axis, const Vector3& ViewDirection)
{
    const Vector3 Orthogonal = Axis.CrossProduct(ViewDirection);
    if (Orthogonal.GetLength() < Math::Constants::CmpThreshold)
    {
        const Vector3 Fallback = Math::Abs(Axis.Z) < 0.9f ? Vector3(0.0f, 0.0f, 1.0f) : Vector3(1.0f, 0.0f, 0.0f);
        return Axis.CrossProduct(Axis.CrossProduct(Fallback)).GetNormalized();
    }

    return Axis.CrossProduct(Orthogonal).GetNormalized();
}

float FGizmoMath::DistanceToSegment(const Vector2& Point, const Vector2& Start, const Vector2& End)
{
    const Vector2 Segment       = End - Start;
    const float   LengthSquared = Segment.DotProduct(Segment);

    if (LengthSquared <= Math::Constants::Epsilon)
    {
        return (Point - Start).GetLength();
    }

    const float   Parameter = Math::Clamp((Point - Start).DotProduct(Segment) / LengthSquared, 0.0f, 1.0f);
    const Vector2 Closest   = Start + (Segment * Parameter);

    return (Point - Closest).GetLength();
}

bool FGizmoMath::IsPointInsideQuad(const Vector2& Point, const Vector2 Quad[4])
{
    bool bHasPositive = false;
    bool bHasNegative = false;

    for (int32 EdgeIndex = 0; EdgeIndex < 4; ++EdgeIndex)
    {
        const Vector2& Corner = Quad[EdgeIndex];
        const Vector2& Next   = Quad[(EdgeIndex + 1) % 4];

        const float Cross = ((Next.X - Corner.X) * (Point.Y - Corner.Y)) - ((Next.Y - Corner.Y) * (Point.X - Corner.X));

        bHasPositive |= Cross > 0.0f;
        bHasNegative |= Cross < 0.0f;
    }

    return !(bHasPositive && bHasNegative);
}

bool FGizmoMath::IsPointOverQuad(const Vector2& Point, const Vector2 Quad[4], float Padding)
{
    if (IsPointInsideQuad(Point, Quad))
    {
        return true;
    }

    for (int32 EdgeIndex = 0; EdgeIndex < 4; ++EdgeIndex)
    {
        if (DistanceToSegment(Point, Quad[EdgeIndex], Quad[(EdgeIndex + 1) % 4]) <= Padding)
        {
            return true;
        }
    }

    return false;
}

float FGizmoMath::SnapValue(float Value, float Snap)
{
    if (Snap <= Math::Constants::Epsilon)
    {
        return Value;
    }

    return Math::Round(Value / Snap) * Snap;
}

Vector3 FGizmoMath::SnapVector(const Vector3& Value, float Snap)
{
    return Vector3(SnapValue(Value.X, Snap), SnapValue(Value.Y, Snap), SnapValue(Value.Z, Snap));
}

float FGizmoMath::GetSegmentLengthInClipSpace(const Matrix4& ViewProjection, float AspectRatio, const Vector3& Start, const Vector3& End)
{
    Vector4 StartClip = ViewProjection.Transform(Vector4(Start.X, Start.Y, Start.Z, 1.0f));
    Vector4 EndClip   = ViewProjection.Transform(Vector4(End.X, End.Y, End.Z, 1.0f));

    if (Math::Abs(StartClip.W) > Math::Constants::Epsilon)
    {
        StartClip *= 1.0f / StartClip.W;
    }

    if (Math::Abs(EndClip.W) > Math::Constants::Epsilon)
    {
        EndClip *= 1.0f / EndClip.W;
    }

    const float   SafeAspect = AspectRatio > Math::Constants::Epsilon ? AspectRatio : 1.0f;
    const Vector2 Delta      = Vector2(EndClip.X - StartClip.X, (EndClip.Y - StartClip.Y) / SafeAspect);

    return Delta.GetLength();
}

float FGizmoMath::GetParallelogramArea(
    const Matrix4& ViewProjection, 
    float          AspectRatio, 
    const Vector3& Origin, 
    const Vector3& PointA, 
    const Vector3& PointB)
{
    const Vector3 Points[3] = { Origin, PointA, PointB };

    Vector2 Projected[3];
    for (int32 Index = 0; Index < 3; ++Index)
    {
        Vector4 Clip = ViewProjection.Transform(Vector4(Points[Index].X, Points[Index].Y, Points[Index].Z, 1.0f));
        if (Math::Abs(Clip.W) > Math::Constants::Epsilon)
        {
            Clip *= 1.0f / Clip.W;
        }

        Projected[Index] = Vector2(Clip.X, Clip.Y);
    }

    const float   SafeAspect = AspectRatio > Math::Constants::Epsilon ? AspectRatio : 1.0f;
    const Vector2 SideA      = Vector2(Projected[1].X - Projected[0].X, (Projected[1].Y - Projected[0].Y) / SafeAspect);
    const Vector2 SideB      = Vector2(Projected[2].X - Projected[0].X, (Projected[2].Y - Projected[0].Y) / SafeAspect);

    return Math::Abs((SideA.X * SideB.Y) - (SideA.Y * SideB.X));
}

float FGizmoMath::ComputeScreenFactor(const Matrix4& ViewProjection, const Vector3& CameraRight, const Vector3& Pivot, float SizeInClipSpace)
{
    const Vector4 PivotClip = ViewProjection.Transform(Vector4(Pivot.X, Pivot.Y, Pivot.Z, 1.0f));

    const Vector3 RightPoint = Pivot + CameraRight;
    const Vector4 RightClip  = ViewProjection.Transform(Vector4(RightPoint.X, RightPoint.Y, RightPoint.Z, 1.0f));

    if (Math::Abs(PivotClip.W) < Math::Constants::Epsilon || Math::Abs(RightClip.W) < Math::Constants::Epsilon)
    {
        return 1.0f;
    }

    const float ClipPerUnit = Math::Abs((RightClip.X / RightClip.W) - (PivotClip.X / PivotClip.W));
    if (ClipPerUnit < Math::Constants::Epsilon)
    {
        return 1.0f;
    }

    return SizeInClipSpace / ClipPerUnit;
}
