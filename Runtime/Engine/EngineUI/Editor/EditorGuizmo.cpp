#include "Core/CoreTypes.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Matrix3.h"
#include "Core/Math/Plane.h"
#include "Core/Math/Quaternion.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Containers/Array.h"
#include "Engine/EngineUI/Editor/EditorGuizmo.h"
#include "ImGuiPlugin/ImGuiCore.h"

struct EMoveType
{
    enum Type
    {
        None,
        MoveX,
        MoveY,
        MoveZ,
        MoveYZ,
        MoveZX,
        MoveXY,
        MoveScreen,
        RotateX,
        RotateY,
        RotateZ,
        RotateScreen,
        ScaleX,
        ScaleY,
        ScaleZ,
        ScaleXYZ
    };
};

struct Context
{
    Context()
        : bUsing(false)
        , bUsingViewManipulate(false)
        , bEnable(true)
        , bIsViewManipulatorHovered(false)
        , bUsingBounds(false)
    {
    }

    ImGuiID GetCurrentID()
    {
        if (IDStack.empty())
        {
            IDStack.push_back(static_cast<ImGuiID>(-1));
        }

        return IDStack.back();
    }

    // 16-byte alignment (largest first to minimize padding)
    EditorGuizmo::Style           Style;
    Matrix4                       ViewMat;
    Matrix4                       ProjectionMat;
    Matrix4                       Model;
    Matrix4                       ModelLocal; // Orthonormalized model
    Matrix4                       ModelInverse;
    Matrix4                       ModelSource;
    Matrix4                       ModelSourceInverse;
    Matrix4                       MVP;
    Matrix4                       MVPLocal; // MVP with full model; MVP's model may be Translation-only for World space
    Matrix4                       ViewProjection;
    Matrix4                       BoundsMatrix;
    Plane                         TranslationPlan;
    Plane                         BoundsPlan;
    Vector4                       ModelScaleOrigin;
    Vector4                       CameraEye;
    Vector4                       CameraRight;
    Vector4                       CameraDir;
    Vector4                       CameraUp;
    Vector4                       RayOrigin;
    Vector4                       RayVector;
    Vector4                       RelativeOrigin;
    Vector4                       TranslationPlanOrigin;
    Vector4                       MatrixOrigin;
    Vector4                       TranslationLastDelta;
    Vector4                       RotationVectorSource;
    Vector4                       Scale;
    Vector4                       ScaleValueOrigin;
    Vector4                       ScaleLast;
    Vector4                       BoundsPivot;
    Vector4                       BoundsAnchor;
    Vector4                       BoundsLocalPivot;

    // 8-byte alignment
    ImVector<ImGuiID>              IDStack;
    ImDrawList*                    DrawList          = nullptr;
    ImGuiWindow*                   AlternativeWindow = nullptr;
    ImVec2                         ScreenSquareCenter;
    ImVec2                         ScreenSquareMin;
    ImVec2                         ScreenSquareMax;

    // 4-byte alignment
    float                          RadiusSquareCenter;
    float                          ScreenFactor;
    float                          RotationAngle;
    float                          RotationAngleOrigin;
    float                          SaveMousePosX;
    float                          AxisLimit          = 0.0025f;
    float                          PlaneLimit         = 0.02f;
    float                          X                  = 0.0f;
    float                          Y                  = 0.0f;
    float                          Width              = 0.0f;
    float                          Height             = 0.0f;
    float                          XMax               = 0.0f;
    float                          YMax               = 0.0f;
    float                          DisplayRatio       = 1.0f;
    float                          GizmoSizeClipSpace = 0.1f;
    ImGuiID                        EditingID          = static_cast<ImGuiID>(-1);
    EditorGuizmo::EOperation::Type Operation          = static_cast<EditorGuizmo::EOperation::Type>(-1);
    EditorGuizmo::EMode            Mode;
    int32                          AxisMask = 0;
    int32                          BoundsBestAxis;
    int32                          CurrentOperation;
    float                          AxisFactor[3]; // Save axis factor when using gizmo
    int32                          BoundsAxis[2];

    // 1-byte alignment (bools last to pack into trailing space)
    bool                           bUsing;
    bool                           bUsingViewManipulate;
    bool                           bEnable;
    bool                           bMouseOver;
    bool                           bReversed; // Reversed Projection Matrix
    bool                           bIsViewManipulatorHovered;
    bool                           bUsingBounds;
    bool                           bIsOrthographic    = false;
    bool                           bOverGizmoHotspot  = false;
    bool                           bAllowAxisFlip     = true;
    bool                           bBelowAxisLimit[3];
    bool                           bBelowPlaneLimit[3];
};

// Static variables and constants
const float ScreenRotateSize      = 0.06f;
const float RotationDisplayFactor = 1.2f; // Scale a bit so translate axis do not touch when in universal

// Matches EMoveType::MoveYZ/ZX/XY order
static const EditorGuizmo::EOperation::Type TRANSLATE_PLANS[3] =
{
    static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::TranslateY | EditorGuizmo::EOperation::TranslateZ),
    static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::TranslateX | EditorGuizmo::EOperation::TranslateZ),
    static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::TranslateX | EditorGuizmo::EOperation::TranslateY)
};

static Context GuizmoContext;

static const Vector4 DirectionUnary[3] =
{
    Vector4(1.0f, 0.0f, 0.0f, 0.0f),
    Vector4(0.0f, 1.0f, 0.0f, 0.0f),
    Vector4(0.0f, 0.0f, 1.0f, 0.0f)
};

static const char* TranslationInfoMask[] =
{
    "X : %5.3f", "Y : %5.3f", "Z : %5.3f",
    "Y : %5.3f Z : %5.3f", "X : %5.3f Z : %5.3f", "X : %5.3f Y : %5.3f",
    "X : %5.3f Y : %5.3f Z : %5.3f"
};

static const char* ScaleInfoMask[] = 
{ 
    "X : %5.2f", "Y : %5.2f", "Z : %5.2f", "XYZ : %5.2f"
};

static const char* RotationInfoMask[] =
{
    "X : %5.2f deg %5.2f rad", "Y : %5.2f deg %5.2f rad", "Z : %5.2f deg %5.2f rad",
    "Screen : %5.2f deg %5.2f rad"
};

static const int32 TranslationInfoIndex[] = { 0,0,0, 1,0,0, 2,0,0, 1,2,0, 0,2,0, 0,1,0, 0,1,2 };

static const float QuadMin   = 0.5f;
static const float QuadMax   = 0.8f;
static const float QuadUV[8] =
{
    QuadMin,
    QuadMin,
    QuadMin,
    QuadMax,
    QuadMax,
    QuadMax,
    QuadMax,
    QuadMin
};

static const int32 HalfCircleSegmentCount = 64;
static const float SnapTension            = 0.5f;

// Static functions
static int32 GetMoveType(EditorGuizmo::EOperation::Type Op, Vector4* GizmoHitProportion);
static int32 GetRotateType(EditorGuizmo::EOperation::Type Op);
static int32 GetScaleType(EditorGuizmo::EOperation::Type Op);

static bool Intersects(EditorGuizmo::EOperation::Type LHS, EditorGuizmo::EOperation::Type RHS)
{
    return (LHS & RHS) != static_cast<EditorGuizmo::EOperation::Type>(0);
}

// True if LHS contains RHS
static bool Contains(EditorGuizmo::EOperation::Type LHS, EditorGuizmo::EOperation::Type RHS)
{
    return (LHS & RHS) == RHS;
}

static bool IsTranslateType(int32 Type)
{
    return Type >= EMoveType::MoveX && Type <= EMoveType::MoveScreen;
}

static bool IsRotateType(int32 Type)
{
    return Type >= EMoveType::RotateX && Type <= EMoveType::RotateScreen;
}

static bool IsScaleType(int32 Type)
{
    return Type >= EMoveType::ScaleX && Type <= EMoveType::ScaleXYZ;
}

static Matrix4 LoadMatrix(const float* Matrix)
{
    return Matrix4(Matrix);
}

static void StoreMatrix(float* OutMatrix, const Matrix4& Matrix)
{
    CHECK(OutMatrix != nullptr);
    Memory::Memcpy(OutMatrix, &Matrix.M[0][0], sizeof(Matrix.M));
}

static ImU32 GetColorU32(int32 IDX)
{
    IM_ASSERT(IDX < EditorGuizmo::EColor::Count);
    return ImGui::ColorConvertFloat4ToU32(GuizmoContext.Style.Colors[IDX]);
}

static ImVec2 WorldToPos(const Vector4& WorldPos, const Matrix4& Matrix,
    ImVec2 Position = ImVec2(GuizmoContext.X, GuizmoContext.Y),
    ImVec2 Size = ImVec2(GuizmoContext.Width, GuizmoContext.Height))
{
    Vector4 Trans = Matrix.Transform(Vector4(WorldPos.X, WorldPos.Y, WorldPos.Z, 1.0f));

    const float SafeW = (Math::Abs(Trans.W) > Math::Constants::Epsilon)
        ? Trans.W
        : ((Trans.W < 0.0f) ? -Math::Constants::Epsilon : Math::Constants::Epsilon);

    Trans *= 0.5f / SafeW;
    Trans += Vector4(0.5f, 0.5f, 0.0f, 0.0f);

    Trans.Y = 1.0f - Trans.Y;
    Trans.X *= Size.x;
    Trans.Y *= Size.y;
    Trans.X += Position.x;
    Trans.Y += Position.y;

    return ImVec2(Trans.X, Trans.Y);
}

static void ComputeCameraRay(Vector4& RayOrigin, Vector4& RayDir,
    ImVec2 Position = ImVec2(GuizmoContext.X, GuizmoContext.Y),
    ImVec2 Size = ImVec2(GuizmoContext.Width, GuizmoContext.Height))
{
    ImGuiIO& State = ImGui::GetIO();

    Matrix4 MViewProjInverse = (GuizmoContext.ViewMat * GuizmoContext.ProjectionMat).GetInverse();

    const float MouseX = ((State.MousePos.x - Position.x) / Size.x) * 2.0f - 1.0f;
    const float MouseY = (1.0f - ((State.MousePos.y - Position.y) / Size.y)) * 2.0f - 1.0f;
    const float ZNear  = GuizmoContext.bReversed ? (1.0f - Math::Constants::Epsilon) : 0.0f;
    const float ZFar   = GuizmoContext.bReversed ? 0.0f : (1.0f - Math::Constants::Epsilon);

    RayOrigin  = MViewProjInverse.Transform(Vector4(MouseX, MouseY, ZNear, 1.0f));
    RayOrigin *= 1.0f / RayOrigin.W;

    Vector4 RayEnd = MViewProjInverse.Transform(Vector4(MouseX, MouseY, ZFar, 1.0f));
    RayEnd *= 1.0f / RayEnd.W;
    RayDir  = (RayEnd - RayOrigin).GetNormalized();
}

static float GetSegmentLengthClipSpace(const Vector4& Start, const Vector4& End, const bool bLocalCoordinates = false)
{
    const Matrix4& MVP = bLocalCoordinates ? GuizmoContext.MVPLocal : GuizmoContext.MVP;

    Vector4 StartOfSegment = MVP.Transform(Vector4(Start.X, Start.Y, Start.Z, 1.0f));
    if (Math::Abs(StartOfSegment.W) > Math::Constants::Epsilon) // Check for axis aligned with camera direction
    {
        StartOfSegment *= 1.0f / StartOfSegment.W;
    }

    Vector4 EndOfSegment = MVP.Transform(Vector4(End.X, End.Y, End.Z, 1.0f));
    if (Math::Abs(EndOfSegment.W) > Math::Constants::Epsilon) // Check for axis aligned with camera direction
    {
        EndOfSegment *= 1.0f / EndOfSegment.W;
    }

    Vector4 ClipSpaceAxis = EndOfSegment - StartOfSegment;
    if (GuizmoContext.DisplayRatio < 1.0)
    {
        ClipSpaceAxis.X *= GuizmoContext.DisplayRatio;
    }
    else
    {
        ClipSpaceAxis.Y /= GuizmoContext.DisplayRatio;
    }

    float SegmentLengthInClipSpace = Math::Sqrt(ClipSpaceAxis.X * ClipSpaceAxis.X + ClipSpaceAxis.Y * ClipSpaceAxis.Y);
    return SegmentLengthInClipSpace;
}

static float GetParallelogram(const Vector4& PointOrigin, const Vector4& PointA, const Vector4& PointB)
{
    Vector4 Points[] = 
    {
        PointOrigin,
        PointA,
        PointB
    };

    for (uint32 PointIndex = 0; PointIndex < 3; PointIndex++)
    {
        Points[PointIndex] = GuizmoContext.MVP.Transform(Vector4(Points[PointIndex].X, Points[PointIndex].Y, Points[PointIndex].Z, 1.0f));
        if (Math::Abs(Points[PointIndex].W) > Math::Constants::Epsilon) // Check for axis aligned with camera direction
        {
            Points[PointIndex] *= 1.0f / Points[PointIndex].W;
        }
    }

    Vector4 SegmentA = Points[1] - Points[0];
    Vector4 SegmentB = Points[2] - Points[0];
    SegmentA.Y /= GuizmoContext.DisplayRatio;
    SegmentB.Y /= GuizmoContext.DisplayRatio;

    Vector4 SegmentAOrtho = Vector4(-SegmentA.Y, SegmentA.X, 0.0f, 0.0f);
    SegmentAOrtho.Normalize();

    float DotProductResult = SegmentAOrtho.DotProduct(SegmentB);
    float Surface          = Math::Sqrt(SegmentA.X * SegmentA.X + SegmentA.Y * SegmentA.Y) * Math::Abs(DotProductResult);
    return Surface;
}

inline Vector4 PointOnSegment(const Vector4& Point, const Vector4& VertPos1, const Vector4& VertPos2)
{
    Vector4 PointToVert1     = Point - VertPos1;
    Vector4 SegmentDirection = (VertPos2 - VertPos1).GetNormalized();

    float SegmentLength   = (VertPos2 - VertPos1).GetLength();
    float ProjectionParam = SegmentDirection.DotProduct(PointToVert1);

    if (ProjectionParam < 0.0f)
    {
        return VertPos1;
    }

    if (ProjectionParam > SegmentLength)
    {
        return VertPos2;
    }

    return VertPos1 + SegmentDirection * ProjectionParam;
}

static bool IsInContextRect(ImVec2 Point)
{
    return (Point.x >= GuizmoContext.X && Point.x <= GuizmoContext.XMax)
        && (Point.y >= GuizmoContext.Y && Point.y <= GuizmoContext.YMax);
}

static bool IsHoveringWindow()
{
    ImGuiContext& Context = *ImGui::GetCurrentContext();

    if (GuizmoContext.DrawList == nullptr || GuizmoContext.DrawList->_OwnerName == nullptr)
    {
        return false;
    }

    ImGuiWindow* Window = ImGui::FindWindowByName(GuizmoContext.DrawList->_OwnerName);
    if (Window == nullptr)
    {
        return false;
    }

    if (Context.HoveredWindow == Window) // Mouse hovering Drawlist window
    {
        return true;
    }

    if (GuizmoContext.AlternativeWindow != nullptr && Context.HoveredWindow == GuizmoContext.AlternativeWindow)
    {
        return true;
    }

    if (Context.HoveredWindow != nullptr) // Any other window is hovered
    {
        return false;
    }

    // Hovering Drawlist window rect, no other window hovered (for _NoInputs windows)
    if (ImGui::IsMouseHoveringRect(Window->InnerRect.Min, Window->InnerRect.Max, false))
    {
        return true;
    }

    return false;
}

static void ComputeContext(const float* View, const float* Projection, float* Matrix, EditorGuizmo::EMode Mode)
{
    GuizmoContext.Mode          = Mode;
    GuizmoContext.ViewMat       = LoadMatrix(View);
    GuizmoContext.ProjectionMat = LoadMatrix(Projection);
    GuizmoContext.bMouseOver    = IsHoveringWindow();
    GuizmoContext.ModelSource   = LoadMatrix(Matrix);
    GuizmoContext.ModelLocal    = GuizmoContext.ModelSource;
    GuizmoContext.ModelLocal.OrthoNormalize();

    if (Mode == EditorGuizmo::EMode::Local)
    {
        GuizmoContext.Model = GuizmoContext.ModelLocal;
    }
    else
    {
        const Vector3 Translation = GuizmoContext.ModelSource.GetTranslation();
        GuizmoContext.Model = Matrix4::Translation(Translation.X, Translation.Y, Translation.Z);
    }

    Vector3 RightVec        = Vector3(GuizmoContext.ModelSource.M[0][0], GuizmoContext.ModelSource.M[0][1], GuizmoContext.ModelSource.M[0][2]);
    Vector3 UpVec           = Vector3(GuizmoContext.ModelSource.M[1][0], GuizmoContext.ModelSource.M[1][1], GuizmoContext.ModelSource.M[1][2]);
    Vector3 DirectionVector = Vector3(GuizmoContext.ModelSource.M[2][0], GuizmoContext.ModelSource.M[2][1], GuizmoContext.ModelSource.M[2][2]);

    GuizmoContext.ModelScaleOrigin   = Vector4(RightVec.GetLength(), UpVec.GetLength(), DirectionVector.GetLength(), 0.0f);
    GuizmoContext.ModelInverse       = GuizmoContext.Model.GetInverse();
    GuizmoContext.ModelSourceInverse = GuizmoContext.ModelSource.GetInverse();
    GuizmoContext.ViewProjection     = GuizmoContext.ViewMat * GuizmoContext.ProjectionMat;
    GuizmoContext.MVP                = GuizmoContext.Model * GuizmoContext.ViewProjection;
    GuizmoContext.MVPLocal           = GuizmoContext.ModelLocal * GuizmoContext.ViewProjection;

    Matrix4 ViewInverse       = GuizmoContext.ViewMat.GetInverse();
    GuizmoContext.CameraDir   = Vector4(ViewInverse.M[2][0], ViewInverse.M[2][1], ViewInverse.M[2][2], 0.0f);
    GuizmoContext.CameraEye   = Vector4(ViewInverse.M[3][0], ViewInverse.M[3][1], ViewInverse.M[3][2], 0.0f);
    GuizmoContext.CameraRight = Vector4(ViewInverse.M[0][0], ViewInverse.M[0][1], ViewInverse.M[0][2], 0.0f);
    GuizmoContext.CameraUp    = Vector4(ViewInverse.M[1][0], ViewInverse.M[1][1], ViewInverse.M[1][2], 0.0f);

    // Projection reverse
    Vector4 NearPos = GuizmoContext.ProjectionMat.Transform(Vector4(0.0f, 0.0f, 1.0f, 1.0f));
    Vector4 FarPos  = GuizmoContext.ProjectionMat.Transform(Vector4(0.0f, 0.0f, 2.0f, 1.0f));

    GuizmoContext.bReversed = (NearPos.Z/NearPos.W) > (FarPos.Z / FarPos.W);

    Vector4 RightViewInverse = Vector4(ViewInverse.M[0][0], ViewInverse.M[0][1], ViewInverse.M[0][2], 0.0f);
    RightViewInverse = GuizmoContext.ModelInverse.Transform(Vector4(RightViewInverse.X, RightViewInverse.Y, RightViewInverse.Z, 0.0f));

    const float RightLength    = GetSegmentLengthClipSpace(Vector4(0.0f, 0.0f, 0.0f, 0.0f), RightViewInverse);
    GuizmoContext.ScreenFactor = (Math::Abs(RightLength) > Math::Constants::Epsilon) ? (GuizmoContext.GizmoSizeClipSpace / RightLength) : 0.0f;

    ImVec2 CenterSSpace              = WorldToPos(Vector4(0.0f, 0.0f, 0.0f, 0.0f), GuizmoContext.MVP);
    GuizmoContext.ScreenSquareCenter = CenterSSpace;
    GuizmoContext.ScreenSquareMin    = ImVec2(CenterSSpace.x - 10.0f, CenterSSpace.y - 10.0f);
    GuizmoContext.ScreenSquareMax    = ImVec2(CenterSSpace.x + 10.0f, CenterSSpace.y + 10.0f);

    ComputeCameraRay(GuizmoContext.RayOrigin, GuizmoContext.RayVector);
}

