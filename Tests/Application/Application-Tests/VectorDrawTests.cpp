#include "VectorDrawTests.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Application/Draw/DrawCommandList.h>
#include <Application/Draw/UIDrawData.h>
#include <Core/Math/Math.h>

/** @brief How far apart two positions may be and still count as the same pixel. */
constexpr float GTolerance = 0.01f;

static bool IsNearly(float First, float Second, float Tolerance = GTolerance)
{
    return Math::Abs(First - Second) <= Tolerance;
}

/** @brief The axis-aligned extent of the geometry, which is what a stroke width can be read off. */
struct FGeometryExtent
{
    float MinX = 0.0f;
    float MinY = 0.0f;
    float MaxX = 0.0f;
    float MaxY = 0.0f;
};

static FGeometryExtent MeasureGeometry(const FUIDrawData& DrawData)
{
    FGeometryExtent Extent;
    bool            bHasVertex = false;

    auto Include = [&](float X, float Y)
    {
        if (!bHasVertex)
        {
            Extent.MinX = Extent.MaxX = X;
            Extent.MinY = Extent.MaxY = Y;
            bHasVertex = true;
            return;
        }

        Extent.MinX = Math::Min(Extent.MinX, X);
        Extent.MinY = Math::Min(Extent.MinY, Y);
        Extent.MaxX = Math::Max(Extent.MaxX, X);
        Extent.MaxY = Math::Max(Extent.MaxY, Y);
    };

    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        Include(Vertex.Position.X, Vertex.Position.Y);
    }

    for (const FUIShapeVertex& Vertex : DrawData.GetShapeVertices())
    {
        Include(Vertex.Position.X, Vertex.Position.Y);
    }

    return Extent;
}

bool CornerRadiiTypes_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A default radii rounds nothing");
    const FCornerRadii Square;
    TEST_EXPECT(Square.IsZero());
    TEST_EXPECT_EQ(Square.GetLargest(), 0.0f);

    TEST_SECTION("One number applies to every corner, which is what keeps the old call sites compiling");
    const FCornerRadii Uniform = 4.0f;
    TEST_EXPECT_EQ(Uniform.TopLeft, 4.0f);
    TEST_EXPECT_EQ(Uniform.TopRight, 4.0f);
    TEST_EXPECT_EQ(Uniform.BottomRight, 4.0f);
    TEST_EXPECT_EQ(Uniform.BottomLeft, 4.0f);
    TEST_EXPECT(!Uniform.IsZero());

    TEST_SECTION("Top rounds the two corners a title bar shows and leaves the two it does not");
    const FCornerRadii TitleBar = FCornerRadii::Top(6.0f);
    TEST_EXPECT_EQ(TitleBar.TopLeft, 6.0f);
    TEST_EXPECT_EQ(TitleBar.TopRight, 6.0f);
    TEST_EXPECT_EQ(TitleBar.BottomRight, 0.0f);
    TEST_EXPECT_EQ(TitleBar.BottomLeft, 0.0f);

    const FCornerRadii Footer = FCornerRadii::Bottom(6.0f);
    TEST_EXPECT_EQ(Footer.TopLeft, 0.0f);
    TEST_EXPECT_EQ(Footer.BottomRight, 6.0f);

    TEST_SECTION("The four-argument form goes clockwise from the top-left");
    const FCornerRadii Mixed(1.0f, 2.0f, 3.0f, 4.0f);
    TEST_EXPECT_EQ(Mixed.TopLeft, 1.0f);
    TEST_EXPECT_EQ(Mixed.TopRight, 2.0f);
    TEST_EXPECT_EQ(Mixed.BottomRight, 3.0f);
    TEST_EXPECT_EQ(Mixed.BottomLeft, 4.0f);
    TEST_EXPECT_EQ(Mixed.GetLargest(), 4.0f);

    TEST_SECTION("Clamping stops opposing corners meeting in the middle");
    const FCornerRadii Clamped = FCornerRadii(50.0f).ClampToBounds(FRectangle(IntVector2(0, 0), 100, 20));
    TEST_EXPECT_EQ(Clamped.TopLeft, 10.0f);
    TEST_EXPECT_EQ(Clamped.BottomRight, 10.0f);

    TEST_SECTION("Equality compares every corner");
    TEST_EXPECT(FCornerRadii(1.0f, 2.0f, 3.0f, 4.0f) == Mixed);
    TEST_EXPECT(FCornerRadii(1.0f, 2.0f, 3.0f, 5.0f) != Mixed);

    TEST_END();
}

