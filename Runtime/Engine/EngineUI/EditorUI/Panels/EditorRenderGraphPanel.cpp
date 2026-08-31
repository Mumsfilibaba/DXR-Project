#include "Engine/EngineUI/EditorUI/Panels/EditorRenderGraphPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Elements/ToolBar.h"
#include "Application/Graph/GraphCanvas.h"
#include "Application/Graph/GraphModel.h"
#include "RendererCore/Interfaces/IRendererModule.h"

FEditorRenderGraphPanel::FEditorRenderGraphPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "RenderGraph", "Render Graph")
    , Canvas(nullptr)
    , Model(nullptr)
    , StatisticsText(nullptr)
    , ToolBar(nullptr)
    , PassAccessPinIds()
    , bIsCapturing(false)
    , bAutoLayoutPending(false)
{
}

FEditorRenderGraphPanel::~FEditorRenderGraphPanel()
{
}

bool FEditorRenderGraphPanel::Initialize()
{
    Model = MakeSharedPtr<FGraphModel>();

    FGraphCanvas::FDesc CanvasDesc;
    CanvasDesc.Font = FEditorStyle::GetFonts().Body;

    Canvas = FGraphCanvas::Create(CanvasDesc);
    if (!Canvas)
    {
        return false;
    }

    Canvas->SetModel(Model);

    ToolBar = BuildToolBar();
    if (!ToolBar)
    {
        return false;
    }

    FTextBlock::FDesc StatisticsDesc;
    StatisticsDesc.Font = FEditorStyle::GetFonts().Body;
    StatisticsDesc.Text = "No capture";

    StatisticsText = FTextBlock::Create(StatisticsDesc);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(ToolBar);
    Column->AddSlot(Canvas).SetFillCoefficient(1.0f);
    Column->AddSlot(StatisticsText).SetPadding(FMargin(6, 3, 6, 3));

    Content = Column;
    return true;
}

TSharedPtr<FToolBar> FEditorRenderGraphPanel::BuildToolBar()
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

    Bar->AddToggle(FToolBarItemDesc().SetLabel("Capture"), ECheckBoxState::Unchecked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            SetCaptureEnabled(State == ECheckBoxState::Checked);
        }));

    Bar->AddSeparator();

    Bar->AddButton(FToolBarItemDesc().SetLabel("Auto Layout"), FOnClicked::CreateLambda([this]()
    {
        Canvas->AutoLayout();
        Canvas->FitToNodes();
    }));

    Bar->AddButton(FToolBarItemDesc().SetLabel("Fit"), FOnClicked::CreateLambda([this]()
    {
        Canvas->FitToNodes();
    }));

    return Bar;
}

void FEditorRenderGraphPanel::Release()
{
    SetCaptureEnabled(false);

    Canvas.Reset();
    Model.Reset();
    StatisticsText.Reset();
    ToolBar.Reset();
    PassAccessPinIds.Clear();

    FEditorPanel::Release();
}

void FEditorRenderGraphPanel::OnVisibilityChanged(bool bInIsVisible)
{
    FEditorPanel::OnVisibilityChanged(bInIsVisible);

    if (!bInIsVisible && bIsCapturing)
    {
        SetCaptureEnabled(false);
    }
}

void FEditorRenderGraphPanel::SetCaptureEnabled(bool bEnabled)
{
    if (bIsCapturing == bEnabled)
    {
        return;
    }

    bIsCapturing = bEnabled;

#if EDITOR_BUILD
    if (IRendererModule* RendererModule = IRendererModule::Get())
    {
        RendererModule->SetRenderGraphDebugCaptureEnabled(bEnabled);
    }
#endif
}

void FEditorRenderGraphPanel::Tick(float /*DeltaTime*/)
{
    if (!bIsCapturing || !IsVisible())
    {
        return;
    }

#if EDITOR_BUILD
    IRendererModule* RendererModule = IRendererModule::Get();
    if (!RendererModule)
    {
        return;
    }

    FRenderGraphDebugSnapshot Snapshot;
    if (!RendererModule->CopyLatestRenderGraphDebugSnapshot(Snapshot))
    {
        return;
    }

    RebuildModel(Snapshot);
    RefreshStatistics(Snapshot);

    if (bAutoLayoutPending)
    {
        Canvas->AutoLayout();
        Canvas->FitToNodes();
        bAutoLayoutPending = false;
    }
#endif
}

#if EDITOR_BUILD