static void ComputeColors(ImU32* Colors, int32 Type, EditorGuizmo::EOperation::Type Operation)
{
    if (GuizmoContext.bEnable)
    {
        ImU32 SelectionColor = GetColorU32(EditorGuizmo::EColor::Selection);

        switch (Operation)
        {
            case EditorGuizmo::EOperation::Translate:
            {
                Colors[0] = (Type == EMoveType::MoveScreen) ? SelectionColor : IM_COL32_WHITE;

                for (int32 AxisIndex = 0; AxisIndex < 3; AxisIndex++)
                {
                    Colors[AxisIndex + 1] = (Type == static_cast<int32>(EMoveType::MoveX + AxisIndex)) ? SelectionColor : GetColorU32(EditorGuizmo::EColor::DirectionX + AxisIndex);
                    Colors[AxisIndex + 4] = (Type == static_cast<int32>(EMoveType::MoveYZ + AxisIndex)) ? SelectionColor : GetColorU32(EditorGuizmo::EColor::PlaneX + AxisIndex);
                    Colors[AxisIndex + 4] = (Type == EMoveType::MoveScreen) ? SelectionColor : Colors[AxisIndex + 4];
                }

                break;
            }

            case EditorGuizmo::EOperation::Rotate:
            {
                Colors[0] = (Type == EMoveType::RotateScreen) ? SelectionColor : IM_COL32_WHITE;

                for (int32 AxisIndex = 0; AxisIndex < 3; AxisIndex++)
                {
                    Colors[AxisIndex + 1] = (Type == static_cast<int32>(EMoveType::RotateX + AxisIndex)) ? SelectionColor : GetColorU32(EditorGuizmo::EColor::DirectionX + AxisIndex);
                }

                break;
            }

            case EditorGuizmo::EOperation::ScaleU:
            case EditorGuizmo::EOperation::Scale:
            {
                Colors[0] = (Type == EMoveType::ScaleXYZ) ? SelectionColor : IM_COL32_WHITE;

                for (int32 AxisIndex = 0; AxisIndex < 3; AxisIndex++)
                {
                    Colors[AxisIndex + 1] = (Type == static_cast<int32>(EMoveType::ScaleX + AxisIndex)) ? SelectionColor : GetColorU32(EditorGuizmo::EColor::DirectionX + AxisIndex);
                }

                break;
            }

            // NOTE: this internal function is only called with three possible values for operation
            default:
            {
                break;
            }
        }
    }
    else
    {
        ImU32 InactiveColor = GetColorU32(EditorGuizmo::EColor::Inactive);
        for (int32 ColorIndex = 0; ColorIndex < 7; ColorIndex++)
        {
            Colors[ColorIndex] = InactiveColor;
        }
    }
}

static void ComputeTripodAxisAndVisibility(const int32 AxisIndex, Vector4& DirAxis, Vector4& DirPlaneX, Vector4& DirPlaneY, bool& bBelowAxisLimit, bool& bBelowPlaneLimit, 
    const bool bLocalCoordinates = false)
{
    DirAxis   = DirectionUnary[AxisIndex];
    DirPlaneX = DirectionUnary[(AxisIndex + 1) % 3];
    DirPlaneY = DirectionUnary[(AxisIndex + 2) % 3];

    if (GuizmoContext.bUsing && (GuizmoContext.GetCurrentID() == GuizmoContext.EditingID))
    {
        // When using, use stored factors so the gizmo doesn't flip when we translate
        // Apply axis mask to axes and planes

        bBelowAxisLimit  = GuizmoContext.bBelowAxisLimit[AxisIndex] && (((1 << AxisIndex) & GuizmoContext.AxisMask) == 0);
        bBelowPlaneLimit = GuizmoContext.bBelowPlaneLimit[AxisIndex] && (((1 << AxisIndex) == GuizmoContext.AxisMask) || (GuizmoContext.AxisMask == 0));

        DirAxis   *= GuizmoContext.AxisFactor[AxisIndex];
        DirPlaneX *= GuizmoContext.AxisFactor[(AxisIndex + 1) % 3];
        DirPlaneY *= GuizmoContext.AxisFactor[(AxisIndex + 2) % 3];
    }
    else
    {
        // New method
        const Vector4 Origin = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

        float LengthDir            = GetSegmentLengthClipSpace(Origin, DirAxis, bLocalCoordinates);
        float LengthDirMinus       = GetSegmentLengthClipSpace(Origin, -DirAxis, bLocalCoordinates);
        float LengthDirPlaneX      = GetSegmentLengthClipSpace(Origin, DirPlaneX, bLocalCoordinates);
        float LengthDirMinusPlaneX = GetSegmentLengthClipSpace(Origin, -DirPlaneX, bLocalCoordinates);
        float LengthDirPlaneY      = GetSegmentLengthClipSpace(Origin, DirPlaneY, bLocalCoordinates);
        float LengthDirMinusPlaneY = GetSegmentLengthClipSpace(Origin, -DirPlaneY, bLocalCoordinates);

        float MulAxis = (GuizmoContext.bAllowAxisFlip && (LengthDir < LengthDirMinus) && 
            (Math::Abs(LengthDir - LengthDirMinus) > Math::Constants::Epsilon)) ? -1.0f : 1.0f;
        float MulAxisX = (GuizmoContext.bAllowAxisFlip && (LengthDirPlaneX < LengthDirMinusPlaneX) && 
            (Math::Abs(LengthDirPlaneX - LengthDirMinusPlaneX) > Math::Constants::Epsilon)) ? -1.0f : 1.0f;
        float MulAxisY = (GuizmoContext.bAllowAxisFlip && (LengthDirPlaneY < LengthDirMinusPlaneY) && 
            (Math::Abs(LengthDirPlaneY - LengthDirMinusPlaneY) > Math::Constants::Epsilon)) ? -1.0f : 1.0f;

        DirAxis   *= MulAxis;
        DirPlaneX *= MulAxisX;
        DirPlaneY *= MulAxisY;

        // For axis
        float AxisLengthInClipSpace = GetSegmentLengthClipSpace(Origin, DirAxis * GuizmoContext.ScreenFactor, bLocalCoordinates);
        float ParaSurf              = GetParallelogram(Origin, DirPlaneX * GuizmoContext.ScreenFactor, DirPlaneY * GuizmoContext.ScreenFactor);

        // Apply axis mask to axes and planes
        bBelowPlaneLimit = (ParaSurf > GuizmoContext.AxisLimit) && (((1 << AxisIndex) == GuizmoContext.AxisMask) || (GuizmoContext.AxisMask == 0));
        bBelowAxisLimit  = (AxisLengthInClipSpace > GuizmoContext.PlaneLimit) && (((1 << AxisIndex) & GuizmoContext.AxisMask) == 0);

        // And store values
        GuizmoContext.AxisFactor[AxisIndex]           = MulAxis;
        GuizmoContext.AxisFactor[(AxisIndex + 1) % 3] = MulAxisX;
        GuizmoContext.AxisFactor[(AxisIndex + 2) % 3] = MulAxisY;
        GuizmoContext.bBelowAxisLimit[AxisIndex]      = bBelowAxisLimit;
        GuizmoContext.bBelowPlaneLimit[AxisIndex]     = bBelowPlaneLimit;
    }
}

static void ComputeSnap(float* Value, float Snap)
{
    if (Snap <= Math::Constants::Epsilon)
    {
        return;
    }

    float Modulo      = Math::FMod(*Value, Snap);
    float ModuloRatio = Math::Abs(Modulo) / Snap;

    if (ModuloRatio < SnapTension)
    {
        *Value -= Modulo;
    }
    else if (ModuloRatio > (1.0f - SnapTension))
    {
        *Value = *Value - Modulo + Snap * ((*Value < 0.0f) ? -1.0f : 1.0f);
    }
}

static void ComputeSnap(Vector4& Value, const float* Snap)
{
    for (int32 AxisIndex = 0; AxisIndex < 3; AxisIndex++)
    {
        ComputeSnap(&Value[AxisIndex], Snap[AxisIndex]);
    }
}

static float ComputeAngleOnPlan() 
{ 
    const float Length = GuizmoContext.TranslationPlan.IntersectRay( 
        Vector3(GuizmoContext.RayOrigin.X, GuizmoContext.RayOrigin.Y, GuizmoContext.RayOrigin.Z), 
        Vector3(GuizmoContext.RayVector.X, GuizmoContext.RayVector.Y, GuizmoContext.RayVector.Z)); 
 
    if (Length < 0.0f) 
    { 
        return GuizmoContext.RotationAngleOrigin; 
    } 
 
    const Vector3 PlaneNormal3 = GuizmoContext.TranslationPlan.GetNormal(); 
    const Vector4 PlaneNormal  = Vector4(PlaneNormal3.X, PlaneNormal3.Y, PlaneNormal3.Z, 0.0f); 
 
    const Vector4 ModelPosition = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f); 
    const Vector4 LocalPosition = (GuizmoContext.RayOrigin + GuizmoContext.RayVector * Length - ModelPosition).GetNormalized(); 
 
    const float CosAngle = Math::Clamp(LocalPosition.DotProduct(GuizmoContext.RotationVectorSource), -1.0f, 1.0f); 
    const float SinAngle = PlaneNormal.DotProduct(GuizmoContext.RotationVectorSource.CrossProduct(LocalPosition)); 
    return Math::Atan2(SinAngle, CosAngle); 
} 

static void DrawRotationGizmo(EditorGuizmo::EOperation::Type Op, int32 Type)
{
    if (!Intersects(Op, EditorGuizmo::EOperation::Rotate))
    {
        return;
    }

    ImDrawList* DrawList = GuizmoContext.DrawList;

    const bool bIsNoAxesMasked = (GuizmoContext.AxisMask == 0);

    // Colors
    ImU32 Colors[7];
    ComputeColors(Colors, Type, EditorGuizmo::EOperation::Rotate);

    Vector4 ViewDirNormalized;
    if (GuizmoContext.bIsOrthographic)
    {
        Matrix4 ViewInverse = GuizmoContext.ViewMat.GetInverse();
        ViewDirNormalized    = Vector4(-ViewInverse.M[2][0], -ViewInverse.M[2][1], -ViewInverse.M[2][2], 0.0f);
    }
    else
    {
        // Match the "front" half used for picking (camera-forward depth test).
        // The arc we render should be the half that's closer to the camera, which is opposite the camera forward direction.
        ViewDirNormalized = (-GuizmoContext.CameraDir).GetNormalized();
    }

    ViewDirNormalized = GuizmoContext.ModelInverse.Transform(Vector4(ViewDirNormalized.X, ViewDirNormalized.Y, ViewDirNormalized.Z, 0.0f));
    GuizmoContext.RadiusSquareCenter = ScreenRotateSize * GuizmoContext.Height;

    bool bHasRSC = Intersects(Op, EditorGuizmo::EOperation::RotateScreen);
    for (int32 Axis = 0; Axis < 3; Axis++)
    {
        if (!Intersects(Op, static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::RotateZ >> Axis)))
        {
            continue;
        }

        const bool bIsAxisMasked = ((1 << (2 - Axis)) & GuizmoContext.AxisMask) != 0;
        if (bIsAxisMasked)
        {
            continue;
        }

        const bool  bUsingAxis = (GuizmoContext.bUsing && Type == EMoveType::RotateZ - Axis);
        const int32 CircleMul  = (bHasRSC && !bUsingAxis) ? 1 : 2;

        ImVec2* CirclePos = reinterpret_cast<ImVec2*>(alloca(sizeof(ImVec2) * (CircleMul * HalfCircleSegmentCount + 1)));

        float AngleStart = Math::Atan2(
            ViewDirNormalized[(4 - Axis) % 3], 
            ViewDirNormalized[(3 - Axis) % 3]) + (GuizmoContext.bIsOrthographic ? Math::Constants::PI : -Math::Constants::PI) * 0.5f;

        for (int32 CircleSegmentIndex = 0; CircleSegmentIndex < CircleMul * HalfCircleSegmentCount + 1; CircleSegmentIndex++)
        {
            float Angle = AngleStart + static_cast<float>(CircleMul) * Math::Constants::PI * (static_cast<float>(CircleSegmentIndex) / static_cast<float>(CircleMul * HalfCircleSegmentCount));
           
            Vector4 AxisPos  = Vector4(Math::Cos(Angle), Math::Sin(Angle), 0.0f, 0.0f);
            Vector4 Position = Vector4(AxisPos[Axis], AxisPos[(Axis + 1) % 3], AxisPos[(Axis + 2) % 3], 0.0f);
            Position = Position * GuizmoContext.ScreenFactor * RotationDisplayFactor;

            CirclePos[CircleSegmentIndex] = WorldToPos(Position, GuizmoContext.MVP);
        }

        if (!GuizmoContext.bUsing || bUsingAxis)
        {
            DrawList->AddPolyline(CirclePos, CircleMul * HalfCircleSegmentCount + 1, Colors[3 - Axis], false,
                GuizmoContext.Style.RotationLineThickness);
        }

        Vector4 ModelPosition = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);

        float RadiusAxis = Math::Sqrt((ImLengthSqr(WorldToPos(ModelPosition, GuizmoContext.ViewProjection) - CirclePos[0])));
        if (RadiusAxis > GuizmoContext.RadiusSquareCenter)
        {
            GuizmoContext.RadiusSquareCenter = RadiusAxis;
        }
    }

    if (bHasRSC && (!GuizmoContext.bUsing || Type == EMoveType::RotateScreen) && bIsNoAxesMasked)
    {
        Vector4 ModelPosition = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
        DrawList->AddCircle(WorldToPos(ModelPosition, GuizmoContext.ViewProjection), 
            GuizmoContext.RadiusSquareCenter, Colors[0], 64, GuizmoContext.Style.RotationOuterLineThickness);
    }

    if (GuizmoContext.bUsing && (GuizmoContext.GetCurrentID() == GuizmoContext.EditingID) && IsRotateType(Type))
    {
        ImVec2 CirclePos[HalfCircleSegmentCount + 1];

        Vector4 ModelPosition = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
        CirclePos[0] = WorldToPos(ModelPosition, GuizmoContext.ViewProjection);

        for (uint32 CircleSegmentIndex = 1; CircleSegmentIndex < HalfCircleSegmentCount + 1; CircleSegmentIndex++)
        {
            float Angle = GuizmoContext.RotationAngle * (static_cast<float>(CircleSegmentIndex - 1) / static_cast<float>(HalfCircleSegmentCount - 1));

            Matrix4 RotateVectorMatrix = Quaternion::FromAxisAngle(GuizmoContext.TranslationPlan.GetNormal(), Angle).ToMatrix4();

            Vector4 Position = RotateVectorMatrix.Transform(Vector4(
                GuizmoContext.RotationVectorSource.X,
                GuizmoContext.RotationVectorSource.Y,
                GuizmoContext.RotationVectorSource.Z,
                1.0f));

            Position *= GuizmoContext.ScreenFactor * RotationDisplayFactor;

            Vector4 ModelPosition2 = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
            CirclePos[CircleSegmentIndex] = WorldToPos(Position + ModelPosition2, GuizmoContext.ViewProjection);
        }

        DrawList->AddConvexPolyFilled(CirclePos, HalfCircleSegmentCount + 1, GetColorU32(EditorGuizmo::EColor::RotationUsingFill));
        DrawList->AddPolyline(CirclePos, HalfCircleSegmentCount + 1, GetColorU32(EditorGuizmo::EColor::RotationUsingBorder), true, GuizmoContext.Style.RotationLineThickness);

        ImVec2 DestinationPosOnScreen = CirclePos[1];

        char TempString[512];
        ImFormatString(TempString, sizeof(TempString), 
            RotationInfoMask[Type - EMoveType::RotateX], 
            GuizmoContext.RotationAngle * Math::Constants::RadToDeg,
            GuizmoContext.RotationAngle);

        DrawList->AddText(ImVec2(DestinationPosOnScreen.x + 15, DestinationPosOnScreen.y + 15), GetColorU32(EditorGuizmo::EColor::TextShadow), TempString);
        DrawList->AddText(ImVec2(DestinationPosOnScreen.x + 14, DestinationPosOnScreen.y + 14), GetColorU32(EditorGuizmo::EColor::Text), TempString);
    }
}

static void DrawHatchedAxis(const Vector4& Axis) 
{ 
    if (GuizmoContext.Style.HatchedAxisLineThickness <= 0.0f) 
    { 
        return; 
    } 

    for (int32 HatchSegmentIndex = 1; HatchSegmentIndex < 10; HatchSegmentIndex++)
    {
        ImVec2 BaseSSpace2     = WorldToPos(Axis * 0.05f * static_cast<float>(HatchSegmentIndex * 2) * GuizmoContext.ScreenFactor, GuizmoContext.MVP);
        ImVec2 WorldDirSSpace2 = WorldToPos(Axis * 0.05f * static_cast<float>(HatchSegmentIndex * 2 + 1) * GuizmoContext.ScreenFactor, GuizmoContext.MVP);
        GuizmoContext.DrawList->AddLine(BaseSSpace2, WorldDirSSpace2, GetColorU32(EditorGuizmo::EColor::HatchedAxisLines), GuizmoContext.Style.HatchedAxisLineThickness); 
    } 
} 

static void DrawScaleHandleCircle(ImDrawList* DrawList, const ImVec2& Center, float Radius, ImU32 Color)
{
    DrawList->AddCircleFilled(Center, Radius, Color);
}

static void DrawScaleHandleSquare(ImDrawList* DrawList, const ImVec2& Center, float HalfSize, ImU32 Color)
{
    const ImVec2 Half(HalfSize, HalfSize);
    DrawList->AddRectFilled(Center - Half, Center + Half, Color);
}

static void DrawScaleHandle(ImDrawList* DrawList, const ImVec2& Center, float Size, ImU32 Color)
{
    if (GuizmoContext.Style.ScaleHandleShape == EditorGuizmo::EScaleHandleShape::Square)
    {
        DrawScaleHandleSquare(DrawList, Center, Size, Color);
        return;
    }

    DrawScaleHandleCircle(DrawList, Center, Size, Color);
}
 
