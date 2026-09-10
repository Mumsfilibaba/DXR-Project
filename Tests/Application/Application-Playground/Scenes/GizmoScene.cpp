#include <Core/Containers/SharedPtr.h>
#include <Core/Math/Math.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Overlay.h>
#include <Application/Elements/Spacer.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Elements/ToolBar.h>
#include <Application/Gizmo/Gizmo.h>
#include <Application/Input/Keys.h>
#include <Application/Style/UIStyle.h>

#include "PlaygroundScene.h"
#include "ScenePanel.h"

// Tall enough that the gizmo is a comfortable size at the distance the camera starts at
constexpr int32 GIZMO_VIEWPORT_HEIGHT = 460;

// The ground plane runs this many units either side of the origin, ruled once per unit
constexpr int32 GROUND_EXTENT = 6;

// How far the orbit camera can be pushed in and pulled out, in world units
constexpr float MIN_CAMERA_DISTANCE = 2.0f;
constexpr float MAX_CAMERA_DISTANCE = 40.0f;

// How far a drag turns the camera, so a quarter of the viewport is about a quarter turn
constexpr float ORBIT_DEGREES_PER_PIXEL = 0.4f;

// Looking straight down is a singularity for a yaw-pitch camera, so it stops just short of it
constexpr float MAX_CAMERA_PITCH_DEGREES = 88.0f;

static void RefreshGizmoReadout();

class FGizmoViewport final : public FVisualElement
{
public:
    FGizmoViewport()
        : FVisualElement()
        , Gizmo(nullptr)
        , Font(nullptr)
        , Transform(Matrix4::Identity())
        , Target(0.0f, 0.0f, 0.0f)
        , YawDegrees(35.0f)
        , PitchDegrees(25.0f)
        , Distance(9.0f)
        , bIsOrbiting(false)
        , bIsPanning(false)
        , LastCursorPosition(0, 0)
    {
    }

    virtual ~FGizmoViewport() = default;

    void Initialize(const TSharedPtr<FGizmo>& InGizmo, const TSharedPtr<IFontFace>& InFont)
    {
        Gizmo = InGizmo;
        Font  = InFont;

        if (Gizmo)
        {
            Gizmo->SetParentElement(AsWeakPtr());
            Gizmo->SetTransform(Transform);
        }
    }

    virtual IntVector2 ComputeDesiredSize() const override
    {
        return IntVector2(0, GIZMO_VIEWPORT_HEIGHT);
    }

