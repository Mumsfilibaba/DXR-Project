#include "GizmoTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Gizmo/Gizmo.h>
#include <Application/Input/Keys.h>

// A viewport three units wide for every two tall, so the aspect correction has something to do
constexpr int32 ViewportWidth  = 800;
constexpr int32 ViewportHeight = 600;

// An orthographic view that puts exactly one hundred pixels on a world unit, in both directions
constexpr float OrthoWidth  = 8.0f;
constexpr float OrthoHeight = 6.0f;

static FRectangle GetViewport()
{
    return FRectangle(IntVector2(0, 0), ViewportWidth, ViewportHeight);
}

/** @brief Straight down the positive Z axis, so screen space and world space line up. */
static Matrix4 MakeFrontView()
{
    return Matrix4::LookAt(Vector3(0.0f, 0.0f, -10.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
}

/** @brief A corner view, which is the only one where all three rings are worth picking. */
static Matrix4 MakeCornerView()
{
    return Matrix4::LookAt(Vector3(6.0f, 5.0f, -7.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
}

static Matrix4 MakeOrthographic()
{
    return Matrix4::OrthographicProjection(OrthoWidth, OrthoHeight, 0.1f, 100.0f);
}

static Matrix4 MakePerspective()
{
    return Matrix4::PerspectiveProjection(Math::Constants::PI * 0.25f, 4.0f / 3.0f, 0.1f, 100.0f);
}

/** @brief Where a world point lands, which is how a test aims at a handle it cannot see. */
static IntVector2 ToPixel(const Matrix4& View, const Matrix4& Projection, const Vector3& WorldPosition)
{
    Vector2 Client(0.0f, 0.0f);
    FGizmoMath::WorldToClient(View * Projection, GetViewport(), WorldPosition, Client);

    return IntVector2(Math::RoundToInt(Client.X), Math::RoundToInt(Client.Y));
}

static TSharedPtr<FGizmo> MakeGizmo(EGizmoOperation Operation, EGizmoMode Mode, const FGizmo::FDesc& Base = FGizmo::FDesc())
{
    FGizmo::FDesc Desc = Base;
    Desc.Operation     = Operation;
    Desc.Mode          = Mode;

    TSharedPtr<FGizmo> Gizmo = FGizmo::Create(Desc);
    Gizmo->PrepareDesiredSize();
    Gizmo->Tick(GetViewport());
    Gizmo->SetCamera(MakeFrontView(), MakeOrthographic(), true);

    return Gizmo;
}

static FCursorEvent MakeMoveEvent(const IntVector2& ClientPosition)
{
    return FCursorEvent(EInputEventType::MouseMoved, ClientPosition, IntVector2(0, 0), FModifierKeyState());
}

static FCursorEvent MakeButtonEvent(EInputEventType Type, const IntVector2& ClientPosition)
{
    return FCursorEvent(Type, Keys::MouseButtonLeft, ClientPosition, IntVector2(0, 0), FModifierKeyState(), Type == EInputEventType::MouseButtonDown);
}

/** @brief Presses, moves and releases, which is one drag end to end. */
static void DragGizmo(const TSharedPtr<FGizmo>& Gizmo, const IntVector2& From, const IntVector2& To)
{
    Gizmo->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, From));
    Gizmo->OnMouseMove(MakeMoveEvent(To));
    Gizmo->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, To));
}

static bool IsNearly(float First, float Second, float Tolerance = 0.001f)
{
    return Math::Abs(First - Second) <= Tolerance;
}

static bool IsNearly(const Vector2& First, const Vector2& Second, float Tolerance = 0.01f)
{
    return IsNearly(First.X, Second.X, Tolerance) && IsNearly(First.Y, Second.Y, Tolerance);
}

static bool IsNearly(const Vector3& First, const Vector3& Second, float Tolerance = 0.005f)
{
    return IsNearly(First.X, Second.X, Tolerance) && IsNearly(First.Y, Second.Y, Tolerance) && IsNearly(First.Z, Second.Z, Tolerance);
}

static Vector3 GetRow(const Matrix4& Matrix, int32 RowIndex)
{
    const Vector4 Row = Matrix.GetRow(RowIndex);
    return Vector3(Row.X, Row.Y, Row.Z);
}