bool VectorDrawCommands_Test()
{
    TEST_BEGIN();

    FDrawCommandList CommandList;

    TEST_SECTION("Each emitter records its own command type");
    const Vector2 Path[3] = { Vector2(0.0f, 0.0f), Vector2(10.0f, 10.0f), Vector2(20.0f, 0.0f) };

    CommandList.AddPolyline(0, TArrayView<const Vector2>(Path, 3), FFloatColor::White, 2.0f);
    CommandList.AddConvexPolygon(0, TArrayView<const Vector2>(Path, 3), FFloatColor::White);
    CommandList.AddBoxOutline(0, FRectangle(IntVector2(0, 0), 40, 40), FFloatColor::White, 1.0f);
    CommandList.AddImage(0, FRectangle(IntVector2(0, 0), 40, 40), FUIBrush(), FFloatColor::White);

    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Polyline), 1);
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::ConvexPolygon), 1);
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::BoxOutline), 1);
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Image), 1);

    TEST_SECTION("The points live in one pool and each command names its own slice of it");
    TEST_EXPECT_EQ(CommandList.GetPoints().Size(), 6);
    TEST_EXPECT_EQ(CommandList[0].PayloadOffset, 0);
    TEST_EXPECT_EQ(CommandList[0].PayloadCount, 3);
    TEST_EXPECT_EQ(CommandList[1].PayloadOffset, 3);
    TEST_EXPECT_EQ(CommandList[1].PayloadCount, 3);

    const TArrayView<const Vector2> FirstPoints = CommandList.GetCommandPoints(CommandList[0]);
    TEST_EXPECT_EQ(FirstPoints.Size(), 3);
    TEST_EXPECT(FirstPoints[1] == Vector2(10.0f, 10.0f));

    TEST_SECTION("A command that carries no points names an empty slice");
    TEST_EXPECT_EQ(CommandList[2].PayloadCount, 0);
    TEST_EXPECT(CommandList.GetCommandPoints(CommandList[2]).IsEmpty());

    TEST_SECTION("A degenerate primitive is refused rather than recorded and skipped later");
    FDrawCommandList DegenerateList;
    DegenerateList.AddPolyline(0, TArrayView<const Vector2>(Path, 1), FFloatColor::White, 2.0f);
    DegenerateList.AddPolyline(0, TArrayView<const Vector2>(Path, 3), FFloatColor::White, 0.0f);
    DegenerateList.AddConvexPolygon(0, TArrayView<const Vector2>(Path, 2), FFloatColor::White);
    DegenerateList.AddBoxOutline(0, FRectangle(IntVector2(0, 0), 40, 40), FFloatColor::White, 0.0f);

    TEST_EXPECT(DegenerateList.IsEmpty());
    TEST_EXPECT(DegenerateList.GetPoints().IsEmpty());

    TEST_SECTION("Reset drops the point pool with the commands");
    CommandList.Reset();
    TEST_EXPECT(CommandList.IsEmpty());
    TEST_EXPECT(CommandList.GetPoints().IsEmpty());

    TEST_END();
}

