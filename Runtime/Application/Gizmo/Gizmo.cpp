#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Gizmo/Gizmo.h"
#include "Application/Input/Keys.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/NumericLimits.h"

static TAutoConsoleVariable<float> CVarGizmoSize(
    "Editor.Gizmo.Size",
    "Scales the transform gizmo, where one is the size the editor ships with",
    1.2f,
    EConsoleVariableFlags::Default);

constexpr float GIZMO_SHAFT_START       = 0.16f;
constexpr float GIZMO_ARROW_LENGTH      = 13.0f;
constexpr float GIZMO_ARROW_HALF_WIDTH  = 5.0f;
constexpr float GIZMO_CONE_SHADE        = 0.72f;
constexpr float GIZMO_KNOB_HALF_SIZE    = 6.0f;
constexpr float GIZMO_SHAFT_THICKNESS   = 3.0f;
constexpr float GIZMO_RING_THICKNESS    = 4.0f;
constexpr float GIZMO_OUTLINE_THICKNESS = 1.0f;
constexpr float GIZMO_PLANE_OPACITY     = 0.32f;
constexpr float GIZMO_WEDGE_OPACITY     = 0.28f;
constexpr float GIZMO_MINIMUM_SCALE     = 0.001f;

constexpr int32 GIZMO_READOUT_OFFSET = 16;

static FFloatColor GetAxisColor(int32 AxisIndex)
{
    switch (AxisIndex)
    {
        case 0:  return FFloatColor(204.0f / 255.0f,  39.0f / 255.0f,  39.0f / 255.0f, 1.0f);
        case 1:  return FFloatColor(103.0f / 255.0f, 168.0f / 255.0f,   0.0f / 255.0f, 1.0f);
        default: return FFloatColor( 44.0f / 255.0f, 126.0f / 255.0f, 239.0f / 255.0f, 1.0f);
    }
}

static FFloatColor GetSelectionColor()
{
    return FFloatColor(1.0f, 1.0f, 0.0f, 1.0f);
}

static FFloatColor GetScreenSpaceColor()
{
    return FFloatColor(0.78f, 0.78f, 0.80f, 1.0f);
}

static EGizmoHandle CreateHandle(EGizmoHandle First, int32 Offset)
{
    return static_cast<EGizmoHandle>(static_cast<uint8>(First) + static_cast<uint8>(Offset));
}

static Matrix4 CreateAxisAngleRotation(const Vector3& Axis, float AngleRadians)
{
    const Vector3 RotatedX = Vector3(1.0f, 0.0f, 0.0f).GetRotated(Axis, AngleRadians);
    const Vector3 RotatedY = Vector3(0.0f, 1.0f, 0.0f).GetRotated(Axis, AngleRadians);
    const Vector3 RotatedZ = Vector3(0.0f, 0.0f, 1.0f).GetRotated(Axis, AngleRadians);

    return Matrix4(
        RotatedX.X, RotatedX.Y, RotatedX.Z, 0.0f,
        RotatedY.X, RotatedY.Y, RotatedY.Z, 0.0f,
        RotatedZ.X, RotatedZ.Y, RotatedZ.Z, 0.0f,
        0.0f,       0.0f,       0.0f,       1.0f);
}

static float WrapAngle(float Angle)
{
    while (Angle > Math::Constants::PI)
    {
        Angle -= Math::Constants::TwoPI;
    }

    while (Angle < -Math::Constants::PI)
    {
        Angle += Math::Constants::TwoPI;
    }

    return Angle;
}

static Vector3 GetMatrixRow(const Matrix4& Matrix, int32 RowIndex)
{
    const Vector4 Row = Matrix.GetRow(RowIndex);
    return Vector3(Row.X, Row.Y, Row.Z);
}

TSharedPtr<FGizmo> FGizmo::Create(const FDesc& Desc)
{
    TSharedPtr<FGizmo> NewGizmo = MakeSharedPtr<FGizmo>();
    NewGizmo->Initialize(Desc);
    return NewGizmo;
}

FGizmo::FGizmo()
    : FVisualElement()
    , ViewMatrix(Matrix4::Identity())
    , ProjectionMatrix(Matrix4::Identity())
    , ViewProjectionMatrix(Matrix4::Identity())
    , Transform(Matrix4::Identity())
    , TransformAtDragStart(Matrix4::Identity())
    , Font(nullptr)
    , Operation(EGizmoOperation::Translate)
    , Mode(EGizmoMode::World)
    , Snap()
    , HoveredHandle(EGizmoHandle::None)
    , ActiveHandle(EGizmoHandle::None)
    , Pivot(0.0f, 0.0f, 0.0f)
    , CameraRight(1.0f, 0.0f, 0.0f)
    , CameraUp(0.0f, 1.0f, 0.0f)
    , ViewDirection(0.0f, 0.0f, 1.0f)
    , DragStartHit(0.0f, 0.0f, 0.0f)
    , DragPlanePoint(0.0f, 0.0f, 0.0f)
    , DragPlaneNormal(0.0f, 0.0f, 1.0f)
    , DragAxis(1.0f, 0.0f, 0.0f)
    , DragBasisU(1.0f, 0.0f, 0.0f)
    , DragBasisV(0.0f, 1.0f, 0.0f)
    , DragStartAngle(0.0f)
    , DragLastAngle(0.0f)
    , DragTotalAngle(0.0f)
    , DragStartLength(1.0f)
    , DragStartLengthU(1.0f)
    , DragStartLengthV(1.0f)
    , Readout()
    , ScreenFactor(1.0f)
    , bIsOrthographic(false)
    , bIsProjected(true)
    , OnTransformChangedDelegate()
    , OnDragStartedDelegate()
    , OnDragFinishedDelegate()
{
    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        Axes[AxisIndex]          = Vector3(AxisIndex == 0 ? 1.0f : 0.0f, AxisIndex == 1 ? 1.0f : 0.0f, AxisIndex == 2 ? 1.0f : 0.0f);
        bAxisVisible[AxisIndex]  = true;
        bAxisFlipped[AxisIndex]  = false;
        bPlaneVisible[AxisIndex] = true;
    }
}

