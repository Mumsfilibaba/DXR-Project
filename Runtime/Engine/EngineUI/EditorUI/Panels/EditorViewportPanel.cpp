#include "Engine/EngineUI/EditorUI/Panels/EditorViewportPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorViewportImage.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorViewportSurface.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Engine/EngineUI/Editor/EditorActorFactory.h"
#include "Engine/EngineUI/Editor/EditorCameraController.h"
#include "Engine/EditorEngine.h"
#include "Engine/World/World.h"
#include "Engine/World/SceneViewport.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Engine/World/Components/DirectionalLightComponent.h"
#include "Engine/World/Components/LightProbeComponent.h"
#include "Engine/World/Components/PointLightComponent.h"
#include "Engine/World/Components/StaticMeshComponent.h"
#include "Application/Application.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/CheckBox.h"
#include "Application/Elements/Slider.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Elements/ToolBar.h"
#include "Application/Elements/Viewport.h"
#include "Application/Elements/Window.h"
#include "Application/Gizmo/Gizmo.h"
#include "Application/Menus/ComboBox.h"
#include "Application/Menus/Menu.h"
#include "Application/Menus/MenuItem.h"
#include "Application/Menus/MenuStack.h"
#include "RendererCore/Interfaces/IRendererModule.h"

// Wide enough for the camera menu's captions and slider tracks to sit side by side without crowding
constexpr int32 CAMERA_MENU_WIDTH = 260;

struct FDebugViewEntry
{
    const CHAR*                  Label;
    FSceneRenderView::EDebugView View;
};

// Listed in the groups the ImGui viewport shows them in: the base views, then the CSM views, then the ray-tracing views.
constexpr FDebugViewEntry DEBUG_VIEW_ENTRIES[] =
{
    { "Lit",                         FSceneRenderView::EDebugView::None },
    { "Shadow Mask",                 FSceneRenderView::EDebugView::ShadowMask },
    { "GBuffer: Albedo",             FSceneRenderView::EDebugView::GBufferAlbedo },
    { "GBuffer: Normal",             FSceneRenderView::EDebugView::GBufferNormal },
    { "GBuffer: Material",           FSceneRenderView::EDebugView::GBufferMaterial },
    { "GBuffer: Velocity",           FSceneRenderView::EDebugView::GBufferVelocity },
    { "SSAO",                        FSceneRenderView::EDebugView::SSAO },
    { "Depth",                       FSceneRenderView::EDebugView::Depth },
    { "CSM Cascades (2x2)",          FSceneRenderView::EDebugView::ShadowCascades },
    { "CSM Cascade Index",           FSceneRenderView::EDebugView::ShadowCascadeIndex },
    { "CSM Cascade Overlay",         FSceneRenderView::EDebugView::ShadowCascadeOverlay },
    { "Reflections Radiance",        FSceneRenderView::EDebugView::RayTracingReflectionsRaw },
    { "Reflections Temporal Filter", FSceneRenderView::EDebugView::RayTracingReflectionsTemporal },
    { "Reflections Spatial Filter",  FSceneRenderView::EDebugView::RayTracingReflectionsSpatial },
    { "Reflections Variance",        FSceneRenderView::EDebugView::RayTracingReflectionsVariance },
    { "Reflections History Length",  FSceneRenderView::EDebugView::RayTracingReflectionsHistory },
    { "Geometry Debug",              FSceneRenderView::EDebugView::RayTracingPrimaryID },
};

constexpr int32 NUM_DEBUG_VIEW_ENTRIES = static_cast<int32>(ARRAY_COUNT(DEBUG_VIEW_ENTRIES));

constexpr EEditorLightType PLACEABLE_LIGHT_TYPES[] =
{
    EEditorLightType::Point,
    EEditorLightType::Spot,
    EEditorLightType::Directional,
    EEditorLightType::Sky,
};

// How far ahead of the camera a spawn lands when the cursor ray misses the ground plane.
constexpr float PLACEMENT_DEFAULT_DISTANCE = 10.0f;

// A ground hit farther out than this is grazing the horizon, so it counts as a miss rather than a placement.
constexpr float PLACEMENT_MAX_GROUND_DISTANCE = 1000.0f;

static int32 FindDebugViewIndex(FSceneRenderView::EDebugView InDebugView)
{
    for (int32 Index = 0; Index < NUM_DEBUG_VIEW_ENTRIES; ++Index)
    {
        if (DEBUG_VIEW_ENTRIES[Index].View == InDebugView)
        {
            return Index;
        }
    }

    return -1;
}

FEditorViewportPanel::FEditorViewportPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "Viewport", "Viewport")
    , CameraController(MakeUniquePtr<FEditorCameraController>())
    , Surface(nullptr)
    , Image(nullptr)
    , Gizmo(nullptr)
    , DebugViewBar(nullptr)
    , DebugViewCombo(nullptr)
    , SecondaryDebugViewCombo(nullptr)
    , PlayItem(nullptr)
    , PauseItem(nullptr)
    , ViewportImage(nullptr)
    , CachedViewportSize(0, 0)
    , ContextMenuViewProjectionInverse(Matrix4::Identity())
    , ContextMenuLocation(0.0f, 0.0f, 0.0f)
    , ContextMenuNdc(0.0f, 0.0f)
    , ContextMenuScreenPosition()
    , ContextMenuPickRequestId(0)
    , DebugView(FSceneRenderView::EDebugView::None)
    , SecondaryDebugView(FSceneRenderView::EDebugView::None)
    , DebugViewChannelMask(FSceneRenderView::EDebugViewChannel::All)
    , GizmoPlacement(EEditorGizmoPlacement::Center)
    , RequestedGizmoOperation(EGizmoOperation::Translate)
{
}

