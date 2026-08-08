#pragma once
#include "ImGuiPlugin/ImGuiCore.h"

struct ImGuiContext;
struct ImGuiWindow;

class Matrix4;
class Vector3;

struct ENGINE_API EditorGuizmo
{
public:
    struct EOperation
    {
        enum Type : uint32
        {
            TranslateX   = (1u << 0),
            TranslateY   = (1u << 1),
            TranslateZ   = (1u << 2),
            RotateX      = (1u << 3),
            RotateY      = (1u << 4),
            RotateZ      = (1u << 5),
            RotateScreen = (1u << 6),
            ScaleX       = (1u << 7),
            ScaleY       = (1u << 8),
            ScaleZ       = (1u << 9),
            Bounds       = (1u << 10),
            ScaleXU      = (1u << 11),
            ScaleYU      = (1u << 12),
            ScaleZU      = (1u << 13),
        
            Translate = TranslateX | TranslateY | TranslateZ,
            Rotate    = RotateX | RotateY | RotateZ | RotateScreen,
            Scale     = ScaleX | ScaleY | ScaleZ,
            ScaleU    = ScaleXU | ScaleYU | ScaleZU,
            Universal = Translate | Rotate | ScaleU
        };
    };

    enum class EMode
    {
        Local,
        World
    };

    enum class EScaleHandleShape
    {
        Circle,
        Square
    };

    struct EColor
    {
        enum Type
        {
            DirectionX,      // directionColor[0]
            DirectionY,      // directionColor[1]
            DirectionZ,      // directionColor[2]
            PlaneX,          // planeColor[0]
            PlaneY,          // planeColor[1]
            PlaneZ,          // planeColor[2]
            Selection,       // selectionColor
            Inactive,        // inactiveColor
            TranslationLine, // translationLineColor
            ScaleLine,
            RotationUsingBorder,
            RotationUsingFill,
            HatchedAxisLines,
            Text,
            TextShadow,
            Count
        };
    };

public:
    struct Style 
    { 
        Style(); 
 
        float             TranslationLineThickness;   // Thickness of lines for translation gizmo 
        float             TranslationLineArrowSize;   // Size of arrow at the end of lines for translation gizmo 
        float             RotationLineThickness;      // Thickness of lines for rotation gizmo 
        float             RotationOuterLineThickness; // Thickness of line surrounding the rotation gizmo 
        float             ScaleLineThickness;         // Thickness of lines for scale gizmo 
        float             ScaleLineCircleSize;        // Size of circle at the end of lines for scale gizmo 
        EScaleHandleShape ScaleHandleShape;           // Shape of axis handles for scale gizmo
        float             HatchedAxisLineThickness;   // Thickness of hatched axis lines 
        float             CenterCircleSize;           // Size of circle at the center of the translate/scale gizmo 
        ImVec4            Colors[EColor::Count]; 
    }; 

public:

    // Setup
    static void SetDrawlist(ImDrawList* DrawList = nullptr);
    static void BeginFrame();
    static void SetImGuiContext(ImGuiContext* Context);
    static void SetAlternativeWindow(ImGuiWindow* Window);
    static void SetRect(float X, float Y, float Width, float Height);
    static void SetOrthographic(bool bIsOrthographic);

    // State queries / enable
    static bool IsOver();
    static bool IsOver(EOperation::Type Op);
    static bool IsOver(float* Position, float PixelRadius);
    static bool IsOver(Vector3& Position, float PixelRadius);
    static bool IsUsing();
    static bool IsUsingViewManipulate();
    static bool IsViewManipulateHovered();
    static bool IsUsingAny();

    static void Enable(bool bEnable);
    static void CancelUsing();

    static void DecomposeMatrixToComponents(const float* Matrix, float* Translation, float* Rotation, float* Scale);
    static void RecomposeMatrixFromComponents(const float* Translation, const float* Rotation, const float* Scale, float* Matrix);
    static void DecomposeMatrixToComponents(const Matrix4& InMatrix, Vector3& OutTranslation, Vector3& OutRotation, Vector3& OutScale);
    static void RecomposeMatrixFromComponents(const Vector3& InTranslation, const Vector3& InRotation, const Vector3& InScale, Matrix4& OutMatrix);

    static void DrawCubes(const float* View, const float* Projection, const float* Matrices, int32 MatrixCount);
    static void DrawGrid(const float* View, const float* Projection, const float* Matrix, float GridSize);
    static void DrawCubes(const Matrix4& View, const Matrix4& Projection, const Matrix4* Matrices, int32 MatrixCount);
    static void DrawGrid(const Matrix4& View, const Matrix4& Projection, const Matrix4& Matrix, float GridSize);

    // Manipulation
    static bool Manipulate(const float* View, const float* Projection, EOperation::Type Operation, EMode Mode, float* InOutMatrix, float* OutDeltaMatrix = nullptr,
        const float* Snap = nullptr, const float* LocalBounds = nullptr, const float* BoundsSnap = nullptr);
    static bool Manipulate(const Matrix4& View, const Matrix4& Projection, EOperation::Type Operation, EMode Mode, Matrix4& InOutMatrix, Matrix4* OutDeltaMatrix = nullptr,
        const float* Snap = nullptr, const float* LocalBounds = nullptr, const float* BoundsSnap = nullptr);

    // View manipulator
    static void ViewManipulate(float* InOutView, float Length, ImVec2 Position, ImVec2 Size, ImU32 BackgroundColor);
    static void ViewManipulate(float* InOutView, const float* Projection, EOperation::Type Operation, EMode Mode, float* InOutMatrix, float Length, ImVec2 Position,
        ImVec2 Size, ImU32 BackgroundColor);
    static void ViewManipulate(Matrix4& InOutView, float Length, ImVec2 Position, ImVec2 Size, ImU32 BackgroundColor);
    static void ViewManipulate(Matrix4& InOutView, const Matrix4& Projection, EOperation::Type Operation, EMode Mode, Matrix4& InOutMatrix, float Length, ImVec2 Position,
        ImVec2 Size, ImU32 BackgroundColor);

    static void PushID(const char* StrID);
    static void PushID(const char* StrIDBegin, const char* StrIDEnd);
    static void PushID(const void* PtrID);
    static void PushID(int32 IntID);
    static void PopID();
    
    static ImGuiID GetID(const char* StrID);
    static ImGuiID GetID(const char* StrIDBegin, const char* StrIDEnd);
    static ImGuiID GetID(const void* PtrID);
    static ImGuiID GetID(int32 IntID);

    // Advanced configuration
    static void SetGizmoSizeClipSpace(float Value);
    static void AllowAxisFlip(bool bAllow);
    static void SetAxisLimit(float Value);
    static void SetAxisMask(bool bHideX, bool bHideY, bool bHideZ);
    static void SetPlaneLimit(float Value);

    // Style
    static Style& GetStyle();
};