FGizmo::~FGizmo() = default;

void FGizmo::Initialize(const FDesc& Desc)
{
    Operation                  = Desc.Operation;
    Mode                       = Desc.Mode;
    Snap                       = Desc.Snap;
    Font                       = Desc.Font;
    OnTransformChangedDelegate = Desc.OnTransformChanged;
    OnDragStartedDelegate      = Desc.OnDragStarted;
    OnDragFinishedDelegate     = Desc.OnDragFinished;

    UpdateContext();
}

IntVector2 FGizmo::ComputeDesiredSize() const
{
    return IntVector2(0, 0);
}

void FGizmo::OnArrange(const FRectangle& AllottedBounds)
{
    FVisualElement::OnArrange(AllottedBounds);
    UpdateContext();
}

int32 FGizmo::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    UNREFERENCED_VARIABLE(AllottedGeometry);

    if (!bIsProjected)
    {
        return LayerId;
    }

    switch (Operation)
    {
        case EGizmoOperation::Translate:
        {
            DrawTranslate(OutCommandList, LayerId);
            break;
        }

        case EGizmoOperation::Rotate:
        {
            DrawRotate(OutCommandList, LayerId);
            break;
        }

        default:
        {
            DrawScale(OutCommandList, LayerId);
            break;
        }
    }

    DrawReadout(OutCommandList, LayerId + 3);
    return LayerId + 4;
}

FEventResponse FGizmo::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (IsDragging())
    {
        UpdateDrag(CursorEvent.GetClientPosition());
        return FEventResponse::Handled();
    }

    HoveredHandle = HitTest(CursorEvent.GetClientPosition());
    return FEventResponse::Unhandled();
}

FEventResponse FGizmo::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft || !bIsProjected)
    {
        return FEventResponse::Unhandled();
    }

    const EGizmoHandle Handle = HitTest(CursorEvent.GetClientPosition());
    if (Handle == EGizmoHandle::None)
    {
        return FEventResponse::Unhandled();
    }

    BeginDrag(Handle, CursorEvent.GetClientPosition());
    return FEventResponse::Handled();
}

FEventResponse FGizmo::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (!IsDragging())
    {
        return FEventResponse::Unhandled();
    }

    EndDrag();
    HoveredHandle = HitTest(CursorEvent.GetClientPosition());

    return FEventResponse::Handled();
}

bool FGizmo::IsInteractive() const
{
    return true;
}

void FGizmo::SetCamera(const Matrix4& InViewMatrix, const Matrix4& InProjectionMatrix, bool bInIsOrthographic)
{
    ViewMatrix       = InViewMatrix;
    ProjectionMatrix = InProjectionMatrix;
    bIsOrthographic  = bInIsOrthographic;

    UpdateContext();
}

void FGizmo::SetTransform(const Matrix4& InTransform)
{
    if (IsDragging())
    {
        return;
    }

    Transform = InTransform;
    UpdateContext();
}

void FGizmo::SetOperation(EGizmoOperation InOperation)
{
    if (IsDragging())
    {
        return;
    }

    Operation     = InOperation;
    HoveredHandle = EGizmoHandle::None;

    UpdateContext();
}

void FGizmo::SetMode(EGizmoMode InMode)
{
    if (IsDragging())
    {
        return;
    }

    Mode          = InMode;
    HoveredHandle = EGizmoHandle::None;

    UpdateContext();
}

void FGizmo::SetSnap(const FGizmoSnapSettings& InSnap)
{
    Snap = InSnap;
}

bool FGizmo::IsAxisVisible(int32 AxisIndex) const
{
    return AxisIndex >= 0 && AxisIndex < 3 ? bAxisVisible[AxisIndex] : false;
}

bool FGizmo::IsAxisFlipped(int32 AxisIndex) const
{
    return AxisIndex >= 0 && AxisIndex < 3 ? bAxisFlipped[AxisIndex] : false;
}

bool FGizmo::IsPlaneVisible(int32 AxisIndex) const
{
    return AxisIndex >= 0 && AxisIndex < 3 ? bPlaneVisible[AxisIndex] : false;
}

