#pragma once
#include "Core/Math/Matrix4.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Application/Layout/LayoutTypes.h"

struct APPLICATION_API FGizmoMath
{
    /** @brief How close to an axis shaft the cursor has to be to grab it, in pixels. */
    static constexpr float AxisGrabDistance = 12.0f;

    /** @brief How far outside a plane quad the cursor may sit and still grab it, in pixels. */
    static constexpr float PlaneGrabPadding = 6.0f;

    /** @brief How close to a rotation ring the cursor has to be to grab it, in pixels. */
    static constexpr float RingGrabDistance = 8.0f;

    /**
     * @brief Projects a world point into the client space of a viewport.
     *
     * @param ViewProjection  The world-to-clip matrix.
     * @param Viewport        The rectangle clip space is mapped onto.
     * @param WorldPosition   The point to project.
     * @param OutClientPosition Receives the projected point.
     * @return False when the point is behind the camera, in which case nothing was written.
     */
    NODISCARD static bool WorldToClient(
        const FMatrix4&   ViewProjection, 
        const FRectangle& Viewport, 
        const FVector3&   WorldPosition, 
        FVector2&         OutClientPosition);

    /**
     * @brief The ray through a client point, which is what a drag is resolved against.
     *
     * @param ViewProjection The world-to-clip matrix.
     * @param Viewport       The rectangle clip space is mapped onto.
     * @param ClientPosition The point to shoot through.
     * @param OutOrigin      Receives the point on the near plane.
     * @param OutDirection   Receives the normalized direction.
     */
    static void ComputeCameraRay(
        const FMatrix4&    ViewProjection,
        const FRectangle&  Viewport,
        const FIntVector2& ClientPosition,
        FVector3&          OutOrigin,
        FVector3&          OutDirection);

    /**
     * @brief Where a ray meets a plane.
     *
     * @param PlanePoint  A point on the plane.
     * @param PlaneNormal The plane normal, which need not be normalized.
     * @param RayOrigin   Where the ray starts.
     * @param RayDirection The ray direction.
     * @param OutHit      Receives the intersection.
     * @return False when the ray runs parallel to the plane or meets it behind its origin.
     */
    NODISCARD static bool IntersectRayPlane(
        const FVector3& PlanePoint,
        const FVector3& PlaneNormal,
        const FVector3& RayOrigin,
        const FVector3& RayDirection,
        FVector3&       OutHit);

    /**
     * @brief The normal of the plane that contains an axis and faces the camera as squarely as it can.
     *
     * @param Axis          The direction the plane must contain.
     * @param ViewDirection The direction from the camera to the pivot.
     * @return The plane normal, normalized.
     */
    NODISCARD static FVector3 ComputeAxisPlaneNormal(const FVector3& Axis, const FVector3& ViewDirection);

    /**
     * @brief The distance from a point to a line segment, which is how an axis shaft is picked.
     *
     * @param Point The point to measure from.
     * @param Start One end of the segment.
     * @param End   The other end.
     * @return The distance in the same units as the points.
     */
    NODISCARD static float DistanceToSegment(const FVector2& Point, const FVector2& Start, const FVector2& End);

    /**
     * @brief Whether a point falls inside a convex quadrilateral.
     *
     * @param Point The point to test.
     * @param Quad  The four corners, in order around the quad.
     * @return True when the point is inside or on an edge.
     */
    NODISCARD static bool IsPointInsideQuad(const FVector2& Point, const FVector2 Quad[4]);

    /**
     * @brief Whether a point falls inside a quadrilateral or within a margin of one of its edges.
     *
     * @param Point   The point to test.
     * @param Quad    The four corners, in order around the quad.
     * @param Padding How far outside an edge still counts as a hit.
     * @return True when the point is inside or near enough.
     */
    NODISCARD static bool IsPointOverQuad(const FVector2& Point, const FVector2 Quad[4], float Padding);

    /**
     * @brief Quantizes a value to the nearest multiple of an increment.
     *
     * @param Value The value to quantize.
     * @param Snap  The increment, which is ignored when zero or negative.
     * @return The quantized value.
     */
    NODISCARD static float SnapValue(float Value, float Snap);

    /**
     * @brief Quantizes each component of a vector.
     *
     * @param Value The vector to quantize.
     * @param Snap  The increment applied to every component.
     * @return The quantized vector.
     */
    NODISCARD static FVector3 SnapVector(const FVector3& Value, float Snap);

    /**
     * @brief The length of a projected segment in clip space, which is how an axis facing the camera is spotted.
     *
     * @param ViewProjection The world-to-clip matrix.
     * @param AspectRatio    The viewport's width over its height, so the length is measured squarely.
     * @param Start          One end of the segment, in world space.
     * @param End            The other end.
     * @return The length, in clip space units.
     */
    NODISCARD static float GetSegmentLengthInClipSpace(
        const FMatrix4& ViewProjection, 
        float           AspectRatio, 
        const FVector3& Start, 
        const FVector3& End);

    /**
     * @brief The area a projected parallelogram covers, which is how a plane seen edge-on is spotted.
     *
     * @param ViewProjection The world-to-clip matrix.
     * @param AspectRatio    The viewport's width over its height.
     * @param Origin         The corner the two sides leave.
     * @param PointA         The end of the first side.
     * @param PointB         The end of the second side.
     * @return The area, in clip space units squared.
     */
    NODISCARD static float GetParallelogramArea(
        const FMatrix4& ViewProjection, 
        float           AspectRatio, 
        const FVector3& Origin, 
        const FVector3& PointA, 
        const FVector3& PointB);

    /**
     * @brief The world length that draws the gizmo at a constant size on screen, whatever the distance.
     *
     * @param ViewProjection  The world-to-clip matrix.
     * @param CameraRight     The camera's right axis in world space.
     * @param Pivot           Where the gizmo sits.
     * @param SizeInClipSpace How much of clip space the gizmo should span.
     * @return The length one gizmo axis reaches, in world units.
     */
    NODISCARD static float ComputeScreenFactor(
        const FMatrix4& ViewProjection, 
        const FVector3& CameraRight, 
        const FVector3& Pivot, 
        float           SizeInClipSpace);
};
