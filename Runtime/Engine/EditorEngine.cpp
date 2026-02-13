#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/Editor/EditorDockspaceWidget.h"
#include "Engine/EngineUI/Editor/EditorFooterWidget.h"
#include "Engine/EngineUI/Editor/EditorViewportWidget.h"
#include "Engine/EngineUI/Editor/EditorSceneHierarchyWidget.h"
#include "Engine/EngineUI/Editor/EditorPropertiesWidget.h"
#include "Engine/EngineUI/Editor/EditorRendererSettingsWidget.h"
#include "Engine/EngineUI/Editor/EditorContentBrowserWidget.h"
#include "Engine/EngineUI/Editor/EditorGuizmoWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
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
        DockspaceWidget      = MakeSharedPtr<FEditorDockspaceWidget>(this);
        OutputLogWidget      = MakeSharedPtr<FEditorOutputLogWidget>();
        SceneHierarchyWidget = MakeSharedPtr<FEditorSceneHierarchyWidget>(this);
        FooterWidget         = MakeSharedPtr<FEditorFooterWidget>(OutputLogWidget);
        PropertiesWidget	 = MakeSharedPtr<FEditorPropertiesWidget>(this);
        RendererSettingsWidget = MakeSharedPtr<FEditorRendererSettingsWidget>();
        ContentBrowserWidget = MakeSharedPtr<FEditorContentBrowserWidget>();
        GuizmoWidget         = MakeSharedPtr<FEditorGuizmoWidget>(this);
        
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
        RendererSettingsWidget.Reset();
        ContentBrowserWidget.Reset();
        GuizmoWidget.Reset();
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

                LOG_INFO("[EditorPick] Completed. ObjectID=%u Actor=%s", PickedObjectID, PickedActor ? *PickedActor->GetName() : "nullptr");

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
    RenderView.DebugView    = ViewportWidget ? ViewportWidget->GetDebugView() : FSceneRenderView::EDebugView::None;

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
    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(RenderSettings::GetBackBufferFormat(), Size.X, Size.Y, 1, 1, UsageFlags);

    FRHITextureRef NewViewportImage = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::RenderTarget);
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
