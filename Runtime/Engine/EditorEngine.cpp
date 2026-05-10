#include "Engine/EditorEngine.h"
#include "Core/Misc/ConsoleManager.h"
#include "Engine/EngineUI/Editor/EditorDockspaceWidget.h"
#include "Engine/EngineUI/Editor/EditorFooterWidget.h"
#include "Engine/EngineUI/Editor/EditorViewportWidget.h"
#include "Engine/EngineUI/Editor/EditorSceneHierarchyWidget.h"
#include "Engine/EngineUI/Editor/EditorPropertiesWidget.h"
#include "Engine/EngineUI/Editor/EditorContentBrowserWidget.h"
#include "Engine/EngineUI/Editor/EditorGuizmoWidget.h"
#include "Engine/EngineUI/Editor/EditorRendererSettingsWidget.h"
#include "Engine/EngineUI/Editor/EditorGPUProfilerWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/EngineUI/Editor/EditorFrameProfilerWidget.h"
#include "Engine/EngineUI/Editor/EditorRHIInfoWidget.h"
#include "Engine/EngineUI/Editor/EditorStatsWidget.h"
#include "RendererCore/RenderSettings.h"
#include "RendererCore/Interfaces/IRendererModule.h"

FEditorEngine::FEditorEngine()
    : FEngine()
    , SelectedActor(nullptr)
    , SelectedLight(nullptr)
    , SelectedCamera(nullptr)
    , SelectedLightProbe(nullptr)
    , DockspaceWidget(nullptr)
    , FooterWidget(nullptr)
    , OutputLogWidget(nullptr)
    , SceneHierarchyWidget(nullptr)
    , ContentBrowserWidget(nullptr)
    , GuizmoWidget(nullptr)
    , RendererSettingsWidget(nullptr)
    , GPUProfilerWidget(nullptr)
    , FrameProfilerWidget(nullptr)
    , RHIInfoWidget(nullptr)
    , StatsWidget(nullptr)
    , ViewportImage(nullptr)
    , ViewportImageSize()
{
}

FEditorEngine::~FEditorEngine()
{
}

bool FEditorEngine::Init()
{
    if (!FEngine::Init())
    {
        return false;
    }

    if (IImguiPlugin::IsEnabled())
    {
        DockspaceWidget        = MakeSharedPtr<FEditorDockspaceWidget>(this);
        OutputLogWidget        = MakeSharedPtr<FEditorOutputLogWidget>();
        SceneHierarchyWidget   = MakeSharedPtr<FEditorSceneHierarchyWidget>(this);
        FooterWidget           = MakeSharedPtr<FEditorFooterWidget>(OutputLogWidget);
        PropertiesWidget	   = MakeSharedPtr<FEditorPropertiesWidget>(this);
        ContentBrowserWidget   = MakeSharedPtr<FEditorContentBrowserWidget>();
        GuizmoWidget           = MakeSharedPtr<FEditorGuizmoWidget>(this);
        RendererSettingsWidget = MakeSharedPtr<FEditorRendererSettingsWidget>();
        GPUProfilerWidget      = MakeSharedPtr<FEditorGPUProfilerWidget>();
        FrameProfilerWidget    = MakeSharedPtr<FEditorFrameProfilerWidget>();
        RHIInfoWidget          = MakeSharedPtr<FEditorRHIInfoWidget>();
        StatsWidget            = MakeSharedPtr<FEditorStatsWidget>();

        ViewportWidget = MakeSharedPtr<FEditorViewportWidget>();
        ViewportWidget->SetViewportWidget(GetViewportWidget());
    }

    if (!CreateViewportRenderTarget())
    {
        return false;
    }

    return true;
}

bool FEditorEngine::InitPostRenderer()
{
    // Load fonts
    if (!EditorFonts::Initialize())
    {
        return false;
    }

    return true;
}

void FEditorEngine::Release()
{
    if (IImguiPlugin::IsEnabled())
    {
        DockspaceWidget.Reset();
        OutputLogWidget.Reset();
        SceneHierarchyWidget.Reset();
        FooterWidget.Reset();
        ViewportWidget.Reset();
        PropertiesWidget.Reset();
        ContentBrowserWidget.Reset();
        GuizmoWidget.Reset();
        RendererSettingsWidget.Reset();
        GPUProfilerWidget.Reset();
        FrameProfilerWidget.Reset();
        RHIInfoWidget.Reset();
    }

    FEngine::Release();
}