Vector3 FGizmo::GetAxisDirection(int32 AxisIndex) const
{
    if (AxisIndex < 0 || AxisIndex >= 3)
    {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    return bAxisFlipped[AxisIndex] ? -Axes[AxisIndex] : Axes[AxisIndex];
}

Vector3 FGizmo::GetPivot() const
{
    return Pivot;
}

void FGizmo::UpdateContext()
{
    ViewProjectionMatrix = ViewMatrix * ProjectionMatrix;

    const Matrix4 InverseView    = ViewMatrix.GetInverse();
    const Vector3 CameraPosition = InverseView.GetTranslation();
    const Vector3 CameraForward  = GetMatrixRow(InverseView, 2).GetNormalized();

    CameraRight = GetMatrixRow(InverseView, 0).GetNormalized();
    CameraUp    = GetMatrixRow(InverseView, 1).GetNormalized();
    Pivot       = Transform.GetTranslation();

    const Vector3 ToPivot = Pivot - CameraPosition;
    ViewDirection = (bIsOrthographic || ToPivot.GetLength() < Math::Constants::CmpThreshold) ? CameraForward : ToPivot.GetNormalized();

    const float SizeScale = Math::Clamp(CVarGizmoSize.GetValue(), SizeScaleMin, SizeScaleMax);
    ScreenFactor = FGizmoMath::ComputeScreenFactor(ViewProjectionMatrix, CameraRight, Pivot, SizeInClipSpace * SizeScale);

    Vector2 PivotClient;
    bIsProjected = FGizmoMath::WorldToClient(ViewProjectionMatrix, GetContentRectangle(), Pivot, PivotClient);

    if (IsDragging())
    {
        return;
    }

    const bool bUseLocalAxes = Mode == EGizmoMode::Local
        || Operation == EGizmoOperation::Scale
        || Operation == EGizmoOperation::UniversalScale;

    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        const Vector3 WorldAxis(AxisIndex == 0 ? 1.0f : 0.0f, AxisIndex == 1 ? 1.0f : 0.0f, AxisIndex == 2 ? 1.0f : 0.0f);
        if (!bUseLocalAxes)
        {
            Axes[AxisIndex] = WorldAxis;
            continue;
        }

        const Vector3 LocalAxis = GetMatrixRow(Transform, AxisIndex);
        Axes[AxisIndex] = LocalAxis.GetLength() > Math::Constants::CmpThreshold ? LocalAxis.GetNormalized() : WorldAxis;
    }

    ComputeAxisVisibility();
}

void FGizmo::ComputeAxisVisibility()
{
    const FRectangle& Bounds = GetContentRectangle();
    const float AspectRatio  = Bounds.Height > 0 ? (static_cast<float>(Bounds.Width) / static_cast<float>(Bounds.Height)) : 1.0f;

    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        const Vector3 Reach = Axes[AxisIndex] * ScreenFactor;

        const float Forward  = FGizmoMath::GetSegmentLengthInClipSpace(ViewProjectionMatrix, AspectRatio, Pivot, Pivot + Reach);
        const float Backward = FGizmoMath::GetSegmentLengthInClipSpace(ViewProjectionMatrix, AspectRatio, Pivot, Pivot - Reach);

        bAxisFlipped[AxisIndex] = Backward > Forward + Math::Constants::CmpThreshold;
        bAxisVisible[AxisIndex] = Math::Max(Forward, Backward) > AxisVisibilityLimit;
    }

    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        const Vector3 SideU = GetAxisDirection((AxisIndex + 1) % 3) * ScreenFactor;
        const Vector3 SideV = GetAxisDirection((AxisIndex + 2) % 3) * ScreenFactor;

        const float Area = FGizmoMath::GetParallelogramArea(ViewProjectionMatrix, AspectRatio, Pivot, Pivot + SideU, Pivot + SideV);
        bPlaneVisible[AxisIndex] = Area > PlaneVisibilityLimit;
    }
}