void FEditorRenderGraphPanel::RebuildModel(const FRenderGraphDebugSnapshot& Snapshot)
{
    Canvas->ClearSelection();

    Model->SetReadOnly(false);
    Model->Clear();

    PassAccessPinIds.Clear();
    PassAccessPinIds.Resize(Snapshot.Passes.Size());

    for (int32 PassIndex = 0; PassIndex < Snapshot.Passes.Size(); ++PassIndex)
    {
        const FRenderGraphDebugPass& Pass = Snapshot.Passes[PassIndex];

        FGraphNode Node;
        Node.Title     = Pass.Name;
        Node.TitleTint = GetPassTint(Pass);
        Node.bIsMuted  = Pass.bCulled || !Pass.bEnabled;

        for (const FRenderGraphDebugAccess& Access : Pass.Accesses)
        {
            const bool bHasResource = Access.ResourceIndex >= 0 && Access.ResourceIndex < Snapshot.Resources.Size();

            FGraphPin Pin;
            Pin.Direction = Access.bIsWrite ? EGraphPinDirection::Output : EGraphPinDirection::Input;
            Pin.Name      = bHasResource ? Snapshot.Resources[Access.ResourceIndex].Name : String("<unknown>");
            Pin.TypeTag   = bHasResource && Snapshot.Resources[Access.ResourceIndex].Kind == ERenderGraphDebugResourceKind::Buffer
                ? String("Buffer")
                : String("Texture");
            Pin.Tint = bHasResource ? GetResourceTint(Snapshot.Resources[Access.ResourceIndex]) : FFloatColor::White;

            Node.Pins.Emplace(Pin);
        }

        const int32 NodeId = Model->AddNode(Node);

        TArray<int32>& PinIds = PassAccessPinIds[PassIndex];
        if (const FGraphNode* AddedNode = Model->FindNode(NodeId))
        {
            PinIds.Reserve(AddedNode->Pins.Size());
            for (const FGraphPin& Pin : AddedNode->Pins)
            {
                PinIds.Emplace(Pin.PinId);
            }
        }
    }

    for (const FRenderGraphDebugLink& Link : Snapshot.Links)
    {
        if (!PassAccessPinIds.IsValidIndex(Link.FromPass) || !PassAccessPinIds.IsValidIndex(Link.ToPass))
        {
            continue;
        }

        const TArray<int32>& FromPins = PassAccessPinIds[Link.FromPass];
        const TArray<int32>& ToPins   = PassAccessPinIds[Link.ToPass];

        if (!FromPins.IsValidIndex(Link.FromAccess) || !ToPins.IsValidIndex(Link.ToAccess))
        {
            continue;
        }

        Model->AddLink(FromPins[Link.FromAccess], ToPins[Link.ToAccess]);
    }

    Model->SetReadOnly(true);
}

void FEditorRenderGraphPanel::RefreshStatistics(const FRenderGraphDebugSnapshot& Snapshot)
{
    const FRenderGraphStatistics& Statistics = Snapshot.Statistics;

    StatisticsText->SetText(String::Printf("%s  |  %d passes (%d culled, %d disabled)  |  %d textures, %d buffers",
        *Snapshot.GraphName, Statistics.NumPasses, Statistics.NumCulledPasses, Statistics.NumDisabledPasses,
        Statistics.NumTexturesAllocated, Statistics.NumBuffersAllocated));
}

FFloatColor FEditorRenderGraphPanel::GetPassTint(const FRenderGraphDebugPass& Pass)
{
    if (Pass.bCulled)
    {
        return FFloatColor(0.45f, 0.20f, 0.20f, 1.0f);
    }

    if (!Pass.bEnabled)
    {
        return FFloatColor(0.35f, 0.35f, 0.35f, 1.0f);
    }

    if (IsEnumFlagSet(Pass.Flags, ERenderGraphPassFlags::Compute))
    {
        return FFloatColor(0.20f, 0.40f, 0.55f, 1.0f);
    }

    if (IsEnumFlagSet(Pass.Flags, ERenderGraphPassFlags::Copy))
    {
        return FFloatColor(0.40f, 0.35f, 0.20f, 1.0f);
    }

    return FFloatColor(0.22f, 0.42f, 0.28f, 1.0f);
}

FFloatColor FEditorRenderGraphPanel::GetResourceTint(const FRenderGraphDebugResource& Resource)
{
    if (Resource.bIsExternal)
    {
        return FFloatColor(0.85f, 0.65f, 0.25f, 1.0f);
    }

    return Resource.Kind == ERenderGraphDebugResourceKind::Buffer
        ? FFloatColor(0.45f, 0.70f, 0.95f, 1.0f)
        : FFloatColor(0.55f, 0.85f, 0.55f, 1.0f);
}

#endif
