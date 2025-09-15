#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/DockspaceWidget.h"
#include "Renderer/FrameResources.h"
#include "RendererCore/RenderSettings.h"

FEditorEngine::FEditorEngine()
    : FEngine()
    , DockspaceWidget(nullptr)
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
		DockspaceWidget = MakeSharedPtr<FDockspaceWidget>();
	}

	if (!CreateViewportRenderTarget())
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
	}

	FEngine::Release();
}

void FEditorEngine::Tick(float DeltaTime)
{
	FEngine::Tick(DeltaTime);

	CreateViewportRenderTarget();
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

bool FEditorEngine::CreateViewportRenderTarget()
{
	const FIntVector2 Size = DockspaceWidget->GetViewportSize();
	FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(RenderSettings::GetBackBufferFormat(), Size.X, Size.Y, 1, 1,
		ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResource);

	FRHITextureRef NewViewportImage = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::RenderTarget);
	if (NewViewportImage)
	{
		ViewportImage = NewViewportImage;
		ViewportImage->SetDebugName("Editor Viewport Image");

		DockspaceWidget->SetViewportImage(ViewportImage);

		RenderSettings::ChangeRenderResolution(Size.X, Size.Y);
		return true;
	}
	else
	{
		return false;
	}
}