    virtual void OnArrange(const FRectangle& AllottedBounds) override
    {
        FVisualElement::OnArrange(AllottedBounds);

        if (Gizmo)
        {
            Gizmo->Tick(AllottedBounds);
            Gizmo->SetCamera(GetViewMatrix(), GetProjectionMatrix(), false);
        }
    }

    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override
    {
        if (Gizmo)
        {
            OutChildren.Add(Gizmo);
        }
    }

    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override
    {
        FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

        if (Gizmo && Gizmo->GetContentRectangle().EncapsulatesPoint(ClientPosition))
        {
            Gizmo->FindChildrenContainingPoint(ClientPosition, OutChildElements);
        }
    }

    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override
    {
        const FUIStyle&   Style  = FUIStyle::GetDefault();
        const FRectangle& Bounds = AllottedGeometry.Bounds;

        OutCommandList.AddBox(LayerId, Bounds, FFloatColor(0.09f, 0.10f, 0.12f, 1.0f));
        OutCommandList.PushClip(LayerId, Bounds);

        const FCameraFrame Frame = GetCameraFrame(Bounds);

        DrawGround(Frame, OutCommandList, LayerId + 1);
        DrawCube(Frame, OutCommandList, LayerId + 2);

        int32 MaxLayerId = LayerId + 2;
        if (Gizmo)
        {
            const FDrawGeometry GizmoGeometry(Gizmo->GetContentRectangle(), AllottedGeometry.Scale);
            MaxLayerId = Gizmo->OnDraw(GizmoGeometry, OutCommandList, MaxLayerId + 1);
        }

        if (Font)
        {
            const String Caption = String::Printf("yaw %.0f  pitch %.0f  distance %.1f", YawDegrees, PitchDegrees, Distance);

            FRectangle CaptionBounds = Bounds.Deflate(FMargin(10, 10, 10, 10));
            CaptionBounds.Height     = Font->GetLineHeight();

            OutCommandList.AddText(MaxLayerId + 1, CaptionBounds, Caption, Font.Get(), Style.Colors.TextDisabled);
            MaxLayerId += 1;
        }

        OutCommandList.PopClip(MaxLayerId);
        return MaxLayerId;
    }

    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override
    {
        const FKey Key = CursorEvent.GetKey();

        bIsPanning  = Key == Keys::MouseButtonMiddle || (Key == Keys::MouseButtonLeft && CursorEvent.GetModifierKeys().IsShiftDown());
        bIsOrbiting = !bIsPanning && (Key == Keys::MouseButtonLeft || Key == Keys::MouseButtonRight);

        if (!bIsPanning && !bIsOrbiting)
        {
            return FEventResponse::Unhandled();
        }

        LastCursorPosition = CursorEvent.GetClientPosition();
        return FEventResponse::Handled();
    }

    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override
    {
        UNREFERENCED_VARIABLE(CursorEvent);

        if (!bIsOrbiting && !bIsPanning)
        {
            return FEventResponse::Unhandled();
        }

        bIsOrbiting = false;
        bIsPanning  = false;

        return FEventResponse::Handled();
    }

    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override
    {
        const IntVector2 ClientPosition = CursorEvent.GetClientPosition();
        const IntVector2 Step           = ClientPosition - LastCursorPosition;

        LastCursorPosition = ClientPosition;

        if (bIsOrbiting)
        {
            YawDegrees   -= static_cast<float>(Step.X) * ORBIT_DEGREES_PER_PIXEL;
            PitchDegrees += static_cast<float>(Step.Y) * ORBIT_DEGREES_PER_PIXEL;
            PitchDegrees  = Math::Clamp(PitchDegrees, -MAX_CAMERA_PITCH_DEGREES, MAX_CAMERA_PITCH_DEGREES);
        }
        else if (bIsPanning)
        {
            const Matrix4 InverseView = GetViewMatrix().GetInverse();
            const Vector4 Right       = InverseView.GetRow(0);
            const Vector4 Up          = InverseView.GetRow(1);

            const float PixelsToWorld = Distance * 0.002f;

            Target -= Vector3(Right.X, Right.Y, Right.Z) * (static_cast<float>(Step.X) * PixelsToWorld);
            Target += Vector3(Up.X, Up.Y, Up.Z) * (static_cast<float>(Step.Y) * PixelsToWorld);
        }
        else
        {
            RefreshGizmoReadout();
            return FEventResponse::Unhandled();
        }

        PushCamera();
        return FEventResponse::Handled();
    }

    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) override
    {
        const float Delta = CursorEvent.GetScrollDelta();
        if (Delta == 0.0f)
        {
            return FEventResponse::Unhandled();
        }

        Distance = Math::Clamp(Distance * (Delta > 0.0f ? 0.9f : (1.0f / 0.9f)), MIN_CAMERA_DISTANCE, MAX_CAMERA_DISTANCE);

        PushCamera();
        return FEventResponse::Handled();
    }

    /** @brief The transform the cube is drawn with, which is the one the gizmo edits. */
    void SetTransform(const Matrix4& InTransform)
    {
        Transform = InTransform;

        if (Gizmo)
        {
            Gizmo->SetTransform(Transform);
        }
    }

    /** @brief Back to where the camera started, which is the Frame action. */
    void ResetCamera()
    {
        Target       = Vector3(0.0f, 0.0f, 0.0f);
        YawDegrees   = 35.0f;
        PitchDegrees = 25.0f;
        Distance     = 9.0f;

        PushCamera();
    }