void FEditorEngine::Tick(float DeltaTime)
{
    FEngine::Tick(DeltaTime);

    // Consume any completed async editor pick results.
    if (FWorld* LocalWorld = GetWorld())
    {
        if (IRendererModule* RendererModule = IRendererModule::Get())
        {
            uint32 PickedObjectID = 0;
            if (RendererModule->PollEditorObjectPickResult(LocalWorld->GetSceneInterface(), PickedObjectID))
            {
                IScene* Scene       = LocalWorld->GetSceneInterface();
                FActor* PickedActor = Scene ? Scene->GetActorByObjectID(PickedObjectID) : nullptr;

                if (IConsoleVariable* PickDebug = FConsoleManager::Get().FindConsoleVariable("Editor.Pick.Debug"))
                {
                    if (PickDebug->GetBool())
                    {
                        LOG_INFO("[EditorPick] Completed. ObjectID=%u Actor=%s", PickedObjectID, PickedActor ? *PickedActor->GetName() : "nullptr");
                    }
                }

                if (PickedActor)
                {
                    SetSelectedActor(PickedActor);
                }
                else
                {
                    ClearSelection();
                }
            }
        }
    }

    const FIntVector2 Size = ViewportWidget->GetViewportSize();
    if (ViewportImageSize != Size)
    {
        CreateViewportRenderTarget();
    }
}

void FEditorEngine::RenderFrame()
{
    TRACE_FUNCTION_SCOPE();

    // Render to a separate render-target
    FSceneRenderView RenderView;
    RenderView.Scene        = GetWorld()->GetSceneInterface();
    RenderView.RenderTarget = ViewportImage.Get();
    RenderView.DebugView          = ViewportWidget->GetDebugView();
    RenderView.SecondaryDebugView = ViewportWidget->GetSecondaryDebugView();

    IRendererModule* RendererModule = IRendererModule::Get();
    RendererModule->RenderSceneView(RenderView);

    // Render the rest
    FEngine::RenderFrame();
}

void FEditorEngine::SetSelectedActor(FActor* InActor)
{
    SelectedActor      = InActor;
    SelectedLight      = nullptr;
    SelectedCamera     = nullptr;
    SelectedLightProbe = nullptr;
}

void FEditorEngine::SetSelectedLight(FLight* InLight)
{
    SelectedLight      = InLight;
    SelectedActor      = nullptr;
    SelectedCamera     = nullptr;
    SelectedLightProbe = nullptr;
}

void FEditorEngine::SetSelectedCamera(FCamera* InCamera)
{
    SelectedCamera     = InCamera;
    SelectedActor      = nullptr;
    SelectedLight      = nullptr;
    SelectedLightProbe = nullptr;
}

void FEditorEngine::SetSelectedLightProbe(FLightProbe* InProbe)
{
    SelectedLightProbe = InProbe;
    SelectedActor      = nullptr;
    SelectedLight      = nullptr;
    SelectedCamera     = nullptr;
}

void FEditorEngine::ClearSelection()
{
    SelectedActor      = nullptr;
    SelectedLight      = nullptr;
    SelectedCamera     = nullptr;
    SelectedLightProbe = nullptr;
}

bool FEditorEngine::CreateViewportRenderTarget()
{
    const FIntVector2 Size = ViewportWidget->GetViewportSize();
    if (Size.X == 0 || Size.Y == 0)
    {
        return ViewportImage != nullptr;
    }

    const ETextureUsageFlags UsageFlags = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture;
    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(FEditorEngine::ViewportImageFormat, Size.X, Size.Y, 1, 1, UsageFlags);

    FRHITextureRef NewViewportImage = RHI::CreateTexture(TextureDesc, EResourceAccess::RenderTarget);
    if (NewViewportImage)
    {
        ViewportImage = NewViewportImage;
        ViewportImage->SetDebugName("Editor Viewport Image");

        ViewportWidget->SetViewportImage(ViewportImage);

        RenderSettings::ChangeRenderResolution(Size.X, Size.Y);

        ViewportImageSize = Size;
        return true;
    }
    else
    {
        return false;
    }
}