static void DrawScaleGizmo(EditorGuizmo::EOperation::Type Op, int32 Type) 
{ 
    ImDrawList* DrawList = GuizmoContext.DrawList; 
 
        if (!Intersects(Op, EditorGuizmo::EOperation::Scale))
    {
        return;
    }

    // Colors
    ImU32 Colors[7];
    ComputeColors(Colors, Type, EditorGuizmo::EOperation::Scale);

    // Draw
    Vector4 ScaleDisplay = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    if (GuizmoContext.bUsing && (GuizmoContext.GetCurrentID() == GuizmoContext.EditingID))
    {
        ScaleDisplay = GuizmoContext.Scale;
    }

    for (int32 AxisIndex = 0; AxisIndex < 3; AxisIndex++)
    {
        if (!Intersects(Op, static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::ScaleX << AxisIndex)))
        {
            continue;
        }

        const bool bUsingAxis = (GuizmoContext.bUsing && Type == EMoveType::ScaleX + AxisIndex);
        if (!GuizmoContext.bUsing || bUsingAxis)
        {
            Vector4 DirPlaneX;
            Vector4 DirPlaneY;
            Vector4 DirAxis;

            bool bBelowAxisLimit;
            bool bBelowPlaneLimit;
            ComputeTripodAxisAndVisibility(AxisIndex, DirAxis, DirPlaneX, DirPlaneY, bBelowAxisLimit, bBelowPlaneLimit, true);

            // Draw axis
            if (bBelowAxisLimit)
            {
                bool bHasTranslateOnAxis = Contains(Op, static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::TranslateX << AxisIndex));

                float  MarkerScale           = bHasTranslateOnAxis ? 1.4f : 1.0f;
                ImVec2 BaseSSpace            = WorldToPos(DirAxis * 0.1f * GuizmoContext.ScreenFactor, GuizmoContext.MVP);
                ImVec2 WorldDirSSpaceNoScale = WorldToPos(DirAxis * MarkerScale * GuizmoContext.ScreenFactor, GuizmoContext.MVP);
                ImVec2 WorldDirSSpace        = WorldToPos((DirAxis * MarkerScale * ScaleDisplay[AxisIndex]) * GuizmoContext.ScreenFactor, GuizmoContext.MVP);

                if (GuizmoContext.bUsing && (GuizmoContext.GetCurrentID() == GuizmoContext.EditingID)) 
                { 
                    ImU32 ScaleLineColor = GetColorU32(EditorGuizmo::EColor::ScaleLine); 
                    DrawList->AddLine(BaseSSpace, WorldDirSSpaceNoScale, ScaleLineColor, GuizmoContext.Style.ScaleLineThickness); 
                    DrawScaleHandle(DrawList, WorldDirSSpaceNoScale, GuizmoContext.Style.ScaleLineCircleSize, ScaleLineColor);
                } 
 
                if (!bHasTranslateOnAxis || GuizmoContext.bUsing) 
                { 
                    DrawList->AddLine(BaseSSpace, WorldDirSSpace, Colors[AxisIndex + 1], GuizmoContext.Style.ScaleLineThickness); 
                } 
 
                DrawScaleHandle(DrawList, WorldDirSSpace, GuizmoContext.Style.ScaleLineCircleSize, Colors[AxisIndex + 1]);
 
                if (GuizmoContext.AxisFactor[AxisIndex] < 0.0f) 
                { 
                    DrawHatchedAxis(DirAxis * ScaleDisplay[AxisIndex]); 
                } 
            }
        }
    }

    // Draw screen circle
    DrawList->AddCircleFilled(GuizmoContext.ScreenSquareCenter, GuizmoContext.Style.CenterCircleSize, Colors[0], 32);

    if (GuizmoContext.bUsing && (GuizmoContext.GetCurrentID() == GuizmoContext.EditingID) && IsScaleType(Type))
    {
        Vector4 ModelPosition = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);

        char TempString[512];
        int32 ComponentInfoIndex = (Type - EMoveType::ScaleX) * 3;
        ImFormatString(TempString, sizeof(TempString), ScaleInfoMask[Type - EMoveType::ScaleX],
            ScaleDisplay[TranslationInfoIndex[ComponentInfoIndex]]);

        ImVec2 DestinationPosOnScreen = WorldToPos(ModelPosition, GuizmoContext.ViewProjection);
        DrawList->AddText(ImVec2(DestinationPosOnScreen.x + 15, DestinationPosOnScreen.y + 15), GetColorU32(EditorGuizmo::EColor::TextShadow), TempString);
        DrawList->AddText(ImVec2(DestinationPosOnScreen.x + 14, DestinationPosOnScreen.y + 14), GetColorU32(EditorGuizmo::EColor::Text), TempString);
    }
}

static void DrawScaleUniveralGizmo(EditorGuizmo::EOperation::Type Op, int32 Type)
{
    ImDrawList* DrawList = GuizmoContext.DrawList;

    if (!Intersects(Op, EditorGuizmo::EOperation::ScaleU))
    {
        return;
    }

    // Colors
    ImU32 Colors[7];
    ComputeColors(Colors, Type, EditorGuizmo::EOperation::ScaleU);

    // Draw
    Vector4 ScaleDisplay = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    if (GuizmoContext.bUsing && (GuizmoContext.GetCurrentID() == GuizmoContext.EditingID))
    {
        ScaleDisplay = GuizmoContext.Scale;
    }

    for (int32 AxisIndex = 0; AxisIndex < 3; AxisIndex++)
    {
        if (!Intersects(Op, static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::ScaleXU << AxisIndex)))
        {
            continue;
        }

        const bool bUsingAxis = (GuizmoContext.bUsing && Type == EMoveType::ScaleX + AxisIndex);
        if (!GuizmoContext.bUsing || bUsingAxis)
        {
            Vector4 DirPlaneX;
            Vector4 DirPlaneY;
            Vector4 DirAxis;

            bool bBelowAxisLimit;
            bool bBelowPlaneLimit;
            ComputeTripodAxisAndVisibility(AxisIndex, DirAxis, DirPlaneX, DirPlaneY, bBelowAxisLimit, bBelowPlaneLimit, true);

            // Draw axis
            if (bBelowAxisLimit)
            {
                bool  bHasTranslateOnAxis = Contains(Op, static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::TranslateX << AxisIndex));
                float MarkerScale         = bHasTranslateOnAxis ? 1.4f : 1.0f; 
 
                ImVec2 WorldDirSSpace = WorldToPos((DirAxis * MarkerScale * ScaleDisplay[AxisIndex]) * GuizmoContext.ScreenFactor, GuizmoContext.MVPLocal); 
                DrawScaleHandle(DrawList, WorldDirSSpace, GuizmoContext.Style.ScaleLineCircleSize * 2.0f, Colors[AxisIndex + 1]);
            } 
        } 
    } 
 
    // Draw screen circle
    DrawList->AddCircle(GuizmoContext.ScreenSquareCenter, 20.0f, Colors[0], 32, GuizmoContext.Style.CenterCircleSize);

    if (GuizmoContext.bUsing && (GuizmoContext.GetCurrentID() == GuizmoContext.EditingID) && IsScaleType(Type))
    {
        Vector4 ModelPosition = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);

        char TempString[512];
        int32 ComponentInfoIndex = (Type - EMoveType::ScaleX) * 3;
        ImFormatString(TempString, sizeof(TempString), ScaleInfoMask[Type - EMoveType::ScaleX],
            ScaleDisplay[TranslationInfoIndex[ComponentInfoIndex]]);

        ImVec2 DestinationPosOnScreen = WorldToPos(ModelPosition, GuizmoContext.ViewProjection);
        DrawList->AddText(ImVec2(DestinationPosOnScreen.x + 15, DestinationPosOnScreen.y + 15), GetColorU32(EditorGuizmo::EColor::TextShadow), TempString);
        DrawList->AddText(ImVec2(DestinationPosOnScreen.x + 14, DestinationPosOnScreen.y + 14), GetColorU32(EditorGuizmo::EColor::Text), TempString);
    }
}

static void DrawTranslationGizmo(EditorGuizmo::EOperation::Type Op, int32 Type)
{
    ImDrawList* DrawList = GuizmoContext.DrawList;
    if (!DrawList)
    {
        return;
    }

    if (!Intersects(Op, EditorGuizmo::EOperation::Translate))
    {
        return;
    }

    // Colors
    ImU32 Colors[7];
    ComputeColors(Colors, Type, EditorGuizmo::EOperation::Translate);

    Vector4 ModelPosition = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
    const ImVec2 Origin = WorldToPos(ModelPosition, GuizmoContext.ViewProjection);

    // Draw
    bool bBelowAxisLimit  = false;
    bool bBelowPlaneLimit = false;

    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        Vector4 DirPlaneX, DirPlaneY, DirAxis;
        ComputeTripodAxisAndVisibility(AxisIndex, DirAxis, DirPlaneX, DirPlaneY, bBelowAxisLimit, bBelowPlaneLimit);

        if (!GuizmoContext.bUsing || (GuizmoContext.bUsing && Type == EMoveType::MoveX + AxisIndex))
        {
            // Draw axis
            if (bBelowAxisLimit && Intersects(Op, static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::TranslateX << AxisIndex)))
            {
                ImVec2 BaseSSpace     = WorldToPos(DirAxis * 0.1f * GuizmoContext.ScreenFactor, GuizmoContext.MVP);
                ImVec2 WorldDirSSpace = WorldToPos(DirAxis * GuizmoContext.ScreenFactor, GuizmoContext.MVP);

                DrawList->AddLine(BaseSSpace, WorldDirSSpace, Colors[AxisIndex + 1], GuizmoContext.Style.TranslationLineThickness);

                // Arrow head begin
                ImVec2 Direction = ImVec2(Origin - WorldDirSSpace);

                float DirectionLength = Math::Sqrt(ImLengthSqr(Direction));
                Direction /= DirectionLength; // Normalize
                Direction *= GuizmoContext.Style.TranslationLineArrowSize;

                ImVec2 OrthogonalDirection = ImVec2(Direction.y, -Direction.x); // Perpendicular vector
                ImVec2 ArrowTipPosition    = ImVec2(WorldDirSSpace + Direction);
                DrawList->AddTriangleFilled(
                    WorldDirSSpace - Direction, 
                    ArrowTipPosition + OrthogonalDirection, 
                    ArrowTipPosition - OrthogonalDirection,
                    Colors[AxisIndex + 1]);
                
                // Arrow head end
                if (GuizmoContext.AxisFactor[AxisIndex] < 0.0f)
                {
                    DrawHatchedAxis(DirAxis);
                }
            }
        }

        // Draw plane
        if (!GuizmoContext.bUsing || (GuizmoContext.bUsing && Type == EMoveType::MoveYZ + AxisIndex))
        {
            if (bBelowPlaneLimit && Contains(Op, TRANSLATE_PLANS[AxisIndex]))
            {
                ImVec2 ScreenQuadPts[4];
                for (int32 QuadCornerIndex = 0; QuadCornerIndex < 4; ++QuadCornerIndex)
                {
                    Vector4 CornerWorldPos = (DirPlaneX * QuadUV[QuadCornerIndex * 2] + DirPlaneY * QuadUV[QuadCornerIndex * 2 + 1]) * GuizmoContext.ScreenFactor;
                    ScreenQuadPts[QuadCornerIndex] = WorldToPos(CornerWorldPos, GuizmoContext.MVP);
                }

                DrawList->AddPolyline(ScreenQuadPts, 4, GetColorU32(EditorGuizmo::EColor::DirectionX + AxisIndex), true, 1.0f);
                DrawList->AddConvexPolyFilled(ScreenQuadPts, 4, Colors[AxisIndex + 4]);
            }
        }
    }

    DrawList->AddCircleFilled(GuizmoContext.ScreenSquareCenter, GuizmoContext.Style.CenterCircleSize, Colors[0], 32);

    if (GuizmoContext.bUsing && (GuizmoContext.GetCurrentID() == GuizmoContext.EditingID) && IsTranslateType(Type))
    {
        ImU32   TranslationLineColor   = GetColorU32(EditorGuizmo::EColor::TranslationLine);
        ImVec2  SourcePosOnScreen      = WorldToPos(GuizmoContext.MatrixOrigin, GuizmoContext.ViewProjection);
        Vector4 ModelPositionTranslate = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
        ImVec2  DestinationPosOnScreen = WorldToPos(ModelPositionTranslate, GuizmoContext.ViewProjection);

        Vector4 Difference = Vector4(DestinationPosOnScreen.x - SourcePosOnScreen.x, DestinationPosOnScreen.y - SourcePosOnScreen.y, 0.0f, 0.0f);
        Difference.Normalize();
        Difference *= 5.0f;

        DrawList->AddCircle(SourcePosOnScreen, 6.0f, TranslationLineColor);
        DrawList->AddCircle(DestinationPosOnScreen, 6.0f, TranslationLineColor);
        DrawList->AddLine(
            ImVec2(SourcePosOnScreen.x + Difference.X, SourcePosOnScreen.y + Difference.Y), 
            ImVec2(DestinationPosOnScreen.x - Difference.X, DestinationPosOnScreen.y - Difference.Y), 
            TranslationLineColor,
            2.0f);

        char TempString[512];
        Vector4 ModelPosition2 = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
        Vector4 DeltaInfo      = ModelPosition2 - GuizmoContext.MatrixOrigin;

        int32 ComponentInfoIndex = (Type - EMoveType::MoveX) * 3;
        ImFormatString(TempString, sizeof(TempString), 
            TranslationInfoMask[Type - EMoveType::MoveX], 
            DeltaInfo[TranslationInfoIndex[ComponentInfoIndex]], 
            DeltaInfo[TranslationInfoIndex[ComponentInfoIndex + 1]], 
            DeltaInfo[TranslationInfoIndex[ComponentInfoIndex + 2]]);
         
        DrawList->AddText(ImVec2(DestinationPosOnScreen.x + 15, DestinationPosOnScreen.y + 15), GetColorU32(EditorGuizmo::EColor::TextShadow), TempString);
        DrawList->AddText(ImVec2(DestinationPosOnScreen.x + 14, DestinationPosOnScreen.y + 14), GetColorU32(EditorGuizmo::EColor::Text), TempString);
    }
}

static bool CanActivate()
{
    // NOTE:
    // In the editor viewport we often have an InvisibleButton covering the entire viewport
    // to handle focus / input routing. That makes ImGui report "some item is hovered/active",
    // which would prevent the gizmo from ever activating on click/drag.
    return ImGui::IsMouseClicked(0) && GuizmoContext.bMouseOver;
}