private:
    NODISCARD Vector3 GetCameraPosition() const
    {
        const float Yaw   = Math::DegreesToRadians(YawDegrees);
        const float Pitch = Math::DegreesToRadians(PitchDegrees);

        const Vector3 Offset(
            Math::Sin(Yaw) * Math::Cos(Pitch),
            Math::Sin(Pitch),
            -Math::Cos(Yaw) * Math::Cos(Pitch));

        return Target + (Offset * Distance);
    }

    NODISCARD Matrix4 GetViewMatrix() const
    {
        return Matrix4::LookAt(GetCameraPosition(), Target, Vector3(0.0f, 1.0f, 0.0f));
    }

    NODISCARD Matrix4 GetProjectionMatrix() const
    {
        const FRectangle& Bounds      = GetContentRectangle();
        const float       AspectRatio = Bounds.Height > 0 ? (static_cast<float>(Bounds.Width) / static_cast<float>(Bounds.Height)) : 1.0f;

        return Matrix4::PerspectiveProjection(Math::Constants::PI * 0.25f, AspectRatio, 0.1f, 500.0f);
    }

    void PushCamera()
    {
        if (Gizmo)
        {
            Gizmo->SetCamera(GetViewMatrix(), GetProjectionMatrix(), false);
        }
    }

    struct FCameraFrame
    {
        Matrix4    ViewProjection;
        Vector3    Position;
        Vector3    Forward;
        FRectangle Viewport;
    };

    NODISCARD FCameraFrame GetCameraFrame(const FRectangle& Bounds) const
    {
        FCameraFrame Frame;
        Frame.ViewProjection = GetViewMatrix() * GetProjectionMatrix();
        Frame.Position       = GetCameraPosition();
        Frame.Forward        = (Target - Frame.Position).GetNormalized();
        Frame.Viewport       = Bounds;

        return Frame;
    }

    NODISCARD bool ProjectSegment(const FCameraFrame& Frame, const Vector3& Start, const Vector3& End, Vector2& OutStart, Vector2& OutEnd) const
    {
        constexpr float NearDepth = 0.2f;

        const float StartDepth = (Start - Frame.Position).DotProduct(Frame.Forward);
        const float EndDepth   = (End - Frame.Position).DotProduct(Frame.Forward);

        if (StartDepth < NearDepth && EndDepth < NearDepth)
        {
            return false;
        }

        Vector3 NearEnd = Start;
        Vector3 FarEnd  = End;

        if (StartDepth < NearDepth || EndDepth < NearDepth)
        {
            const float   Fraction = (NearDepth - StartDepth) / (EndDepth - StartDepth);
            const Vector3 OnPlane  = NearEnd + ((FarEnd - NearEnd) * Fraction);

            if (StartDepth < NearDepth)
            {
                NearEnd = OnPlane;
            }
            else
            {
                FarEnd = OnPlane;
            }
        }

        return FGizmoMath::WorldToClient(Frame.ViewProjection, Frame.Viewport, NearEnd, OutStart)
            && FGizmoMath::WorldToClient(Frame.ViewProjection, Frame.Viewport, FarEnd, OutEnd);
    }

    void DrawWorldLine(const FCameraFrame& Frame, FDrawCommandList& OutCommandList, int32 LayerId, const Vector3& Start, const Vector3& End, const FFloatColor& Tint, float Thickness) const
    {
        Vector2 ClientStart;
        Vector2 ClientEnd;

        if (ProjectSegment(Frame, Start, End, ClientStart, ClientEnd))
        {
            OutCommandList.AddLine(LayerId, ClientStart, ClientEnd, Tint, Thickness);
        }
    }

    void DrawGround(const FCameraFrame& Frame, FDrawCommandList& OutCommandList, int32 LayerId) const
    {
        const FFloatColor GridTint(0.22f, 0.24f, 0.28f, 1.0f);
        const FFloatColor AxisTintX(0.68f, 0.28f, 0.30f, 1.0f);
        const FFloatColor AxisTintZ(0.28f, 0.42f, 0.68f, 1.0f);

        const float Extent = static_cast<float>(GROUND_EXTENT);

        for (int32 Step = -GROUND_EXTENT; Step <= GROUND_EXTENT; ++Step)
        {
            const float Offset = static_cast<float>(Step);

            const bool bIsOrigin = Step == 0;

            DrawWorldLine(Frame, OutCommandList, LayerId,
                Vector3(Offset, 0.0f, -Extent), Vector3(Offset, 0.0f, Extent),
                bIsOrigin ? AxisTintZ : GridTint, bIsOrigin ? 1.6f : 1.0f);

            DrawWorldLine(Frame, OutCommandList, LayerId,
                Vector3(-Extent, 0.0f, Offset), Vector3(Extent, 0.0f, Offset),
                bIsOrigin ? AxisTintX : GridTint, bIsOrigin ? 1.6f : 1.0f);
        }
    }

    void DrawCube(const FCameraFrame& Frame, FDrawCommandList& OutCommandList, int32 LayerId) const
    {
        static const Vector3 Corners[8] =
        {
            Vector3(-0.5f, -0.5f, -0.5f), Vector3( 0.5f, -0.5f, -0.5f),
            Vector3( 0.5f,  0.5f, -0.5f), Vector3(-0.5f,  0.5f, -0.5f),
            Vector3(-0.5f, -0.5f,  0.5f), Vector3( 0.5f, -0.5f,  0.5f),
            Vector3( 0.5f,  0.5f,  0.5f), Vector3(-0.5f,  0.5f,  0.5f),
        };

        static const int32 Edges[12][2] =
        {
            { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
            { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
            { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
        };

        Vector3 World[8];
        for (int32 Index = 0; Index < 8; ++Index)
        {
            World[Index] = Transform.TransformCoord(Corners[Index]);
        }

        const FFloatColor Tint = FUIStyle::GetDefault().Colors.Accent;
        for (const int32 (&Edge)[2] : Edges)
        {
            DrawWorldLine(Frame, OutCommandList, LayerId, World[Edge[0]], World[Edge[1]], Tint, 1.8f);
        }
    }

    TSharedPtr<FGizmo>    Gizmo;
    TSharedPtr<IFontFace> Font;
    Matrix4               Transform;
    Vector3               Target;
    float                 YawDegrees;
    float                 PitchDegrees;
    float                 Distance;
    bool                  bIsOrbiting;
    bool                  bIsPanning;
    IntVector2            LastCursorPosition;
};

static TSharedPtr<FGizmo>         GGizmo;
static TSharedPtr<FGizmoViewport> GViewport;
static TSharedPtr<FTextBlock>     GGizmoReadout;
static TSharedPtr<FToolBarButton> GOperationButtons[4];
static TSharedPtr<FToolBarButton> GModeButtons[2];

static const CHAR* GetHandleName(EGizmoHandle Handle)
{
    switch (Handle)
    {
        case EGizmoHandle::TranslateX:      return "Translate X";
        case EGizmoHandle::TranslateY:      return "Translate Y";
        case EGizmoHandle::TranslateZ:      return "Translate Z";
        case EGizmoHandle::TranslateYZ:     return "Translate YZ";
        case EGizmoHandle::TranslateZX:     return "Translate ZX";
        case EGizmoHandle::TranslateXY:     return "Translate XY";
        case EGizmoHandle::TranslateScreen: return "Translate screen";
        case EGizmoHandle::RotateX:         return "Rotate X";
        case EGizmoHandle::RotateY:         return "Rotate Y";
        case EGizmoHandle::RotateZ:         return "Rotate Z";
        case EGizmoHandle::RotateScreen:    return "Rotate screen";
        case EGizmoHandle::ScaleX:          return "Scale X";
        case EGizmoHandle::ScaleY:          return "Scale Y";
        case EGizmoHandle::ScaleZ:          return "Scale Z";
        case EGizmoHandle::ScaleYZ:         return "Scale YZ";
        case EGizmoHandle::ScaleZX:         return "Scale ZX";
        case EGizmoHandle::ScaleXY:         return "Scale XY";
        case EGizmoHandle::ScaleUniform:    return "Scale uniform";
        default:                            return "nothing";
    }
}

static Vector3 GetTransformScale(const Matrix4& Transform)
{
    const Vector4 Rows[3] = { Transform.GetRow(0), Transform.GetRow(1), Transform.GetRow(2) };

    return Vector3(
        Vector3(Rows[0].X, Rows[0].Y, Rows[0].Z).GetLength(),
        Vector3(Rows[1].X, Rows[1].Y, Rows[1].Z).GetLength(),
        Vector3(Rows[2].X, Rows[2].Y, Rows[2].Z).GetLength());
}

static void RefreshGizmoReadout()
{
    if (!GGizmoReadout || !GGizmo)
    {
        return;
    }

    const Matrix4 Transform   = GGizmo->GetTransform();
    const Vector3 Translation = Transform.GetTranslation();
    const Vector3 Scale       = GetTransformScale(Transform);

    const EGizmoHandle Handle = GGizmo->IsDragging() ? GGizmo->GetActiveHandle() : GGizmo->GetHoveredHandle();

    GGizmoReadout->SetText(String::Printf(
        "position  %7.2f %7.2f %7.2f\n"
        "scale     %7.2f %7.2f %7.2f\n"
        "%-9s %s",
        Translation.X, Translation.Y, Translation.Z,
        Scale.X, Scale.Y, Scale.Z,
        GGizmo->IsDragging() ? "dragging" : "over", GetHandleName(Handle)));
}

static void SetOperation(EGizmoOperation Operation)
{
    GGizmo->SetOperation(Operation);

    for (int32 Index = 0; Index < 4; ++Index)
    {
        if (GOperationButtons[Index])
        {
            GOperationButtons[Index]->SetCheckState(static_cast<int32>(Operation) == Index ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
        }
    }

    RefreshGizmoReadout();
}

static void SetMode(EGizmoMode Mode)
{
    GGizmo->SetMode(Mode);

    for (int32 Index = 0; Index < 2; ++Index)
    {
        if (GModeButtons[Index])
        {
            GModeButtons[Index]->SetCheckState(static_cast<int32>(Mode) == Index ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
        }
    }

    RefreshGizmoReadout();
}

static TSharedPtr<FVisualElement> MakeGizmoViewport(const FPlaygroundFonts& Fonts)
{
    FGizmo::FDesc Desc;
    Desc.Operation = EGizmoOperation::Translate;
    Desc.Mode      = EGizmoMode::Local;
    Desc.Font      = Fonts.Monospace;

    Desc.OnTransformChanged = FOnGizmoTransformChanged::CreateLambda([](const Matrix4& NewTransform, const Matrix4& Delta)
    {
        UNREFERENCED_VARIABLE(Delta);

        GViewport->SetTransform(NewTransform);
        RefreshGizmoReadout();
    });

    Desc.OnDragFinished = FOnGizmoDragFinished::CreateLambda([](const Matrix4& TransformAtDragStart, const Matrix4& Transform)
    {
        UNREFERENCED_VARIABLE(TransformAtDragStart);
        UNREFERENCED_VARIABLE(Transform);

        RefreshGizmoReadout();
    });

    GGizmo    = FGizmo::Create(Desc);
    GViewport = MakeSharedPtr<FGizmoViewport>();

    GViewport->Initialize(GGizmo, Fonts.Monospace);

    FBorder::FDesc BorderDesc;
    BorderDesc.BorderColor     = FUIStyle::GetDefault().Colors.Border;
    BorderDesc.BorderThickness = FUIStyle::GetDefault().Metrics.BorderThickness;
    BorderDesc.Content         = GViewport;

    return FBorder::Create(BorderDesc);
}

static TSharedPtr<FVisualElement> MakeGizmoControls(const FPlaygroundFonts& Fonts)
{
    FTextBlock::FDesc ReadoutDesc;
    ReadoutDesc.Text            = "";
    ReadoutDesc.Font            = Fonts.Monospace;
    ReadoutDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

    GGizmoReadout = FTextBlock::Create(ReadoutDesc);

    FToolBar::FDesc ToolBarDesc;
    ToolBarDesc.Font           = Fonts.Body;
    ToolBarDesc.bHasBackground = false;

    TSharedPtr<FToolBar> ToolBar = FToolBar::Create(ToolBarDesc);

    struct FOperationEntry
    {
        const CHAR*     Label;
        const CHAR*     ToolTipText;
        EGizmoOperation Operation;
    };

    const FOperationEntry Operations[] =
    {
        { "Translate", "Arrows, plane quads and the sphere at the pivot",   EGizmoOperation::Translate },
        { "Rotate",    "Three rings and the outer one facing the camera",   EGizmoOperation::Rotate    },
        { "Scale",     "A knob per axis, plane quads and the one at the pivot for all", EGizmoOperation::Scale },
        { "Universal", "Every knob scales all three axes together",         EGizmoOperation::UniversalScale },
    };

    for (const FOperationEntry& Entry : Operations)
    {
        const EGizmoOperation Operation = Entry.Operation;

        GOperationButtons[static_cast<int32>(Operation)] = ToolBar->AddToggle(
            FToolBarItemDesc().SetLabel(Entry.Label).SetToolTipText(Entry.ToolTipText),
            Operation == EGizmoOperation::Translate ? ECheckBoxState::Checked : ECheckBoxState::Unchecked,
            FOnCheckStateChanged::CreateLambda([Operation](ECheckBoxState NewState)
            {
                UNREFERENCED_VARIABLE(NewState);
                SetOperation(Operation);
            }));
    }

    ToolBar->AddSeparator();

    struct FModeEntry
    {
        const CHAR* Label;
        const CHAR* ToolTipText;
        EGizmoMode  Mode;
    };

    const FModeEntry Modes[] =
    {
        { "Local", "Handles follow the transform's own axes", EGizmoMode::Local },
        { "World", "Handles stay on the world axes",          EGizmoMode::World },
    };

    for (const FModeEntry& Entry : Modes)
    {
        const EGizmoMode Mode = Entry.Mode;

        GModeButtons[static_cast<int32>(Mode)] = ToolBar->AddToggle(
            FToolBarItemDesc().SetLabel(Entry.Label).SetToolTipText(Entry.ToolTipText),
            Mode == EGizmoMode::Local ? ECheckBoxState::Checked : ECheckBoxState::Unchecked,
            FOnCheckStateChanged::CreateLambda([Mode](ECheckBoxState NewState)
            {
                UNREFERENCED_VARIABLE(NewState);
                SetMode(Mode);
            }));
    }

    ToolBar->AddSeparator();

    ToolBar->AddToggle(
        FToolBarItemDesc().SetLabel("Snap").SetToolTipText("Quarter units, fifteen degrees and quarter steps of scale"),
        ECheckBoxState::Unchecked,
        FOnCheckStateChanged::CreateLambda([](ECheckBoxState NewState)
        {
            FGizmoSnapSettings Snap;
            if (NewState == ECheckBoxState::Checked)
            {
                Snap.TranslationSnap     = 0.25f;
                Snap.RotationSnapDegrees = 15.0f;
                Snap.ScaleSnap           = 0.25f;
            }

            GGizmo->SetSnap(Snap);
        }));

    ToolBar->AddSeparator();

    ToolBar->AddButton(
        FToolBarItemDesc().SetLabel("Reset cube").SetToolTipText("Back to the identity, at the origin"),
        FOnClicked::CreateLambda([]()
        {
            GViewport->SetTransform(Matrix4::Identity());
            RefreshGizmoReadout();
        }));

    ToolBar->AddButton(
        FToolBarItemDesc().SetLabel("Frame").SetToolTipText("Back to the camera the page opened with"),
        FOnClicked::CreateLambda([]()
        {
            GViewport->ResetCamera();
        }));

    FBorder::FDesc FrameDesc;
    FrameDesc.BackgroundColor = FUIStyle::GetDefault().Colors.PanelBackground;
    FrameDesc.BorderColor     = FUIStyle::GetDefault().Colors.Border;
    FrameDesc.BorderThickness = FUIStyle::GetDefault().Metrics.BorderThickness;
    FrameDesc.CornerRadius    = FUIStyle::GetDefault().Metrics.CornerRadius;
    FrameDesc.Content         = ToolBar;

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FBorder::Create(FrameDesc)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    Column->AddSlot(GGizmoReadout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);

    RefreshGizmoReadout();
    return Column;
}

FPlaygroundScene CreateGizmoScene(const FPlaygroundFonts& Fonts)
{
    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Transform gizmo";
        Desc.Description = "There is no renderer behind this: the ground and the cube are world points run through the same projection the gizmo picks with, drawn as lines. Drag a handle to edit the cube, drag the background to orbit, hold Shift or the middle button to slide the camera, and use the wheel to come in and out.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeGizmoViewport(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "What the gizmo is showing";
        Desc.Description = "The operation decides which handles appear, and the mode decides whether they follow the cube or the world. Scale ignores the mode and always runs along the cube's own axes, because scaling a turned transform along a world axis shears it into something no scale could have produced.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeGizmoControls(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene("Gizmo", MakeSceneColumn("Gizmo", Fonts, Panels));
}