EGizmoHandle FGizmo::HitTest(const IntVector2& ClientPosition) const
{
    if (!bIsProjected)
    {
        return EGizmoHandle::None;
    }

    const FRectangle& Bounds = GetContentRectangle();
    const Vector2     Point(static_cast<float>(ClientPosition.X), static_cast<float>(ClientPosition.Y));

    Vector2 PivotClient;
    if (!FGizmoMath::WorldToClient(ViewProjectionMatrix, Bounds, Pivot, PivotClient))
    {
        return EGizmoHandle::None;
    }

    if (Operation == EGizmoOperation::Rotate)
    {
        EGizmoHandle NearestHandle = EGizmoHandle::None;
        float        NearestDepth  = TNumericLimits<float>::Max();

        for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
        {
            const Vector3& BasisU = Axes[(AxisIndex + 1) % 3];
            const Vector3& BasisV = Axes[(AxisIndex + 2) % 3];

            TArray<Vector2> Points;
            if (!GetRingPoints(BasisU, BasisV, ScreenFactor, Points))
            {
                continue;
            }

            int32 ClosestStep     = -1;
            float ClosestDistance = FGizmoMath::RingGrabDistance;

            for (int32 Index = 0; Index < Points.Size(); ++Index)
            {
                const float Distance = FGizmoMath::DistanceToSegment(Point, Points[Index], Points[(Index + 1) % Points.Size()]);
                if (Distance <= ClosestDistance)
                {
                    ClosestDistance = Distance;
                    ClosestStep     = Index;
                }
            }

            if (ClosestStep < 0)
            {
                continue;
            }

            const float   Angle  = (Math::Constants::TwoPI * static_cast<float>(ClosestStep)) / static_cast<float>(RingSegments);
            const Vector3 OnRing = Pivot + (BasisU * (Math::Cos(Angle) * ScreenFactor)) + (BasisV * (Math::Sin(Angle) * ScreenFactor));

            const float Depth = ViewMatrix.TransformCoord(OnRing).Z;
            if (Depth < NearestDepth)
            {
                NearestDepth  = Depth;
                NearestHandle = CreateHandle(EGizmoHandle::RotateX, AxisIndex);
            }
        }

        if (NearestHandle != EGizmoHandle::None)
        {
            return NearestHandle;
        }

        TArray<Vector2> OuterPoints;
        if (GetRingPoints(CameraRight, CameraUp, ScreenFactor * OuterRingScale, OuterPoints))
        {
            for (int32 Index = 0; Index < OuterPoints.Size(); ++Index)
            {
                if (FGizmoMath::DistanceToSegment(Point, OuterPoints[Index], OuterPoints[(Index + 1) % OuterPoints.Size()]) <= FGizmoMath::RingGrabDistance)
                {
                    return EGizmoHandle::RotateScreen;
                }
            }
        }

        return EGizmoHandle::None;
    }

    const bool bIsTranslate  = Operation == EGizmoOperation::Translate;
    const bool bIsOverCenter = bIsTranslate
        ? (Point - PivotClient).GetLengthSquared() <= (CenterHandleSize * CenterHandleSize)
        : (Math::Abs(Point.X - PivotClient.X) <= CenterHandleSize && Math::Abs(Point.Y - PivotClient.Y) <= CenterHandleSize);

    if (bIsOverCenter)
    {
        return bIsTranslate ? EGizmoHandle::TranslateScreen : EGizmoHandle::ScaleUniform;
    }

    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        Vector2 Quad[4];
        if (bPlaneVisible[AxisIndex] && GetPlaneQuad(AxisIndex, Quad) && FGizmoMath::IsPointOverQuad(Point, Quad, FGizmoMath::PlaneGrabPadding))
        {
            return CreateHandle(bIsTranslate ? EGizmoHandle::TranslateYZ : EGizmoHandle::ScaleYZ, AxisIndex);
        }
    }

    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        Vector2 Start;
        Vector2 End;

        if (!bAxisVisible[AxisIndex] || !GetAxisSegment(AxisIndex, Start, End))
        {
            continue;
        }

        if (FGizmoMath::DistanceToSegment(Point, Start, End) <= FGizmoMath::AxisGrabDistance)
        {
            return CreateHandle(bIsTranslate ? EGizmoHandle::TranslateX : EGizmoHandle::ScaleX, AxisIndex);
        }
    }

    return EGizmoHandle::None;
}

bool FGizmo::GetAxisSegment(int32 AxisIndex, Vector2& OutStart, Vector2& OutEnd) const
{
    const FRectangle& Bounds    = GetContentRectangle();
    const Vector3     Direction = GetAxisDirection(AxisIndex);

    return FGizmoMath::WorldToClient(ViewProjectionMatrix, Bounds, Pivot + (Direction * (ScreenFactor * GIZMO_SHAFT_START)), OutStart)
        && FGizmoMath::WorldToClient(ViewProjectionMatrix, Bounds, Pivot + (Direction * ScreenFactor), OutEnd);
}

