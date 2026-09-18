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
#include "Core/Containers/Set.h"
#include "Application/Application.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/CheckBox.h"
#include "Application/Elements/Overlay.h"
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

// The gap ImGui leaves between two groups of the viewport strip, which the bar's own default of 2 leaves too tight
constexpr int32 TOOLBAR_GROUP_GAP = 8;

// The strip's own inset, which is ImGui's twelve either side and six above and below its 26px entries
static const FMargin TOOLBAR_INSET = FMargin(12, 6, 12, 6);

// ImGui's widths for the strip's entries, which are what keep the three radio groups reading as one row
constexpr int32 TRANSLATION_BUTTON_WIDTH = 76;
constexpr int32 ROTATE_BUTTON_WIDTH      = 56;
constexpr int32 SCALE_BUTTON_WIDTH       = 52;
constexpr int32 PLACEMENT_BUTTON_WIDTH   = 56;
constexpr int32 ORIENTATION_BUTTON_WIDTH = 52;
constexpr int32 PLAY_BUTTON_WIDTH        = 52;
constexpr int32 PAUSE_BUTTON_WIDTH       = 56;
constexpr int32 CAMERA_BUTTON_WIDTH      = 118;

// How wide the view dropdown may grow to fit the longest debug view name before that name is left to clip
constexpr int32 VIEW_BUTTON_MAX_WIDTH = 128;

// What the view dropdown's label needs beyond the name itself, being the two insets and the arrow
constexpr int32 VIEW_BUTTON_LABEL_OVERHEAD = 36;

struct FDebugViewEntry
{
    const CHAR*                  Label;
    FSceneRenderView::EDebugView View;
};

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

constexpr int32 NUM_DEBUG_VIEW_ENTRIES       = static_cast<int32>(ARRAY_COUNT(DEBUG_VIEW_ENTRIES));
constexpr int32 FIRST_SHADOW_DEBUG_VIEW      = 8;
constexpr int32 FIRST_RAY_TRACING_DEBUG_VIEW = 11;

static_assert(DEBUG_VIEW_ENTRIES[FIRST_SHADOW_DEBUG_VIEW].View == FSceneRenderView::EDebugView::ShadowCascades,
    "The CSM group no longer starts where FIRST_SHADOW_DEBUG_VIEW says it does");
static_assert(DEBUG_VIEW_ENTRIES[FIRST_RAY_TRACING_DEBUG_VIEW].View == FSceneRenderView::EDebugView::RayTracingReflectionsRaw,
    "The ray-tracing group no longer starts where FIRST_RAY_TRACING_DEBUG_VIEW says it does");

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

static const CHAR* FindDebugViewLabel(FSceneRenderView::EDebugView InDebugView)
{
    const int32 Index = FindDebugViewIndex(InDebugView);
    return DEBUG_VIEW_ENTRIES[Index >= 0 ? Index : 0].Label;
}

static bool HasSelectedAncestor(const TSet<FActor*>& SelectionLookup, FActor* Actor)
{
    for (FActor* Parent = Actor->GetParentActor(); Parent; Parent = Parent->GetParentActor())
    {
        if (SelectionLookup.Contains(Parent))
        {
            return true;
        }
    }

    return false;
}

FEditorViewportPanel::FEditorViewportPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "Viewport", "Viewport")
    , CameraController(MakeUniquePtr<FEditorCameraController>())
    , Surface(nullptr)
    , Image(nullptr)
    , Gizmo(nullptr)
    , DebugViewBar(nullptr)
    , SecondaryDebugViewCombo(nullptr)
    , TranslateItem(nullptr)
    , RotateItem(nullptr)
    , ScaleItem(nullptr)
    , CenterItem(nullptr)
    , PivotItem(nullptr)
    , LocalItem(nullptr)
    , WorldItem(nullptr)
    , PlayItem(nullptr)
    , PauseItem(nullptr)
    , ViewItem(nullptr)
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