bool GizmoProjection_Test()
{
    TEST_BEGIN();

    const Matrix4    ViewProjection = MakeFrontView() * MakeOrthographic();
    const FRectangle Viewport       = GetViewport();

    TEST_SECTION("A known world point lands on a known pixel");
    Vector2 Client(0.0f, 0.0f);

    TEST_EXPECT(FGizmoMath::WorldToClient(ViewProjection, Viewport, Vector3(0.0f, 0.0f, 0.0f), Client));
    TEST_EXPECT(IsNearly(Client, Vector2(400.0f, 300.0f)));

    TEST_EXPECT(FGizmoMath::WorldToClient(ViewProjection, Viewport, Vector3(1.0f, 0.0f, 0.0f), Client));
    TEST_EXPECT(IsNearly(Client, Vector2(500.0f, 300.0f)));

    TEST_SECTION("Up in the world is up on the screen, which is down the pixels");
    TEST_EXPECT(FGizmoMath::WorldToClient(ViewProjection, Viewport, Vector3(0.0f, 1.0f, 0.0f), Client));
    TEST_EXPECT(IsNearly(Client, Vector2(400.0f, 200.0f)));

    TEST_SECTION("A point behind the camera has nowhere to land");
    const Matrix4 PerspectiveViewProjection = MakeFrontView() * MakePerspective();

    TEST_EXPECT(FGizmoMath::WorldToClient(PerspectiveViewProjection, Viewport, Vector3(0.0f, 0.0f, 0.0f), Client));
    TEST_EXPECT(!FGizmoMath::WorldToClient(PerspectiveViewProjection, Viewport, Vector3(0.0f, 0.0f, -20.0f), Client));

    TEST_SECTION("The ray through a pixel passes back through the point that projected onto it");
    const Vector3 Sample(0.5f, -0.25f, 0.0f);

    Vector3 RayOrigin;
    Vector3 RayDirection;
    FGizmoMath::ComputeCameraRay(ViewProjection, Viewport, ToPixel(MakeFrontView(), MakeOrthographic(), Sample), RayOrigin, RayDirection);

    const Vector3 Closest = RayOrigin + (RayDirection * (Sample - RayOrigin).DotProduct(RayDirection));
    TEST_EXPECT(IsNearly(Closest, Sample));

    TEST_SECTION("A ray meets the plane it is cast at, and misses the one behind it");
    Vector3 Hit;
    TEST_EXPECT(FGizmoMath::IntersectRayPlane(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), RayOrigin, RayDirection, Hit));
    TEST_EXPECT(IsNearly(Hit, Sample));

    TEST_EXPECT(!FGizmoMath::IntersectRayPlane(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 5.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Hit));

    TEST_SECTION("The plane an axis is dragged on holds that axis and turns to face the camera");
    const Vector3 PlaneNormal = FGizmoMath::ComputeAxisPlaneNormal(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f));

    TEST_EXPECT(IsNearly(PlaneNormal.DotProduct(Vector3(1.0f, 0.0f, 0.0f)), 0.0f));
    TEST_EXPECT(IsNearly(Math::Abs(PlaneNormal.DotProduct(Vector3(0.0f, 0.0f, 1.0f))), 1.0f));

    TEST_SECTION("Distance to a segment measures across it between the ends and to an end beyond them");
    TEST_EXPECT(IsNearly(FGizmoMath::DistanceToSegment(Vector2(5.0f, 3.0f), Vector2(0.0f, 0.0f), Vector2(10.0f, 0.0f)), 3.0f));
    TEST_EXPECT(IsNearly(FGizmoMath::DistanceToSegment(Vector2(-4.0f, 0.0f), Vector2(0.0f, 0.0f), Vector2(10.0f, 0.0f)), 4.0f));
    TEST_EXPECT(IsNearly(FGizmoMath::DistanceToSegment(Vector2(0.0f, 0.0f), Vector2(2.0f, 2.0f), Vector2(2.0f, 2.0f)), Math::Sqrt(8.0f)));

    TEST_SECTION("A quad takes the points inside it, and those within the padding of an edge");
    const Vector2 Quad[4] =
    {
        Vector2(0.0f, 0.0f),
        Vector2(10.0f, 0.0f),
        Vector2(10.0f, 10.0f),
        Vector2(0.0f, 10.0f),
    };

    TEST_EXPECT(FGizmoMath::IsPointInsideQuad(Vector2(5.0f, 5.0f), Quad));
    TEST_EXPECT(!FGizmoMath::IsPointInsideQuad(Vector2(15.0f, 5.0f), Quad));
    TEST_EXPECT(FGizmoMath::IsPointOverQuad(Vector2(13.0f, 5.0f), Quad, 6.0f));
    TEST_EXPECT(!FGizmoMath::IsPointOverQuad(Vector2(20.0f, 5.0f), Quad, 6.0f));

    TEST_SECTION("Snapping quantises to the nearest stop, and does nothing at all when it is off");
    TEST_EXPECT_EQ(FGizmoMath::SnapValue(1.3f, 0.25f), 1.25f);
    TEST_EXPECT_EQ(FGizmoMath::SnapValue(1.4f, 0.25f), 1.5f);
    TEST_EXPECT_EQ(FGizmoMath::SnapValue(-1.3f, 0.25f), -1.25f);
    TEST_EXPECT_EQ(FGizmoMath::SnapValue(7.7f, 0.0f), 7.7f);
    TEST_EXPECT(IsNearly(FGizmoMath::SnapVector(Vector3(1.3f, -0.4f, 2.6f), 0.5f), Vector3(1.5f, -0.5f, 2.5f)));

    TEST_SECTION("An axis pointing at the camera projects to nothing, which is how it is spotted");
    const float AspectRatio = static_cast<float>(ViewportWidth) / static_cast<float>(ViewportHeight);

    const float AcrossLength = FGizmoMath::GetSegmentLengthInClipSpace(ViewProjection, AspectRatio, Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));
    const float AwayLength   = FGizmoMath::GetSegmentLengthInClipSpace(ViewProjection, AspectRatio, Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f));

    TEST_EXPECT(AcrossLength > 0.2f);
    TEST_EXPECT(AwayLength < 0.0001f);

    TEST_SECTION("A plane seen edge-on covers no area, and one facing the camera covers plenty");
    const float FacingArea = FGizmoMath::GetParallelogramArea(ViewProjection, AspectRatio, Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
    const float EdgeOnArea = FGizmoMath::GetParallelogramArea(ViewProjection, AspectRatio, Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f));

    TEST_EXPECT(FacingArea > 0.05f);
    TEST_EXPECT(EdgeOnArea < 0.0001f);

    TEST_SECTION("The screen factor holds the gizmo one size on screen however far away it is");
    const Vector3 CameraRight(1.0f, 0.0f, 0.0f);

    const float NearFactor = FGizmoMath::ComputeScreenFactor(ViewProjection, CameraRight, Vector3(0.0f, 0.0f, 0.0f), FGizmo::SizeInClipSpace);
    const float FarFactor  = FGizmoMath::ComputeScreenFactor(ViewProjection, CameraRight, Vector3(0.0f, 0.0f, 40.0f), FGizmo::SizeInClipSpace);

    TEST_EXPECT(IsNearly(NearFactor, 0.4f));
    TEST_EXPECT(IsNearly(FarFactor, NearFactor));

    TEST_SECTION("Under perspective the same size on screen costs twice the world units at twice the distance");
    const float CloseFactor  = FGizmoMath::ComputeScreenFactor(PerspectiveViewProjection, CameraRight, Vector3(0.0f, 0.0f, 0.0f), FGizmo::SizeInClipSpace);
    const float DistantFactor = FGizmoMath::ComputeScreenFactor(PerspectiveViewProjection, CameraRight, Vector3(0.0f, 0.0f, 10.0f), FGizmo::SizeInClipSpace);

    TEST_EXPECT(IsNearly(DistantFactor, CloseFactor * 2.0f, 0.01f));

    TEST_END();
}