bool FGizmo::GetPlaneQuad(int32 AxisIndex, Vector2 OutQuad[4]) const
{
    const FRectangle& Bounds = GetContentRectangle();

    const Vector3 SideU = GetAxisDirection((AxisIndex + 1) % 3) * ScreenFactor;
    const Vector3 SideV = GetAxisDirection((AxisIndex + 2) % 3) * ScreenFactor;

    const float Corners[4][2] =
    {
        { PlaneHandleStart, PlaneHandleStart },
        { PlaneHandleEnd,   PlaneHandleStart },
        { PlaneHandleEnd,   PlaneHandleEnd   },
        { PlaneHandleStart, PlaneHandleEnd   },
    };

    for (int32 CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
    {
        const Vector3 Corner = Pivot + (SideU * Corners[CornerIndex][0]) + (SideV * Corners[CornerIndex][1]);
        if (!FGizmoMath::WorldToClient(ViewProjectionMatrix, Bounds, Corner, OutQuad[CornerIndex]))
        {
            return false;
        }
    }

    return true;
}

bool FGizmo::GetRingPoints(const Vector3& BasisU, const Vector3& BasisV, float Radius, TArray<Vector2>& OutPoints) const
{
    const FRectangle& Bounds = GetContentRectangle();

    OutPoints.Clear();
    OutPoints.Reserve(RingSegments);

    for (int32 Step = 0; Step < RingSegments; ++Step)
    {
        const float Angle = (Math::Constants::TwoPI * static_cast<float>(Step)) / static_cast<float>(RingSegments);
        const Vector3 Corner = Pivot + (BasisU * (Math::Cos(Angle) * Radius)) + (BasisV * (Math::Sin(Angle) * Radius));

        Vector2 Projected;
        if (!FGizmoMath::WorldToClient(ViewProjectionMatrix, Bounds, Corner, Projected))
        {
            OutPoints.Clear();
            return false;
        }

        OutPoints.Add(Projected);
    }

    return true;
}

float FGizmo::GetAngleOnDragPlane(const Vector3& WorldPosition) const
{
    const Vector3 Local = WorldPosition - DragPlanePoint;
    return Math::Atan2(Local.DotProduct(DragBasisV), Local.DotProduct(DragBasisU));
}

void FGizmo::GetDragPlane(EGizmoHandle Handle, Vector3& OutPoint, Vector3& OutNormal) const
{
    OutPoint = Pivot;

    const int32 AxisIndex = GetHandleAxis(Handle);

    switch (Handle)
    {
        case EGizmoHandle::TranslateYZ:
        case EGizmoHandle::TranslateZX:
        case EGizmoHandle::TranslateXY:
        case EGizmoHandle::RotateX:
        case EGizmoHandle::RotateY:
        case EGizmoHandle::RotateZ:
        case EGizmoHandle::ScaleYZ:
        case EGizmoHandle::ScaleZX:
        case EGizmoHandle::ScaleXY:
        {
            OutNormal = Axes[AxisIndex];
            break;
        }

        case EGizmoHandle::TranslateScreen:
        case EGizmoHandle::RotateScreen:
        case EGizmoHandle::ScaleUniform:
        {
            OutNormal = ViewDirection;
            break;
        }

        default:
        {
            OutNormal = FGizmoMath::ComputeAxisPlaneNormal(Axes[AxisIndex], ViewDirection);
            break;
        }
    }
}

bool FGizmo::ProjectOntoDragPlane(const IntVector2& ClientPosition, Vector3& OutHit) const
{
    Vector3 RayOrigin;
    Vector3 RayDirection;

    FGizmoMath::ComputeCameraRay(ViewProjectionMatrix, GetContentRectangle(), ClientPosition, RayOrigin, RayDirection);
    return FGizmoMath::IntersectRayPlane(DragPlanePoint, DragPlaneNormal, RayOrigin, RayDirection, OutHit);
}

void FGizmo::BeginDrag(EGizmoHandle Handle, const IntVector2& ClientPosition)
{
    ActiveHandle         = Handle;
    HoveredHandle        = Handle;
    TransformAtDragStart = Transform;
    DragTotalAngle       = 0.0f;
    Readout.Clear();

    GetDragPlane(Handle, DragPlanePoint, DragPlaneNormal);

    const int32 AxisIndex = GetHandleAxis(Handle);
    DragAxis   = AxisIndex >= 0 ? Axes[AxisIndex] : ViewDirection;
    DragBasisU = AxisIndex >= 0 ? Axes[(AxisIndex + 1) % 3] : CameraRight;
    DragBasisV = AxisIndex >= 0 ? Axes[(AxisIndex + 2) % 3] : CameraUp;

    if (!ProjectOntoDragPlane(ClientPosition, DragStartHit))
    {
        DragStartHit = DragPlanePoint;
    }

    DragStartAngle = GetAngleOnDragPlane(DragStartHit);
    DragLastAngle  = DragStartAngle;

    if (Handle == EGizmoHandle::ScaleUniform)
    {
        Vector2 PivotClient;
        FGizmoMath::WorldToClient(ViewProjectionMatrix, GetContentRectangle(), Pivot, PivotClient);

        const Vector2 Cursor(static_cast<float>(ClientPosition.X), static_cast<float>(ClientPosition.Y));
        DragStartLength = Math::Max((Cursor - PivotClient).GetLength(), 1.0f);
    }
    else
    {
        const Vector3 Grabbed = DragStartHit - DragPlanePoint;

        const float GrabbedAt = Grabbed.DotProduct(DragAxis);
        DragStartLength = Math::Abs(GrabbedAt) > Math::Constants::CmpThreshold ? GrabbedAt : ScreenFactor;

        const float GrabbedAlongU = Grabbed.DotProduct(DragBasisU);
        const float GrabbedAlongV = Grabbed.DotProduct(DragBasisV);

        DragStartLengthU = Math::Abs(GrabbedAlongU) > Math::Constants::CmpThreshold ? GrabbedAlongU : ScreenFactor;
        DragStartLengthV = Math::Abs(GrabbedAlongV) > Math::Constants::CmpThreshold ? GrabbedAlongV : ScreenFactor;
    }

    if (FApplication::IsInitialized())
    {
        FApplication::Get().CaptureMouse(AsSharedPtr());
    }

    OnDragStartedDelegate.ExecuteIfBound(Handle);
}

void FGizmo::UpdateDrag(const IntVector2& ClientPosition)
{
    Vector3 Hit;
    if (!ProjectOntoDragPlane(ClientPosition, Hit))
    {
        return;
    }

    const int32 AxisIndex = GetHandleAxis(ActiveHandle);
    const CHAR* AxisNames[3] = { "X", "Y", "Z" };

    if (IsTranslateHandle(ActiveHandle))
    {
        const Vector3 RawDelta = Hit - DragStartHit;

        Vector3 Delta = RawDelta;
        switch (ActiveHandle)
        {
            case EGizmoHandle::TranslateX:
            case EGizmoHandle::TranslateY:
            case EGizmoHandle::TranslateZ:
            {
                const float Distance = FGizmoMath::SnapValue(RawDelta.DotProduct(DragAxis), Snap.TranslationSnap);

                Delta   = DragAxis * Distance;
                Readout = String::Printf("%s  %.3f", AxisNames[AxisIndex], Distance);
                break;
            }

            case EGizmoHandle::TranslateYZ:
            case EGizmoHandle::TranslateZX:
            case EGizmoHandle::TranslateXY:
            {
                const float AlongU = FGizmoMath::SnapValue(RawDelta.DotProduct(DragBasisU), Snap.TranslationSnap);
                const float AlongV = FGizmoMath::SnapValue(RawDelta.DotProduct(DragBasisV), Snap.TranslationSnap);

                Delta   = (DragBasisU * AlongU) + (DragBasisV * AlongV);
                Readout = String::Printf("%s %.3f  %s %.3f", AxisNames[(AxisIndex + 1) % 3], AlongU, AxisNames[(AxisIndex + 2) % 3], AlongV);
                break;
            }

            default:
            {
                Delta   = FGizmoMath::SnapVector(RawDelta, Snap.TranslationSnap);
                Readout = String::Printf("%.3f  %.3f  %.3f", Delta.X, Delta.Y, Delta.Z);
                break;
            }
        }

        Matrix4 NewTransform = TransformAtDragStart;
        NewTransform.SetTranslation(TransformAtDragStart.GetTranslation() + Delta);

        ApplyTransform(NewTransform);
        return;
    }

    if (IsRotateHandle(ActiveHandle))
    {
        const float Angle = GetAngleOnDragPlane(Hit);
        DragTotalAngle += WrapAngle(Angle - DragLastAngle);
        DragLastAngle   = Angle;

        float Applied = DragTotalAngle;
        if (Snap.RotationSnapDegrees > 0.0f)
        {
            Applied = Math::DegreesToRadians(FGizmoMath::SnapValue(Math::RadiansToDegrees(DragTotalAngle), Snap.RotationSnapDegrees));
        }

        const Vector3 RotationAxis = IsRotateHandle(ActiveHandle) && AxisIndex >= 0 ? Axes[AxisIndex] : ViewDirection;
        const Matrix4 Rotation     = CreateAxisAngleRotation(RotationAxis, Applied);

        const Matrix4 AboutPivot = Matrix4::Translation(-DragPlanePoint) * Rotation * Matrix4::Translation(DragPlanePoint);

        Readout = ActiveHandle == EGizmoHandle::RotateScreen
            ? String::Printf("%.2f deg", Math::RadiansToDegrees(Applied))
            : String::Printf("%s  %.2f deg", AxisNames[AxisIndex], Math::RadiansToDegrees(Applied));

        ApplyTransform(TransformAtDragStart * AboutPivot);
        return;
    }

    const bool bIsPlane = ActiveHandle == EGizmoHandle::ScaleYZ
        || ActiveHandle == EGizmoHandle::ScaleZX
        || ActiveHandle == EGizmoHandle::ScaleXY;

    float Factor = 1.0f;
    if (ActiveHandle == EGizmoHandle::ScaleUniform)
    {
        Vector2 PivotClient;
        FGizmoMath::WorldToClient(ViewProjectionMatrix, GetContentRectangle(), Pivot, PivotClient);

        const Vector2 Cursor(static_cast<float>(ClientPosition.X), static_cast<float>(ClientPosition.Y));
        Factor = (Cursor - PivotClient).GetLength() / DragStartLength;
    }
    else
    {
        Factor = (Hit - DragPlanePoint).DotProduct(DragAxis) / DragStartLength;
    }

    Factor = Math::Max(FGizmoMath::SnapValue(Factor, Snap.ScaleSnap), GIZMO_MINIMUM_SCALE);

    const bool bIsUniform = ActiveHandle == EGizmoHandle::ScaleUniform || Operation == EGizmoOperation::UniversalScale;

    Vector3 ScaleVector(1.0f, 1.0f, 1.0f);
    if (bIsUniform)
    {
        ScaleVector = Vector3(Factor, Factor, Factor);
        Readout     = String::Printf("%.3f", Factor);
    }
    else if (bIsPlane)
    {
        const int32 IndexU = (AxisIndex + 1) % 3;
        const int32 IndexV = (AxisIndex + 2) % 3;

        const Vector3 Along  = Hit - DragPlanePoint;
        const float   AlongU = Math::Max(FGizmoMath::SnapValue(Along.DotProduct(DragBasisU) / DragStartLengthU, Snap.ScaleSnap), GIZMO_MINIMUM_SCALE);
        const float   AlongV = Math::Max(FGizmoMath::SnapValue(Along.DotProduct(DragBasisV) / DragStartLengthV, Snap.ScaleSnap), GIZMO_MINIMUM_SCALE);

        ScaleVector[IndexU] = AlongU;
        ScaleVector[IndexV] = AlongV;
        Readout             = String::Printf("%s %.3f  %s %.3f", AxisNames[IndexU], AlongU, AxisNames[IndexV], AlongV);
    }
    else
    {
        ScaleVector[AxisIndex] = Factor;
        Readout                = String::Printf("%s  %.3f", AxisNames[AxisIndex], Factor);
    }

    ApplyTransform(Matrix4::Scale(ScaleVector) * TransformAtDragStart);
}

void FGizmo::EndDrag()
{
    if (!IsDragging())
    {
        return;
    }

    ActiveHandle = EGizmoHandle::None;
    Readout.Clear();

    if (FApplication::IsInitialized())
    {
        FApplication::Get().ReleaseMouseCapture(AsSharedPtr());
    }

    OnDragFinishedDelegate.ExecuteIfBound(TransformAtDragStart, Transform);
    UpdateContext();
}

void FGizmo::ApplyTransform(const Matrix4& NewTransform)
{
    if (NewTransform.IsEqual(Transform))
    {
        return;
    }

    Transform = NewTransform;
    UpdateContext();

    OnTransformChangedDelegate.ExecuteIfBound(Transform, TransformAtDragStart.GetInverse() * Transform);
}

void FGizmo::DrawTranslate(FDrawCommandList& OutCommandList, int32 LayerId) const
{
    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        Vector2 Quad[4];
        if (!bPlaneVisible[AxisIndex] || !GetPlaneQuad(AxisIndex, Quad))
        {
            continue;
        }

        const FFloatColor Color = GetHandleColor(CreateHandle(EGizmoHandle::TranslateYZ, AxisIndex), AxisIndex);

        FFloatColor FillColor = Color;
        FillColor.A *= GIZMO_PLANE_OPACITY;

        OutCommandList.AddConvexPolygon(LayerId, MakeArrayView<const Vector2>(Quad, 4), FillColor);
        OutCommandList.AddPolyline(LayerId + 1, MakeArrayView<const Vector2>(Quad, 4), Color, GIZMO_OUTLINE_THICKNESS, true);
    }

    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        Vector2 Start;
        Vector2 End;

        if (!bAxisVisible[AxisIndex] || !GetAxisSegment(AxisIndex, Start, End))
        {
            continue;
        }

        const FFloatColor Color = GetHandleColor(CreateHandle(EGizmoHandle::TranslateX, AxisIndex), AxisIndex);
        OutCommandList.AddLine(LayerId + 1, Start, End, Color, GIZMO_SHAFT_THICKNESS);

        const Vector2 Along = (End - Start).GetNormalized();
        const Vector2 Side(-Along.Y, Along.X);
        const Vector2 Base = End - (Along * GIZMO_ARROW_LENGTH);

        FFloatColor ShadeColor = Color;
        ShadeColor.R *= GIZMO_CONE_SHADE;
        ShadeColor.G *= GIZMO_CONE_SHADE;
        ShadeColor.B *= GIZMO_CONE_SHADE;

        OutCommandList.AddTriangle(LayerId + 2, End, Base + (Side * GIZMO_ARROW_HALF_WIDTH), Base, Color);
        OutCommandList.AddTriangle(LayerId + 2, End, Base, Base - (Side * GIZMO_ARROW_HALF_WIDTH), ShadeColor);
    }

    DrawCenterHandle(OutCommandList, LayerId + 2, EGizmoHandle::TranslateScreen, true);
}