static bool HandleAndDrawLocalBounds(const float* Bounds, float* Matrix, const float* SnapValues, EditorGuizmo::EOperation::Type Operation)
{
    ImGuiIO& State = ImGui::GetIO();
    ImDrawList* DrawList = GuizmoContext.DrawList;

    bool bManipulated = false;

    // Compute best Projection axis
    Vector4 AxesWorldDirections[3];
    Vector4 BestAxisWorldDirection = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    int32   Axes[3];

    uint32 NumAxes = 1;
    Axes[0] = GuizmoContext.BoundsBestAxis;

    int32 BestAxis = Axes[0];
    if (!GuizmoContext.bUsingBounds)
    {
        NumAxes = 0;

        float BestDot = 0.0f;
        for (int32 AxisIndex = 0; AxisIndex < 3; AxisIndex++)
        {
            Vector4 DirPlaneNormalWorld;
            DirPlaneNormalWorld = GuizmoContext.ModelSource.Transform(
                Vector4(DirectionUnary[AxisIndex].X, DirectionUnary[AxisIndex].Y, DirectionUnary[AxisIndex].Z, 0.0f));
            DirPlaneNormalWorld.Normalize();

            Vector4 ModelSourcePos = Vector4(GuizmoContext.ModelSource.GetTranslation(), 1.0f);

            float DotProductResult = Math::Abs((GuizmoContext.CameraEye - ModelSourcePos).GetNormalized().DotProduct(DirPlaneNormalWorld));
            if (DotProductResult >= BestDot)
            {
                BestDot                = DotProductResult;
                BestAxis               = AxisIndex;
                BestAxisWorldDirection = DirPlaneNormalWorld;
            }

            if (DotProductResult >= 0.1f)
            {
                Axes[NumAxes]                = AxisIndex;
                AxesWorldDirections[NumAxes] = DirPlaneNormalWorld;
                ++NumAxes;
            }
        }
    }

    if (NumAxes == 0)
    {
        Axes[0]                = BestAxis;
        AxesWorldDirections[0] = BestAxisWorldDirection;
        NumAxes                = 1;
    }

    else if (BestAxis != Axes[0])
    {
        uint32 BestIndex = 0;
        for (uint32 AxisIndex = 0; AxisIndex < NumAxes; AxisIndex++)
        {
            if (Axes[AxisIndex] == BestAxis)
            {
                BestIndex = AxisIndex;
                break;
            }
        }

        int32 TempAxis  = Axes[0];
        Axes[0]         = Axes[BestIndex];
        Axes[BestIndex] = TempAxis;

        Vector4 TempDirection         = AxesWorldDirections[0];
        AxesWorldDirections[0]         = AxesWorldDirections[BestIndex];
        AxesWorldDirections[BestIndex] = TempDirection;
    }

    for (uint32 AxisIndex = 0; AxisIndex < NumAxes; ++AxisIndex)
    {
        BestAxis               = Axes[AxisIndex];
        BestAxisWorldDirection = AxesWorldDirections[AxisIndex];

        // Corners
        Vector4 AABB[4];

        int32 SecondAxis = (BestAxis + 1) % 3;
        int32 ThirdAxis  = (BestAxis + 2) % 3;

        for (int32 CornerIndex = 0; CornerIndex < 4; CornerIndex++)
        {
            AABB[CornerIndex][3]          = AABB[CornerIndex][BestAxis] = 0.0f;
            AABB[CornerIndex][SecondAxis] = Bounds[SecondAxis + 3 * (CornerIndex >> 1)];
            AABB[CornerIndex][ThirdAxis]  = Bounds[ThirdAxis + 3 * ((CornerIndex >> 1) ^ (CornerIndex & 1))];
        }

        // Draw bounds
        uint32 AnchorAlpha = GuizmoContext.bEnable ? IM_COL32_BLACK : IM_COL32(0, 0, 0, 0x80);

        Matrix4 BoundsMVP = GuizmoContext.ModelSource * GuizmoContext.ViewProjection;
        for (int32 EdgeIndex = 0; EdgeIndex < 4; EdgeIndex++)
        {
            ImVec2 WorldBound1 = WorldToPos(AABB[EdgeIndex], BoundsMVP);
            ImVec2 WorldBound2 = WorldToPos(AABB[(EdgeIndex + 1) % 4], BoundsMVP);

            if (!IsInContextRect(WorldBound1) || !IsInContextRect(WorldBound2))
            {
                continue;
            }

            float BoundDistance = Math::Sqrt(ImLengthSqr(WorldBound1 - WorldBound2));
            int32 StepCount     = static_cast<int32>(BoundDistance / 10.0f);
            StepCount = Math::Min(StepCount, 1000);

            for (int32 StepIndex = 0; StepIndex < StepCount; StepIndex++)
            {
                float StepLength = 1.0f / static_cast<float>(StepCount);
                float T1         = static_cast<float>(StepIndex) * StepLength;
                float T2         = static_cast<float>(StepIndex) * StepLength + StepLength * 0.5f;

                ImVec2 WorldBoundSS1 = ImLerp(WorldBound1, WorldBound2, ImVec2(T1, T1));
                ImVec2 WorldBoundSS2 = ImLerp(WorldBound1, WorldBound2, ImVec2(T2, T2));
                DrawList->AddLine(WorldBoundSS1, WorldBoundSS2, IM_COL32(0xAA, 0xAA, 0xAA, 0) + AnchorAlpha, 2.0f);
            }

            Vector4 MidPoint = (AABB[EdgeIndex] + AABB[(EdgeIndex + 1) % 4]) * 0.5f;
            ImVec2   MidBound = WorldToPos(MidPoint, BoundsMVP);

            static const float AnchorBigRadius   = 8.0f;
            static const float AnchorSmallRadius = 6.0f;

            bool bOverBigAnchor   = ImLengthSqr(WorldBound1 - State.MousePos) <= (AnchorBigRadius * AnchorBigRadius);
            bool bOverSmallAnchor = ImLengthSqr(MidBound - State.MousePos) <= (AnchorBigRadius * AnchorBigRadius);

            int32 Type = EMoveType::None;

            if (Intersects(Operation, EditorGuizmo::EOperation::Translate))
            {
                Type = GetMoveType(Operation, nullptr);
            }

            if (Intersects(Operation, EditorGuizmo::EOperation::Rotate) && Type == EMoveType::None)
            {
                Type = GetRotateType(Operation);
            }

            if (Intersects(Operation, EditorGuizmo::EOperation::Scale) && Type == EMoveType::None)
            {
                Type = GetScaleType(Operation);
            }

            if (Type != EMoveType::None)
            {
                bOverBigAnchor   = false;
                bOverSmallAnchor = false;
            }

            ImU32 SelectionColor   = GetColorU32(EditorGuizmo::EColor::Selection);
            ImU32 BigAnchorColor   = bOverBigAnchor ? SelectionColor : (IM_COL32(0xAA, 0xAA, 0xAA, 0) + AnchorAlpha);
            ImU32 SmallAnchorColor = bOverSmallAnchor ? SelectionColor : (IM_COL32(0xAA, 0xAA, 0xAA, 0) + AnchorAlpha);

            DrawList->AddCircleFilled(WorldBound1, AnchorBigRadius, IM_COL32_BLACK);
            DrawList->AddCircleFilled(WorldBound1, AnchorBigRadius - 1.2f, BigAnchorColor);
            DrawList->AddCircleFilled(MidBound, AnchorSmallRadius, IM_COL32_BLACK);
            DrawList->AddCircleFilled(MidBound, AnchorSmallRadius - 1.2f, SmallAnchorColor);
            
            // Big anchor on corners
            int32 OppositeIndex = (EdgeIndex + 2) % 4;
            if (!GuizmoContext.bUsingBounds && GuizmoContext.bEnable && bOverBigAnchor && CanActivate())
            {
                GuizmoContext.BoundsPivot = GuizmoContext.ModelSource.Transform(
                    Vector4(AABB[(EdgeIndex + 2) % 4].X, AABB[(EdgeIndex + 2) % 4].Y, AABB[(EdgeIndex + 2) % 4].Z, 1.0f));
                GuizmoContext.BoundsAnchor = GuizmoContext.ModelSource.Transform(
                    Vector4(AABB[EdgeIndex].X, AABB[EdgeIndex].Y, AABB[EdgeIndex].Z, 1.0f));
                GuizmoContext.BoundsPlan = Plane::FromPointAndNormal(
                    Vector3(GuizmoContext.BoundsAnchor.X, GuizmoContext.BoundsAnchor.Y, GuizmoContext.BoundsAnchor.Z),
                    Vector3(BestAxisWorldDirection.X, BestAxisWorldDirection.Y, BestAxisWorldDirection.Z));

                GuizmoContext.BoundsBestAxis               = BestAxis;
                GuizmoContext.BoundsAxis[0]                = SecondAxis;
                GuizmoContext.BoundsAxis[1]                = ThirdAxis;
                GuizmoContext.BoundsLocalPivot             = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
                GuizmoContext.BoundsLocalPivot[SecondAxis] = AABB[OppositeIndex][SecondAxis];
                GuizmoContext.BoundsLocalPivot[ThirdAxis]  = AABB[OppositeIndex][ThirdAxis];
                GuizmoContext.bUsingBounds                 = true;
                GuizmoContext.EditingID                    = GuizmoContext.GetCurrentID();
                GuizmoContext.BoundsMatrix                 = GuizmoContext.ModelSource;
            }

            // Small anchor on middle of segment
            if (!GuizmoContext.bUsingBounds && GuizmoContext.bEnable && bOverSmallAnchor && CanActivate())
            {
                Vector4 MidPointOpposite = (AABB[(EdgeIndex + 2) % 4] + AABB[(EdgeIndex + 3) % 4]) * 0.5f;
                GuizmoContext.BoundsPivot = GuizmoContext.ModelSource.Transform(
                    Vector4(MidPointOpposite.X, MidPointOpposite.Y, MidPointOpposite.Z, 1.0f));
                GuizmoContext.BoundsAnchor = GuizmoContext.ModelSource.Transform(
                    Vector4(MidPoint.X, MidPoint.Y, MidPoint.Z, 1.0f));
                GuizmoContext.BoundsPlan = Plane::FromPointAndNormal(
                    Vector3(GuizmoContext.BoundsAnchor.X, GuizmoContext.BoundsAnchor.Y, GuizmoContext.BoundsAnchor.Z), 
                    Vector3(BestAxisWorldDirection.X, BestAxisWorldDirection.Y, BestAxisWorldDirection.Z));

                GuizmoContext.BoundsBestAxis = BestAxis;

                int32 Indices[] =
                {
                    SecondAxis,
                    ThirdAxis
                };

                GuizmoContext.BoundsAxis[0]                                 = Indices[EdgeIndex % 2];
                GuizmoContext.BoundsAxis[1]                                 = -1;
                GuizmoContext.BoundsLocalPivot                              = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
                GuizmoContext.BoundsLocalPivot[GuizmoContext.BoundsAxis[0]] = AABB[OppositeIndex][Indices[EdgeIndex % 2]];
                GuizmoContext.bUsingBounds                                  = true;
                GuizmoContext.EditingID                                     = GuizmoContext.GetCurrentID();
                GuizmoContext.BoundsMatrix                                  = GuizmoContext.ModelSource;
            }
        }

        if (GuizmoContext.bUsingBounds && (GuizmoContext.GetCurrentID() == GuizmoContext.EditingID))
        {
            Matrix4 ScaleMat;
            ScaleMat.SetIdentity();

            // Compute projected mouse position on plan
            const float Length = GuizmoContext.BoundsPlan.IntersectRay(
                Vector3(GuizmoContext.RayOrigin.X, GuizmoContext.RayOrigin.Y, GuizmoContext.RayOrigin.Z), 
                Vector3(GuizmoContext.RayVector.X, GuizmoContext.RayVector.Y, GuizmoContext.RayVector.Z));

            if (Length >= 0.0f)
            {
                Vector4 NewPos          = GuizmoContext.RayOrigin + GuizmoContext.RayVector * Length;
                Vector4 DeltaDiff       = NewPos - GuizmoContext.BoundsPivot; // Reference and delta vectors based on mouse move
                Vector4 RefDiff         = GuizmoContext.BoundsAnchor - GuizmoContext.BoundsPivot;
                Vector4 DeltaVector     = Vector4(Math::Abs(DeltaDiff.X), Math::Abs(DeltaDiff.Y), Math::Abs(DeltaDiff.Z), 0.0f);
                Vector4 ReferenceVector = Vector4(Math::Abs(RefDiff.X), Math::Abs(RefDiff.Y), Math::Abs(RefDiff.Z), 0.0f);

                // For 1 or 2 axes, compute a ratio that's used for Scale and snap it based on resulting length
                for (int32 BoundsAxisIndex = 0; BoundsAxisIndex < 2; BoundsAxisIndex++)
                {
                    int32 AxisIndex1 = GuizmoContext.BoundsAxis[BoundsAxisIndex];
                    if (AxisIndex1 == -1)
                    {
                        continue;
                    }

                    Vector4 AxisDir = Vector4(
                        GuizmoContext.BoundsMatrix.M[0][AxisIndex1],
                        GuizmoContext.BoundsMatrix.M[1][AxisIndex1],
                        GuizmoContext.BoundsMatrix.M[2][AxisIndex1],
                        0.0f);

                    AxisDir = Vector4(Math::Abs(AxisDir.X), Math::Abs(AxisDir.Y), Math::Abs(AxisDir.Z), AxisDir.W);

                    float DtAxis    = AxisDir.DotProduct(ReferenceVector);
                    float BoundSize = Bounds[AxisIndex1 + 3] - Bounds[AxisIndex1];    
                    float RatioAxis = 1.0f;

                    if (DtAxis > Math::Constants::Epsilon)
                    {
                        RatioAxis = AxisDir.DotProduct(DeltaVector) / DtAxis;
                    }

                    if (SnapValues)
                    {
                        float ScaledBoundSize = BoundSize * RatioAxis;
                        ComputeSnap(&ScaledBoundSize, SnapValues[AxisIndex1]);

                        if (BoundSize > Math::Constants::Epsilon)
                        {
                            RatioAxis = ScaledBoundSize / BoundSize;
                        }
                    }

                    ScaleMat.M[AxisIndex1][AxisIndex1] *= RatioAxis;

                    if (Math::Abs(RatioAxis - 1.0f) > Math::Constants::Epsilon)
                    {
                        bManipulated = true;
                    }
                }

                // Transform Matrix
                Matrix4 PreScale  = Matrix4::Translation(-GuizmoContext.BoundsLocalPivot.X, -GuizmoContext.BoundsLocalPivot.Y, -GuizmoContext.BoundsLocalPivot.Z);
                Matrix4 PostScale = Matrix4::Translation(GuizmoContext.BoundsLocalPivot.X, GuizmoContext.BoundsLocalPivot.Y, GuizmoContext.BoundsLocalPivot.Z);
                Matrix4 Result    = PreScale * ScaleMat * PostScale * GuizmoContext.BoundsMatrix;
                StoreMatrix(Matrix, Result);

                // Info text
                char TempString[512];
                Vector4 ModelPosition          = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
                ImVec2  DestinationPosOnScreen = WorldToPos(ModelPosition, GuizmoContext.ViewProjection);
                Vector3 BoundsMatrixRight      = Vector3(GuizmoContext.BoundsMatrix.M[0][0], GuizmoContext.BoundsMatrix.M[0][1], GuizmoContext.BoundsMatrix.M[0][2]);
                Vector3 BoundsMatrixUp         = Vector3(GuizmoContext.BoundsMatrix.M[1][0], GuizmoContext.BoundsMatrix.M[1][1], GuizmoContext.BoundsMatrix.M[1][2]);
                Vector3 BoundsMatrixDir        = Vector3(GuizmoContext.BoundsMatrix.M[2][0], GuizmoContext.BoundsMatrix.M[2][1], GuizmoContext.BoundsMatrix.M[2][2]);
                Vector3 ScaleMatRight          = Vector3(ScaleMat.M[0][0], ScaleMat.M[0][1], ScaleMat.M[0][2]);
                Vector3 ScaleMatUp             = Vector3(ScaleMat.M[1][0], ScaleMat.M[1][1], ScaleMat.M[1][2]);
                Vector3 ScaleMatDir            = Vector3(ScaleMat.M[2][0], ScaleMat.M[2][1], ScaleMat.M[2][2]);

                ImFormatString(TempString, sizeof(TempString), "X: %.2f Y: %.2f Z: %.2f",
                    (Bounds[3] - Bounds[0]) * BoundsMatrixRight.GetLength() * ScaleMatRight.GetLength(),
                    (Bounds[4] - Bounds[1]) * BoundsMatrixUp.GetLength() * ScaleMatUp.GetLength(),
                    (Bounds[5] - Bounds[2]) * BoundsMatrixDir.GetLength() * ScaleMatDir.GetLength());

                DrawList->AddText(ImVec2(DestinationPosOnScreen.x + 15, DestinationPosOnScreen.y + 15), GetColorU32(EditorGuizmo::EColor::TextShadow), TempString);
                DrawList->AddText(ImVec2(DestinationPosOnScreen.x + 14, DestinationPosOnScreen.y + 14), GetColorU32(EditorGuizmo::EColor::Text), TempString);
            }
        }

        if (!State.MouseDown[0])
        {
            GuizmoContext.bUsingBounds = false;
            GuizmoContext.EditingID    = static_cast<ImGuiID>(-1);
        }

        if (GuizmoContext.bUsingBounds)
        {
            break;
        }
    }

    return bManipulated;
}

static int32 GetScaleType(EditorGuizmo::EOperation::Type Op)
{
    if (GuizmoContext.bUsing)
    {
        return EMoveType::None;
    }

    ImGuiIO& State = ImGui::GetIO();

    // Screen
    int32 Type = EMoveType::None;
    if (State.MousePos.x >= GuizmoContext.ScreenSquareMin.x && State.MousePos.x <= GuizmoContext.ScreenSquareMax.x &&
        State.MousePos.y >= GuizmoContext.ScreenSquareMin.y && State.MousePos.y <= GuizmoContext.ScreenSquareMax.y &&
        Contains(Op, EditorGuizmo::EOperation::Scale))
    {
        Type = EMoveType::ScaleXYZ;
    }

    // Compute
    for (int32 AxisIndex = 0; AxisIndex < 3 && Type == EMoveType::None; AxisIndex++)
    {
        if (!Intersects(Op, static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::ScaleX << AxisIndex)))
        {
            continue;
        }

        bool bIsAxisMasked = ((1 << AxisIndex) & GuizmoContext.AxisMask) != 0;

        Vector4 DirPlaneX;
        Vector4 DirPlaneY;
        Vector4 DirAxis;

        bool bBelowAxisLimit;
        bool bBelowPlaneLimit;
        ComputeTripodAxisAndVisibility(AxisIndex, DirAxis, DirPlaneX, DirPlaneY, bBelowAxisLimit, bBelowPlaneLimit, true);

        DirAxis   = GuizmoContext.ModelLocal.Transform(Vector4(DirAxis.X, DirAxis.Y, DirAxis.Z, 0.0f));
        DirPlaneX = GuizmoContext.ModelLocal.Transform(Vector4(DirPlaneX.X, DirPlaneX.Y, DirPlaneX.Z, 0.0f));
        DirPlaneY = GuizmoContext.ModelLocal.Transform(Vector4(DirPlaneY.X, DirPlaneY.Y, DirPlaneY.Z, 0.0f));

        Vector4 ModelLocalPosition = Vector4(GuizmoContext.ModelLocal.GetTranslation(), 1.0f);

        const float Length = Plane::FromPointAndNormal(
            Vector3(ModelLocalPosition.X, ModelLocalPosition.Y, ModelLocalPosition.Z), 
            Vector3(DirAxis.X, DirAxis.Y, DirAxis.Z)).IntersectRay(
                Vector3(GuizmoContext.RayOrigin.X, GuizmoContext.RayOrigin.Y, GuizmoContext.RayOrigin.Z), 
                Vector3(GuizmoContext.RayVector.X, GuizmoContext.RayVector.Y, GuizmoContext.RayVector.Z));

        if (Length < 0.0f)
        {
            continue;
        }

        const float StartOffset = Contains(Op, static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::TranslateX << AxisIndex)) ? 1.0f : 0.1f;
        const float EndOffset   = Contains(Op, static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::TranslateX << AxisIndex)) ? 1.4f : 1.0f;

        Vector4      PositionOnPlane       = GuizmoContext.RayOrigin + GuizmoContext.RayVector * Length;
        const ImVec2 PositionOnPlaneScreen = WorldToPos(PositionOnPlane, GuizmoContext.ViewProjection);
        const ImVec2 AxisStartOnScreen     = WorldToPos(ModelLocalPosition + DirAxis * GuizmoContext.ScreenFactor * StartOffset, GuizmoContext.ViewProjection);
        const ImVec2 AxisEndOnScreen       = WorldToPos(ModelLocalPosition + DirAxis * GuizmoContext.ScreenFactor * EndOffset, GuizmoContext.ViewProjection);

        Vector4 ClosestPointOnAxis = PointOnSegment(
            Vector4(PositionOnPlaneScreen.x, PositionOnPlaneScreen.y, 0.0f, 0.0f), 
            Vector4(AxisStartOnScreen.x, AxisStartOnScreen.y, 0.0f, 0.0f), 
            Vector4(AxisEndOnScreen.x, AxisEndOnScreen.y, 0.0f, 0.0f));

        if ((ClosestPointOnAxis - Vector4(PositionOnPlaneScreen.x, PositionOnPlaneScreen.y, 0.0f, 0.0f)).GetLength() < 12.0f) // Pixel size
        {
            if (!bIsAxisMasked)
            {
                Type = EMoveType::ScaleX + AxisIndex;
            }
        }
    }

    // Universal

    Vector4 DeltaScreen = Vector4(State.MousePos.x - GuizmoContext.ScreenSquareCenter.x, State.MousePos.y - GuizmoContext.ScreenSquareCenter.y, 0.0f, 0.0f);

    float Distance = DeltaScreen.GetLength();
    if (Contains(Op, EditorGuizmo::EOperation::ScaleU) && Distance >= 17.0f && Distance < 23.0f)
    {
        Type = EMoveType::ScaleXYZ;
    }

    for (int32 AxisIndex = 0; AxisIndex < 3 && Type == EMoveType::None; AxisIndex++)
    {
        if (!Intersects(Op, static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::ScaleXU << AxisIndex)))
        {
            continue;
        }

        Vector4 DirPlaneX;
        Vector4 DirPlaneY;
        Vector4 DirAxis;

        bool bBelowAxisLimit;
        bool bBelowPlaneLimit;
        ComputeTripodAxisAndVisibility(AxisIndex, DirAxis, DirPlaneX, DirPlaneY, bBelowAxisLimit, bBelowPlaneLimit, true);

        // Draw axis
        if (bBelowAxisLimit)
        {
            bool   bHasTranslateOnAxis = Contains(Op, static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::TranslateX << AxisIndex));
            float  MarkerScale         = bHasTranslateOnAxis ? 1.4f : 1.0f;
            ImVec2 WorldDirSSpace      = WorldToPos((DirAxis * MarkerScale) * GuizmoContext.ScreenFactor, GuizmoContext.MVPLocal);
            float  DistanceToAxis      = Math::Sqrt(ImLengthSqr(WorldDirSSpace - State.MousePos));

            if (DistanceToAxis < 12.0f)
            {
                Type = EMoveType::ScaleX + AxisIndex;
            }
        }
    }

    return Type;
}