bool VectorDrawPolyline_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;
    DrawData.SetAntiAliasingEnabled(false);

    TEST_SECTION("An open two-point stroke is one quad, whatever its angle");
    FDrawCommandList LineList;
    LineList.AddLine(0, Vector2(0.0f, 10.0f), Vector2(100.0f, 10.0f), FFloatColor::White, 4.0f);
    DrawData.BuildFromCommandList(LineList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 6);

    TEST_SECTION("The stroke is centred on the path and is exactly as wide as it was asked to be");
    FGeometryExtent Extent = MeasureGeometry(DrawData);
    TEST_EXPECT(IsNearly(Extent.MinY, 8.0f));
    TEST_EXPECT(IsNearly(Extent.MaxY, 12.0f));
    TEST_EXPECT(IsNearly(Extent.MinX, 0.0f));
    TEST_EXPECT(IsNearly(Extent.MaxX, 100.0f));

    TEST_SECTION("A diagonal stroke keeps its width measured across the line rather than along an axis");
    FDrawCommandList DiagonalList;
    DiagonalList.AddLine(0, Vector2(0.0f, 0.0f), Vector2(100.0f, 100.0f), FFloatColor::White, 4.0f);
    DrawData.BuildFromCommandList(DiagonalList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4);

    const Vector2 Across = DrawData.GetVertices()[0].Position - DrawData.GetVertices()[1].Position;
    TEST_EXPECT(IsNearly(Across.GetLength(), 4.0f));

    TEST_SECTION("Every extra point adds one quad, and the joins share their vertices");
    const Vector2 Path[4] =
    {
        Vector2(0.0f, 0.0f),
        Vector2(50.0f, 0.0f),
        Vector2(50.0f, 50.0f),
        Vector2(0.0f, 50.0f),
    };

    FDrawCommandList OpenList;
    OpenList.AddPolyline(0, TArrayView<const Vector2>(Path, 4), FFloatColor::White, 2.0f, false);
    DrawData.BuildFromCommandList(OpenList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 8);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 3 * 6);

    TEST_SECTION("Closing the run joins the last point back to the first, which is one more segment");
    FDrawCommandList ClosedList;
    ClosedList.AddPolyline(0, TArrayView<const Vector2>(Path, 4), FFloatColor::White, 2.0f, true);
    DrawData.BuildFromCommandList(ClosedList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 8);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 4 * 6);

    TEST_SECTION("A right-angle join miters, so the outer corner reaches the corner of the path");
    Extent = MeasureGeometry(DrawData);
    TEST_EXPECT(IsNearly(Extent.MinX, -1.0f));
    TEST_EXPECT(IsNearly(Extent.MaxX, 51.0f));

    TEST_SECTION("A doubled point does not produce a not-a-number, since the direction is degenerate");
    const Vector2 Repeated[3] = { Vector2(0.0f, 0.0f), Vector2(0.0f, 0.0f), Vector2(20.0f, 0.0f) };

    FDrawCommandList RepeatedList;
    RepeatedList.AddPolyline(0, TArrayView<const Vector2>(Repeated, 3), FFloatColor::White, 2.0f);
    DrawData.BuildFromCommandList(RepeatedList);

    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        TEST_EXPECT(Vertex.Position.X == Vertex.Position.X);
        TEST_EXPECT(Vertex.Position.Y == Vertex.Position.Y);
    }

    TEST_SECTION("A stroke shares the untextured batch with the boxes around it");
    FDrawCommandList MixedList;
    MixedList.AddBox(0, FRectangle(IntVector2(0, 0), 10, 10), FFloatColor::White);
    MixedList.AddLine(1, Vector2(0.0f, 0.0f), Vector2(10.0f, 10.0f), FFloatColor::White, 1.0f);
    DrawData.BuildFromCommandList(MixedList);

    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 1);
    TEST_EXPECT(DrawData.GetBatches()[0].Texture.IsEmpty());

    TEST_END();
}

bool VectorDrawConvexPolygon_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;
    DrawData.SetAntiAliasingEnabled(false);

    TEST_SECTION("A triangle is three vertices and one face");
    FDrawCommandList TriangleList;
    TriangleList.AddTriangle(0, Vector2(0.0f, 0.0f), Vector2(10.0f, 0.0f), Vector2(5.0f, 10.0f), FFloatColor::White);
    DrawData.BuildFromCommandList(TriangleList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 3);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 3);

    TEST_SECTION("The fan turns around the first point, so N points make N minus two faces");
    const Vector2 Quad[4] =
    {
        Vector2(0.0f, 0.0f),
        Vector2(10.0f, 0.0f),
        Vector2(10.0f, 10.0f),
        Vector2(0.0f, 10.0f),
    };

    FDrawCommandList QuadList;
    QuadList.AddConvexPolygon(0, TArrayView<const Vector2>(Quad, 4), FFloatColor::White);
    DrawData.BuildFromCommandList(QuadList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 2 * 3);
    TEST_EXPECT_EQ(DrawData.GetIndices()[0], 0u);
    TEST_EXPECT_EQ(DrawData.GetIndices()[3], 0u);

    TEST_SECTION("The vertices land exactly where the points were, with no rounding to the pixel grid");
    TEST_EXPECT(DrawData.GetVertices()[2].Position == Vector2(10.0f, 10.0f));

    TEST_SECTION("The tint reaches every vertex");
    FDrawCommandList RedList;
    RedList.AddTriangle(0, Vector2(0.0f, 0.0f), Vector2(10.0f, 0.0f), Vector2(5.0f, 10.0f), FFloatColor(1.0f, 0.0f, 0.0f, 1.0f));
    DrawData.BuildFromCommandList(RedList);

    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        TEST_EXPECT_EQ(Vertex.Color, 0xff0000ffu);
    }

    TEST_END();
}