FEditorViewportPanel::~FEditorViewportPanel()
{
}

bool FEditorViewportPanel::Initialize()
{
    Image = FEditorViewportImage::Create();
    if (!Image)
    {
        return false;
    }

    FGizmo::FDesc GizmoDesc;
    GizmoDesc.Operation          = EGizmoOperation::Translate;
    GizmoDesc.Mode               = EGizmoMode::World;
    GizmoDesc.Font               = FEditorStyle::GetFonts().Body;
    GizmoDesc.OnTransformChanged = FOnGizmoTransformChanged::CreateRaw(this, &FEditorViewportPanel::OnGizmoTransformChanged);
    GizmoDesc.OnDragFinished     = FOnGizmoDragFinished::CreateRaw(this, &FEditorViewportPanel::OnGizmoDragFinished);

    Gizmo = FGizmo::Create(GizmoDesc);
    if (!Gizmo)
    {
        return false;
    }

    DebugViewBar = BuildToolBar();

    Surface = FEditorViewportSurface::Create();
    if (!Surface)
    {
        return false;
    }

    Surface->SetLayers(Image, EditorEngine->GetViewport(), Gizmo, DebugViewBar);
    Surface->OnClickedDelegate       = FOnViewportClicked::CreateRaw(this, &FEditorViewportPanel::OnViewportClicked);
    Surface->OnContextMenuDelegate   = FOnViewportContextMenu::CreateRaw(this, &FEditorViewportPanel::OnViewportContextMenu);
    Surface->OnMarqueeSelectDelegate = FOnViewportMarqueeSelect::CreateRaw(this, &FEditorViewportPanel::OnViewportMarqueeSelect);
    Surface->OnShortcutDelegate      = FOnViewportShortcut::CreateRaw(this, &FEditorViewportPanel::OnViewportShortcut);

    if (const TSharedPtr<FSceneViewport> SceneViewport = EditorEngine->GetSceneViewport())
    {
        SceneViewport->SetPlayerInputEnabled(false);
    }

    Content = Surface;
    return true;
}

void FEditorViewportPanel::Release()
{
    if (Surface)
    {
        Surface->SetLayers(nullptr, nullptr, nullptr, nullptr);
    }

    PauseItem.Reset();
    PlayItem.Reset();
    SecondaryDebugViewCombo.Reset();
    DebugViewCombo.Reset();
    DebugViewBar.Reset();
    Gizmo.Reset();
    Image.Reset();
    Surface.Reset();
    ViewportImage.Reset();

    CameraController.Reset();

    FEditorPanel::Release();
}

TSharedPtr<FToolBar> FEditorViewportPanel::BuildToolBar()
{
    FToolBar::FDesc Desc;
    Desc.Font           = FEditorStyle::GetFonts().Body;
    Desc.IconSize       = FEditorStyle::IconSize;
    Desc.bHasBackground = true;

    TSharedPtr<FToolBar> Bar = FToolBar::Create(Desc);
    if (!Bar)
    {
        return nullptr;
    }

    PlayItem = Bar->AddButton(FToolBarItemDesc().SetLabel("Play").SetToolTipText("Run the world in the editor"),
        FOnClicked::CreateRaw(this, &FEditorViewportPanel::TogglePlay));

    PauseItem = Bar->AddToggle(FToolBarItemDesc().SetLabel("Pause").SetToolTipText("Freeze the running world"), ECheckBoxState::Unchecked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState)
        {
            EditorEngine->TogglePause();
            RefreshTransportItems();
        }));

    RefreshTransportItems();

    Bar->AddSeparator();

    Bar->AddToggle(FToolBarItemDesc().SetLabel("Move").SetToolTipText("Translate the selection"), ECheckBoxState::Checked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            if (State == ECheckBoxState::Checked)
            {
                SetGizmoOperation(EGizmoOperation::Translate);
            }
        }));

    Bar->AddToggle(FToolBarItemDesc().SetLabel("Rotate").SetToolTipText("Rotate the selection"), ECheckBoxState::Unchecked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            if (State == ECheckBoxState::Checked)
            {
                SetGizmoOperation(EGizmoOperation::Rotate);
            }
        }));

    Bar->AddToggle(FToolBarItemDesc().SetLabel("Scale").SetToolTipText("Scale the selection"), ECheckBoxState::Unchecked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            if (State == ECheckBoxState::Checked)
            {
                SetGizmoOperation(EGizmoOperation::Scale);
            }
        }));

    Bar->AddSeparator();

    Bar->AddToggle(FToolBarItemDesc().SetLabel("World").SetToolTipText("Align the handles to the world axes rather than to the selection"),
        ECheckBoxState::Checked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            SetGizmoMode(State == ECheckBoxState::Checked ? EGizmoMode::World : EGizmoMode::Local);
        }));

    Bar->AddToggle(FToolBarItemDesc().SetLabel("Center").SetToolTipText("Put the handles at the bounds centre rather than at the pivot"),
        GizmoPlacement == EEditorGizmoPlacement::Center ? ECheckBoxState::Checked : ECheckBoxState::Unchecked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            SetGizmoPlacement(State == ECheckBoxState::Checked ? EEditorGizmoPlacement::Center : EEditorGizmoPlacement::Pivot);
        }));

    Bar->AddSeparator();

    Bar->AddDropDown(FToolBarItemDesc().SetLabel("Camera").SetToolTipText("Speeds, lens and framing"), BuildCameraMenu());

    Bar->AddSeparator();

    DebugViewCombo = BuildDebugViewCombo();
    if (DebugViewCombo)
    {
        Bar->AddWidget(DebugViewCombo);
    }

    Bar->AddDropDown(FToolBarItemDesc().SetLabel("View").SetToolTipText("Split view and channel mask"), BuildViewOptionsMenu());

    return Bar;
}