void FGizmo::DrawRotate(FDrawCommandList& OutCommandList, int32 LayerId) const
{
    TArray<Vector2> Points;

    if (GetRingPoints(CameraRight, CameraUp, ScreenFactor * OuterRingScale, Points))
    {
        OutCommandList.AddPolyline(LayerId, Points, GetHandleColor(EGizmoHandle::RotateScreen, -1), GIZMO_RING_THICKNESS, true);
    }

    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        if (!GetRingPoints(Axes[(AxisIndex + 1) % 3], Axes[(AxisIndex + 2) % 3], ScreenFactor, Points))
        {
            continue;
        }

        const FFloatColor Color = GetHandleColor(CreateHandle(EGizmoHandle::RotateX, AxisIndex), AxisIndex);
        OutCommandList.AddPolyline(LayerId + 1, Points, Color, GIZMO_RING_THICKNESS, true);
    }

    if (!IsRotateHandle(ActiveHandle))
    {
        return;
    }

    Vector2 PivotClient;
    if (!FGizmoMath::WorldToClient(ViewProjectionMatrix, GetContentRectangle(), Pivot, PivotClient))
    {
        return;
    }

    const float Radius = ActiveHandle == EGizmoHandle::RotateScreen ? ScreenFactor * OuterRingScale : ScreenFactor;

    FFloatColor WedgeColor = GetSelectionColor();
    WedgeColor.A *= GIZMO_WEDGE_OPACITY;

    const int32 Steps = Math::Max(static_cast<int32>(Math::Abs(DragTotalAngle) / (Math::Constants::TwoPI / static_cast<float>(RingSegments))), 1);

    Vector2 Previous;
    for (int32 Step = 0; Step <= Steps; ++Step)
    {
        const float Angle  = DragStartAngle + (DragTotalAngle * (static_cast<float>(Step) / static_cast<float>(Steps)));
        const Vector3 Edge = DragPlanePoint + (DragBasisU * (Math::Cos(Angle) * Radius)) + (DragBasisV * (Math::Sin(Angle) * Radius));

        Vector2 Projected;
        if (!FGizmoMath::WorldToClient(ViewProjectionMatrix, GetContentRectangle(), Edge, Projected))
        {
            break;
        }

        if (Step > 0)
        {
            OutCommandList.AddTriangle(LayerId + 2, PivotClient, Previous, Projected, WedgeColor);
        }

        Previous = Projected;
    }
}

