#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Math/Math.h"
#include "Core/Math/Vector2.h"
#include "Core/Templates/NumericLimits.h"

namespace Geometry2D
{
    /**
     * @brief The squared distance from a point to a line segment, which is what to compare against a squared
     * threshold when many segments are tested.
     *
     * @param Point The point to measure from.
     * @param Start One end of the segment.
     * @param End   The other end.
     * @return The squared distance.
     */
    NODISCARD inline float DistanceSquaredToSegment(const Vector2& Point, const Vector2& Start, const Vector2& End)
    {
        const Vector2 Segment       = End - Start;
        const float   LengthSquared = Segment.DotProduct(Segment);

        if (LengthSquared <= Math::Constants::Epsilon)
        {
            const Vector2 Offset = Point - Start;
            return Offset.DotProduct(Offset);
        }

        const float   Parameter = Math::Clamp((Point - Start).DotProduct(Segment) / LengthSquared, 0.0f, 1.0f);
        const Vector2 Offset    = Point - (Start + (Segment * Parameter));
        return Offset.DotProduct(Offset);
    }

    /**
     * @brief The distance from a point to a line segment.
     *
     * @param Point The point to measure from.
     * @param Start One end of the segment.
     * @param End   The other end.
     * @return The distance in the same units as the points.
     */
    NODISCARD inline float DistanceToSegment(const Vector2& Point, const Vector2& Start, const Vector2& End)
    {
        return Math::Sqrt(DistanceSquaredToSegment(Point, Start, End));
    }

    /**
     * @brief The distance from a point to the nearest segment of an open polyline.
     *
     * @param Points The polyline's points, in order. Fewer than two measures to the single point, or gives the
     * largest float for none at all.
     * @param Point  The point to measure from.
     * @return The distance in the same units as the points.
     */
    NODISCARD inline float DistanceToPolyline(TArrayView<const Vector2> Points, const Vector2& Point)
    {
        if (Points.IsEmpty())
        {
            return TNumericLimits<float>::Max();
        }

        if (Points.Size() == 1)
        {
            return (Point - Points[0]).GetLength();
        }

        float Closest = TNumericLimits<float>::Max();
        for (int32 Index = 1; Index < Points.Size(); ++Index)
        {
            Closest = Math::Min(Closest, DistanceSquaredToSegment(Point, Points[Index - 1], Points[Index]));
        }

        return Math::Sqrt(Closest);
    }

    /**
     * @brief Whether a point falls inside a convex quadrilateral.
     *
     * @param Point The point to test.
     * @param Quad  The four corners, in order around the quad.
     * @return True when the point is inside or on an edge.
     */
    NODISCARD inline bool IsPointInsideQuad(const Vector2& Point, const Vector2 Quad[4])
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

    /**
     * @brief A point on a cubic Bezier curve.
     *
     * @param P0 The start point.
     * @param P1 The first control point.
     * @param P2 The second control point.
     * @param P3 The end point.
     * @param T  How far along the curve, from zero at the start to one at the end.
     * @return The point.
     */
    NODISCARD inline Vector2 EvaluateCubicBezier(const Vector2& P0, const Vector2& P1, const Vector2& P2, const Vector2& P3, float T)
    {
        const float InverseT     = 1.0f - T;
        const float StartWeight  = InverseT * InverseT * InverseT;
        const float FirstWeight  = 3.0f * InverseT * InverseT * T;
        const float SecondWeight = 3.0f * InverseT * T * T;
        const float EndWeight    = T * T * T;

        return (P0 * StartWeight) + (P1 * FirstWeight) + (P2 * SecondWeight) + (P3 * EndWeight);
    }

    /**
     * @brief Flattens a cubic Bezier curve into evenly spaced points along its parameter.
     *
     * @param P0          The start point.
     * @param P1          The first control point.
     * @param P2          The second control point.
     * @param P3          The end point.
     * @param NumSegments How many straight segments to cut the curve into, at least one.
     * @param OutPoints   Receives NumSegments + 1 points appended, from the start point to the end point.
     */
    template<typename AllocatorType>
    inline void FlattenCubicBezier(const Vector2& P0, const Vector2& P1, const Vector2& P2, const Vector2& P3, int32 NumSegments, TArray<Vector2, AllocatorType>& OutPoints)
    {
        const int32 Segments = Math::Max(NumSegments, 1);
        OutPoints.Reserve(OutPoints.Size() + Segments + 1);

        OutPoints.Add(P0);
        for (int32 Step = 1; Step <= Segments; ++Step)
        {
            OutPoints.Add(EvaluateCubicBezier(P0, P1, P2, P3, static_cast<float>(Step) / static_cast<float>(Segments)));
        }
    }
}