void FEditorViewportPanel::RefreshTransportItems()
{
    const bool bIsEditing = EditorEngine->IsEditing();

    if (PlayItem)
    {
        PlayItem->SetLabel(bIsEditing ? "Play" : "Stop");
    }

    if (PauseItem)
    {
        PauseItem->SetEnabled(!bIsEditing);
        PauseItem->SetCheckState(EditorEngine->IsPaused() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }
}

void FEditorViewportPanel::TogglePlay()
{
    if (EditorEngine->IsEditing())
    {
        EditorEngine->StartPlay();
    }
    else
    {
        EditorEngine->StopPlay();
    }

    RefreshTransportItems();
}

TSharedPtr<FComboBox> FEditorViewportPanel::BuildDebugViewCombo()
{
    TArray<String> Options;
    Options.Reserve(NUM_DEBUG_VIEW_ENTRIES);

    for (const FDebugViewEntry& Entry : DEBUG_VIEW_ENTRIES)
    {
        Options.Add(Entry.Label);
    }

    FComboBox::FDesc Desc;
    Desc.Options            = Options;
    Desc.SelectedIndex      = FindDebugViewIndex(DebugView);
    Desc.Font               = FEditorStyle::GetFonts().Body;
    Desc.PlaceholderText    = "Lit";
    Desc.OnSelectionChanged = FOnComboSelectionChanged::CreateLambda([this](int32 SelectedIndex)
    {
        if (SelectedIndex >= 0 && SelectedIndex < NUM_DEBUG_VIEW_ENTRIES)
        {
            SetDebugView(DEBUG_VIEW_ENTRIES[SelectedIndex].View);
        }
    });

    return FComboBox::Create(Desc);
}

TSharedPtr<FComboBox> FEditorViewportPanel::BuildSecondaryDebugViewCombo()
{
    TArray<String> Options;
    Options.Reserve(NUM_DEBUG_VIEW_ENTRIES);

    for (const FDebugViewEntry& Entry : DEBUG_VIEW_ENTRIES)
    {
        Options.Add(Entry.Label);
    }

    FComboBox::FDesc Desc;
    Desc.Options            = Options;
    Desc.SelectedIndex      = FindDebugViewIndex(SecondaryDebugView);
    Desc.Font               = FEditorStyle::GetFonts().Body;
    Desc.PlaceholderText    = "Off";
    Desc.OnSelectionChanged = FOnComboSelectionChanged::CreateLambda([this](int32 SelectedIndex)
    {
        if (SelectedIndex >= 0 && SelectedIndex < NUM_DEBUG_VIEW_ENTRIES)
        {
            SecondaryDebugView = DEBUG_VIEW_ENTRIES[SelectedIndex].View;
        }
    });

    return FComboBox::Create(Desc);
}

TSharedPtr<FVisualElement> FEditorViewportPanel::BuildCameraMenu()
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();
    if (!Menu)
    {
        return nullptr;
    }

    Menu->SetMinDesiredWidth(CAMERA_MENU_WIDTH);

    const auto AddSliderRow = [&Menu, &Font](const CHAR* Label, float MinValue, float MaxValue, float Value,
        int32 Precision, const FOnSliderValueChanged& OnValueChanged)
    {
        FTextBlock::FDesc CaptionDesc;
        CaptionDesc.Text = Label;
        CaptionDesc.Font = Font;

        FSlider::FDesc SliderDesc;
        SliderDesc.MinValue       = MinValue;
        SliderDesc.MaxValue       = MaxValue;
        SliderDesc.Value          = Value;
        SliderDesc.Font           = Font;
        SliderDesc.Precision      = Precision;
        SliderDesc.bShowValueText = true;
        SliderDesc.OnValueChanged = OnValueChanged;

        TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
        Row->AddSlot(FTextBlock::Create(CaptionDesc))
            .SetPadding(FMargin(0, 0, 8, 0))
            .SetVerticalAlignment(EVerticalAlignment::Center);
        Row->AddSlot(FSlider::Create(SliderDesc))
            .SetFillCoefficient(1.0f)
            .SetVerticalAlignment(EVerticalAlignment::Center);

        TSharedPtr<FHorizontalBox> Padded = FHorizontalBox::Create();
        Padded->AddSlot(Row).SetFillCoefficient(1.0f).SetPadding(FMargin(8, 2, 8, 2));

        Menu->AddCustomEntry(Padded);
    };

    FEditorCameraController* Camera = CameraController.Get();

    AddSliderRow("Move", FEditorCameraController::MinMoveSpeed, FEditorCameraController::MaxMoveSpeed, Camera->GetMoveSpeed(), 1,
        FOnSliderValueChanged::CreateLambda([this](float NewValue)
        {
            CameraController->SetMoveSpeed(NewValue);
        }));

    AddSliderRow("Rotate", 1.0f, 20.0f, Camera->GetRotationSpeed(), 1,
        FOnSliderValueChanged::CreateLambda([this](float NewValue)
        {
            CameraController->SetRotationSpeed(NewValue);
        }));

    AddSliderRow("Look", 0.01f, 1.0f, Camera->GetMouseSensitivity(), 2,
        FOnSliderValueChanged::CreateLambda([this](float NewValue)
        {
            CameraController->SetMouseSensitivity(NewValue);
        }));

    AddSliderRow("Pan", 0.1f, 20.0f, Camera->GetPanSpeed(), 1,
        FOnSliderValueChanged::CreateLambda([this](float NewValue)
        {
            CameraController->SetPanSpeed(NewValue);
        }));

    Menu->AddSeparator();

    AddSliderRow("FOV", 20.0f, 120.0f, Camera->GetFieldOfView(), 0,
        FOnSliderValueChanged::CreateLambda([this](float NewValue)
        {
            CameraController->SetFieldOfView(NewValue);
        }));

    AddSliderRow("Near", 0.01f, 10.0f, Camera->GetNearPlane(), 2,
        FOnSliderValueChanged::CreateLambda([this](float NewValue)
        {
            CameraController->SetNearPlane(NewValue);
        }));

    AddSliderRow("Far", 100.0f, 10000.0f, Camera->GetFarPlane(), 0,
        FOnSliderValueChanged::CreateLambda([this](float NewValue)
        {
            CameraController->SetFarPlane(NewValue);
        }));

    Menu->AddSeparator();

    FMenuItem::FDesc ResetDesc;
    ResetDesc.Label       = "Reset";
    ResetDesc.Font        = Font;
    ResetDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this]()
    {
        CameraController->Reset();
    });

    Menu->AddItem(FMenuItem::Create(ResetDesc));

    FMenuItem::FDesc FocusDesc;
    FocusDesc.Label       = "Focus Selected";
    FocusDesc.Font        = Font;
    FocusDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this]()
    {
        FocusOnActor(EditorEngine->GetSelectedActor());
    });

    Menu->AddItem(FMenuItem::Create(FocusDesc));

    FMenuItem::FDesc AttachDesc;
    AttachDesc.Label       = "Attach To Selected";
    AttachDesc.Font        = Font;
    AttachDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this]()
    {
        if (FActor* SelectedActor = EditorEngine->GetSelectedActor())
        {
            CameraController->AttachTo(SelectedActor);
        }
    });

    Menu->AddItem(FMenuItem::Create(AttachDesc));

    FMenuItem::FDesc DetachDesc;
    DetachDesc.Label       = "Detach";
    DetachDesc.Font        = Font;
    DetachDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this]()
    {
        CameraController->Detach();
    });

    Menu->AddItem(FMenuItem::Create(DetachDesc));

    return Menu;
}