void FGizmo::DrawScale(FDrawCommandList& OutCommandList, int32 LayerId) const
{
    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        Vector2 Quad[4];
        if (!bPlaneVisible[AxisIndex] || !GetPlaneQuad(AxisIndex, Quad))
        {
            continue;
        }

        const FFloatColor Color = GetHandleColor(CreateHandle(EGizmoHandle::ScaleYZ, AxisIndex), AxisIndex);

        FFloatColor FillColor = Color;
        FillColor.A *= GIZMO_PLANE_OPACITY;

        OutCommandList.AddConvexPolygon(LayerId, MakeArrayView<const Vector2>(Quad, 4), FillColor);
        OutCommandList.AddPolyline(LayerId + 1, MakeArrayView<const Vector2>(Quad, 4), Color, GIZMO_OUTLINE_THICKNESS, true);
    }

    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        Vector2 Start;
        Vector2 End;

        if (!bAxisVisible[AxisIndex] || !GetAxisSegment(AxisIndex, Start, End))
        {
            continue;
        }

        const FFloatColor Color = GetHandleColor(CreateHandle(EGizmoHandle::ScaleX, AxisIndex), AxisIndex);
        OutCommandList.AddLine(LayerId + 1, Start, End, Color, GIZMO_SHAFT_THICKNESS);

        const FRectangle Knob(
            IntVector2(static_cast<int32>(End.X - GIZMO_KNOB_HALF_SIZE), static_cast<int32>(End.Y - GIZMO_KNOB_HALF_SIZE)),
            static_cast<int32>(GIZMO_KNOB_HALF_SIZE * 2.0f),
            static_cast<int32>(GIZMO_KNOB_HALF_SIZE * 2.0f));

        OutCommandList.AddBox(LayerId + 2, Knob, Color);
    }

    DrawCenterHandle(OutCommandList, LayerId + 2, EGizmoHandle::ScaleUniform, false);

    if (Operation != EGizmoOperation::UniversalScale)
    {
        return;
    }

    Vector2 PivotClient;
    if (FGizmoMath::WorldToClient(ViewProjectionMatrix, GetContentRectangle(), Pivot, PivotClient))
    {
        OutCommandList.AddCircle(LayerId + 2, PivotClient, CenterHandleSize * 2.0f, GetHandleColor(EGizmoHandle::ScaleUniform, -1), GIZMO_OUTLINE_THICKNESS);
    }
}