static int32 GetRotateType(EditorGuizmo::EOperation::Type Op)
{
    if (GuizmoContext.bUsing)
    {
        return EMoveType::None;
    }

    const bool bIsNoAxesMasked = (GuizmoContext.AxisMask == 0);

    ImGuiIO& State = ImGui::GetIO();
    int32 Type = EMoveType::None;

    Vector4 DeltaScreen = Vector4(State.MousePos.x - GuizmoContext.ScreenSquareCenter.x, State.MousePos.y - GuizmoContext.ScreenSquareCenter.y, 0.0f, 0.0f);

    float Distance = DeltaScreen.GetLength();
    if (Intersects(Op, EditorGuizmo::EOperation::RotateScreen) && bIsNoAxesMasked && Distance >= (GuizmoContext.RadiusSquareCenter - 4.0f) && 
        Distance < (GuizmoContext.RadiusSquareCenter + 4.0f))
    {
        Type = EMoveType::RotateScreen;
    }

    Vector4 ModelRight = Vector4(GuizmoContext.Model.M[0][0], GuizmoContext.Model.M[0][1], GuizmoContext.Model.M[0][2], 0.0f);
    Vector4 ModelUp    = Vector4(GuizmoContext.Model.M[1][0], GuizmoContext.Model.M[1][1], GuizmoContext.Model.M[1][2], 0.0f);
    Vector4 ModelDir   = Vector4(GuizmoContext.Model.M[2][0], GuizmoContext.Model.M[2][1], GuizmoContext.Model.M[2][2], 0.0f);

    const Vector4 PlanNormals[] =
    {
        ModelRight,
        ModelUp,
        ModelDir
    };

    Vector4 ModelPosition = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
    Vector4 ModelViewPos  = GuizmoContext.ViewMat.Transform(Vector4(ModelPosition.X, ModelPosition.Y, ModelPosition.Z, 1.0f));

    for (int32 AxisIndex = 0; AxisIndex < 3 && Type == EMoveType::None; AxisIndex++)
    {
        if (!Intersects(Op, static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::RotateX << AxisIndex)))
        {
            continue;
        }

        const bool bIsAxisMasked = ((1 << AxisIndex) & GuizmoContext.AxisMask) != 0;
        if (bIsAxisMasked)
        {
            continue;
        }

        // Pickup plan
        Vector4 ModelPositionRotate = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);

        const Plane PickupPlan = Plane::FromPointAndNormal(
            Vector3(ModelPositionRotate.X, ModelPositionRotate.Y, ModelPositionRotate.Z), 
            Vector3(PlanNormals[AxisIndex].X, PlanNormals[AxisIndex].Y, PlanNormals[AxisIndex].Z));

        const float Length = PickupPlan.IntersectRay(
            Vector3(GuizmoContext.RayOrigin.X, GuizmoContext.RayOrigin.Y, GuizmoContext.RayOrigin.Z), 
            Vector3(GuizmoContext.RayVector.X, GuizmoContext.RayVector.Y, GuizmoContext.RayVector.Z));

        if (Length < 0.0f)
        {
            continue;
        }

        const Vector4 IntersectWorldPos = GuizmoContext.RayOrigin + GuizmoContext.RayVector * Length;
        Vector4 IntersectViewPos = GuizmoContext.ViewMat.Transform(Vector4(IntersectWorldPos.X, IntersectWorldPos.Y, IntersectWorldPos.Z, 1.0f));

        if (Math::Abs(ModelViewPos.Z) - Math::Abs(IntersectViewPos.Z) < -Math::Constants::Epsilon)
        {
            continue;
        }

        const Vector4 LocalPosition = IntersectWorldPos - ModelPositionRotate;
        Vector4 IdealPosOnCircle = LocalPosition.GetNormalized();
        IdealPosOnCircle = GuizmoContext.ModelInverse.Transform(Vector4(IdealPosOnCircle.X, IdealPosOnCircle.Y, IdealPosOnCircle.Z, 0.0f));

        const ImVec2 IdealPosOnCircleScreen = WorldToPos(IdealPosOnCircle * RotationDisplayFactor * GuizmoContext.ScreenFactor, GuizmoContext.MVP);
        const ImVec2 DistanceOnScreen       = IdealPosOnCircleScreen - State.MousePos;

        const float DistanceToIdealPos = Vector4(DistanceOnScreen.x, DistanceOnScreen.y, 0.0f, 0.0f).GetLength();
        if (DistanceToIdealPos < 8.0f) // Pixel size
        {
            Type = EMoveType::RotateX + AxisIndex;
        }
    }

    return Type;
}

static int32 GetMoveType(EditorGuizmo::EOperation::Type Op, Vector4* GizmoHitProportion)
{
    if (!Intersects(Op, EditorGuizmo::EOperation::Translate) || GuizmoContext.bUsing || !GuizmoContext.bMouseOver)
    {
        return EMoveType::None;
    }

    bool bIsNoAxesMasked       = !GuizmoContext.AxisMask;
    bool bIsMultipleAxesMasked = (GuizmoContext.AxisMask & (GuizmoContext.AxisMask - 1)) != 0;

    ImGuiIO& State = ImGui::GetIO();

    // Screen
    int32 Type = EMoveType::None;
    if (State.MousePos.x >= GuizmoContext.ScreenSquareMin.x && State.MousePos.x <= GuizmoContext.ScreenSquareMax.x &&
        State.MousePos.y >= GuizmoContext.ScreenSquareMin.y && State.MousePos.y <= GuizmoContext.ScreenSquareMax.y && 
        Contains(Op, EditorGuizmo::EOperation::Translate))
    {
        Type = EMoveType::MoveScreen;
    }

    // Compute
    const Vector4 ScreenCoord = Vector4(State.MousePos.x - GuizmoContext.X, State.MousePos.y - GuizmoContext.Y, 0.0f, 0.0f);
    for (int32 AxisIndex = 0; AxisIndex < 3 && Type == EMoveType::None; AxisIndex++)
    {
        bool bIsAxisMasked = ((1 << AxisIndex) & GuizmoContext.AxisMask) != 0;

        Vector4 DirPlaneX;
        Vector4 DirPlaneY;
        Vector4 DirAxis;

        bool bBelowAxisLimit;
        bool bBelowPlaneLimit;
        ComputeTripodAxisAndVisibility(AxisIndex, DirAxis, DirPlaneX, DirPlaneY, bBelowAxisLimit, bBelowPlaneLimit);

        DirAxis   = GuizmoContext.Model.Transform(Vector4(DirAxis.X, DirAxis.Y, DirAxis.Z, 0.0f));
        DirPlaneX = GuizmoContext.Model.Transform(Vector4(DirPlaneX.X, DirPlaneX.Y, DirPlaneX.Z, 0.0f));
        DirPlaneY = GuizmoContext.Model.Transform(Vector4(DirPlaneY.X, DirPlaneY.Y, DirPlaneY.Z, 0.0f));

        Vector4 ModelPosition = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);

        const float Length = Plane::FromPointAndNormal(
            Vector3(ModelPosition.X, ModelPosition.Y, ModelPosition.Z),
            Vector3(DirAxis.X, DirAxis.Y, DirAxis.Z)).IntersectRay(
                Vector3(GuizmoContext.RayOrigin.X, GuizmoContext.RayOrigin.Y, GuizmoContext.RayOrigin.Z),
                Vector3(GuizmoContext.RayVector.X, GuizmoContext.RayVector.Y, GuizmoContext.RayVector.Z));

        if (Length < 0.0f)
        {
            continue;
        }

        Vector4 PositionOnPlane = GuizmoContext.RayOrigin + GuizmoContext.RayVector * Length;

        const ImVec2 AxisStartOnScreen = WorldToPos(
            ModelPosition + DirAxis * GuizmoContext.ScreenFactor * 0.1f,
            GuizmoContext.ViewProjection) - ImVec2(GuizmoContext.X, GuizmoContext.Y);
        const ImVec2 AxisEndOnScreen = WorldToPos(
            ModelPosition + DirAxis * GuizmoContext.ScreenFactor,
            GuizmoContext.ViewProjection) - ImVec2(GuizmoContext.X, GuizmoContext.Y);

        Vector4 ClosestPointOnAxis = PointOnSegment(ScreenCoord,
            Vector4(AxisStartOnScreen.x, AxisStartOnScreen.y, 0.0f, 0.0f),
            Vector4(AxisEndOnScreen.x, AxisEndOnScreen.y, 0.0f, 0.0f));

        if ((ClosestPointOnAxis - ScreenCoord).GetLength() < 12.0f && 
            Intersects(Op, static_cast<EditorGuizmo::EOperation::Type>(EditorGuizmo::EOperation::TranslateX << AxisIndex))) // Pixel size
        {
            if (bIsAxisMasked)
            {
                break;
            }

            Type = EMoveType::MoveX + AxisIndex;
        }

        Vector4 ModelPosition2 = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);

        const float PlaneCoordX = DirPlaneX.DotProduct((PositionOnPlane - ModelPosition2) * (1.0f / GuizmoContext.ScreenFactor));
        const float PlaneCoordY = DirPlaneY.DotProduct((PositionOnPlane - ModelPosition2) * (1.0f / GuizmoContext.ScreenFactor));

        if (bBelowPlaneLimit && PlaneCoordX >= QuadUV[0] && PlaneCoordX <= QuadUV[4] && PlaneCoordY >= QuadUV[1] && 
            PlaneCoordY <= QuadUV[3] && Contains(Op, TRANSLATE_PLANS[AxisIndex]))
        {
            if ((!bIsAxisMasked || bIsMultipleAxesMasked) && !bIsNoAxesMasked)
            {
                break;
            }

            Type = EMoveType::MoveYZ + AxisIndex;
        }

        if (GizmoHitProportion)
        {
            *GizmoHitProportion = Vector4(PlaneCoordX, PlaneCoordY, 0.0f, 0.0f);
        }
    }

    return Type;
}

static bool HandleTranslation(float* Matrix, float* DeltaMatrix, EditorGuizmo::EOperation::Type Op, int32& Type, const float* Snap)
{
    if (!Intersects(Op, EditorGuizmo::EOperation::Translate) || Type != EMoveType::None)
    {
        return false;
    }

    const ImGuiIO& State = ImGui::GetIO();
    const bool bApplyRotationLocally = GuizmoContext.Mode == EditorGuizmo::EMode::Local || Type == EMoveType::MoveScreen;

    bool bModified = false;

    // Move
    if (GuizmoContext.bUsing && (GuizmoContext.GetCurrentID() == GuizmoContext.EditingID) && IsTranslateType(GuizmoContext.CurrentOperation))
    {
        ImGui::SetNextFrameWantCaptureMouse(true);

        const float SignedLength = GuizmoContext.TranslationPlan.IntersectRay(
            Vector3(GuizmoContext.RayOrigin.X, GuizmoContext.RayOrigin.Y, GuizmoContext.RayOrigin.Z), 
            Vector3(GuizmoContext.RayVector.X, GuizmoContext.RayVector.Y, GuizmoContext.RayVector.Z));

        if (SignedLength >= 0.0f)
        {
            const float Length = SignedLength;
            const Vector4 NewPos = GuizmoContext.RayOrigin + GuizmoContext.RayVector * Length;
            const Vector4 NewOrigin = NewPos - GuizmoContext.RelativeOrigin * GuizmoContext.ScreenFactor; // Compute delta

            const Vector4 ModelPosition = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
            Vector4 Delta = NewOrigin - ModelPosition;

            // 1 axis constraint 
            if (GuizmoContext.CurrentOperation >= EMoveType::MoveX && GuizmoContext.CurrentOperation <= EMoveType::MoveZ) 
            { 
                const int32 AxisIndex = GuizmoContext.CurrentOperation - EMoveType::MoveX; 
                const Vector4 AxisValue = Vector4(GuizmoContext.Model.M[AxisIndex][0], GuizmoContext.Model.M[AxisIndex][1], GuizmoContext.Model.M[AxisIndex][2], 0.0f); 
 
                const float LengthOnAxis = AxisValue.DotProduct(Delta); 
                Delta = AxisValue * LengthOnAxis; 
            } 

            // Snap
            if (Snap)
            {
                const Vector4 ModelPositionSnap = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
                Vector4 CumulativeDelta = ModelPositionSnap + Delta - GuizmoContext.MatrixOrigin;

                if (bApplyRotationLocally)
                {
                    Matrix4 ModelSourceNormalized = GuizmoContext.ModelSource;
                    ModelSourceNormalized.OrthoNormalize();

                    const Matrix4 ModelSourceNormalizedInverse = ModelSourceNormalized.GetInverse();
                    CumulativeDelta = ModelSourceNormalizedInverse.Transform(Vector4(CumulativeDelta.X, CumulativeDelta.Y, CumulativeDelta.Z, 0.0f));

                    ComputeSnap(CumulativeDelta, Snap);
                    CumulativeDelta = ModelSourceNormalized.Transform(Vector4(CumulativeDelta.X, CumulativeDelta.Y, CumulativeDelta.Z, 0.0f));
                }
                else
                {
                    ComputeSnap(CumulativeDelta, Snap);
                }

                const Vector4 UpdatedModelPosition = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
                Delta = GuizmoContext.MatrixOrigin + CumulativeDelta - UpdatedModelPosition;
            }

            if (Delta != GuizmoContext.TranslationLastDelta)
            {
                bModified = true;
            }

            GuizmoContext.TranslationLastDelta = Delta;

            // Compute Matrix & delta
            const Matrix4 DeltaMatrixTranslation = Matrix4::Translation(Delta.X, Delta.Y, Delta.Z);
            if (DeltaMatrix)
            {
                Memory::Memcpy(DeltaMatrix, &DeltaMatrixTranslation.M[0][0], sizeof(float) * 16);
            }

            const Matrix4 Result = GuizmoContext.ModelSource * DeltaMatrixTranslation;
            StoreMatrix(Matrix, Result);

            if (!State.MouseDown[0])
            {
                GuizmoContext.bUsing = false;
            }

            Type = GuizmoContext.CurrentOperation;
        }
    }
    else
    {
        // Find new possible way to move
        Type = GuizmoContext.bOverGizmoHotspot ? EMoveType::None : GetMoveType(Op, nullptr);
        GuizmoContext.bOverGizmoHotspot |= Type != EMoveType::None;

        if (Type != EMoveType::None)
        {
            ImGui::SetNextFrameWantCaptureMouse(true);
        }

        if (CanActivate() && Type != EMoveType::None)
        {
            Vector4 ModelRight = Vector4(GuizmoContext.Model.M[0][0], GuizmoContext.Model.M[0][1], GuizmoContext.Model.M[0][2], 0.0f);
            Vector4 ModelUp    = Vector4(GuizmoContext.Model.M[1][0], GuizmoContext.Model.M[1][1], GuizmoContext.Model.M[1][2], 0.0f);
            Vector4 ModelDir   = Vector4(GuizmoContext.Model.M[2][0], GuizmoContext.Model.M[2][1], GuizmoContext.Model.M[2][2], 0.0f);

            Vector4 MovePlanNormal[] =
            {
                ModelRight,
                ModelUp,
                ModelDir,
                ModelRight,
                ModelUp,
                ModelDir,
                -GuizmoContext.CameraDir
            };

            Vector4 ModelPosition           = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
            Vector4 CameraToModelNormalized = (ModelPosition - GuizmoContext.CameraEye).GetNormalized();

            for (uint32 AxisIndex = 0; AxisIndex < 3; AxisIndex++)
            {
                Vector4 OrthoVector      = MovePlanNormal[AxisIndex].CrossProduct(CameraToModelNormalized);
                MovePlanNormal[AxisIndex] = MovePlanNormal[AxisIndex].CrossProduct(OrthoVector);
                MovePlanNormal[AxisIndex].Normalize();
            }

            GuizmoContext.TranslationPlan = Plane::FromPointAndNormal(
                Vector3(ModelPosition.X, ModelPosition.Y, ModelPosition.Z),
                Vector3(MovePlanNormal[Type - EMoveType::MoveX].X, MovePlanNormal[Type - EMoveType::MoveX].Y, MovePlanNormal[Type - EMoveType::MoveX].Z));

            const float Length = GuizmoContext.TranslationPlan.IntersectRay(
                Vector3(GuizmoContext.RayOrigin.X, GuizmoContext.RayOrigin.Y, GuizmoContext.RayOrigin.Z),
                Vector3(GuizmoContext.RayVector.X, GuizmoContext.RayVector.Y, GuizmoContext.RayVector.Z));

            if (Length >= 0.0f)
            {
                GuizmoContext.bUsing                = true;
                GuizmoContext.EditingID             = GuizmoContext.GetCurrentID();
                GuizmoContext.CurrentOperation      = Type;
                GuizmoContext.TranslationPlanOrigin = GuizmoContext.RayOrigin + GuizmoContext.RayVector * Length;
                GuizmoContext.MatrixOrigin          = ModelPosition;
                GuizmoContext.RelativeOrigin        = (GuizmoContext.TranslationPlanOrigin - ModelPosition) * (1.0f / GuizmoContext.ScreenFactor);
            }
        }
    }

    return bModified;
}