TSharedPtr<FVisualElement> FEditorViewportPanel::BuildViewOptionsMenu()
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();
    if (!Menu)
    {
        return nullptr;
    }

    Menu->SetMinDesiredWidth(CAMERA_MENU_WIDTH);

    FTextBlock::FDesc SplitCaptionDesc;
    SplitCaptionDesc.Text = "Split View";
    SplitCaptionDesc.Font = Font;

    TSharedPtr<FHorizontalBox> SplitCaptionRow = FHorizontalBox::Create();
    SplitCaptionRow->AddSlot(FTextBlock::Create(SplitCaptionDesc)).SetPadding(FMargin(8, 4, 8, 0));

    Menu->AddCustomEntry(SplitCaptionRow);

    SecondaryDebugViewCombo = BuildSecondaryDebugViewCombo();
    if (SecondaryDebugViewCombo)
    {
        TSharedPtr<FHorizontalBox> ComboRow = FHorizontalBox::Create();
        ComboRow->AddSlot(SecondaryDebugViewCombo).SetFillCoefficient(1.0f).SetPadding(FMargin(8, 2, 8, 4));

        Menu->AddCustomEntry(ComboRow);
    }

    Menu->AddSeparator();

    FTextBlock::FDesc ChannelCaptionDesc;
    ChannelCaptionDesc.Text = "Channels";
    ChannelCaptionDesc.Font = Font;

    TSharedPtr<FHorizontalBox> ChannelCaptionRow = FHorizontalBox::Create();
    ChannelCaptionRow->AddSlot(FTextBlock::Create(ChannelCaptionDesc)).SetPadding(FMargin(8, 4, 8, 0));

    Menu->AddCustomEntry(ChannelCaptionRow);

    struct FChannelEntry
    {
        const CHAR*                         Label;
        FSceneRenderView::EDebugViewChannel Channel;
    };

    constexpr FChannelEntry CHANNEL_ENTRIES[] =
    {
        { "R", FSceneRenderView::EDebugViewChannel::Red   },
        { "G", FSceneRenderView::EDebugViewChannel::Green },
        { "B", FSceneRenderView::EDebugViewChannel::Blue  },
        { "A", FSceneRenderView::EDebugViewChannel::Alpha },
    };

    TSharedPtr<FHorizontalBox> ChannelRow = FHorizontalBox::Create();

    for (const FChannelEntry& Entry : CHANNEL_ENTRIES)
    {
        const FSceneRenderView::EDebugViewChannel Channel = Entry.Channel;

        FCheckBox::FDesc ChannelDesc;
        ChannelDesc.Text           = Entry.Label;
        ChannelDesc.Font           = Font;
        ChannelDesc.InitialState   = IsEnumFlagSet(DebugViewChannelMask, Channel) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
        ChannelDesc.OnStateChanged = FOnCheckStateChanged::CreateLambda([this, Channel](ECheckBoxState State)
        {
            if (State == ECheckBoxState::Checked)
            {
                DebugViewChannelMask |= Channel;
            }
            else
            {
                DebugViewChannelMask &= ~Channel;
            }
        });

        ChannelRow->AddSlot(FCheckBox::Create(ChannelDesc)).SetPadding(FMargin(0, 0, 8, 0));
    }

    TSharedPtr<FHorizontalBox> PaddedChannelRow = FHorizontalBox::Create();
    PaddedChannelRow->AddSlot(ChannelRow).SetFillCoefficient(1.0f).SetPadding(FMargin(8, 2, 8, 4));

    Menu->AddCustomEntry(PaddedChannelRow);

    return Menu;
}

