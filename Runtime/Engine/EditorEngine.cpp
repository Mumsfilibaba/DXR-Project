#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/Editor/EditorDockspaceWidget.h"
#include "Engine/EngineUI/Editor/EditorFooterWidget.h"
#include "Engine/EngineUI/Editor/EditorViewportWidget.h"
#include "Engine/EngineUI/Editor/EditorSceneHierarchyWidget.h"
#include "Engine/EngineUI/Editor/EditorPropertiesWidget.h"
#include "Engine/EngineUI/Editor/EditorContentBrowserWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Renderer/FrameResources.h"
#include "RendererCore/RenderSettings.h"

FEditorEngine::FEditorEngine()
    : FEngine()
    , SelectedActor(nullptr)
    , SelectedLight(nullptr)
    , SelectedCamera(nullptr)
    , SelectedLightProbe(nullptr)
    , DockspaceWidget(nullptr)
    , FooterWidget(nullptr)
    , LogOutputWidget(nullptr)
    , SceneHierarchyWidget(nullptr)
    , ContentBrowserWidget(nullptr)
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
        LogOutputWidget      = MakeSharedPtr<FEditorLogOutputWidget>();
        SceneHierarchyWidget = MakeSharedPtr<FEditorSceneHierarchyWidget>(this);
        FooterWidget         = MakeSharedPtr<FEditorFooterWidget>(LogOutputWidget);
        PropertiesWidget	 = MakeSharedPtr<FEditorPropertiesWidget>(this);
        ContentBrowserWidget = MakeSharedPtr<FEditorContentBrowserWidget>();
        
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
        LogOutputWidget.Reset();
        SceneHierarchyWidget.Reset();
        FooterWidget.Reset();
        ViewportWidget.Reset();
        PropertiesWidget.Reset();
        ContentBrowserWidget.Reset();
    }

    FEngine::Release();
}

void FEditorEngine::Tick(float DeltaTime)
{
    FEngine::Tick(DeltaTime);

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