TSharedPtr<FViewportTransportLayer> FViewportTransportLayer::Create(const TSharedPtr<FToolBar>& InTransport, const TSharedPtr<FToolBar>& InBar)
{
    TSharedPtr<FViewportTransportLayer> NewLayer = MakeSharedPtr<FViewportTransportLayer>();
    NewLayer->Transport = InTransport;
    NewLayer->Bar       = InBar;

    if (InTransport)
    {
        InTransport->SetParentElement(NewLayer->AsWeakPtr());
    }

    return NewLayer;
}

FViewportTransportLayer::FViewportTransportLayer()
    : FVisualElement()
    , Transport(nullptr)
    , Bar(nullptr)
{
}

FViewportTransportLayer::~FViewportTransportLayer() = default;

IntVector2 FViewportTransportLayer::ComputeDesiredSize() const
{
    return Transport ? Transport->GetCachedDesiredSize() : IntVector2();
}

void FViewportTransportLayer::OnArrange(const FRectangle& AllottedBounds)
{
    if (!Transport)
    {
        return;
    }

    const IntVector2 Size = Transport->GetCachedDesiredSize();

    const int32 MinX = GetLeadingRight(AllottedBounds) + TOOLBAR_GROUP_GAP;
    const int32 MaxX = GetTrailingLeft(AllottedBounds) - TOOLBAR_GROUP_GAP - Size.X;

    if (MinX > MaxX)
    {
        Transport->SetVisibility(EVisibility::Hidden);

        SetContentRectangle(FRectangle());
        return;
    }

    Transport->SetVisibility(EVisibility::Visible);

    const int32 CenteredX = AllottedBounds.Position.X + ((AllottedBounds.Width - Size.X) / 2);
    const int32 PositionX = Math::Clamp(CenteredX, MinX, MaxX);
    const int32 PositionY = AllottedBounds.Position.Y + ((AllottedBounds.Height - Size.Y) / 2);

    const FRectangle TransportBounds(IntVector2(PositionX, PositionY), Size.X, Size.Y);

    Transport->Tick(TransportBounds);
    SetContentRectangle(TransportBounds);
}

void FViewportTransportLayer::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    if (Transport)
    {
        OutChildren.Add(Transport);
    }
}

int32 FViewportTransportLayer::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (!Transport || !Transport->IsVisible())
    {
        return LayerId;
    }

    const FDrawGeometry TransportGeometry(Transport->GetContentRectangle(), AllottedGeometry.Scale);
    return Transport->OnDraw(TransportGeometry, OutCommandList, LayerId);
}

void FViewportTransportLayer::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    if (Transport && Transport->IsVisible())
    {
        Transport->FindChildrenContainingPoint(ClientPosition, OutChildElements);
    }
}

int32 FViewportTransportLayer::GetLeadingRight(const FRectangle& AllottedBounds) const
{
    int32 Right = AllottedBounds.Position.X;
    if (!Bar)
    {
        return Right;
    }

    for (const FToolBarEntry& Entry : Bar->GetItems())
    {
        if (Entry.Type == EToolBarItemType::FlexibleSpace)
        {
            break;
        }

        if (Entry.Element)
        {
            Right = Math::Max(Right, Entry.Element->GetContentRectangle().GetRight());
        }
    }

    return Right;
}