void FEditorViewportPanel::Tick(float DeltaTime)
{
    if (!Surface)
    {
        return;
    }

    RefreshTransportItems();
    Surface->SetPlayBorderVisible(!EditorEngine->IsEditing());

    const FRectangle Bounds = Surface->GetContentRectangle();

    CachedViewportSize = IntVector2(Math::Max(Bounds.Width, 0), Math::Max(Bounds.Height, 0));

    if (EditorEngine->IsEditing())
    {
        CameraController->UpdateProjection(CachedViewportSize);
        CameraController->Tick(DeltaTime, Surface->GetCameraInput(), EditorEngine->GetSelectedActor());
        Surface->ConsumeCameraInputDeltas();
    }

    UpdateGizmoCamera();
    UpdateGizmoFromSelection();
}

void FEditorViewportPanel::UpdateGizmoCamera()
{
    if (!Gizmo)
    {
        return;
    }

    if (const FCameraComponent* Camera = GetViewCamera())
    {
        Gizmo->SetCamera(Camera->GetViewMatrix(), Camera->GetProjectionMatrix(), false);
    }
}

void FEditorViewportPanel::UpdateGizmoFromSelection()
{
    if (!Gizmo)
    {
        return;
    }

    const TArray<FActor*>& Selection = EditorEngine->GetSelectedActors();

    const bool bShow = !Selection.IsEmpty() && EditorEngine->IsEditing() && DebugView == FSceneRenderView::EDebugView::None;
    Gizmo->SetVisibility(bShow ? EVisibility::Visible : EVisibility::Hidden);

    if (!bShow || Gizmo->IsDragging())
    {
        return;
    }

    Gizmo->SetOperation(ResolveGizmoOperation(Selection));

    Matrix4 Pivot = Matrix4::Identity();

    if (Gizmo->GetMode() == EGizmoMode::Local && Selection.Size() == 1)
    {
        Pivot = Matrix4::RotationRollPitchYaw(Selection[0]->GetWorldTransform().GetRotation());
    }

    Pivot.SetTranslation(ComputeGizmoLocation(Selection));
    Gizmo->SetTransform(Pivot);
}

Vector3 FEditorViewportPanel::ComputeGizmoLocation(const TArray<FActor*>& Selection) const
{
    Vector3 Center     = Vector3(0.0f, 0.0f, 0.0f);
    int32   PointCount = 0;

    for (FActor* Actor : Selection)
    {
        if (!Actor)
        {
            continue;
        }

        Center = Center + GetActorGizmoPoint(Actor);
        PointCount++;
    }

    return PointCount > 0 ? Center / static_cast<float>(PointCount) : Center;
}

Vector3 FEditorViewportPanel::GetActorGizmoPoint(FActor* Actor) const
{
    const Matrix4 ActorModel = Actor->GetWorldTransform().GetTransformMatrix();

    if (GizmoPlacement == EEditorGizmoPlacement::Center)
    {
        if (FStaticMeshComponent* MeshComponent = Actor->GetComponentOfType<FStaticMeshComponent>())
        {
            const TSharedPtr<FMesh> Mesh = MeshComponent->GetMesh();
            if (Mesh && Mesh->GetVertexCount() > 0)
            {
                return ActorModel.Transform(Mesh->GetAABB().GetCenter());
            }
        }
    }

    return ActorModel.GetTranslation();
}

void FEditorViewportPanel::OnGizmoTransformChanged(const Matrix4& /*NewTransform*/, const Matrix4& Delta)
{
    const TArray<FActor*>& Selection = EditorEngine->GetSelectedActors();
    for (FActor* Actor : Selection)
    {
        if (!Actor)
        {
            continue;
        }

        Actor->SetWorldTransformMatrix(Actor->GetWorldTransform().GetTransformMatrix() * Delta);

        if (FCameraComponent* CameraComponent = Actor->GetComponentOfType<FCameraComponent>())
        {
            CameraComponent->SetRotation(Actor->GetTransform().GetRotation());
        }
    }
}

void FEditorViewportPanel::OnGizmoDragFinished(const Matrix4& /*TransformAtDragStart*/, const Matrix4& /*Transform*/)
{
    UpdateGizmoFromSelection();
}