bool VectorDrawCircles_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;
    DrawData.SetAntiAliasingEnabled(false);

    TEST_SECTION("A filled circle is one fan with the segment count it was asked for");
    FDrawCommandList FilledList;
    FilledList.AddCircleFilled(0, Vector2(50.0f, 50.0f), 20.0f, FFloatColor::White, 16);
    DrawData.BuildFromCommandList(FilledList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 16);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 14 * 3);

    TEST_SECTION("Every point sits on the circle");
    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        const Vector2 Offset = Vertex.Position - Vector2(50.0f, 50.0f);
        TEST_EXPECT(IsNearly(Offset.GetLength(), 20.0f, 0.05f));
    }

    TEST_SECTION("An outline is a closed ring, so it has two vertices and one quad per segment");
    FDrawCommandList OutlineList;
    OutlineList.AddCircle(0, Vector2(50.0f, 50.0f), 20.0f, FFloatColor::White, 2.0f, 16);
    DrawData.BuildFromCommandList(OutlineList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 32);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 16 * 6);

    TEST_SECTION("The ring straddles the radius rather than sitting inside or outside it");
    const FGeometryExtent Extent = MeasureGeometry(DrawData);
    TEST_EXPECT(IsNearly(Extent.MinX, 29.0f, 0.2f));
    TEST_EXPECT(IsNearly(Extent.MaxX, 71.0f, 0.2f));

    TEST_SECTION("A bigger circle earns more segments when the caller does not choose");
    FDrawCommandList SmallList;
    SmallList.AddCircleFilled(0, Vector2(0.0f, 0.0f), 4.0f, FFloatColor::White);
    DrawData.BuildFromCommandList(SmallList);
    const int32 SmallVertexCount = DrawData.GetVertices().Size();

    FDrawCommandList LargeList;
    LargeList.AddCircleFilled(0, Vector2(0.0f, 0.0f), 60.0f, FFloatColor::White);
    DrawData.BuildFromCommandList(LargeList);
    const int32 LargeVertexCount = DrawData.GetVertices().Size();

    TEST_EXPECT(LargeVertexCount > SmallVertexCount);
    TEST_EXPECT(SmallVertexCount >= FDrawCommandList::MinCircleSegments);
    TEST_EXPECT(LargeVertexCount <= FDrawCommandList::MaxCircleSegments);

    TEST_SECTION("A rotation arc walks between two angles rather than all the way round");
    FDrawCommandList ArcList;
    ArcList.AddArc(0, Vector2(0.0f, 0.0f), 10.0f, 0.0f, Math::Constants::HalfPI, FFloatColor::White, 1.0f, 8);
    DrawData.BuildFromCommandList(ArcList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 9 * 2);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 8 * 6);

    TEST_SECTION("The drag wedge is a fan led by the center, which closes it back through the pivot");
    FDrawCommandList WedgeList;
    WedgeList.AddArcFilled(0, Vector2(0.0f, 0.0f), 10.0f, 0.0f, Math::Constants::HalfPI, FFloatColor::White, 8);
    DrawData.BuildFromCommandList(WedgeList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 1 + 9);
    TEST_EXPECT(DrawData.GetVertices()[0].Position == Vector2(0.0f, 0.0f));

    TEST_SECTION("A circle with no radius draws nothing");
    FDrawCommandList EmptyList;
    EmptyList.AddCircle(0, Vector2(0.0f, 0.0f), 0.0f, FFloatColor::White, 1.0f);
    EmptyList.AddCircleFilled(0, Vector2(0.0f, 0.0f), -5.0f, FFloatColor::White);
    TEST_EXPECT(EmptyList.IsEmpty());

    TEST_END();
}