static bool HandleScale(float* Matrix, float* DeltaMatrix, EditorGuizmo::EOperation::Type Op, int32& Type, const float* Snap)
{
    if ((!Intersects(Op, EditorGuizmo::EOperation::Scale) && !Intersects(Op, EditorGuizmo::EOperation::ScaleU)) || Type != EMoveType::None || !GuizmoContext.bMouseOver)
    {
        return false;
    }

    ImGuiIO& State = ImGui::GetIO();

    bool bModified = false;
    if (!GuizmoContext.bUsing)
    {
        // Find new possible way to Scale
        Type = GuizmoContext.bOverGizmoHotspot ? EMoveType::None : GetScaleType(Op);
        GuizmoContext.bOverGizmoHotspot |= Type != EMoveType::None;

        if (Type != EMoveType::None)
        {
            ImGui::SetNextFrameWantCaptureMouse(true);
        }

        if (CanActivate() && Type != EMoveType::None)
        {
            Vector4 ModelLocalUp    = Vector4(GuizmoContext.ModelLocal.M[1][0], GuizmoContext.ModelLocal.M[1][1], GuizmoContext.ModelLocal.M[1][2], 0.0f);
            Vector4 ModelLocalDir   = Vector4(GuizmoContext.ModelLocal.M[2][0], GuizmoContext.ModelLocal.M[2][1], GuizmoContext.ModelLocal.M[2][2], 0.0f);
            Vector4 ModelLocalRight = Vector4(GuizmoContext.ModelLocal.M[0][0], GuizmoContext.ModelLocal.M[0][1], GuizmoContext.ModelLocal.M[0][2], 0.0f);
                
            const Vector4 MovePlanNormal[] =
            {
                ModelLocalUp, 
                ModelLocalDir, 
                ModelLocalRight, 
                ModelLocalDir, 
                ModelLocalUp, 
                ModelLocalRight,
                -GuizmoContext.CameraDir
            };
            
            Vector4 ModelLocalPosition = Vector4(GuizmoContext.ModelLocal.GetTranslation(), 1.0f);
            GuizmoContext.TranslationPlan = Plane::FromPointAndNormal(
                Vector3(ModelLocalPosition.X, ModelLocalPosition.Y, ModelLocalPosition.Z), 
                Vector3(MovePlanNormal[Type - EMoveType::ScaleX].X, MovePlanNormal[Type - EMoveType::ScaleX].Y, MovePlanNormal[Type - EMoveType::ScaleX].Z));

            const float Length = GuizmoContext.TranslationPlan.IntersectRay(
                Vector3(GuizmoContext.RayOrigin.X, GuizmoContext.RayOrigin.Y, GuizmoContext.RayOrigin.Z), 
                Vector3(GuizmoContext.RayVector.X, GuizmoContext.RayVector.Y, GuizmoContext.RayVector.Z));

            if (Length >= 0.0f)
            {
                GuizmoContext.bUsing                = true;
                GuizmoContext.EditingID             = GuizmoContext.GetCurrentID();
                GuizmoContext.CurrentOperation      = Type;
                GuizmoContext.TranslationPlanOrigin = GuizmoContext.RayOrigin + GuizmoContext.RayVector * Length;
                GuizmoContext.MatrixOrigin          = ModelLocalPosition;
                GuizmoContext.Scale                 = Vector4(1.0f, 1.0f, 1.0f, 0.0f);
                GuizmoContext.RelativeOrigin        = (GuizmoContext.TranslationPlanOrigin - ModelLocalPosition) * (1.0f / GuizmoContext.ScreenFactor);

                Vector3 ModelSourceRight = Vector3(GuizmoContext.ModelSource.M[0][0], GuizmoContext.ModelSource.M[0][1], GuizmoContext.ModelSource.M[0][2]);
                Vector3 ModelSourceUp    = Vector3(GuizmoContext.ModelSource.M[1][0], GuizmoContext.ModelSource.M[1][1], GuizmoContext.ModelSource.M[1][2]);
                Vector3 ModelSourceDir   = Vector3(GuizmoContext.ModelSource.M[2][0], GuizmoContext.ModelSource.M[2][1], GuizmoContext.ModelSource.M[2][2]);

                GuizmoContext.ScaleValueOrigin = Vector4(ModelSourceRight.GetLength(), ModelSourceUp.GetLength(), ModelSourceDir.GetLength(), 0.0f);
                GuizmoContext.SaveMousePosX    = State.MousePos.x;
            }
        }
    }

    // Scale
    if (GuizmoContext.bUsing && (GuizmoContext.GetCurrentID() == GuizmoContext.EditingID) && IsScaleType(GuizmoContext.CurrentOperation))
    {
        ImGui::SetNextFrameWantCaptureMouse(true);

        const float Length = GuizmoContext.TranslationPlan.IntersectRay(
            Vector3(GuizmoContext.RayOrigin.X, GuizmoContext.RayOrigin.Y, GuizmoContext.RayOrigin.Z),
            Vector3(GuizmoContext.RayVector.X, GuizmoContext.RayVector.Y, GuizmoContext.RayVector.Z));

        if (Length >= 0.0f)
        {
            const Vector4 NewPos = GuizmoContext.RayOrigin + GuizmoContext.RayVector * Length;
            const Vector4 NewOrigin = NewPos - GuizmoContext.RelativeOrigin * GuizmoContext.ScreenFactor;
            const Vector4 ModelLocalPosition = Vector4(GuizmoContext.ModelLocal.GetTranslation(), 1.0f);
            Vector4 Delta = NewOrigin - ModelLocalPosition;

            // 1 axis constraint
            if (GuizmoContext.CurrentOperation >= EMoveType::ScaleX && GuizmoContext.CurrentOperation <= EMoveType::ScaleZ) 
            { 
                const int32 AxisIndex = GuizmoContext.CurrentOperation - EMoveType::ScaleX; 
 
                const Vector4 AxisValue = Vector4( 
                    GuizmoContext.ModelLocal.M[AxisIndex][0], 
                    GuizmoContext.ModelLocal.M[AxisIndex][1], 
                    GuizmoContext.ModelLocal.M[AxisIndex][2], 
                    0.0f); 
 
                const float LengthOnAxis = AxisValue.DotProduct(Delta); 
                Delta = AxisValue * LengthOnAxis; 

                const Vector4 ModelLocalPositionScale = Vector4(GuizmoContext.ModelLocal.GetTranslation(), 1.0f);
                const Vector4 BaseVector = GuizmoContext.TranslationPlanOrigin - ModelLocalPositionScale;

                const float Ratio = AxisValue.DotProduct(BaseVector + Delta) / AxisValue.DotProduct(BaseVector);
                GuizmoContext.Scale[AxisIndex] = Math::Max(Ratio, 0.001f);
            }
            else
            {
                const float ScaleDelta = (State.MousePos.x - GuizmoContext.SaveMousePosX) * 0.01f;
                const float ScaleValue = Math::Max(1.0f + ScaleDelta, 0.001f);
                GuizmoContext.Scale = Vector4(ScaleValue, ScaleValue, ScaleValue, 0.0f);
            }

            // Snap
            if (Snap)
            {
                float ScaleSnap[] =
                {
                    Snap[0],
                    Snap[0],
                    Snap[0]
                };

                ComputeSnap(GuizmoContext.Scale, ScaleSnap);
            }

            // No 0 allowed
            for (int32 AxisIndex = 0; AxisIndex < 3; AxisIndex++)
            {
                GuizmoContext.Scale[AxisIndex] = Math::Max(GuizmoContext.Scale[AxisIndex], 0.001f);
            }

            if (GuizmoContext.ScaleLast != GuizmoContext.Scale)
            {
                bModified = true;
            }

            GuizmoContext.ScaleLast = GuizmoContext.Scale;

            // Compute Matrix & delta
            const Vector4 ScaleVec   = GuizmoContext.Scale * GuizmoContext.ScaleValueOrigin;
            Matrix4 DeltaMatrixScale = Matrix4::Scale(ScaleVec.X, ScaleVec.Y, ScaleVec.Z);

            const Matrix4 Result = DeltaMatrixScale * GuizmoContext.ModelLocal;
            StoreMatrix(Matrix, Result);

            if (DeltaMatrix)
            {
                Vector4 DeltaScale = GuizmoContext.Scale * GuizmoContext.ScaleValueOrigin;

                Vector4 OriginalScaleDivider;
                OriginalScaleDivider.X = 1.0f / GuizmoContext.ModelScaleOrigin.X;
                OriginalScaleDivider.Y = 1.0f / GuizmoContext.ModelScaleOrigin.Y;
                OriginalScaleDivider.Z = 1.0f / GuizmoContext.ModelScaleOrigin.Z;

                DeltaScale = DeltaScale * OriginalScaleDivider;

                DeltaMatrixScale = Matrix4::Scale(DeltaScale.X, DeltaScale.Y, DeltaScale.Z);
                Memory::Memcpy(DeltaMatrix, &DeltaMatrixScale.M[0][0], sizeof(float) * 16);
            }

            if (!State.MouseDown[0])
            {
                GuizmoContext.bUsing = false;
                GuizmoContext.Scale = Vector4(1.0f, 1.0f, 1.0f, 0.0f);
            }

            Type = GuizmoContext.CurrentOperation;
        }
    }

    return bModified;
}

static bool HandleRotation(float* Matrix, float* DeltaMatrix, EditorGuizmo::EOperation::Type Op, int32& Type, const float* Snap)
{
    if (!Intersects(Op, EditorGuizmo::EOperation::Rotate) || Type != EMoveType::None || !GuizmoContext.bMouseOver)
    {
        return false;
    }

    ImGuiIO& State = ImGui::GetIO();

    bool bModified             = false;
    bool bApplyRotationLocally = GuizmoContext.Mode == EditorGuizmo::EMode::Local;

    if (!GuizmoContext.bUsing)
    {
        Type = GuizmoContext.bOverGizmoHotspot ? EMoveType::None : GetRotateType(Op);
        GuizmoContext.bOverGizmoHotspot |= Type != EMoveType::None;

        if (Type != EMoveType::None)
        {
            ImGui::SetNextFrameWantCaptureMouse(true);
        }

        if (Type == EMoveType::RotateScreen)
        {
            bApplyRotationLocally = true;
        }

        if (CanActivate() && Type != EMoveType::None)
        {
            Vector4 ModelRight = Vector4(GuizmoContext.Model.M[0][0], GuizmoContext.Model.M[0][1], GuizmoContext.Model.M[0][2], 0.0f);
            Vector4 ModelUp    = Vector4(GuizmoContext.Model.M[1][0], GuizmoContext.Model.M[1][1], GuizmoContext.Model.M[1][2], 0.0f);
            Vector4 ModelDir   = Vector4(GuizmoContext.Model.M[2][0], GuizmoContext.Model.M[2][1], GuizmoContext.Model.M[2][2], 0.0f);

            const Vector4 RotatePlanNormal[] =
            {
                ModelRight,
                ModelUp,
                ModelDir,
                -GuizmoContext.CameraDir
            };

            if (bApplyRotationLocally)
            {
                Vector4 ModelPositionRot     = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
                GuizmoContext.TranslationPlan = Plane::FromPointAndNormal(
                    Vector3(ModelPositionRot.X, ModelPositionRot.Y, ModelPositionRot.Z), 
                    Vector3(RotatePlanNormal[Type - EMoveType::RotateX].X, RotatePlanNormal[Type - EMoveType::RotateX].Y, RotatePlanNormal[Type - EMoveType::RotateX].Z));
            }
            else
            {
                Vector4 ModelSourcePos       = Vector4(GuizmoContext.ModelSource.GetTranslation(), 1.0f);
                GuizmoContext.TranslationPlan = Plane::FromPointAndNormal(
                    Vector3(ModelSourcePos.X, ModelSourcePos.Y, ModelSourcePos.Z), 
                    Vector3(DirectionUnary[Type - EMoveType::RotateX].X, DirectionUnary[Type - EMoveType::RotateX].Y, DirectionUnary[Type - EMoveType::RotateX].Z));
            }

            const float Length = GuizmoContext.TranslationPlan.IntersectRay(
                Vector3(GuizmoContext.RayOrigin.X, GuizmoContext.RayOrigin.Y, GuizmoContext.RayOrigin.Z),
                Vector3(GuizmoContext.RayVector.X, GuizmoContext.RayVector.Y, GuizmoContext.RayVector.Z));

            if (Length >= 0.0f)
            {
                GuizmoContext.bUsing           = true;
                GuizmoContext.EditingID        = GuizmoContext.GetCurrentID();
                GuizmoContext.CurrentOperation = Type;

                Vector4 ModelPositionRot2 = Vector4(GuizmoContext.Model.GetTranslation(), 1.0f);
                Vector4 LocalPosition     = GuizmoContext.RayOrigin + GuizmoContext.RayVector * Length - ModelPositionRot2;

                GuizmoContext.RotationVectorSource = LocalPosition.GetNormalized();
                GuizmoContext.RotationAngleOrigin = ComputeAngleOnPlan();
            }
        }
    }

    // Rotation
    if (GuizmoContext.bUsing && (GuizmoContext.GetCurrentID() == GuizmoContext.EditingID) && IsRotateType(GuizmoContext.CurrentOperation))
    {
        ImGui::SetNextFrameWantCaptureMouse(true);

        GuizmoContext.RotationAngle = ComputeAngleOnPlan();
        if (Snap) 
        { 
            float SnapInRadian = Snap[0] * Math::Constants::Deg2Rad; 
            ComputeSnap(&GuizmoContext.RotationAngle, SnapInRadian); 
        } 
 
        float RotationAngleDelta = GuizmoContext.RotationAngle - GuizmoContext.RotationAngleOrigin; 
        if (RotationAngleDelta > Math::Constants::PI) 
        { 
            RotationAngleDelta -= Math::Constants::TwoPI; 
        } 
        else if (RotationAngleDelta < -Math::Constants::PI) 
        { 
            RotationAngleDelta += Math::Constants::TwoPI; 
        } 
 
        Vector4 RotationAxisLocalSpace = GuizmoContext.ModelInverse.Transform( 
            Vector4(GuizmoContext.TranslationPlan.X, GuizmoContext.TranslationPlan.Y, GuizmoContext.TranslationPlan.Z, 0.0f)); 
        RotationAxisLocalSpace.Normalize(); 
 
        Matrix4 DeltaRotation; 
        DeltaRotation = Quaternion::FromAxisAngle( 
            Vector3(RotationAxisLocalSpace.X, RotationAxisLocalSpace.Y, RotationAxisLocalSpace.Z),  
            RotationAngleDelta).ToMatrix4(); 
         
        if (Math::Abs(RotationAngleDelta) > Math::Constants::Epsilon) 
        { 
            bModified = true; 
        } 
 
        GuizmoContext.RotationAngleOrigin = GuizmoContext.RotationAngle; 

        Matrix4 ScaleOrigin = Matrix4::Scale(GuizmoContext.ModelScaleOrigin.X, GuizmoContext.ModelScaleOrigin.Y, GuizmoContext.ModelScaleOrigin.Z);
        if (bApplyRotationLocally)
        {
            StoreMatrix(Matrix, ScaleOrigin * DeltaRotation * GuizmoContext.ModelLocal);
        }
        else
        {
            Matrix4 Result = GuizmoContext.ModelSource;
            Result.M[3][0] = 0.0f;
            Result.M[3][1] = 0.0f;
            Result.M[3][2] = 0.0f;
            Result.M[3][3] = 1.0f;

            Matrix4 ResultMatrix   = Result * DeltaRotation;
            Vector4 ModelSourcePos = Vector4(GuizmoContext.ModelSource.GetTranslation(), 1.0f);
            ResultMatrix.SetTranslation(Vector3(ModelSourcePos.X, ModelSourcePos.Y, ModelSourcePos.Z));
            StoreMatrix(Matrix, ResultMatrix);
        }

        if (DeltaMatrix)
        {
            Matrix4 DeltaRotMatrix = GuizmoContext.ModelInverse * DeltaRotation * GuizmoContext.Model;
            Memory::Memcpy(DeltaMatrix, &DeltaRotMatrix.M[0][0], sizeof(float) * 16);
        }

        if (!State.MouseDown[0])
        {
            GuizmoContext.bUsing    = false;
            GuizmoContext.EditingID = static_cast<ImGuiID>(-1);
        }

        Type = GuizmoContext.CurrentOperation;
    }

    return bModified;
}

static void ComputeFrustumPlanes(Plane* Frustum, const float* Clip)
{
    Frustum[0] = Plane(Clip[3] - Clip[0], Clip[7] - Clip[4], Clip[11] - Clip[8], Clip[15] - Clip[12]);
    Frustum[1] = Plane(Clip[3] + Clip[0], Clip[7] + Clip[4], Clip[11] + Clip[8], Clip[15] + Clip[12]);
    Frustum[2] = Plane(Clip[3] + Clip[1], Clip[7] + Clip[5], Clip[11] + Clip[9], Clip[15] + Clip[13]);
    Frustum[3] = Plane(Clip[3] - Clip[1], Clip[7] - Clip[5], Clip[11] - Clip[9], Clip[15] - Clip[13]);
    Frustum[4] = Plane(Clip[3] - Clip[2], Clip[7] - Clip[6], Clip[11] - Clip[10], Clip[15] - Clip[14]);
    Frustum[5] = Plane(Clip[3] + Clip[2], Clip[7] + Clip[6], Clip[11] + Clip[10], Clip[15] + Clip[14]);

    for (int32 PlaneIndex = 0; PlaneIndex < 6; PlaneIndex++)
    {
        Frustum[PlaneIndex].Normalize();
    }
}

// Constructors and remaining EditorGuizmo functions
EditorGuizmo::Style::Style() 
{ 
    // Default values 
    TranslationLineThickness   = 3.0f; 
    TranslationLineArrowSize   = 6.0f; 
    RotationLineThickness      = 2.0f; 
    RotationOuterLineThickness = 3.0f; 
    ScaleLineThickness         = 3.0f; 
    ScaleLineCircleSize        = 6.0f; 
    ScaleHandleShape           = EditorGuizmo::EScaleHandleShape::Square;
    HatchedAxisLineThickness   = 6.0f; 
    CenterCircleSize           = 6.0f; 

    // Initialize default colors
    Colors[EditorGuizmo::EColor::DirectionX]          = ImVec4(0.666f, 0.000f, 0.000f, 1.000f);
    Colors[EditorGuizmo::EColor::DirectionY]          = ImVec4(0.000f, 0.666f, 0.000f, 1.000f);
    Colors[EditorGuizmo::EColor::DirectionZ]          = ImVec4(0.000f, 0.000f, 0.666f, 1.000f);
    Colors[EditorGuizmo::EColor::PlaneX]              = ImVec4(0.666f, 0.000f, 0.000f, 0.380f);
    Colors[EditorGuizmo::EColor::PlaneY]              = ImVec4(0.000f, 0.666f, 0.000f, 0.380f);
    Colors[EditorGuizmo::EColor::PlaneZ]              = ImVec4(0.000f, 0.000f, 0.666f, 0.380f);
    Colors[EditorGuizmo::EColor::Selection]           = ImVec4(1.000f, 0.500f, 0.062f, 0.541f);
    Colors[EditorGuizmo::EColor::Inactive]            = ImVec4(0.600f, 0.600f, 0.600f, 0.600f);
    Colors[EditorGuizmo::EColor::TranslationLine]     = ImVec4(0.666f, 0.666f, 0.666f, 0.666f);
    Colors[EditorGuizmo::EColor::ScaleLine]           = ImVec4(0.250f, 0.250f, 0.250f, 1.000f);
    Colors[EditorGuizmo::EColor::RotationUsingBorder] = ImVec4(1.000f, 0.500f, 0.062f, 1.000f);
    Colors[EditorGuizmo::EColor::RotationUsingFill]   = ImVec4(1.000f, 0.500f, 0.062f, 0.500f);
    Colors[EditorGuizmo::EColor::HatchedAxisLines]    = ImVec4(0.000f, 0.000f, 0.000f, 0.500f);
    Colors[EditorGuizmo::EColor::Text]                = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    Colors[EditorGuizmo::EColor::TextShadow]          = ImVec4(0.000f, 0.000f, 0.000f, 1.000f);
}

void EditorGuizmo::SetDrawlist(ImDrawList* DrawList)
{
    GuizmoContext.DrawList = DrawList ? DrawList : ImGui::GetWindowDrawList();
}

