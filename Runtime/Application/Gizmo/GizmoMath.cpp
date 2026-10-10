#include "Application/Gizmo/GizmoMath.h"
#include "Core/Math/Geometry2D.h"
#include "Core/Math/Math.h"
#include "Core/Math/Plane.h"

bool FGizmoMath::WorldToClient(
    const FMatrix4&   ViewProjection, 
    const FRectangle& Viewport, 
    const FVector3&   WorldPosition, 
    FVector2&         OutClientPosition)
{
    const FVector4 ClipPosition = ViewProjection.Transform(FVector4(WorldPosition.X, WorldPosition.Y, WorldPosition.Z, 1.0f));
    if (ClipPosition.W <= Math::Constants::Epsilon)
    {
        return false;
    }

    const float InverseW = 1.0f / ClipPosition.W;
    const float RangeX   = (ClipPosition.X * InverseW) * 0.5f + 0.5f;
    const float RangeY   = 1.0f - ((ClipPosition.Y * InverseW) * 0.5f + 0.5f);

    OutClientPosition = FVector2(
        static_cast<float>(Viewport.Position.X) + (RangeX * static_cast<float>(Viewport.Width)),
        static_cast<float>(Viewport.Position.Y) + (RangeY * static_cast<float>(Viewport.Height)));

    return true;
}

void FGizmoMath::ComputeCameraRay(
    const FMatrix4&    ViewProjection, 
    const FRectangle&  Viewport, 
    const FIntVector2& ClientPosition, 
    FVector3&          OutOrigin, 
    FVector3&          OutDirection)
{
    const FMatrix4 InverseViewProjection = ViewProjection.GetInverse();

    const float Width  = Viewport.Width > 0 ? static_cast<float>(Viewport.Width) : 1.0f;
    const float Height = Viewport.Height > 0 ? static_cast<float>(Viewport.Height) : 1.0f;
    const float RangeX = (static_cast<float>(ClientPosition.X - Viewport.Position.X)) / Width;
    const float RangeY = (static_cast<float>(ClientPosition.Y - Viewport.Position.Y)) / Height;
    const float ClipX  = (RangeX * 2.0f) - 1.0f;
    const float ClipY  = 1.0f - (RangeY * 2.0f);

    FVector4 NearPoint = InverseViewProjection.Transform(FVector4(ClipX, ClipY, 0.0f, 1.0f));
    FVector4 FarPoint  = InverseViewProjection.Transform(FVector4(ClipX, ClipY, 1.0f, 1.0f));

    if (Math::Abs(NearPoint.W) > Math::Constants::Epsilon)
    {
        NearPoint *= 1.0f / NearPoint.W;
    }

    if (Math::Abs(FarPoint.W) > Math::Constants::Epsilon)
    {
        FarPoint *= 1.0f / FarPoint.W;
    }

    OutOrigin    = FVector3(NearPoint.X, NearPoint.Y, NearPoint.Z);
    OutDirection = (FVector3(FarPoint.X, FarPoint.Y, FarPoint.Z) - OutOrigin).GetNormalized();
}

bool FGizmoMath::IntersectRayPlane(
    const FVector3& PlanePoint, 
    const FVector3& PlaneNormal, 
    const FVector3& RayOrigin, 
    const FVector3& RayDirection, 
    FVector3&       OutHit)
{
    const FPlane HitPlane(PlaneNormal, -PlaneNormal.DotProduct(PlanePoint));

    const float Distance = HitPlane.IntersectRay(RayOrigin, RayDirection);
    if (Distance < 0.0f)
    {
        return false;
    }

    OutHit = RayOrigin + (RayDirection * Distance);
    return true;
}