bool VectorDrawBezier_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;
    DrawData.SetAntiAliasingEnabled(false);

    TEST_SECTION("A bezier is flattened into the number of segments it was asked for");
    FDrawCommandList CurveList;
    CurveList.AddBezier(0, Vector2(0.0f, 0.0f), Vector2(50.0f, 0.0f), Vector2(50.0f, 100.0f), Vector2(100.0f, 100.0f), FFloatColor::White, 2.0f, 16);

    TEST_EXPECT_EQ(CurveList.CountCommandsOfType(EDrawCommandType::Polyline), 1);
    TEST_EXPECT_EQ(CurveList[0].PayloadCount, 17);

    TEST_SECTION("The curve starts and ends on its endpoints");
    const TArrayView<const Vector2> Points = CurveList.GetCommandPoints(CurveList[0]);
    TEST_EXPECT(Points[0] == Vector2(0.0f, 0.0f));
    TEST_EXPECT(Points[Points.Size() - 1] == Vector2(100.0f, 100.0f));

    TEST_SECTION("It stays inside the hull of its control points, which is what makes a link readable");
    for (const Vector2& Point : Points)
    {
        TEST_EXPECT(Point.X >= -GTolerance && Point.X <= 100.0f + GTolerance);
        TEST_EXPECT(Point.Y >= -GTolerance && Point.Y <= 100.0f + GTolerance);
    }

    TEST_SECTION("A straight control polygon flattens onto the straight line between the ends");
    FDrawCommandList StraightList;
    StraightList.AddBezier(0, Vector2(0.0f, 0.0f), Vector2(25.0f, 0.0f), Vector2(75.0f, 0.0f), Vector2(100.0f, 0.0f), FFloatColor::White, 1.0f, 8);

    for (const Vector2& Point : StraightList.GetCommandPoints(StraightList[0]))
    {
        TEST_EXPECT(IsNearly(Point.Y, 0.0f));
    }

    TEST_SECTION("A longer curve earns more segments when the caller does not choose");
    FDrawCommandList ShortList;
    ShortList.AddBezier(0, Vector2(0.0f, 0.0f), Vector2(2.0f, 0.0f), Vector2(4.0f, 0.0f), Vector2(6.0f, 0.0f), FFloatColor::White, 1.0f);

    FDrawCommandList LongList;
    LongList.AddBezier(0, Vector2(0.0f, 0.0f), Vector2(300.0f, 0.0f), Vector2(600.0f, 200.0f), Vector2(900.0f, 200.0f), FFloatColor::White, 1.0f);

    TEST_EXPECT(LongList[0].PayloadCount > ShortList[0].PayloadCount);
    TEST_EXPECT(ShortList[0].PayloadCount >= FDrawCommandList::MinBezierSegments + 1);
    TEST_EXPECT(LongList[0].PayloadCount <= FDrawCommandList::MaxBezierSegments + 1);

    TEST_SECTION("The flattened curve tessellates as an ordinary stroke");
    DrawData.BuildFromCommandList(CurveList);
    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 17 * 2);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 16 * 6);

    TEST_END();
}