void FEditorViewportPanel::OnViewportClicked(const IntVector2& ImagePosition, bool bAdditive)
{
    if (!EditorEngine->IsEditing() || CachedViewportSize.X <= 0 || CachedViewportSize.Y <= 0)
    {
        return;
    }

    const uint32 PixelX = static_cast<uint32>(Math::Clamp(ImagePosition.X, 0, CachedViewportSize.X - 1));
    const uint32 PixelY = static_cast<uint32>(Math::Clamp(ImagePosition.Y, 0, CachedViewportSize.Y - 1));

    EditorEngine->SetPendingPickAdditive(bAdditive);
    EditorEngine->RequestPick(PixelX, PixelY, EEditorPickPurpose::Selection);
}

void FEditorViewportPanel::OnViewportContextMenu(const IntVector2& ImagePosition, const IntVector2& ScreenPosition)
{
    if (!EditorEngine->IsEditing() || CachedViewportSize.X <= 0 || CachedViewportSize.Y <= 0)
    {
        return;
    }

    const uint32 PixelX = static_cast<uint32>(Math::Clamp(ImagePosition.X, 0, CachedViewportSize.X - 1));
    const uint32 PixelY = static_cast<uint32>(Math::Clamp(ImagePosition.Y, 0, CachedViewportSize.Y - 1));

    ContextMenuScreenPosition = ScreenPosition;
    ContextMenuNdc            = Vector2(
        ((static_cast<float>(PixelX) + 0.5f) / static_cast<float>(CachedViewportSize.X)) * 2.0f - 1.0f,
        1.0f - ((static_cast<float>(PixelY) + 0.5f) / static_cast<float>(CachedViewportSize.Y)) * 2.0f);

    if (const FCameraComponent* Camera = GetViewCamera())
    {
        ContextMenuViewProjectionInverse = Camera->GetViewProjectionInverseMatrix();
    }

    if (!ComputeFallbackPlacement(ContextMenuNdc, ContextMenuLocation))
    {
        ContextMenuLocation = Vector3(0.0f, 0.0f, 0.0f);
    }

    ContextMenuPickRequestId = EditorEngine->RequestPick(PixelX, PixelY, EEditorPickPurpose::ContextMenu);

    ShowContextMenu();
}

void FEditorViewportPanel::OnViewportMarqueeSelect(const IntVector2& ImageMin, const IntVector2& ImageMax, bool bAdditive)
{
    if (!EditorEngine->IsEditing() || CachedViewportSize.X <= 0 || CachedViewportSize.Y <= 0)
    {
        return;
    }

    FWorld* World = EditorEngine->GetWorld();
    if (!World)
    {
        return;
    }

    IRendererModule* RendererModule = IRendererModule::Get();
    if (!RendererModule)
    {
        return;
    }

    const uint32 MinPixelX = static_cast<uint32>(Math::Clamp(ImageMin.X, 0, CachedViewportSize.X - 1));
    const uint32 MinPixelY = static_cast<uint32>(Math::Clamp(ImageMin.Y, 0, CachedViewportSize.Y - 1));
    const uint32 MaxPixelX = static_cast<uint32>(Math::Clamp(ImageMax.X, 0, CachedViewportSize.X - 1));
    const uint32 MaxPixelY = static_cast<uint32>(Math::Clamp(ImageMax.Y, 0, CachedViewportSize.Y - 1));

    EditorEngine->SetPendingRectPickAdditive(bAdditive);

    RendererModule->RequestEditorObjectPickRect(World->GetSceneInterface(), MinPixelX, MinPixelY, MaxPixelX, MaxPixelY);
}

void FEditorViewportPanel::ShowContextMenu()
{
    if (!Surface || !FApplication::IsInitialized())
    {
        return;
    }

    TSharedPtr<FWindow> OwningWindow = FApplication::Get().FindWindow(Surface);
    if (!OwningWindow)
    {
        return;
    }

    TSharedPtr<FMenu> Menu = BuildContextMenu();
    if (!Menu)
    {
        return;
    }

    FMenuStack::Get().PushMenu(OwningWindow, FRectangle(ContextMenuScreenPosition, 0, 0), EMenuPlacement::AtCursor, Menu);
}

TSharedPtr<FMenu> FEditorViewportPanel::BuildContextMenu()
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();
    if (!Menu)
    {
        return nullptr;
    }

    FMenuItem::FDesc PlaceDesc;
    PlaceDesc.Label   = "Place Actor";
    PlaceDesc.Font    = Font;
    PlaceDesc.SubMenu = BuildPlaceActorMenu();

    Menu->AddItem(FMenuItem::Create(PlaceDesc));

    Menu->AddSeparator();

    FMenuItem::FDesc FocusDesc;
    FocusDesc.Label        = "Focus Selected";
    FocusDesc.ShortcutText = "F";
    FocusDesc.Font         = Font;
    FocusDesc.OnActivated  = FOnMenuItemActivated::CreateLambda([this]()
    {
        FocusOnActor(EditorEngine->GetSelectedActor());
    });

    Menu->AddItem(FMenuItem::Create(FocusDesc));

    FMenuItem::FDesc DeleteDesc;
    DeleteDesc.Label        = "Delete Selected";
    DeleteDesc.ShortcutText = "Del";
    DeleteDesc.Font         = Font;
    DeleteDesc.OnActivated  = FOnMenuItemActivated::CreateLambda([this]()
    {
        EditorEngine->RequestDeleteActors(EditorEngine->GetSelectedActors());
    });

    Menu->AddItem(FMenuItem::Create(DeleteDesc));

    return Menu;
}