bool GizmoHitTest_Test()
{
    TEST_BEGIN();

    TSharedPtr<FGizmo> Gizmo = MakeGizmo(EGizmoOperation::Translate, EGizmoMode::World);

    TEST_SECTION("The gizmo reaches as far as it needs to for the size it wants on screen");
    TEST_EXPECT(IsNearly(Gizmo->GetScreenFactor(), 0.48f));
    TEST_EXPECT(Gizmo->IsProjected());

    TEST_SECTION("An axis pointing at the camera is culled, and so are the planes standing edge-on");
    TEST_EXPECT(Gizmo->IsAxisVisible(0));
    TEST_EXPECT(Gizmo->IsAxisVisible(1));
    TEST_EXPECT(!Gizmo->IsAxisVisible(2));

    TEST_EXPECT(Gizmo->IsPlaneVisible(2));
    TEST_EXPECT(!Gizmo->IsPlaneVisible(0));
    TEST_EXPECT(!Gizmo->IsPlaneVisible(1));

    TEST_SECTION("The cursor on a shaft picks that axis, and thirty pixels off it picks nothing");
    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(423, 300)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::TranslateX);
    TEST_EXPECT(Gizmo->IsHovered());

    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(400, 277)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::TranslateY);

    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(423, 330)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::None);
    TEST_EXPECT(!Gizmo->IsHovered());

    TEST_SECTION("A host re-pushing what the gizmo already has keeps the hover, since nothing moved under the cursor");
    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(423, 300)));

    Gizmo->SetOperation(EGizmoOperation::Translate);
    Gizmo->SetMode(EGizmoMode::World);
    Gizmo->SetCamera(MakeFrontView(), MakeOrthographic(), true);
    Gizmo->SetTransform(Matrix4::Identity());

    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::TranslateX);

    TEST_SECTION("Changing the handle set does drop it, since the handle it named is gone with the set");
    Gizmo->SetOperation(EGizmoOperation::Rotate);
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::None);

    Gizmo->SetOperation(EGizmoOperation::Translate);
    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(423, 300)));

    Gizmo->SetMode(EGizmoMode::Local);
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::None);

    Gizmo->SetMode(EGizmoMode::World);

    TEST_SECTION("The square at the pivot takes the cursor before any axis does");
    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(400, 300)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::TranslateScreen);

    TEST_SECTION("A plane handle takes the cursor inside its quad and just outside its edge");
    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(420, 280)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::TranslateXY);

    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(427, 280)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::TranslateXY);

    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(444, 280)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::None);

    TEST_SECTION("A press on nothing is left for whatever is behind the gizmo");
    const FEventResponse Response = Gizmo->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(700, 500)));
    TEST_EXPECT(!Response.IsEventHandled());
    TEST_EXPECT(!Gizmo->IsDragging());

    TEST_SECTION("Rotation picks the ring the cursor is on, and the outer ring outside them all");
    Gizmo->SetCamera(MakeCornerView(), MakeOrthographic(), true);
    Gizmo->SetOperation(EGizmoOperation::Rotate);

    const float   Reach     = Gizmo->GetScreenFactor();
    const float   Diagonal  = Reach * Math::Constants::InvSqrt2;
    const Vector3 OnZRing(Diagonal, Diagonal, 0.0f);

    Gizmo->OnMouseMove(MakeMoveEvent(ToPixel(MakeCornerView(), MakeOrthographic(), OnZRing)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::RotateZ);

    const Vector3 OnXRing(0.0f, Diagonal, Diagonal);
    Gizmo->OnMouseMove(MakeMoveEvent(ToPixel(MakeCornerView(), MakeOrthographic(), OnXRing)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::RotateX);

    const Matrix4 InverseCornerView = MakeCornerView().GetInverse();
    const Vector3 CameraUp          = GetRow(InverseCornerView, 1).GetNormalized();
    const Vector3 OnOuterRing       = CameraUp * (Reach * FGizmo::OuterRingScale);

    Gizmo->OnMouseMove(MakeMoveEvent(ToPixel(MakeCornerView(), MakeOrthographic(), OnOuterRing)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::RotateScreen);

    TEST_SECTION("Scale picks the knob at the end of an axis, and the one at the pivot");
    Gizmo->SetCamera(MakeFrontView(), MakeOrthographic(), true);
    Gizmo->SetOperation(EGizmoOperation::Scale);

    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(440, 300)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::ScaleX);

    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(400, 300)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::ScaleUniform);

    TEST_SECTION("Scale has the same plane quads translate does, in the same places");
    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(420, 280)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::ScaleXY);

    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(444, 280)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::None);

    TEST_SECTION("Nothing is drawn and nothing can be grabbed while the pivot is behind the camera");
    Gizmo->SetCamera(MakeFrontView(), MakePerspective(), false);
    Gizmo->SetTransform(Matrix4::Translation(Vector3(0.0f, 0.0f, -30.0f)));

    TEST_EXPECT(!Gizmo->IsProjected());

    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(400, 300)));
    TEST_EXPECT(Gizmo->GetHoveredHandle() == EGizmoHandle::None);

    FDrawCommandList CommandList;
    Gizmo->OnDraw(FDrawGeometry(Gizmo->GetContentRectangle(), 1.0f), CommandList, 0);
    TEST_EXPECT(CommandList.GetCommands().IsEmpty());

    TEST_SECTION("Back in front of the camera it draws again");
    Gizmo->SetTransform(Matrix4::Identity());
    TEST_EXPECT(Gizmo->IsProjected());

    FDrawCommandList VisibleList;
    Gizmo->OnDraw(FDrawGeometry(Gizmo->GetContentRectangle(), 1.0f), VisibleList, 0);
    TEST_EXPECT(!VisibleList.GetCommands().IsEmpty());

    TEST_END();
}