bool VectorDrawPerCornerRounding_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;
    DrawData.SetAntiAliasingEnabled(false);

    const FRectangle Bounds(IntVector2(0, 0), 100, 40);

    TEST_SECTION("A box rounded on the top two corners keeps the bottom two square");
    FDrawCommandList TitleBarList;
    TitleBarList.AddBox(0, Bounds, FFloatColor::White, FCornerRadii::Top(8.0f));
    DrawData.BuildFromCommandList(TitleBarList);

    TEST_EXPECT_EQ(DrawData.GetShapeVertices().Size(), 4);
    TEST_EXPECT(DrawData.GetVertices().IsEmpty());
    TEST_EXPECT_EQ(DrawData.GetShapeVertices()[0].RadiusTL, 8.0f);
    TEST_EXPECT_EQ(DrawData.GetShapeVertices()[0].RadiusTR, 8.0f);
    TEST_EXPECT_EQ(DrawData.GetShapeVertices()[0].RadiusBL, 0.0f);
    TEST_EXPECT_EQ(DrawData.GetShapeVertices()[0].RadiusBR, 0.0f);

    TEST_SECTION("A uniformly rounded box still uses one quad, with every radius set");
    FDrawCommandList UniformList;
    UniformList.AddBox(0, Bounds, FFloatColor::White, FCornerRadii(8.0f));
    DrawData.BuildFromCommandList(UniformList);

    TEST_EXPECT_EQ(DrawData.GetShapeVertices().Size(), 4);
    TEST_EXPECT_EQ(DrawData.GetShapeVertices()[0].RadiusTL, 8.0f);
    TEST_EXPECT_EQ(DrawData.GetShapeVertices()[0].RadiusBR, 8.0f);

    TEST_SECTION("The shape still fills the whole rectangle");
    const FGeometryExtent Extent = MeasureGeometry(DrawData);
    TEST_EXPECT(IsNearly(Extent.MinX, 0.0f));
    TEST_EXPECT(IsNearly(Extent.MinY, 0.0f));
    TEST_EXPECT(IsNearly(Extent.MaxX, 100.0f));
    TEST_EXPECT(IsNearly(Extent.MaxY, 40.0f));

    TEST_SECTION("A radius past half the shorter side is clamped rather than turned inside out");
    FDrawCommandList OversizedList;
    OversizedList.AddBox(0, FRectangle(IntVector2(0, 0), 10, 10), FFloatColor::White, FCornerRadii(0.0f, 50.0f, 0.0f, 0.0f));
    DrawData.BuildFromCommandList(OversizedList);

    TEST_EXPECT_EQ(DrawData.GetShapeVertices().Size(), 4);
    TEST_EXPECT_EQ(DrawData.GetShapeVertices()[0].RadiusTR, 5.0f);

    for (const FUIShapeVertex& Vertex : DrawData.GetShapeVertices())
    {
        TEST_EXPECT(Vertex.Position.X >= -GTolerance && Vertex.Position.X <= 10.0f + GTolerance);
        TEST_EXPECT(Vertex.Position.Y >= -GTolerance && Vertex.Position.Y <= 10.0f + GTolerance);
    }

    TEST_END();
}

bool VectorDrawBoxOutline_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;
    DrawData.SetAntiAliasingEnabled(false);

    TEST_SECTION("A square outline is one SDF stroke quad");
    FDrawCommandList OutlineList;
    OutlineList.AddBoxOutline(0, FRectangle(IntVector2(0, 0), 100, 40), FFloatColor::White, 2.0f);
    DrawData.BuildFromCommandList(OutlineList);

    TEST_EXPECT_EQ(DrawData.GetShapeVertices().Size(), 4);
    TEST_EXPECT_EQ(DrawData.GetShapeIndices().Size(), 6);
    TEST_EXPECT_EQ(DrawData.GetShapeVertices()[0].Thickness, 2.0f);
    TEST_EXPECT_EQ(DrawData.GetShapeVertices()[0].ShapeKind, FUIDrawData::ShapeKindStroke);

    TEST_SECTION("The stroke stays inside the rectangle it was asked to outline");
    const FGeometryExtent Extent = MeasureGeometry(DrawData);
    TEST_EXPECT(Extent.MinX >= -GTolerance);
    TEST_EXPECT(Extent.MinY >= -GTolerance);
    TEST_EXPECT(Extent.MaxX <= 100.0f + GTolerance);
    TEST_EXPECT(Extent.MaxY <= 40.0f + GTolerance);

    TEST_SECTION("It reaches the edges rather than hugging the middle");
    TEST_EXPECT(IsNearly(Extent.MinX, 0.0f));
    TEST_EXPECT(IsNearly(Extent.MaxX, 100.0f));

    TEST_SECTION("A one pixel outline lands on the outermost row rather than straddling the edge");
    FDrawCommandList HairlineList;
    HairlineList.AddBoxOutline(0, FRectangle(IntVector2(0, 0), 100, 40), FFloatColor::White, 1.0f);
    DrawData.BuildFromCommandList(HairlineList);

    const FGeometryExtent HairlineExtent = MeasureGeometry(DrawData);
    TEST_EXPECT(HairlineExtent.MinX >= -GTolerance);
    TEST_EXPECT(HairlineExtent.MinY >= -GTolerance);
    TEST_EXPECT(HairlineExtent.MaxX <= 100.0f + GTolerance);
    TEST_EXPECT(HairlineExtent.MaxY <= 40.0f + GTolerance);
    TEST_EXPECT(IsNearly(HairlineExtent.MinX, 0.0f));
    TEST_EXPECT(IsNearly(HairlineExtent.MaxY, 40.0f));

    TEST_SECTION("A rounded outline follows the same corners the fill does");
    FDrawCommandList RoundedList;
    RoundedList.AddBoxOutline(0, FRectangle(IntVector2(0, 0), 100, 40), FFloatColor::White, 2.0f, FCornerRadii(6.0f));
    DrawData.BuildFromCommandList(RoundedList);

    TEST_EXPECT_EQ(DrawData.GetShapeVertices().Size(), 4);
    TEST_EXPECT_EQ(DrawData.GetShapeVertices()[0].RadiusTL, 6.0f);

    TEST_SECTION("An outline thicker than the rectangle fills it instead of folding through itself");
    FDrawCommandList ThickList;
    ThickList.AddBoxOutline(0, FRectangle(IntVector2(0, 0), 10, 10), FFloatColor::White, 40.0f);
    DrawData.BuildFromCommandList(ThickList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 6);

    TEST_END();
}