void FGizmo::DrawCenterHandle(FDrawCommandList& OutCommandList, int32 LayerId, EGizmoHandle Handle, bool bDrawAsSphere) const
{
    Vector2 PivotClient;
    if (!FGizmoMath::WorldToClient(ViewProjectionMatrix, GetContentRectangle(), Pivot, PivotClient))
    {
        return;
    }

    const FFloatColor Color = GetHandleColor(Handle, -1);

    FFloatColor FillColor = Color;
    FillColor.A *= GIZMO_PLANE_OPACITY;

    if (bDrawAsSphere)
    {
        OutCommandList.AddCircleFilled(LayerId, PivotClient, CenterHandleSize, FillColor);
        OutCommandList.AddCircle(LayerId, PivotClient, CenterHandleSize, Color, GIZMO_OUTLINE_THICKNESS);
        return;
    }

    const FRectangle Bounds(
        IntVector2(static_cast<int32>(PivotClient.X - CenterHandleSize), static_cast<int32>(PivotClient.Y - CenterHandleSize)),
        static_cast<int32>(CenterHandleSize * 2.0f),
        static_cast<int32>(CenterHandleSize * 2.0f));

    OutCommandList.AddBox(LayerId, Bounds, FillColor);
    OutCommandList.AddBoxOutline(LayerId, Bounds, Color, GIZMO_OUTLINE_THICKNESS);
}

void FGizmo::DrawReadout(FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (!Font || !IsDragging() || Readout.IsEmpty())
    {
        return;
    }

    Vector2 PivotClient;
    if (!FGizmoMath::WorldToClient(ViewProjectionMatrix, GetContentRectangle(), Pivot, PivotClient))
    {
        return;
    }

    const FUIStyle& Style = FUIStyle::GetDefault();

    const IntVector2 Position(static_cast<int32>(PivotClient.X) + GIZMO_READOUT_OFFSET, static_cast<int32>(PivotClient.Y) + GIZMO_READOUT_OFFSET);
    const FRectangle Bounds(Position, Font->MeasureWidth(StringView(Readout.Data(), Readout.Length())), Font->GetLineHeight());

    OutCommandList.AddText(LayerId, FRectangle(Position + IntVector2(1, 1), Bounds.Width, Bounds.Height), Readout, Font.Get(), Style.Colors.WindowBackground);
    OutCommandList.AddText(LayerId + 1, Bounds, Readout, Font.Get(), Style.Colors.Text);
}

FFloatColor FGizmo::GetHandleColor(EGizmoHandle Handle, int32 AxisIndex) const
{
    if (ActiveHandle == Handle || (ActiveHandle == EGizmoHandle::None && HoveredHandle == Handle))
    {
        return GetSelectionColor();
    }

    FFloatColor Color = AxisIndex >= 0 ? GetAxisColor(AxisIndex) : GetScreenSpaceColor();
    if (IsDragging())
    {
        Color.A *= 0.35f;
    }

    return Color;
}