FVector3 FGizmoMath::ComputeAxisPlaneNormal(const FVector3& Axis, const FVector3& ViewDirection)
{
    const FVector3 Orthogonal = Axis.CrossProduct(ViewDirection);
    if (Orthogonal.GetLength() < Math::Constants::CmpThreshold)
    {
        const FVector3 Fallback = Math::Abs(Axis.Z) < 0.9f ? FVector3(0.0f, 0.0f, 1.0f) : FVector3(1.0f, 0.0f, 0.0f);
        return Axis.CrossProduct(Axis.CrossProduct(Fallback)).GetNormalized();
    }

    return Axis.CrossProduct(Orthogonal).GetNormalized();
}

float FGizmoMath::DistanceToSegment(const FVector2& Point, const FVector2& Start, const FVector2& End)
{
    return Geometry2D::DistanceToSegment(Point, Start, End);
}

bool FGizmoMath::IsPointInsideQuad(const FVector2& Point, const FVector2 Quad[4])
{
    return Geometry2D::IsPointInsideQuad(Point, Quad);
}

bool FGizmoMath::IsPointOverQuad(const FVector2& Point, const FVector2 Quad[4], float Padding)
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

FVector3 FGizmoMath::SnapVector(const FVector3& Value, float Snap)
{
    return FVector3(SnapValue(Value.X, Snap), SnapValue(Value.Y, Snap), SnapValue(Value.Z, Snap));
}

float FGizmoMath::GetSegmentLengthInClipSpace(const FMatrix4& ViewProjection, float AspectRatio, const FVector3& Start, const FVector3& End)
{
    FVector4 StartClip = ViewProjection.Transform(FVector4(Start.X, Start.Y, Start.Z, 1.0f));
    FVector4 EndClip   = ViewProjection.Transform(FVector4(End.X, End.Y, End.Z, 1.0f));

    if (Math::Abs(StartClip.W) > Math::Constants::Epsilon)
    {
        StartClip *= 1.0f / StartClip.W;
    }

    if (Math::Abs(EndClip.W) > Math::Constants::Epsilon)
    {
        EndClip *= 1.0f / EndClip.W;
    }

    const float    SafeAspect = AspectRatio > Math::Constants::Epsilon ? AspectRatio : 1.0f;
    const FVector2 Delta      = FVector2(EndClip.X - StartClip.X, (EndClip.Y - StartClip.Y) / SafeAspect);

    return Delta.GetLength();
}

float FGizmoMath::GetParallelogramArea(
    const FMatrix4& ViewProjection, 
    float           AspectRatio, 
    const FVector3& Origin, 
    const FVector3& PointA, 
    const FVector3& PointB)
{
    const FVector3 Points[3] = { Origin, PointA, PointB };

    FVector2 Projected[3];
    for (int32 Index = 0; Index < 3; ++Index)
    {
        FVector4 Clip = ViewProjection.Transform(FVector4(Points[Index].X, Points[Index].Y, Points[Index].Z, 1.0f));
        if (Math::Abs(Clip.W) > Math::Constants::Epsilon)
        {
            Clip *= 1.0f / Clip.W;
        }

        Projected[Index] = FVector2(Clip.X, Clip.Y);
    }

    const float    SafeAspect = AspectRatio > Math::Constants::Epsilon ?  AspectRatio : 1.0f;
    const FVector2 SideA      = FVector2(Projected[1].X - Projected[0].X, (Projected[1].Y - Projected[0].Y) / SafeAspect);
    const FVector2 SideB      = FVector2(Projected[2].X - Projected[0].X, (Projected[2].Y - Projected[0].Y) / SafeAspect);

    return Math::Abs((SideA.X * SideB.Y) - (SideA.Y * SideB.X));
}

float FGizmoMath::ComputeScreenFactor(const FMatrix4& ViewProjection, const FVector3& CameraRight, const FVector3& Pivot, float SizeInClipSpace)
{
    const FVector4 PivotClip = ViewProjection.Transform(FVector4(Pivot.X, Pivot.Y, Pivot.Z, 1.0f));

    const FVector3 RightPoint = Pivot + CameraRight;
    const FVector4 RightClip  = ViewProjection.Transform(FVector4(RightPoint.X, RightPoint.Y, RightPoint.Z, 1.0f));

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