bool GizmoDrag_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    int32   Changes = 0;
    Matrix4 LastDelta = Matrix4::Identity();

    FGizmo::FDesc Desc;
    Desc.OnTransformChanged = FOnGizmoTransformChanged::CreateLambda([&Changes, &LastDelta](const Matrix4&, const Matrix4& Delta)
    {
        Changes++;
        LastDelta = Delta;
    });

    TSharedPtr<FGizmo> Gizmo = MakeGizmo(EGizmoOperation::Translate, EGizmoMode::World, Desc);

    TEST_SECTION("A drag along an axis moves along it and leaves the other two alone");
    DragGizmo(Gizmo, IntVector2(423, 300), IntVector2(523, 300));

    TEST_EXPECT(IsNearly(Gizmo->GetTransform().GetTranslation(), Vector3(1.0f, 0.0f, 0.0f)));
    TEST_EXPECT_EQ(Changes, 1);
    TEST_EXPECT(IsNearly(LastDelta.GetTranslation(), Vector3(1.0f, 0.0f, 0.0f)));
    TEST_EXPECT(!Gizmo->IsDragging());

    TEST_SECTION("A cursor wandering off the axis still only moves along it");
    Gizmo->SetTransform(Matrix4::Identity());
    DragGizmo(Gizmo, IntVector2(423, 300), IntVector2(523, 380));

    TEST_EXPECT(IsNearly(Gizmo->GetTransform().GetTranslation(), Vector3(1.0f, 0.0f, 0.0f)));

    TEST_SECTION("Snapping quantises the step the drag made");
    FGizmoSnapSettings Snap;
    Snap.TranslationSnap = 0.25f;

    Gizmo->SetSnap(Snap);
    Gizmo->SetTransform(Matrix4::Identity());
    DragGizmo(Gizmo, IntVector2(423, 300), IntVector2(553, 300));

    TEST_EXPECT(IsNearly(Gizmo->GetTransform().GetTranslation(), Vector3(1.25f, 0.0f, 0.0f)));

    Gizmo->SetSnap(FGizmoSnapSettings());

    TEST_SECTION("A plane handle moves in both of the axes it spans");
    Gizmo->SetTransform(Matrix4::Identity());
    DragGizmo(Gizmo, IntVector2(420, 280), IntVector2(520, 380));

    TEST_EXPECT(IsNearly(Gizmo->GetTransform().GetTranslation(), Vector3(1.0f, -1.0f, 0.0f)));

    TEST_SECTION("The delta is the step from where the drag began, so it lands on the same transform");
    const Matrix4 Composed = Matrix4::Identity() * LastDelta;
    TEST_EXPECT(Composed.IsEqual(Gizmo->GetTransform()));

    TEST_SECTION("A ring turns the transform about its own axis");
    Gizmo->SetCamera(MakeCornerView(), MakeOrthographic(), true);
    Gizmo->SetOperation(EGizmoOperation::Rotate);
    Gizmo->SetTransform(Matrix4::Identity());

    const float   Reach    = Gizmo->GetScreenFactor();
    const float   Diagonal = Reach * Math::Constants::InvSqrt2;
    const Vector3 RingStart(Diagonal, Diagonal, 0.0f);
    const Vector3 RingEnd(-Diagonal, Diagonal, 0.0f);

    DragGizmo(Gizmo,
        ToPixel(MakeCornerView(), MakeOrthographic(), RingStart),
        ToPixel(MakeCornerView(), MakeOrthographic(), RingEnd));

    const Matrix4 Turned = Gizmo->GetTransform();
    TEST_EXPECT(IsNearly(GetRow(Turned, 0), Vector3(0.0f, 1.0f, 0.0f), 0.05f));
    TEST_EXPECT(IsNearly(GetRow(Turned, 1), Vector3(-1.0f, 0.0f, 0.0f), 0.05f));
    TEST_EXPECT(IsNearly(GetRow(Turned, 2), Vector3(0.0f, 0.0f, 1.0f), 0.05f));

    TEST_SECTION("A rotation snap holds the turn to whole increments");
    Snap = FGizmoSnapSettings();
    Snap.RotationSnapDegrees = 45.0f;

    Gizmo->SetSnap(Snap);
    Gizmo->SetTransform(Matrix4::Identity());

    const float   AtNinetyFive = Math::Constants::PI * (95.0f / 180.0f);
    const Vector3 RingAt95(Math::Cos(AtNinetyFive) * Reach, Math::Sin(AtNinetyFive) * Reach, 0.0f);

    DragGizmo(Gizmo,
        ToPixel(MakeCornerView(), MakeOrthographic(), RingStart),
        ToPixel(MakeCornerView(), MakeOrthographic(), RingAt95));

    const Matrix4 Snapped = Gizmo->GetTransform();
    TEST_EXPECT(IsNearly(GetRow(Snapped, 0), Vector3(Math::Constants::InvSqrt2, Math::Constants::InvSqrt2, 0.0f), 0.02f));

    Gizmo->SetSnap(FGizmoSnapSettings());

    TEST_SECTION("A scale knob multiplies the axis it belongs to and nothing else");
    Gizmo->SetCamera(MakeFrontView(), MakeOrthographic(), true);
    Gizmo->SetOperation(EGizmoOperation::Scale);
    Gizmo->SetTransform(Matrix4::Identity());

    DragGizmo(Gizmo, IntVector2(440, 300), IntVector2(480, 300));

    const Matrix4 Scaled = Gizmo->GetTransform();
    TEST_EXPECT(IsNearly(GetRow(Scaled, 0), Vector3(2.0f, 0.0f, 0.0f)));
    TEST_EXPECT(IsNearly(GetRow(Scaled, 1), Vector3(0.0f, 1.0f, 0.0f)));
    TEST_EXPECT(IsNearly(GetRow(Scaled, 2), Vector3(0.0f, 0.0f, 1.0f)));
    TEST_EXPECT(IsNearly(Scaled.GetTranslation(), Vector3(0.0f, 0.0f, 0.0f)));

    TEST_SECTION("A scale snap holds the factor to whole increments");
    Snap = FGizmoSnapSettings();
    Snap.ScaleSnap = 0.5f;

    Gizmo->SetSnap(Snap);
    Gizmo->SetTransform(Matrix4::Identity());
    DragGizmo(Gizmo, IntVector2(440, 300), IntVector2(492, 300));

    TEST_EXPECT(IsNearly(GetRow(Gizmo->GetTransform(), 0), Vector3(2.5f, 0.0f, 0.0f)));

    Gizmo->SetSnap(FGizmoSnapSettings());

    TEST_SECTION("A scale plane handle carries a factor of its own for each of the two axes it spans");
    Gizmo->SetTransform(Matrix4::Identity());
    DragGizmo(Gizmo, IntVector2(420, 280), IntVector2(440, 270));

    const Matrix4 Planar = Gizmo->GetTransform();
    TEST_EXPECT(IsNearly(GetRow(Planar, 0), Vector3(2.0f, 0.0f, 0.0f)));
    TEST_EXPECT(IsNearly(GetRow(Planar, 1), Vector3(0.0f, 1.5f, 0.0f)));
    TEST_EXPECT(IsNearly(GetRow(Planar, 2), Vector3(0.0f, 0.0f, 1.0f)));

    TEST_SECTION("Universal scale drags every axis together, whichever handle it was grabbed by");
    Gizmo->SetOperation(EGizmoOperation::UniversalScale);
    Gizmo->SetTransform(Matrix4::Identity());

    DragGizmo(Gizmo, IntVector2(440, 300), IntVector2(480, 300));

    const Matrix4 Universal = Gizmo->GetTransform();
    TEST_EXPECT(IsNearly(GetRow(Universal, 0), Vector3(2.0f, 0.0f, 0.0f)));
    TEST_EXPECT(IsNearly(GetRow(Universal, 1), Vector3(0.0f, 2.0f, 0.0f)));
    TEST_EXPECT(IsNearly(GetRow(Universal, 2), Vector3(0.0f, 0.0f, 2.0f)));

    TEST_END();
}

