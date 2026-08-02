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
#include "Engine/World/Components/CameraComponent.h"
#include "RendererCore/RenderSettings.h"
#include "RendererCore/Interfaces/IRendererModule.h"

FEditorEngine::FEditorEngine()
    : FEngine()
    , SelectedActor(nullptr)
    , LastViewportCamera(nullptr)
    , ActorRemovedDelegateHandle()
    , DockspaceWidget(nullptr)
    , FooterWidget(nullptr)
    , OutputLogWidget(nullptr)
    , ViewportWidget(nullptr)
    , SceneHierarchyWidget(nullptr)
    , PropertiesWidget(nullptr)
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

    ActorRemovedDelegateHandle = GetWorld()->GetOnActorRemovedEvent().AddRaw(this, &FEditorEngine::OnActorRemoved);

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

        ViewportWidget = MakeSharedPtr<FEditorViewportWidget>(this);
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
    if (GetWorld() && ActorRemovedDelegateHandle.IsValid())
    {
        GetWorld()->GetOnActorRemovedEvent().Unbind(ActorRemovedDelegateHandle);
        ActorRemovedDelegateHandle.Reset();
    }

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

    if (ViewportWidget)
    {
        ViewportWidget->Tick(DeltaTime);
    }

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

    const IntVector2 Size = ViewportWidget->GetViewportSize();
    if (ViewportImageSize != Size)
    {
        CreateViewportRenderTarget();
    }
}

FSceneRenderPacket FEditorEngine::BuildRenderPacket()
{
    TRACE_FUNCTION_SCOPE();

    FSceneRenderPacket Packet = FEngine::BuildRenderPacket();
    Packet.View.Scene              = GetWorld()->GetSceneInterface();
    Packet.View.RenderTarget       = ViewportImage.Get();
    Packet.View.DebugView          = ViewportWidget->GetDebugView();
    Packet.View.SecondaryDebugView = ViewportWidget->GetSecondaryDebugView();

    FCameraComponent* ViewCamera = GetActiveViewportCamera();
    if (ViewCamera)
    {
        ViewCamera->PrepareSceneViewInfo(Packet.View.CameraSnapshot);
        Packet.View.bHasCamera = true;
    }

    const bool bCameraChanged = ViewCamera != LastViewportCamera;
    Packet.View.bCameraCut = bCameraChanged || ViewportWidget->ConsumeCameraCut();
    LastViewportCamera = ViewCamera;

    // Resolve the editor selection to a stable ObjectID on the main thread so the render thread never reads live editor state.
    if (FActor* Selected = GetSelectedActor())
    {
        if (IScene* Scene = Packet.View.Scene)
        {
            Packet.SelectedObjectIDs.Add(Scene->GetOrCreateObjectID(Selected));
        }
    }

    return Packet;
}

FCameraComponent* FEditorEngine::GetActiveViewportCamera() const
{
    return ViewportWidget ? ViewportWidget->GetViewCamera() : nullptr;
}

void FEditorEngine::SetSelectedActor(FActor* InActor)
{
    SelectedActor = InActor;
}

void FEditorEngine::ClearSelection()
{
    SelectedActor = nullptr;
}

void FEditorEngine::OnActorRemoved(FActor* RemovedActor)
{
    if (ViewportWidget)
    {
        ViewportWidget->OnActorRemoved(RemovedActor);
    }

    if (SelectedActor == RemovedActor)
    {
        ClearSelection();
    }
}

bool FEditorEngine::CreateViewportRenderTarget()
{
    const IntVector2 Size = ViewportWidget->GetViewportSize();
    if (Size.X == 0 || Size.Y == 0)
    {
        return ViewportImage != nullptr;
    }

    const ETextureUsageFlags UsageFlags = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture;
    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(FEditorEngine::ViewportImageFormat, Size.X, Size.Y, 1, 1, UsageFlags);

    FRHITextureRef NewViewportImage = RHI::CreateTexture(TextureDesc, ERHIResourceState::RenderTarget);
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