int32 FViewportTransportLayer::GetTrailingLeft(const FRectangle& AllottedBounds) const
{
    int32 Left = AllottedBounds.GetRight();
    if (!Bar)
    {
        return Left;
    }

    bool bIsPastSpace = false;
    for (const FToolBarEntry& Entry : Bar->GetItems())
    {
        if (Entry.Type == EToolBarItemType::FlexibleSpace)
        {
            bIsPastSpace = true;
            continue;
        }

        if (bIsPastSpace && Entry.Element)
        {
            Left = Math::Min(Left, Entry.Element->GetContentRectangle().Position.X);
        }
    }

    return Left;
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
    GizmoDesc.OnDragStarted      = FOnGizmoDragStarted::CreateRaw(this, &FEditorViewportPanel::OnGizmoDragStarted);
    GizmoDesc.OnTransformChanged = FOnGizmoTransformChanged::CreateRaw(this, &FEditorViewportPanel::OnGizmoTransformChanged);
    GizmoDesc.OnDragFinished     = FOnGizmoDragFinished::CreateRaw(this, &FEditorViewportPanel::OnGizmoDragFinished);

    Gizmo = FGizmo::Create(GizmoDesc);
    if (!Gizmo)
    {
        return false;
    }

    DebugViewBar = BuildToolBar();
    if (!DebugViewBar)
    {
        return false;
    }

    TransportBar = BuildTransportBar();
    if (!TransportBar)
    {
        return false;
    }

    RefreshToolBarState();

    Surface = FEditorViewportSurface::Create();
    if (!Surface)
    {
        return false;
    }

    Surface->SetLayers(Image, EditorEngine->GetViewport(), Gizmo);
    Surface->OnClickedDelegate       = FOnViewportClicked::CreateRaw(this, &FEditorViewportPanel::OnViewportClicked);
    Surface->OnContextMenuDelegate   = FOnViewportContextMenu::CreateRaw(this, &FEditorViewportPanel::OnViewportContextMenu);
    Surface->OnMarqueeSelectDelegate = FOnViewportMarqueeSelect::CreateRaw(this, &FEditorViewportPanel::OnViewportMarqueeSelect);
    Surface->OnShortcutDelegate      = FOnViewportShortcut::CreateRaw(this, &FEditorViewportPanel::OnViewportShortcut);

    if (const TSharedPtr<FSceneViewport> SceneViewport = EditorEngine->GetSceneViewport())
    {
        SceneViewport->SetPlayerInputEnabled(false);
    }

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    if (!Column)
    {
        return false;
    }

    TSharedPtr<FOverlay> Strip = FOverlay::Create();
    if (!Strip)
    {
        return false;
    }

    Strip->AddSlot(DebugViewBar);
    Strip->AddSlot(FViewportTransportLayer::Create(TransportBar, DebugViewBar));

    Column->AddSlot(Strip);
    Column->AddSlot(Surface).SetFillCoefficient(1.0f);

    Content = Column;
    return true;
}

void FEditorViewportPanel::Release()
{
    if (Surface)
    {
        Surface->SetLayers(nullptr, nullptr, nullptr);
    }

    ViewItem.Reset();
    PauseItem.Reset();
    PlayItem.Reset();
    WorldItem.Reset();
    LocalItem.Reset();
    PivotItem.Reset();
    CenterItem.Reset();
    ScaleItem.Reset();
    RotateItem.Reset();
    TranslateItem.Reset();
    SecondaryDebugViewCombo.Reset();
    TransportBar.Reset();
    DebugViewBar.Reset();
    Gizmo.Reset();
    Image.Reset();
    Surface.Reset();
    ViewportImage.Reset();

    CameraController.Reset();

    FEditorPanel::Release();
}

FMargin FEditorViewportPanel::GetContentPadding() const
{
    return FMargin();
}