TSharedPtr<FMenu> FEditorViewportPanel::BuildPlaceActorMenu()
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    FWorld* World = EditorEngine->GetWorld();

    TSharedPtr<FMenu> Menu = FMenu::Create();
    if (!Menu)
    {
        return nullptr;
    }

    TSharedPtr<FMenu> MeshMenu = FMenu::Create();
    for (int32 Index = 0; Index < static_cast<int32>(EEditorPrimitiveType::Count); ++Index)
    {
        const EEditorPrimitiveType Type = static_cast<EEditorPrimitiveType>(Index);

        FMenuItem::FDesc PrimitiveDesc;
        PrimitiveDesc.Label       = EditorActorFactory::GetPrimitiveName(Type);
        PrimitiveDesc.Font        = Font;
        PrimitiveDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this, World, Type]()
        {
            SelectSpawnedActor(EditorActorFactory::SpawnPrimitive(World, Type, ContextMenuLocation));
        });

        MeshMenu->AddItem(FMenuItem::Create(PrimitiveDesc));
    }

    FMenuItem::FDesc MeshDesc;
    MeshDesc.Label   = "Mesh";
    MeshDesc.Font    = Font;
    MeshDesc.SubMenu = MeshMenu;

    Menu->AddItem(FMenuItem::Create(MeshDesc));

    TSharedPtr<FMenu> LightMenu = FMenu::Create();
    for (const EEditorLightType Type : PLACEABLE_LIGHT_TYPES)
    {
        if (!EditorActorFactory::CanSpawnLight(World, Type))
        {
            continue;
        }

        FMenuItem::FDesc LightItemDesc;
        LightItemDesc.Label       = EditorActorFactory::GetLightName(Type);
        LightItemDesc.Font        = Font;
        LightItemDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this, World, Type]()
        {
            SelectSpawnedActor(EditorActorFactory::SpawnLight(World, Type, ContextMenuLocation));
        });

        LightMenu->AddItem(FMenuItem::Create(LightItemDesc));
    }

    if (!LightMenu->GetItems().IsEmpty())
    {
        FMenuItem::FDesc LightDesc;
        LightDesc.Label   = "Light";
        LightDesc.Font    = Font;
        LightDesc.SubMenu = LightMenu;

        Menu->AddItem(FMenuItem::Create(LightDesc));
    }

    FMenuItem::FDesc CameraDesc;
    CameraDesc.Label       = "Camera";
    CameraDesc.Font        = Font;
    CameraDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this, World]()
    {
        SelectSpawnedActor(EditorActorFactory::SpawnCamera(World, ContextMenuLocation));
    });

    Menu->AddItem(FMenuItem::Create(CameraDesc));

    return Menu;
}

void FEditorViewportPanel::SelectSpawnedActor(FActor* SpawnedActor)
{
    if (SpawnedActor)
    {
        EditorEngine->SetSelectedActor(SpawnedActor);
    }
}

bool FEditorViewportPanel::ComputeFallbackPlacement(const Vector2& Ndc, Vector3& OutLocation) const
{
    const FCameraComponent* Camera = GetViewCamera();
    if (!Camera)
    {
        return false;
    }

    const Matrix4& ViewProjectionInverse = Camera->GetViewProjectionInverseMatrix();

    const Vector4 NearWorld = ViewProjectionInverse.Transform(Vector4(Ndc.X, Ndc.Y, 0.0f, 1.0f));
    const Vector4 FarWorld  = ViewProjectionInverse.Transform(Vector4(Ndc.X, Ndc.Y, 1.0f, 1.0f));

    if (Math::Abs(NearWorld.W) < 1e-6f || Math::Abs(FarWorld.W) < 1e-6f)
    {
        return false;
    }

    const Vector3 RayStart     = Vector3(NearWorld.X, NearWorld.Y, NearWorld.Z) / NearWorld.W;
    const Vector3 RayEnd       = Vector3(FarWorld.X, FarWorld.Y, FarWorld.Z) / FarWorld.W;
    const Vector3 RayDirection = (RayEnd - RayStart).GetNormalized();

    if (Math::Abs(RayDirection.Y) > 1e-4f)
    {
        const float Distance = -RayStart.Y / RayDirection.Y;
        if (Distance > 0.0f && Distance < PLACEMENT_MAX_GROUND_DISTANCE)
        {
            OutLocation = RayStart + (RayDirection * Distance);
            return true;
        }
    }

    OutLocation = Camera->GetPosition() + (Camera->GetForwardVector() * PLACEMENT_DEFAULT_DISTANCE);
    return true;
}

void FEditorViewportPanel::OnActorRemoved(FActor* Actor)
{
    CameraController->OnActorRemoved(Actor);
}

void FEditorViewportPanel::OnContextMenuPickResult(const FEditorPickResult& Result, FActor* PickedActor)
{
    if (Result.RequestId != ContextMenuPickRequestId)
    {
        return;
    }

    ContextMenuPickRequestId = 0;

    if (PickedActor)
    {
        EditorEngine->SetSelectedActor(PickedActor);
    }

    if (Result.bHasDepth)
    {
        const Vector4 Clip  = Vector4(ContextMenuNdc.X, ContextMenuNdc.Y, Result.DeviceDepth, 1.0f);
        const Vector4 World = ContextMenuViewProjectionInverse.Transform(Clip);

        if (Math::Abs(World.W) > 1e-6f)
        {
            ContextMenuLocation = Vector3(World.X, World.Y, World.Z) / World.W;
        }
    }
}