bool GizmoDeltaContract_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    Matrix4 Compounded = Matrix4::Identity();
    Matrix4 LastDelta  = Matrix4::Identity();
    int32   Changes    = 0;

    FGizmo::FDesc Desc;
    Desc.OnTransformChanged = FOnGizmoTransformChanged::CreateLambda([&](const Matrix4&, const Matrix4& Delta)
    {
        Compounded = Compounded * Delta;
        LastDelta  = Delta;
        Changes++;
    });

    TSharedPtr<FGizmo> Gizmo = MakeGizmo(EGizmoOperation::Translate, EGizmoMode::World, Desc);

    TEST_SECTION("Every frame of a drag reports the whole step from where it began, not the step since the frame before");
    constexpr int32 NumSteps      = 10;
    constexpr int32 PixelsPerStep = 10;

    Gizmo->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(423, 300)));

    for (int32 Step = 1; Step <= NumSteps; ++Step)
    {
        Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(423 + (Step * PixelsPerStep), 300)));
        TEST_EXPECT(IsNearly(LastDelta.GetTranslation(), Vector3(static_cast<float>(Step) * 0.1f, 0.0f, 0.0f)));
    }

    Gizmo->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(423 + (NumSteps * PixelsPerStep), 300)));

    TEST_EXPECT_EQ(Changes, NumSteps);

    TEST_SECTION("So the transform the drag began on, times the last delta, is where the cursor left it");
    TEST_EXPECT(IsNearly((Matrix4::Identity() * LastDelta).GetTranslation(), Gizmo->GetTransform().GetTranslation()));
    TEST_EXPECT(IsNearly(Gizmo->GetTransform().GetTranslation(), Vector3(1.0f, 0.0f, 0.0f)));

    TEST_SECTION("Composing all of them instead overshoots by their sum, which is the runaway a host must not write");
    TEST_EXPECT(IsNearly(Compounded.GetTranslation(), Vector3(5.5f, 0.0f, 0.0f)));

    TEST_SECTION("A second drag measures from where the first one left off rather than from the origin");
    DragGizmo(Gizmo, IntVector2(523, 300), IntVector2(573, 300));

    TEST_EXPECT(IsNearly(LastDelta.GetTranslation(), Vector3(0.5f, 0.0f, 0.0f)));
    TEST_EXPECT(IsNearly(Gizmo->GetTransform().GetTranslation(), Vector3(1.5f, 0.0f, 0.0f)));

    TEST_SECTION("A scale drag holds the pivot still, so a host applying the delta resizes about it and does not slide");
    TSharedPtr<FGizmo> ScaleGizmo = MakeGizmo(EGizmoOperation::Scale, EGizmoMode::World, Desc);
    ScaleGizmo->SetTransform(Matrix4::Translation(Vector3(2.0f, 0.0f, 0.0f)));

    const Vector3 ScalePivot = ScaleGizmo->GetPivot();
    TEST_EXPECT(IsNearly(ScalePivot, Vector3(2.0f, 0.0f, 0.0f)));

    DragGizmo(ScaleGizmo, IntVector2(640, 300), IntVector2(680, 300));

    TEST_EXPECT(IsNearly(GetRow(ScaleGizmo->GetTransform(), 0), Vector3(2.0f, 0.0f, 0.0f)));
    TEST_EXPECT(IsNearly(LastDelta.TransformCoord(ScalePivot), ScalePivot));

    TEST_SECTION("An actor away from that pivot moves out from it by the same factor rather than staying put");
    const Vector3 AwayFromPivot(3.0f, 0.0f, 0.0f);
    TEST_EXPECT(IsNearly(LastDelta.TransformCoord(AwayFromPivot), Vector3(4.0f, 0.0f, 0.0f)));

    TEST_SECTION("A turned gizmo scales along the axis its handle is drawn on, not along the world axis behind it");
    TSharedPtr<FGizmo> TurnedGizmo = MakeGizmo(EGizmoOperation::Scale, EGizmoMode::Local, Desc);
    TurnedGizmo->SetTransform(Matrix4::RotationZ(Math::Constants::PI * 0.5f));

    TEST_EXPECT(IsNearly(TurnedGizmo->GetAxisDirection(0), Vector3(0.0f, 1.0f, 0.0f)));

    DragGizmo(TurnedGizmo, IntVector2(400, 260), IntVector2(400, 240));

    const Matrix4 Turned = TurnedGizmo->GetTransform();
    TEST_EXPECT(IsNearly(GetRow(Turned, 0), Vector3(0.0f, 1.5f, 0.0f)));
    TEST_EXPECT(IsNearly(GetRow(Turned, 1), Vector3(-1.0f, 0.0f, 0.0f)));

    TEST_END();
}