TSharedPtr<FToolBar> FEditorViewportPanel::BuildToolBar()
{
    FToolBar::FDesc Desc;
    Desc.Font           = FEditorStyle::GetFonts().Body;
    Desc.IconSize       = FEditorStyle::IconSize;
    Desc.Padding        = TOOLBAR_INSET;
    Desc.ItemSpacing    = TOOLBAR_GROUP_GAP;
    Desc.bHasBackground = true;

    TSharedPtr<FToolBar> Bar = FToolBar::Create(Desc);
    if (!Bar)
    {
        return nullptr;
    }

    Bar->BeginGroup();

    TranslateItem = Bar->AddButton(
        FToolBarItemDesc().SetLabel("Translation").SetToolTipText("Translate the selection").SetMinWidth(TRANSLATION_BUTTON_WIDTH),
        FOnClicked::CreateLambda([this]()
        {
            SetGizmoOperation(EGizmoOperation::Translate);
        }));

    RotateItem = Bar->AddButton(
        FToolBarItemDesc().SetLabel("Rotate").SetToolTipText("Rotate the selection").SetMinWidth(ROTATE_BUTTON_WIDTH),
        FOnClicked::CreateLambda([this]()
        {
            SetGizmoOperation(EGizmoOperation::Rotate);
        }));

    ScaleItem = Bar->AddButton(
        FToolBarItemDesc().SetLabel("Scale").SetToolTipText("Scale the selection").SetMinWidth(SCALE_BUTTON_WIDTH),
        FOnClicked::CreateLambda([this]()
        {
            SetGizmoOperation(EGizmoOperation::Scale);
        }));

    Bar->EndGroup();

    Bar->BeginGroup();

    CenterItem = Bar->AddButton(
        FToolBarItemDesc().SetLabel("Center").SetToolTipText("Put the handles at the bounds centre").SetMinWidth(PLACEMENT_BUTTON_WIDTH),
        FOnClicked::CreateLambda([this]()
        {
            SetGizmoPlacement(EEditorGizmoPlacement::Center);
        }));

    PivotItem = Bar->AddButton(
        FToolBarItemDesc().SetLabel("Pivot").SetToolTipText("Put the handles at the pivot").SetMinWidth(PLACEMENT_BUTTON_WIDTH),
        FOnClicked::CreateLambda([this]()
        {
            SetGizmoPlacement(EEditorGizmoPlacement::Pivot);
        }));

    Bar->EndGroup();

    Bar->BeginGroup();

    LocalItem = Bar->AddButton(
        FToolBarItemDesc().SetLabel("Local").SetToolTipText("Align the handles to the selection").SetMinWidth(ORIENTATION_BUTTON_WIDTH),
        FOnClicked::CreateLambda([this]()
        {
            SetGizmoMode(EGizmoMode::Local);
        }));

    WorldItem = Bar->AddButton(
        FToolBarItemDesc().SetLabel("World").SetToolTipText("Align the handles to the world axes").SetMinWidth(ORIENTATION_BUTTON_WIDTH),
        FOnClicked::CreateLambda([this]()
        {
            SetGizmoMode(EGizmoMode::World);
        }));

    Bar->EndGroup();

    Bar->AddFlexibleSpace();

    Bar->AddDropDown(
        FToolBarItemDesc().SetLabel("Camera").SetToolTipText("Speeds, lens and framing").SetMinWidth(CAMERA_BUTTON_WIDTH),
        BuildCameraMenu());

    Bar->AddDropDown(
        FToolBarItemDesc().SetToolTipText("View mode, secondary view and channel mask").SetMinWidth(ComputeViewButtonWidth()),
        BuildViewOptionsMenu());

    ViewItem = Bar->GetItems().Last().Button;
    return Bar;
}

TSharedPtr<FToolBar> FEditorViewportPanel::BuildTransportBar()
{
    FToolBar::FDesc Desc;
    Desc.Font           = FEditorStyle::GetFonts().Body;
    Desc.IconSize       = FEditorStyle::IconSize;
    Desc.Padding        = FMargin();
    Desc.ItemSpacing    = TOOLBAR_GROUP_GAP;
    Desc.bHasBackground = false;

    TSharedPtr<FToolBar> Bar = FToolBar::Create(Desc);
    if (!Bar)
    {
        return nullptr;
    }

    Bar->BeginGroup();

    PlayItem = Bar->AddButton(
        FToolBarItemDesc().SetLabel("Play").SetToolTipText("Run the world in the editor").SetMinWidth(PLAY_BUTTON_WIDTH),
        FOnClicked::CreateRaw(this, &FEditorViewportPanel::TogglePlay));

    PauseItem = Bar->AddButton(
        FToolBarItemDesc().SetLabel("Pause").SetToolTipText("Freeze the running world").SetMinWidth(PAUSE_BUTTON_WIDTH),
        FOnClicked::CreateLambda([this]()
        {
            EditorEngine->TogglePause();
            RefreshToolBarState();
        }));

    Bar->EndGroup();

    return Bar;
}