void EditorGuizmo::BeginFrame()
{
    const ImGuiWindowFlags Flags =
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoInputs | 
        ImGuiWindowFlags_NoSavedSettings | 
        ImGuiWindowFlags_NoFocusOnAppearing | 
        ImGuiWindowFlags_NoBringToFrontOnFocus;

#ifdef IMGUI_HAS_VIEWPORT
    ImGuiViewport* TargetViewport = ImGui::GetMainViewport();
    if (GuizmoContext.AlternativeWindow != nullptr && GuizmoContext.AlternativeWindow->Viewport != nullptr)
    {
        TargetViewport = GuizmoContext.AlternativeWindow->Viewport;
    }

    ImGui::SetNextWindowViewport(TargetViewport->ID);
    ImGui::SetNextWindowSize(TargetViewport->Size);
    ImGui::SetNextWindowPos(TargetViewport->Pos);
#else
    ImGuiIO& State = ImGui::GetIO();
    ImGui::SetNextWindowSize(State.DisplaySize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
#endif

    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImGui::Begin("gizmo", nullptr, Flags);
    
    GuizmoContext.DrawList          = ImGui::GetWindowDrawList();
    GuizmoContext.bOverGizmoHotspot = false;

    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void EditorGuizmo::SetImGuiContext(ImGuiContext* Context)
{
    ImGui::SetCurrentContext(Context);
}

bool EditorGuizmo::IsOver()
{
    return GuizmoContext.bOverGizmoHotspot;
}

bool EditorGuizmo::IsUsing()
{
    return GuizmoContext.bUsing;
}

bool EditorGuizmo::IsUsingViewManipulate()
{
    return GuizmoContext.bUsingViewManipulate;
}

bool EditorGuizmo::IsViewManipulateHovered()
{
    return GuizmoContext.bIsViewManipulatorHovered;
}

bool EditorGuizmo::IsUsingAny()
{
    return GuizmoContext.bUsing || GuizmoContext.bUsingViewManipulate;
}

void EditorGuizmo::Enable(bool bEnable)
{
    GuizmoContext.bEnable = bEnable;
    if (!bEnable)
    {
        GuizmoContext.bUsing       = false;
        GuizmoContext.bUsingBounds = false;
    }
}

void EditorGuizmo::DecomposeMatrixToComponents(const float* Matrix, float* Translation, float* Rotation, float* Scale) 
{ 
    const Matrix4 MatrixData = LoadMatrix(Matrix); 
 
    Vector3 RightVec = Vector3(MatrixData.M[0][0], MatrixData.M[0][1], MatrixData.M[0][2]); 
    Vector3 UpVec    = Vector3(MatrixData.M[1][0], MatrixData.M[1][1], MatrixData.M[1][2]); 
    Vector3 DirVec   = Vector3(MatrixData.M[2][0], MatrixData.M[2][1], MatrixData.M[2][2]); 
 
    Scale[0] = RightVec.GetLength(); 
    Scale[1] = UpVec.GetLength(); 
    Scale[2] = DirVec.GetLength(); 
 
    Matrix4 MatrixOrthonormal = MatrixData; 
    MatrixOrthonormal.OrthoNormalize(); 
  
    const Matrix3 RotationMatrix3  = MatrixOrthonormal.GetRotationAndScale(); 
    const Quaternion RotationQuat  = Quaternion::FromRotationMatrix(RotationMatrix3); 
    const Vector3 EulerRadians     = RotationQuat.ToEuler(); 
    const Vector3 EulerDegrees     = EulerRadians * Math::Constants::RadToDeg; 
    Rotation[0] = EulerDegrees.X; 
    Rotation[1] = EulerDegrees.Y; 
    Rotation[2] = EulerDegrees.Z; 
 
    const Vector3 MatrixTranslation = MatrixData.GetTranslation(); 
    Translation[0] = MatrixTranslation.X; 
    Translation[1] = MatrixTranslation.Y; 
    Translation[2] = MatrixTranslation.Z; 
} 
 
void EditorGuizmo::RecomposeMatrixFromComponents(const float* Translation, const float* Rotation, const float* Scale, float* Matrix) 
{ 
    const float RadX = Rotation[0] * Math::Constants::Deg2Rad; 
    const float RadY = Rotation[1] * Math::Constants::Deg2Rad; 
    const float RadZ = Rotation[2] * Math::Constants::Deg2Rad; 
    Matrix4 MatrixData = Matrix4::RotationRollPitchYaw(RadX, RadY, RadZ); 
 
    float ValidScale[3]; 
    for (int32 AxisIndex = 0; AxisIndex < 3; AxisIndex++) 
    { 
        if (Math::Abs(Scale[AxisIndex]) < Math::Constants::Epsilon) 
        {
            ValidScale[AxisIndex] = 0.001f;
        }
        else
        {
            ValidScale[AxisIndex] = Scale[AxisIndex];
        }
    }

    MatrixData.M[0][0] *= ValidScale[0];
    MatrixData.M[0][1] *= ValidScale[0];
    MatrixData.M[0][2] *= ValidScale[0];
    MatrixData.M[1][0] *= ValidScale[1];
    MatrixData.M[1][1] *= ValidScale[1];
    MatrixData.M[1][2] *= ValidScale[1];
    MatrixData.M[2][0] *= ValidScale[2];
    MatrixData.M[2][1] *= ValidScale[2];
    MatrixData.M[2][2] *= ValidScale[2];

    MatrixData.SetTranslation(Vector3(Translation[0], Translation[1], Translation[2]));
    StoreMatrix(Matrix, MatrixData);
}

void EditorGuizmo::DecomposeMatrixToComponents(const Matrix4& InMatrix, Vector3& OutTranslation, Vector3& OutRotation, Vector3& OutScale)
{
    float Translation[3] = { 0.0f, 0.0f, 0.0f };
    float Rotation[3]    = { 0.0f, 0.0f, 0.0f };
    float Scale[3]       = { 1.0f, 1.0f, 1.0f };

    EditorGuizmo::DecomposeMatrixToComponents(&InMatrix.M[0][0], Translation, Rotation, Scale);

    OutTranslation = Vector3(Translation[0], Translation[1], Translation[2]);
    OutRotation    = Vector3(Rotation[0], Rotation[1], Rotation[2]);
    OutScale       = Vector3(Scale[0], Scale[1], Scale[2]);
}

void EditorGuizmo::RecomposeMatrixFromComponents(const Vector3& InTranslation, const Vector3& InRotation, const Vector3& InScale, Matrix4& OutMatrix)
{
    const float Translation[3] = { InTranslation.X, InTranslation.Y, InTranslation.Z };
    const float Rotation[3]    = { InRotation.X,    InRotation.Y,    InRotation.Z    };
    const float Scale[3]       = { InScale.X,       InScale.Y,       InScale.Z       };

    EditorGuizmo::RecomposeMatrixFromComponents(Translation, Rotation, Scale, &OutMatrix.M[0][0]);
}

void EditorGuizmo::SetRect(float X, float Y, float Width, float Height)
{
    GuizmoContext.X            = X;
    GuizmoContext.Y            = Y;
    GuizmoContext.Width        = Width;
    GuizmoContext.Height       = Height;
    GuizmoContext.XMax         = GuizmoContext.X + GuizmoContext.Width;
    GuizmoContext.YMax         = GuizmoContext.Y + GuizmoContext.Height;
    GuizmoContext.DisplayRatio = (Math::Abs(Height) > Math::Constants::Epsilon) ? (Width / Height) : 1.0f;
}

void EditorGuizmo::SetOrthographic(bool bIsOrthographic)
{
    GuizmoContext.bIsOrthographic = bIsOrthographic;
}

void EditorGuizmo::DrawCubes(const float* View, const float* Projection, const float* Matrices, int32 MatrixCount)
{
    const Matrix4 ViewMat       = LoadMatrix(View);
    const Matrix4 ProjectionMat = LoadMatrix(Projection);

    struct CubeFace
    {
       float  Z;
       ImVec2 FaceCoordsScreen[4];
       ImU32  Color;
    };

    TArray<CubeFace> FacesStorage;
    FacesStorage.ResizeUninitialized(MatrixCount * 6);
    
    CubeFace* Faces = FacesStorage.Data();
    if (!Faces)
    {
       return;
    }

    Plane Frustum[6];
    const Matrix4 ViewProjection = ViewMat * ProjectionMat;
    ComputeFrustumPlanes(Frustum, &ViewProjection.M[0][0]);

    int32 CubeFaceCount = 0;
    for (int32 Cube = 0; Cube < MatrixCount; Cube++)
    {
       const float* Matrix = &Matrices[Cube * 16];

       const Matrix4 ModelMat = LoadMatrix(Matrix);
       const Matrix4 Result   = ModelMat * ViewProjection;
       for (int32 IFace = 0; IFace < 6; IFace++)
       {
            const int32 NormalIndex = (IFace % 3);
            const int32 PerpXIndex  = (NormalIndex + 1) % 3;
            const int32 PerpYIndex  = (NormalIndex + 2) % 3;
            const float Invert      = (IFace > 2) ? -1.0f : 1.0f;

            const Vector4 FaceCoords[4] = 
            {
                DirectionUnary[NormalIndex] + DirectionUnary[PerpXIndex] + DirectionUnary[PerpYIndex],
                DirectionUnary[NormalIndex] + DirectionUnary[PerpXIndex] - DirectionUnary[PerpYIndex],
                DirectionUnary[NormalIndex] - DirectionUnary[PerpXIndex] - DirectionUnary[PerpYIndex],
                DirectionUnary[NormalIndex] - DirectionUnary[PerpXIndex] + DirectionUnary[PerpYIndex],
            };

            Vector4 DirectionVector  = DirectionUnary[NormalIndex] * 0.5f * Invert;
            Vector4 CenterPosition   = ModelMat.Transform(Vector4(DirectionVector.X, DirectionVector.Y, DirectionVector.Z, 1.0f));
            Vector4 CenterPositionVP = Result.Transform(Vector4(DirectionVector.X, DirectionVector.Y, DirectionVector.Z, 1.0f));

            bool bInFrustum = true;
            for (int32 IFrustum = 0; IFrustum < 6; IFrustum++)
            {
                float Distance = Frustum[IFrustum].DotProductCoord(Vector3(CenterPosition.X, CenterPosition.Y, CenterPosition.Z));
                if (Distance < 0.0f)
                {
                    bInFrustum = false;
                    break;
                }
            }

            if (!bInFrustum)
            {
                continue;
            }
 
            // 3D -> 2D
            CubeFace& CubeFaceObj = Faces[CubeFaceCount];
            for (uint32 ICoord = 0; ICoord < 4; ICoord++)
            {
                CubeFaceObj.FaceCoordsScreen[ICoord] = WorldToPos(FaceCoords[ICoord] * 0.5f * Invert, Result);
            }

            ImU32 DirectionColor = GetColorU32(EditorGuizmo::EColor::DirectionX + NormalIndex);
            CubeFaceObj.Color = DirectionColor | IM_COL32(0x80, 0x80, 0x80, 0);

            const float SafeW = (Math::Abs(CenterPositionVP.W) > Math::Constants::Epsilon) ? CenterPositionVP.W : Math::Constants::Epsilon;
            CubeFaceObj.Z = CenterPositionVP.Z / SafeW;
            CubeFaceCount++;
        }
    }

    FacesStorage.ResizeUninitialized(CubeFaceCount);
    FacesStorage.SortWithPredicate([](const CubeFace& A, const CubeFace& B) { return A.Z > B.Z; });

    // Draw face with lighter color
    for (int32 IFace = 0; IFace < CubeFaceCount; IFace++)
    {
       const CubeFace& CubeFaceObj = FacesStorage[IFace];
       GuizmoContext.DrawList->AddConvexPolyFilled(CubeFaceObj.FaceCoordsScreen, 4, CubeFaceObj.Color);
    }

    // FacesStorage frees automatically
}

void EditorGuizmo::DrawGrid(const float* View, const float* Projection, const float* Matrix, float GridSize)
{
    const Matrix4 ViewMat       = LoadMatrix(View);
    const Matrix4 ProjectionMat = LoadMatrix(Projection);
    const Matrix4 ViewProjection = ViewMat * ProjectionMat;
    
    Plane Frustum[6];
    ComputeFrustumPlanes(Frustum, &ViewProjection.M[0][0]);

    const Matrix4 ModelMat = LoadMatrix(Matrix);
    const Matrix4 Result   = ModelMat * ViewProjection;
    for (float GridLineOffset = -GridSize; GridLineOffset <= GridSize; GridLineOffset += 1.0f)
    {
        for (int32 AxisDirIndex = 0; AxisDirIndex < 2; AxisDirIndex++)
        {
            Vector4 PointA = Vector4(AxisDirIndex ? -GridSize : GridLineOffset, 0.0f, AxisDirIndex ? GridLineOffset : -GridSize, 0.0f);
            Vector4 PointB = Vector4(AxisDirIndex ? GridSize : GridLineOffset, 0.0f, AxisDirIndex ? GridLineOffset : GridSize, 0.0f);

            bool bVisible = true;
            for (int32 PlaneIndex = 0; PlaneIndex < 6; PlaneIndex++)
            {
                float DistToPlaneA = Frustum[PlaneIndex].DotProductCoord(Vector3(PointA.X, PointA.Y, PointA.Z));
                float DistToPlaneB = Frustum[PlaneIndex].DotProductCoord(Vector3(PointB.X, PointB.Y, PointB.Z));

                if (DistToPlaneA < 0.0f && DistToPlaneB < 0.0f)
                {
                    bVisible = false;
                    break;
                }

                if (DistToPlaneA > 0.0f && DistToPlaneB > 0.0f)
                {
                    continue;
                }

                if (DistToPlaneA < 0.0f)
                {
                    float Length = Math::Abs(DistToPlaneA - DistToPlaneB);
                    float LerpParam = Math::Abs(DistToPlaneA) / Length;
                    PointA = Vector4::Lerp(PointA, PointB, LerpParam);
                }

                if (DistToPlaneB < 0.0f)
                {
                    float Length = Math::Abs(DistToPlaneB - DistToPlaneA);
                    float LerpParam = Math::Abs(DistToPlaneB) / Length;
                    PointB = Vector4::Lerp(PointB, PointA, LerpParam);
                }
            }

            if (bVisible)
            {
                ImU32 Color = IM_COL32(0x80, 0x80, 0x80, 0xFF);
                Color = (Math::FMod(Math::Abs(GridLineOffset), 10.0f) < Math::Constants::Epsilon) ? IM_COL32(0x90, 0x90, 0x90, 0xFF) : Color;
                Color = (Math::Abs(GridLineOffset) < Math::Constants::Epsilon) ? IM_COL32(0x40, 0x40, 0x40, 0xFF): Color;

                float Thickness = 1.0f;
                Thickness = (Math::FMod(Math::Abs(GridLineOffset), 10.0f) < Math::Constants::Epsilon) ? 1.5f : Thickness;
                Thickness = (Math::Abs(GridLineOffset) < Math::Constants::Epsilon) ? 2.3f : Thickness;

                GuizmoContext.DrawList->AddLine(WorldToPos(PointA, Result), WorldToPos(PointB, Result), Color, Thickness);
            }
        }
    }
}

void EditorGuizmo::DrawCubes(const Matrix4& View, const Matrix4& Projection, const Matrix4* Matrices, int32 MatrixCount)
{
    const float* ViewPtr       = &View.M[0][0];
    const float* ProjectionPtr = &Projection.M[0][0];
    const float* MatricesPtr   = reinterpret_cast<const float*>(Matrices);

    EditorGuizmo::DrawCubes(ViewPtr, ProjectionPtr, MatricesPtr, MatrixCount);
}

void EditorGuizmo::DrawGrid(const Matrix4& View, const Matrix4& Projection, const Matrix4& Matrix, float GridSize)
{
    const float* ViewPtr       = &View.M[0][0];
    const float* ProjectionPtr = &Projection.M[0][0];
    const float* MatrixPtr     = &Matrix.M[0][0];

    EditorGuizmo::DrawGrid(ViewPtr, ProjectionPtr, MatrixPtr, GridSize);
}

bool EditorGuizmo::Manipulate(const float* View, const float* Projection, EditorGuizmo::EOperation::Type Operation, EditorGuizmo::EMode Mode, float* InOutMatrix, 
    float* OutDeltaMatrix, const float* Snap, const float* LocalBounds, const float* BoundsSnap)
{
    GuizmoContext.DrawList->PushClipRect(
        ImVec2(GuizmoContext.X, GuizmoContext.Y), 
        ImVec2(GuizmoContext.X + GuizmoContext.Width, GuizmoContext.Y + GuizmoContext.Height),
        false);

    // Scale is always local or Matrix will be skewed when applying world Scale or oriented Matrix
    ComputeContext(View, Projection, InOutMatrix, (Operation & EditorGuizmo::EOperation::Scale) ? EditorGuizmo::EMode::Local : Mode);

    // Set delta to identity
    if (OutDeltaMatrix)
    {
       Matrix4 DeltaMat;
       DeltaMat.SetIdentity();
       StoreMatrix(OutDeltaMatrix, DeltaMat);
    }

    // Behind camera
    Vector4 CamSpacePosition = GuizmoContext.MVP.Transform(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    if (!GuizmoContext.bIsOrthographic && CamSpacePosition.Z < 0.001f && !GuizmoContext.bUsing)
    {
       GuizmoContext.DrawList->PopClipRect();
       return false;
    }

    int32 Type = EMoveType::None;

    bool bManipulated = false;
    if (GuizmoContext.bEnable)
    {
       if (!GuizmoContext.bUsingBounds)
       {
          bManipulated = 
              HandleTranslation(InOutMatrix, OutDeltaMatrix, Operation, Type, Snap) ||
              HandleScale(InOutMatrix, OutDeltaMatrix, Operation, Type, Snap) ||
              HandleRotation(InOutMatrix, OutDeltaMatrix, Operation, Type, Snap);
       }
    }

    if (LocalBounds && !GuizmoContext.bUsing)
    {
       bManipulated |= HandleAndDrawLocalBounds(LocalBounds, InOutMatrix, BoundsSnap, Operation);
    }

    GuizmoContext.Operation = Operation;

    if (!GuizmoContext.bUsingBounds)
    {
       DrawRotationGizmo(Operation, Type);
       DrawTranslationGizmo(Operation, Type);
       DrawScaleGizmo(Operation, Type);
       DrawScaleUniveralGizmo(Operation, Type);
    }

    GuizmoContext.DrawList->PopClipRect();
    return bManipulated;
}

bool EditorGuizmo::Manipulate(const Matrix4& View, const Matrix4& Projection, EditorGuizmo::EOperation::Type Operation, EditorGuizmo::EMode Mode, Matrix4& InOutMatrix,
    Matrix4* OutDeltaMatrix, const float* Snap, const float* LocalBounds, const float* BoundsSnap)
{
    const float* ViewPtr       = &View.M[0][0];
    const float* ProjectionPtr = &Projection.M[0][0];
    float*       MatrixPtr     = &InOutMatrix.M[0][0];
    float*       DeltaPtr      = OutDeltaMatrix ? &OutDeltaMatrix->M[0][0] : nullptr;

    return EditorGuizmo::Manipulate(ViewPtr, ProjectionPtr, Operation, Mode, MatrixPtr, DeltaPtr, Snap, LocalBounds, BoundsSnap);
}

void EditorGuizmo::ViewManipulate(float* InOutView, float Length, ImVec2 Position, ImVec2 Size, ImU32 BackgroundColor)
{
    // Implementation moved from static function - see static ViewManipulate below
    static bool     bIsDragging = false;
    static bool     bIsClicking = false;
    static Vector4 InterpolationUp;
    static Vector4 InterpolationDir;
    static int32    InterpolationFrames = 0;

    const Vector4 ReferenceUp = Vector4(0.0f, 1.0f, 0.0f, 0.0f);

    Matrix4 SvgView       = GuizmoContext.ViewMat;
    Matrix4 SvgProjection = GuizmoContext.ProjectionMat;

    ImGuiIO& State = ImGui::GetIO();
    GuizmoContext.DrawList->AddRectFilled(Position, Position + Size, BackgroundColor);

    const Matrix4 ViewMatrix = LoadMatrix(InOutView);
    Matrix4 ViewInverse = ViewMatrix.GetInverse();

    Vector4 ViewInverseDir = Vector4(ViewInverse.M[2][0], ViewInverse.M[2][1], ViewInverse.M[2][2], 0.0f);
    Vector4 ViewInversePos  = Vector4(ViewInverse.GetTranslation(), 1.0f);
    const Vector4 CamTarget = ViewInversePos - ViewInverseDir * Length;

    // View/Projection matrices
    const float Distance = 3.0f;
    
    Matrix4 CubeProjection;
    Matrix4 CubeView;
    
    float FOV = Math::Acos(Distance / (Math::Sqrt(Distance * Distance + 3.0f))) * Math::Constants::RadToDeg;

    const float FOVRadians = (FOV / Math::Sqrt(2.0f)) * Math::Constants::Deg2Rad;
    CubeProjection = Matrix4::PerspectiveProjection(FOVRadians, Size.x / Size.y, 0.01f, 1000.0f);

    Vector3 Direction = Vector3(ViewInverse.M[2][0], ViewInverse.M[2][1], ViewInverse.M[2][2]);
    Vector3 Up = Vector3(ViewInverse.M[1][0], ViewInverse.M[1][1], ViewInverse.M[1][2]);
    Vector3 Eye = Direction * Distance;
    Vector3 At = Vector3(0.0f, 0.0f, 0.0f);

    CubeView = Matrix4::LookAt(Eye, At, Up);

    // Set context
    GuizmoContext.ViewMat       = CubeView;
    GuizmoContext.ProjectionMat = CubeProjection;
    ComputeCameraRay(GuizmoContext.RayOrigin, GuizmoContext.RayVector, Position, Size);

    const Matrix4 Result = CubeView * CubeProjection;

    // Panels
    static const ImVec2 PanelPosition[9] =
    {
        ImVec2(0.75f,0.75f), ImVec2(0.25f, 0.75f), ImVec2(0.0f, 0.75f),
        ImVec2(0.75f, 0.25f), ImVec2(0.25f, 0.25f), ImVec2(0.0f, 0.25f),
        ImVec2(0.75f, 0.0f), ImVec2(0.25f, 0.0f), ImVec2(0.0f, 0.0f)
    };

    static const ImVec2 PanelSize[9] =
    {
        ImVec2(0.25f,0.25f), ImVec2(0.5f, 0.25f), ImVec2(0.25f, 0.25f),
        ImVec2(0.25f, 0.5f), ImVec2(0.5f, 0.5f), ImVec2(0.25f, 0.5f),
        ImVec2(0.25f, 0.25f), ImVec2(0.5f, 0.25f), ImVec2(0.25f, 0.25f)
    };

    // Tag faces
    bool bBoxes[27]{};
    static int32 OverBox = -1;

    for (int32 IPass = 0; IPass < 2; IPass++)
    {
        for (int32 IFace = 0; IFace < 6; IFace++)
        {
            const int32 NormalIndex = (IFace % 3);
            const int32 PerpXIndex  = (NormalIndex + 1) % 3;
            const int32 PerpYIndex  = (NormalIndex + 2) % 3;
            const float Invert      = (IFace > 2) ? -1.0f : 1.0f;

            const Vector4 IndexVectorX = DirectionUnary[PerpXIndex] * Invert;
            const Vector4 IndexVectorY = DirectionUnary[PerpYIndex] * Invert;
            const Vector4 BoxOrigin    = DirectionUnary[NormalIndex] * -Invert - IndexVectorX - IndexVectorY;

            // Plan local space
            const Vector4 FaceNormal = DirectionUnary[NormalIndex] * Invert;

            Vector4 ViewSpaceNormal = CubeView.Transform(Vector4(FaceNormal.X, FaceNormal.Y, FaceNormal.Z, 0.0f));
            ViewSpaceNormal.Normalize();

            Vector4 ViewSpacePoint = CubeView.Transform(Vector4(FaceNormal.X * 0.5f, FaceNormal.Y * 0.5f, FaceNormal.Z * 0.5f, 1.0f));
            const Plane ViewSpaceFacePlan = Plane::FromPointAndNormal(
                Vector3(ViewSpacePoint.X, ViewSpacePoint.Y, ViewSpacePoint.Z), 
                Vector3(ViewSpaceNormal.X, ViewSpaceNormal.Y, ViewSpaceNormal.Z));

            // Back face culling
            if (ViewSpaceFacePlan.W > 0.0f)
            {
                continue;
            }

            const Plane FacePlan = Plane::FromPointAndNormal(
                Vector3(FaceNormal.X * 0.5f, FaceNormal.Y * 0.5f, FaceNormal.Z * 0.5f),
                Vector3(FaceNormal.X, FaceNormal.Y, FaceNormal.Z));

            const float RayIntersectionLength = FacePlan.IntersectRay(
                Vector3(GuizmoContext.RayOrigin.X, GuizmoContext.RayOrigin.Y, GuizmoContext.RayOrigin.Z), 
                Vector3(GuizmoContext.RayVector.X, GuizmoContext.RayVector.Y, GuizmoContext.RayVector.Z));

            if (RayIntersectionLength < 0.0f)
            {
                continue;
            }

            Vector4 PositionOnPlane = GuizmoContext.RayOrigin + GuizmoContext.RayVector * RayIntersectionLength - (FaceNormal * 0.5f);

            float LocalX = DirectionUnary[PerpXIndex].DotProduct(PositionOnPlane) * Invert + 0.5f;
            float LocalY = DirectionUnary[PerpYIndex].DotProduct(PositionOnPlane) * Invert + 0.5f;

            // Panels
            const Vector4 DirectionX = DirectionUnary[PerpXIndex];
            const Vector4 DirectionY = DirectionUnary[PerpYIndex];
            const Vector4 Origin     = DirectionUnary[NormalIndex] - DirectionX - DirectionY;

            for (int32 IPanel = 0; IPanel < 9; IPanel++)
            {
                Vector4 BoxCoord = BoxOrigin + IndexVectorX * static_cast<float>(IPanel % 3) + IndexVectorY * static_cast<float>(IPanel / 3) + Vector4(1.0f, 1.0f, 1.0f, 0.0f);

                const ImVec2 PanelPositionScaled  = PanelPosition[IPanel] * 2.0f;
                const ImVec2 PanelSizeScaled    = PanelSize[IPanel] * 2.0f;
                
                Vector4 PanelPositions[4] = 
                {
                    DirectionX * PanelPositionScaled.x + DirectionY * PanelPositionScaled.y,
                    DirectionX * PanelPositionScaled.x + DirectionY * (PanelPositionScaled.y + PanelSizeScaled.y),
                    DirectionX * (PanelPositionScaled.x + PanelSizeScaled.x) + DirectionY * (PanelPositionScaled.y + PanelSizeScaled.y),
                    DirectionX * (PanelPositionScaled.x + PanelSizeScaled.x) + DirectionY * PanelPositionScaled.y
                };
                
                ImVec2 FaceCoordsScreen[4];
                for (uint32 ICoord = 0; ICoord < 4; ICoord++)
                {
                    FaceCoordsScreen[ICoord] = WorldToPos((PanelPositions[ICoord] + Origin) * 0.5f * Invert, Result, Position, Size);
                }

                const ImVec2 PanelCorners[2] = { PanelPosition[IPanel], PanelPosition[IPanel] + PanelSize[IPanel] };
                bool bInsidePanel = LocalX > PanelCorners[0].x && LocalX < PanelCorners[1].x && LocalY > PanelCorners[0].y && LocalY < PanelCorners[1].y;
             
                int32 BoxCoordInt = static_cast<int32>(BoxCoord.X * 9.0f + BoxCoord.Y * 3.0f + BoxCoord.Z);
                IM_ASSERT(BoxCoordInt < 27);
                bBoxes[BoxCoordInt] |= bInsidePanel && (!bIsDragging) && GuizmoContext.bMouseOver;

                // Draw face with lighter color
                if (IPass)
                {
                    ImU32 DirectionColor = GetColorU32(EditorGuizmo::EColor::DirectionX + NormalIndex);
                    GuizmoContext.DrawList->AddConvexPolyFilled(FaceCoordsScreen, 4, (DirectionColor | IM_COL32(0x80, 0x80, 0x80, 0x80)) | (GuizmoContext.bIsViewManipulatorHovered ? IM_COL32(0x08, 0x08, 0x08, 0) : 0));

                    if (bBoxes[BoxCoordInt])
                    {
                        ImU32 SelectionColor = GetColorU32(EditorGuizmo::EColor::Selection);
                        GuizmoContext.DrawList->AddConvexPolyFilled(FaceCoordsScreen, 4, SelectionColor);

                        if (State.MouseDown[0] && !bIsClicking && !bIsDragging && GImGui->ActiveId == 0)
                        {
                            OverBox     = BoxCoordInt;
                            bIsClicking = true;
                            bIsDragging = true;
                        }
                    }
                }
            }
        }
    }

    if (InterpolationFrames)
    {
        InterpolationFrames--;

        Vector4 NewDir = Vector4(ViewInverse.M[2][0], ViewInverse.M[2][1], ViewInverse.M[2][2], 0.0f);
        NewDir = Vector4::Lerp(NewDir, InterpolationDir, 0.2f);
        NewDir.Normalize();

        Vector4 NewUp = Vector4(ViewInverse.M[1][0], ViewInverse.M[1][1], ViewInverse.M[1][2], 0.0f);
        NewUp = Vector4::Lerp(NewUp, InterpolationUp, 0.3f);
        NewUp.Normalize();
        NewUp = InterpolationUp;

        Vector4 NewEye = CamTarget + NewDir * Length;
        StoreMatrix(InOutView, Matrix4::LookAt(
            Vector3(NewEye.X, NewEye.Y, NewEye.Z),
            Vector3(CamTarget.X, CamTarget.Y, CamTarget.Z),
            Vector3(NewUp.X, NewUp.Y, NewUp.Z)));
    }

    GuizmoContext.bIsViewManipulatorHovered = GuizmoContext.bMouseOver && ImRect(Position, Position + Size).Contains(State.MousePos);

    if (State.MouseDown[0] && (Math::Abs(State.MouseDelta[0]) || Math::Abs(State.MouseDelta[1])) && bIsClicking)
    {
        bIsClicking = false;
    }

    if (!State.MouseDown[0])
    {
        if (bIsClicking)
        {
            // Apply new view direction
            int32 Cx = OverBox / 9;
            int32 Cy = (OverBox - Cx * 9) / 3;
            int32 Cz = OverBox % 3;

            InterpolationDir = Vector4(1.0f - static_cast<float>(Cx), 1.0f - static_cast<float>(Cy), 1.0f - static_cast<float>(Cz), 0.0f);
            InterpolationDir.Normalize();

            if (Math::Abs(InterpolationDir.DotProduct(ReferenceUp)) > 1.0f - 0.01f)
            {
                Vector4 Right = Vector4(ViewInverse.M[0][0], ViewInverse.M[0][1], ViewInverse.M[0][2], 0.0f);
                if (Math::Abs(Right.X) > Math::Abs(Right.Z))
                {
                    Right.Z = 0.0f;
                }
                else
                {
                    Right.X = 0.0f;
                }

                Right.Normalize();
                InterpolationUp = InterpolationDir.CrossProduct(Right);
                InterpolationUp.Normalize();
            }
            else
            {
                InterpolationUp = ReferenceUp;
            }

            InterpolationFrames = 40;
        }

        bIsClicking = false;
        bIsDragging = false;
    }

    if (bIsDragging)
    {
        Matrix4 Rx;
        Matrix4 Ry;
        Matrix4 Roll;
        Rx = Quaternion::FromAxisAngle(Vector3(ReferenceUp.X, ReferenceUp.Y, ReferenceUp.Z), -State.MouseDelta.x * 0.01f).ToMatrix4();

        Vector4 ViewInverseRight = Vector4(ViewInverse.M[0][0], ViewInverse.M[0][1], ViewInverse.M[0][2], 0.0f);
        Ry = Quaternion::FromAxisAngle(Vector3(ViewInverseRight.X, ViewInverseRight.Y, ViewInverseRight.Z), -State.MouseDelta.y * 0.01f).ToMatrix4();

        Roll = Rx * Ry;

        Vector4 NewDir = Roll.Transform(Vector4(ViewInverse.M[2][0], ViewInverse.M[2][1], ViewInverse.M[2][2], 0.0f));
        NewDir.Normalize();

        // Clamp
        Vector4 PlanDir = ViewInverseRight.CrossProduct(ReferenceUp);
        PlanDir.Y = 0.0f;
        PlanDir.Normalize();

        float DotProductResult = PlanDir.DotProduct(NewDir);
        if (DotProductResult < 0.0f)
        {
            NewDir += PlanDir * DotProductResult;
            NewDir.Normalize();
        }

        Vector4 NewEye = CamTarget + NewDir * Length;
        StoreMatrix(InOutView, Matrix4::LookAt(
            Vector3(NewEye.X, NewEye.Y, NewEye.Z),
            Vector3(CamTarget.X, CamTarget.Y, CamTarget.Z),
            Vector3(ReferenceUp.X, ReferenceUp.Y, ReferenceUp.Z)));
    }

    GuizmoContext.bUsingViewManipulate = (InterpolationFrames != 0) || bIsDragging;
    if (bIsClicking || GuizmoContext.bUsingViewManipulate || GuizmoContext.bIsViewManipulatorHovered) 
    {
        ImGui::SetNextFrameWantCaptureMouse(true);
    }

    // Restore View/Projection because it was used to compute ray
    ComputeContext(&SvgView.M[0][0], &SvgProjection.M[0][0], &GuizmoContext.ModelSource.M[0][0], GuizmoContext.Mode);
}

void EditorGuizmo::ViewManipulate(Matrix4& InOutView, float Length, ImVec2 Position, ImVec2 Size, ImU32 BackgroundColor)
{
    EditorGuizmo::ViewManipulate(&InOutView.M[0][0], Length, Position, Size, BackgroundColor);
}

void EditorGuizmo::ViewManipulate(float* InOutView, const float* Projection, EditorGuizmo::EOperation::Type Operation, EditorGuizmo::EMode Mode, float* InOutMatrix, float Length, 
    ImVec2 Position, ImVec2 Size, ImU32 BackgroundColor)
{
    // Scale is always local or Matrix will be skewed when applying world Scale or oriented Matrix
    ComputeContext(InOutView, Projection, InOutMatrix, (Operation & EditorGuizmo::EOperation::Scale) ? EditorGuizmo::EMode::Local : Mode);
    EditorGuizmo::ViewManipulate(InOutView, Length, Position, Size, BackgroundColor);
}

void EditorGuizmo::ViewManipulate(Matrix4& InOutView, const Matrix4& Projection, EditorGuizmo::EOperation::Type Operation, EditorGuizmo::EMode Mode, Matrix4& InOutMatrix, 
    float Length, ImVec2 Position, ImVec2 Size, ImU32 BackgroundColor)
{
    EditorGuizmo::ViewManipulate(&InOutView.M[0][0], &Projection.M[0][0], Operation, Mode, &InOutMatrix.M[0][0], Length, Position, Size, BackgroundColor);
}

void EditorGuizmo::SetAlternativeWindow(ImGuiWindow* Window)
{
    GuizmoContext.AlternativeWindow = Window;
}

void EditorGuizmo::PushID(const char* StrID)
{
    ImGuiID ID = GetID(StrID);
    GuizmoContext.IDStack.push_back(ID);
}

void EditorGuizmo::PushID(const char* StrIDBegin, const char* StrIDEnd)
{
    ImGuiID ID = GetID(StrIDBegin, StrIDEnd);
    GuizmoContext.IDStack.push_back(ID);
}

void EditorGuizmo::PushID(const void* PtrID)
{
    ImGuiID ID = GetID(PtrID);
    GuizmoContext.IDStack.push_back(ID);
}

void EditorGuizmo::PushID(int32 IntID)
{
    ImGuiID ID = GetID(IntID);
    GuizmoContext.IDStack.push_back(ID);
}

void EditorGuizmo::PopID()
{
    IM_ASSERT(GuizmoContext.IDStack.Size > 1); // Too many PopID(), or could be popping in a wrong/different window?
    GuizmoContext.IDStack.pop_back();

    if (GuizmoContext.IDStack.empty())
    {
        GuizmoContext.IDStack.clear();
    }
}

ImGuiID EditorGuizmo::GetID(const char* StrID)
{
    ImGuiID Seed = GuizmoContext.GetCurrentID();
    ImGuiID ID   = ImHashStr(StrID, 0, Seed);
    return ID;
}

ImGuiID EditorGuizmo::GetID(const char* StrIDBegin, const char* StrIDEnd)
{
    ImGuiID Seed = GuizmoContext.GetCurrentID();
    ImGuiID ID   = ImHashStr(StrIDBegin, StrIDEnd ? (StrIDEnd - StrIDBegin) : 0, Seed);
    return ID;
}

ImGuiID EditorGuizmo::GetID(const void* PtrID)
{
    ImGuiID Seed = GuizmoContext.GetCurrentID();
    ImGuiID ID   = ImHashData(PtrID, sizeof(void*), Seed);
    return ID;
}

ImGuiID EditorGuizmo::GetID(int32 IntID)
{
    ImGuiID Seed = GuizmoContext.GetCurrentID();
    ImGuiID ID   = ImHashData(&IntID, sizeof(IntID), Seed);
    return ID;
}

bool EditorGuizmo::IsOver(EditorGuizmo::EOperation::Type Op)
{
    if (IsUsing())
    {
        return true;
    }
    
    if (Intersects(Op, EditorGuizmo::EOperation::Scale) && GetScaleType(Op) != EMoveType::None)
    {
        return true;
    }
    
    if (Intersects(Op, EditorGuizmo::EOperation::Rotate) && GetRotateType(Op) != EMoveType::None)
    {
        return true;
    }
    
    if (Intersects(Op, EditorGuizmo::EOperation::Translate) && GetMoveType(Op, nullptr) != EMoveType::None)
    {
        return true;
    }

    return false;
}

bool EditorGuizmo::IsOver(float* Position, float PixelRadius)
{
    const ImGuiIO& State = ImGui::GetIO();
    float Radius = Math::Sqrt((ImLengthSqr(WorldToPos(Vector4(Position[0], Position[1], Position[2], 0.0f), GuizmoContext.ViewProjection) - State.MousePos)));
    return Radius < PixelRadius;
}

bool EditorGuizmo::IsOver(Vector3& Position, float PixelRadius)
{
    const ImGuiIO& State = ImGui::GetIO();
    float Radius = Math::Sqrt((ImLengthSqr(WorldToPos(Vector4(Position.X, Position.Y, Position.Z, 0.0f), GuizmoContext.ViewProjection) - State.MousePos)));
    return Radius < PixelRadius;
}

void EditorGuizmo::SetGizmoSizeClipSpace(float Value)
{
    GuizmoContext.GizmoSizeClipSpace = Value;
}

void EditorGuizmo::AllowAxisFlip(bool bAllow)
{
    GuizmoContext.bAllowAxisFlip = bAllow;
}

void EditorGuizmo::SetAxisLimit(float Value)
{
    GuizmoContext.AxisLimit = Value;
}

void EditorGuizmo::SetAxisMask(bool bHideX, bool bHideY, bool bHideZ)
{
    GuizmoContext.AxisMask =
        (bHideX ? (1 << 0) : 0) |
        (bHideY ? (1 << 1) : 0) |
        (bHideZ ? (1 << 2) : 0);
}

void EditorGuizmo::SetPlaneLimit(float Value)
{
    GuizmoContext.PlaneLimit = Value;
}

EditorGuizmo::Style& EditorGuizmo::GetStyle()
{
    return GuizmoContext.Style;
}