bool FEditorViewportPanel::ConsumeCameraCut()
{
    return CameraController->ConsumeCameraCut();
}

void FEditorViewportPanel::ResetInputState()
{
    if (Surface)
    {
        Surface->ResetInputState();
    }
}

void FEditorViewportPanel::FocusOnActor(FActor* Actor)
{
    if (Actor && CameraController)
    {
        CameraController->FocusOn(Actor);
    }
}

void FEditorViewportPanel::SetViewportImage(FRHITextureRef InViewportImage)
{
    ViewportImage = InViewportImage;

    if (Image)
    {
        Image->SetBrush(FUIBrush(ViewportImage.Get()));
    }
}

IntVector2 FEditorViewportPanel::GetViewportSize() const
{
    if (CachedViewportSize.X > 0 && CachedViewportSize.Y > 0)
    {
        return CachedViewportSize;
    }

    if (const TSharedPtr<FWindow> Window = EditorEngine->GetEngineWindow())
    {
        const FRectangle Bounds = Window->GetContentRectangle();
        if (Bounds.Width > 0 && Bounds.Height > 0)
        {
            return IntVector2(Bounds.Width, Bounds.Height);
        }
    }

    return IntVector2(1920, 1080);
}

FSceneRenderView::EDebugView FEditorViewportPanel::GetDebugView() const
{
    return DebugView;
}

FSceneRenderView::EDebugView FEditorViewportPanel::GetSecondaryDebugView() const
{
    return SecondaryDebugView;
}

FSceneRenderView::EDebugViewChannel FEditorViewportPanel::GetDebugViewChannelMask() const
{
    return DebugViewChannelMask;
}

FCameraComponent* FEditorViewportPanel::GetViewCamera() const
{
    return CameraController ? CameraController->GetCamera() : nullptr;
}

void FEditorViewportPanel::SetGizmoOperation(EGizmoOperation InOperation)
{
    RequestedGizmoOperation = InOperation;
    UpdateGizmoFromSelection();
}

EGizmoOperation FEditorViewportPanel::GetGizmoOperation() const
{
    return RequestedGizmoOperation;
}

EGizmoOperation FEditorViewportPanel::ResolveGizmoOperation(const TArray<FActor*>& Selection) const
{
    if (Selection.Size() != 1)
    {
        return RequestedGizmoOperation;
    }

    FActor* Actor = Selection[0];
    if (Actor->HasComponentOfType<FPointLightComponent>() || Actor->HasComponentOfType<FLightProbeComponent>())
    {
        return EGizmoOperation::Translate;
    }

    if (Actor->HasComponentOfType<FDirectionalLightComponent>())
    {
        return EGizmoOperation::Rotate;
    }

    if (Actor->HasComponentOfType<FCameraComponent>() && RequestedGizmoOperation == EGizmoOperation::Scale)
    {
        return EGizmoOperation::Translate;
    }

    return RequestedGizmoOperation;
}

void FEditorViewportPanel::SetGizmoMode(EGizmoMode InMode)
{
    if (Gizmo)
    {
        Gizmo->SetMode(InMode);
    }
}

EGizmoMode FEditorViewportPanel::GetGizmoMode() const
{
    return Gizmo ? Gizmo->GetMode() : EGizmoMode::World;
}

bool FEditorViewportPanel::OnViewportShortcut(const FKeyEvent& KeyEvent)
{
    if (!EditorEngine->IsEditing() || KeyEvent.GetModifierKeys().IsShortcutChordDown())
    {
        return false;
    }

    const FKey Key = KeyEvent.GetKey();
    if (Key == Keys::W || Key == Keys::One)
    {
        SetGizmoOperation(EGizmoOperation::Translate);
        return true;
    }

    if (Key == Keys::E || Key == Keys::Two)
    {
        SetGizmoOperation(EGizmoOperation::Rotate);
        return true;
    }

    if (Key == Keys::R || Key == Keys::Three)
    {
        SetGizmoOperation(EGizmoOperation::Scale);
        return true;
    }

    if (Key == Keys::Space)
    {
        switch (RequestedGizmoOperation)
        {
            case EGizmoOperation::Translate: SetGizmoOperation(EGizmoOperation::Rotate);    break;
            case EGizmoOperation::Rotate:    SetGizmoOperation(EGizmoOperation::Scale);     break;
            default:                         SetGizmoOperation(EGizmoOperation::Translate); break;
        }

        return true;
    }

    if (Key == Keys::Four)
    {
        SetGizmoMode(GetGizmoMode() == EGizmoMode::Local ? EGizmoMode::World : EGizmoMode::Local);
        return true;
    }

    return false;
}

void FEditorViewportPanel::SetGizmoPlacement(EEditorGizmoPlacement InPlacement)
{
    GizmoPlacement = InPlacement;
    UpdateGizmoFromSelection();
}

EEditorGizmoPlacement FEditorViewportPanel::GetGizmoPlacement() const
{
    return GizmoPlacement;
}

void FEditorViewportPanel::SetDebugView(FSceneRenderView::EDebugView InDebugView)
{
    DebugView = InDebugView;

    if (DebugViewCombo)
    {
        DebugViewCombo->SetSelectedIndex(FindDebugViewIndex(InDebugView));
    }
}