int32 FEditorViewportPanel::ComputeViewButtonWidth() const
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;
    if (!Font)
    {
        return VIEW_BUTTON_MAX_WIDTH;
    }

    int32 WidestLabel = 0;
    for (const FDebugViewEntry& Entry : DEBUG_VIEW_ENTRIES)
    {
        WidestLabel = Math::Max(WidestLabel, Font->MeasureWidth(StringView(Entry.Label)));
    }

    return Math::Min(WidestLabel + VIEW_BUTTON_LABEL_OVERHEAD, VIEW_BUTTON_MAX_WIDTH);
}

void FEditorViewportPanel::RefreshToolBarState()
{
    const bool            bIsEditing = EditorEngine->IsEditing();
    const EGizmoMode      Mode       = GetGizmoMode();
    const EGizmoOperation Operation  = RequestedGizmoOperation;

    if (TranslateItem)
    {
        TranslateItem->SetHighlighted(Operation == EGizmoOperation::Translate);
    }

    if (RotateItem)
    {
        RotateItem->SetHighlighted(Operation == EGizmoOperation::Rotate);
    }

    if (ScaleItem)
    {
        ScaleItem->SetHighlighted(Operation == EGizmoOperation::Scale);
    }

    if (CenterItem)
    {
        CenterItem->SetHighlighted(GizmoPlacement == EEditorGizmoPlacement::Center);
    }

    if (PivotItem)
    {
        PivotItem->SetHighlighted(GizmoPlacement == EEditorGizmoPlacement::Pivot);
    }

    if (LocalItem)
    {
        LocalItem->SetHighlighted(Mode == EGizmoMode::Local);
    }

    if (WorldItem)
    {
        WorldItem->SetHighlighted(Mode == EGizmoMode::World);
    }

    if (PlayItem)
    {
        PlayItem->SetLabel(bIsEditing ? "Play" : "Stop");
        PlayItem->SetHighlighted(!bIsEditing);
    }

    if (PauseItem)
    {
        PauseItem->SetEnabled(!bIsEditing);
        PauseItem->SetHighlighted(EditorEngine->IsPaused());
    }

    if (ViewItem)
    {
        ViewItem->SetLabel(FindDebugViewLabel(DebugView));
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

    RefreshToolBarState();
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

    const auto AddSliderRow = [&Menu, &Font](const CHAR* RowLabel, float MinValue, float MaxValue, float Value,
        int32 Precision, const FOnSliderValueChanged& OnValueChanged)
    {
        FTextBlock::FDesc CaptionDesc;
        CaptionDesc.Text = RowLabel;
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

    Menu->AddSection("Movement", Font);

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

    Menu->AddSection("Lens", Font);

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

    Menu->AddSection("Actions", Font);

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

    Menu->AddSection("View Mode", Font);

    const auto AddDebugViewItem = [this, &Font](const TSharedPtr<FMenu>& Target, const FDebugViewEntry& Entry)
    {
        const FSceneRenderView::EDebugView View = Entry.View;

        FMenuItem::FDesc ItemDesc;
        ItemDesc.Label        = Entry.Label;
        ItemDesc.Font         = Font;
        ItemDesc.bIsCheckable = true;
        ItemDesc.CheckState   = DebugView == View ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
        ItemDesc.OnActivated  = FOnMenuItemActivated::CreateLambda([this, View]()
        {
            SetDebugView(View);
        });

        Target->AddItem(FMenuItem::Create(ItemDesc));
    };

    for (int32 Index = 0; Index < FIRST_SHADOW_DEBUG_VIEW; ++Index)
    {
        AddDebugViewItem(Menu, DEBUG_VIEW_ENTRIES[Index]);
    }

    TSharedPtr<FMenu> ShadowMenu = FMenu::Create();
    for (int32 Index = FIRST_SHADOW_DEBUG_VIEW; Index < FIRST_RAY_TRACING_DEBUG_VIEW; ++Index)
    {
        AddDebugViewItem(ShadowMenu, DEBUG_VIEW_ENTRIES[Index]);
    }

    FMenuItem::FDesc ShadowDesc;
    ShadowDesc.Label   = "Shadow Debug";
    ShadowDesc.Font    = Font;
    ShadowDesc.SubMenu = ShadowMenu;

    Menu->AddItem(FMenuItem::Create(ShadowDesc));

    TSharedPtr<FMenu> RayTracingMenu = FMenu::Create();
    for (int32 Index = FIRST_RAY_TRACING_DEBUG_VIEW; Index < NUM_DEBUG_VIEW_ENTRIES; ++Index)
    {
        AddDebugViewItem(RayTracingMenu, DEBUG_VIEW_ENTRIES[Index]);
    }

    FMenuItem::FDesc RayTracingDesc;
    RayTracingDesc.Label   = "Ray Tracing";
    RayTracingDesc.Font    = Font;
    RayTracingDesc.SubMenu = RayTracingMenu;

    Menu->AddItem(FMenuItem::Create(RayTracingDesc));

    Menu->AddSection("Split View", Font);

    SecondaryDebugViewCombo = BuildSecondaryDebugViewCombo();
    if (SecondaryDebugViewCombo)
    {
        TSharedPtr<FHorizontalBox> ComboRow = FHorizontalBox::Create();
        ComboRow->AddSlot(SecondaryDebugViewCombo).SetFillCoefficient(1.0f).SetPadding(FMargin(8, 2, 8, 4));

        Menu->AddCustomEntry(ComboRow);
    }

    Menu->AddSection("Channels", Font);

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

    RefreshToolBarState();
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

void FEditorViewportPanel::OnGizmoDragStarted(EGizmoHandle /*Handle*/)
{
    const TArray<FActor*>& Selection = EditorEngine->GetSelectedActors();

    DragActors.Clear();
    DragStartTransforms.Clear();
    DragActors.Reserve(Selection.Size());
    DragStartTransforms.Reserve(Selection.Size());

    TSet<FActor*> SelectionLookup;
    for (FActor* Actor : Selection)
    {
        if (Actor)
        {
            SelectionLookup.Add(Actor);
        }
    }

    for (FActor* Actor : Selection)
    {
        if (!Actor || HasSelectedAncestor(SelectionLookup, Actor))
        {
            continue;
        }

        DragActors.Emplace(Actor);
        DragStartTransforms.Emplace(Actor->GetWorldTransform().GetTransformMatrix());
    }
}

void FEditorViewportPanel::OnGizmoTransformChanged(const Matrix4& /*NewTransform*/, const Matrix4& Delta)
{
    for (int32 Index = 0; Index < DragActors.Size(); ++Index)
    {
        FActor* Actor = DragActors[Index];
        if (!Actor)
        {
            continue;
        }

        Actor->SetWorldTransformMatrix(DragStartTransforms[Index] * Delta);

        if (FCameraComponent* CameraComponent = Actor->GetComponentOfType<FCameraComponent>())
        {
            CameraComponent->SetRotation(Actor->GetTransform().GetRotation());
        }
    }
}

void FEditorViewportPanel::OnGizmoDragFinished(const Matrix4& /*TransformAtDragStart*/, const Matrix4& /*Transform*/)
{
    DragActors.Clear();
    DragStartTransforms.Clear();

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

    TSharedPtr<FMenu> Menu = BuildContextMenu();
    if (!Menu)
    {
        return;
    }

    FMenuStack::Get().PushMenu(Surface, FRectangle(ContextMenuScreenPosition, 0, 0), EMenuPlacement::AtCursor, Menu);
}

TSharedPtr<FMenu> FEditorViewportPanel::BuildContextMenu()
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();
    if (!Menu)
    {
        return nullptr;
    }

    Menu->AddSection("Create", Font);

    FMenuItem::FDesc PlaceDesc;
    PlaceDesc.Label   = "Place Actor";
    PlaceDesc.Font    = Font;
    PlaceDesc.SubMenu = BuildPlaceActorMenu();

    Menu->AddItem(FMenuItem::Create(PlaceDesc));

    Menu->AddSection("Selection", Font);

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

    const int32 DragIndex = DragActors.Find(Actor);
    if (DragIndex != TArray<FActor*>::InvalidIndex)
    {
        DragActors.RemoveAt(DragIndex, 1);
        DragStartTransforms.RemoveAt(DragIndex, 1);
    }
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
    RefreshToolBarState();
}