bool VectorDrawClipCulling_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;
    DrawData.SetAntiAliasingEnabled(false);

    TEST_SECTION("A stroke that misses the region is dropped rather than scissored away");
    const Vector2 FarPath[2] = { Vector2(500.0f, 500.0f), Vector2(600.0f, 600.0f) };

    FDrawCommandList CulledList;
    CulledList.PushClip(0, FRectangle(IntVector2(0, 0), 50, 50));
    CulledList.AddPolyline(1, TArrayView<const Vector2>(FarPath, 2), FFloatColor::White, 2.0f);
    CulledList.PopClip(2);

    DrawData.BuildFromCommandList(CulledList);
    TEST_EXPECT(DrawData.IsEmpty());

    TEST_SECTION("One that only overlaps it is kept whole, since the scissor takes care of the rest");
    const Vector2 NearPath[2] = { Vector2(40.0f, 40.0f), Vector2(140.0f, 140.0f) };

    FDrawCommandList OverlappingList;
    OverlappingList.PushClip(0, FRectangle(IntVector2(0, 0), 50, 50));
    OverlappingList.AddPolyline(1, TArrayView<const Vector2>(NearPath, 2), FFloatColor::White, 2.0f);
    OverlappingList.PopClip(2);

    DrawData.BuildFromCommandList(OverlappingList);
    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4);
    TEST_EXPECT(DrawData.GetBatches()[0].bIsClipped);

    TEST_SECTION("A polygon is culled the same way");
    const Vector2 FarTriangle[3] = { Vector2(500.0f, 500.0f), Vector2(520.0f, 500.0f), Vector2(510.0f, 520.0f) };

    FDrawCommandList PolygonList;
    PolygonList.PushClip(0, FRectangle(IntVector2(0, 0), 50, 50));
    PolygonList.AddConvexPolygon(1, TArrayView<const Vector2>(FarTriangle, 3), FFloatColor::White);
    PolygonList.PopClip(2);

    DrawData.BuildFromCommandList(PolygonList);
    TEST_EXPECT(DrawData.IsEmpty());

    TEST_SECTION("A stroke just outside the edge survives, since half its width spills back over");
    const Vector2 EdgePath[2] = { Vector2(10.0f, -2.0f), Vector2(40.0f, -2.0f) };

    FDrawCommandList EdgeList;
    EdgeList.PushClip(0, FRectangle(IntVector2(0, 0), 50, 50));
    EdgeList.AddPolyline(1, TArrayView<const Vector2>(EdgePath, 2), FFloatColor::White, 6.0f);
    EdgeList.PopClip(2);

    DrawData.BuildFromCommandList(EdgeList);
    TEST_EXPECT(!DrawData.IsEmpty());

    TEST_END();
}
