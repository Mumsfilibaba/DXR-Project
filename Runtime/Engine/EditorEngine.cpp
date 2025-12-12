#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/Editor/EditorDockspaceWidget.h"
#include "Engine/EngineUI/Editor/EditorConsoleInputFieldWidget.h"
#include "Engine/EngineUI/Editor/EditorViewportWidget.h"
#include "Renderer/FrameResources.h"
#include "RendererCore/RenderSettings.h"

FEditorEngine::FEditorEngine()
    : FEngine()
    , DockspaceWidget(nullptr)
	, ConsoleWidget(nullptr)
    , LogOutputWidget(nullptr)
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
		DockspaceWidget = MakeSharedPtr<FEditorDockspaceWidget>(this);
		LogOutputWidget = MakeSharedPtr<FEditorLogOutputWidget>();
		
		ViewportWidget = MakeSharedPtr<FEditorViewportWidget>();
		ViewportWidget->SetViewportWidget(GetViewportWidget());

		ConsoleWidget = MakeSharedPtr<FEditorConsoleInputFieldWidget>(LogOutputWidget);
		ConsoleWidget->SetVisible(true);
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
		LogOutputWidget.Reset();
		ConsoleWidget.Reset();
		ViewportWidget.Reset();
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