bool GizmoModes_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    int32 DragsStarted  = 0;
    int32 DragsFinished = 0;

    FGizmo::FDesc Desc;
    Desc.OnDragStarted  = FOnGizmoDragStarted::CreateLambda([&DragsStarted](EGizmoHandle) { DragsStarted++; });
    Desc.OnDragFinished = FOnGizmoDragFinished::CreateLambda([&DragsFinished](const Matrix4&, const Matrix4&) { DragsFinished++; });

    TSharedPtr<FGizmo> Gizmo = MakeGizmo(EGizmoOperation::Translate, EGizmoMode::World, Desc);

    const Matrix4 Turned = Matrix4::RotationZ(Math::Constants::HalfPI);

    TEST_SECTION("World mode lines the handles up with the world, whatever the transform is doing");
    Gizmo->SetTransform(Turned);
    TEST_EXPECT(IsNearly(Gizmo->GetAxisDirection(0), Vector3(1.0f, 0.0f, 0.0f)));

    TEST_SECTION("Local mode lines them up with the transform");
    Gizmo->SetMode(EGizmoMode::Local);
    TEST_EXPECT(IsNearly(Gizmo->GetAxisDirection(0), Vector3(0.0f, 1.0f, 0.0f)));

    TEST_SECTION("So a drag on the same handle moves the transform a different way in each mode");
    Gizmo->SetTransform(Matrix4::Identity());
    DragGizmo(Gizmo, IntVector2(423, 300), IntVector2(523, 300));

    const Vector3 MovedLocally = Gizmo->GetTransform().GetTranslation();

    Gizmo->SetMode(EGizmoMode::World);
    Gizmo->SetTransform(Turned);
    DragGizmo(Gizmo, IntVector2(423, 300), IntVector2(523, 300));

    TEST_EXPECT(IsNearly(MovedLocally, Vector3(1.0f, 0.0f, 0.0f)));
    TEST_EXPECT(IsNearly(Gizmo->GetTransform().GetTranslation(), Vector3(1.0f, 0.0f, 0.0f)));

    Gizmo->SetMode(EGizmoMode::Local);
    Gizmo->SetTransform(Turned);

    DragGizmo(Gizmo, IntVector2(400, 277), IntVector2(400, 177));
    TEST_EXPECT(IsNearly(Gizmo->GetTransform().GetTranslation(), Vector3(0.0f, 1.0f, 0.0f)));

    TEST_SECTION("Scale keeps to the transform's own axes even when the mode says otherwise");
    Gizmo->SetMode(EGizmoMode::World);
    Gizmo->SetOperation(EGizmoOperation::Scale);
    Gizmo->SetTransform(Turned);

    TEST_EXPECT(IsNearly(Gizmo->GetAxisDirection(0), Vector3(0.0f, 1.0f, 0.0f)));

    TEST_SECTION("A drag holds the gizmo: the transform, the mode and the operation are all its own until it ends");
    Gizmo->SetOperation(EGizmoOperation::Translate);
    Gizmo->SetMode(EGizmoMode::Local);
    Gizmo->SetTransform(Matrix4::Identity());

    const int32 StartedBefore = DragsStarted;

    Gizmo->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(423, 300)));
    TEST_EXPECT(Gizmo->IsDragging());
    TEST_EXPECT(Gizmo->GetActiveHandle() == EGizmoHandle::TranslateX);
    TEST_EXPECT_EQ(DragsStarted, StartedBefore + 1);

    Gizmo->OnMouseMove(MakeMoveEvent(IntVector2(473, 300)));

    Gizmo->SetTransform(Matrix4::Translation(Vector3(9.0f, 9.0f, 9.0f)));
    Gizmo->SetOperation(EGizmoOperation::Rotate);
    Gizmo->SetMode(EGizmoMode::World);

    TEST_EXPECT(Gizmo->GetOperation() == EGizmoOperation::Translate);
    TEST_EXPECT(Gizmo->GetMode() == EGizmoMode::Local);
    TEST_EXPECT(IsNearly(Gizmo->GetTransform().GetTranslation(), Vector3(0.5f, 0.0f, 0.0f)));

    Gizmo->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(473, 300)));

    TEST_EXPECT(!Gizmo->IsDragging());
    TEST_EXPECT(Gizmo->GetActiveHandle() == EGizmoHandle::None);
    TEST_EXPECT_EQ(DragsFinished, DragsStarted);

    TEST_SECTION("And once it has ended the settings take again");
    Gizmo->SetOperation(EGizmoOperation::Rotate);
    TEST_EXPECT(Gizmo->GetOperation() == EGizmoOperation::Rotate);

    TEST_END();
}
