#include "Application.h"
#include "InputManager.h"

#include "Rendering/Renderer.h"
#include "Rendering/GuiContext.h"
#include "Rendering/TextureFactory.h"

std::shared_ptr<Application> Application::Instance = nullptr;

Application::Application()
{
}

Application::~Application()
{
}

void Application::Release()
{
	SAFEDELETE(CurrentScene);
	SAFEDELETE(PlatformApplication);
}

void Application::Run()
{
	// Run-Loop
	bool IsRunning = true;
	while (IsRunning)
	{
		IsRunning = Tick();
	}
}

bool Application::Tick()
{
	bool ShouldExit = PlatformApplication->Tick();

	GuiContext::Get()->BeginFrame();

	Timer.Tick();

	ImGui::SetNextWindowPos(ImVec2(10, 5));
	ImGui::SetNextWindowSize(ImVec2(300, 1000));
	ImGui::Begin("DebugWindow", nullptr, 
		ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoSavedSettings);
	
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));

	static std::string AdapterName = Renderer::Get()->GetDevice()->GetAdapterName();

	const Float64 Delta = Timer.GetDeltaTime().AsMilliSeconds();
	ImGui::Text("Adapter: %s", AdapterName.c_str());
	ImGui::Text("Frametime: %.4f ms", Delta);
	ImGui::Text("FPS: %d", static_cast<Uint32>(1000 / Delta));

	ImGui::PopStyleColor();

	ImGui::End();

	GuiContext::Get()->EndFrame();

	Renderer::Get()->Tick(*CurrentScene);

	return ShouldExit;
}

void Application::SetCursor(std::shared_ptr<WindowsCursor> Cursor)
{
	PlatformApplication->SetCursor(Cursor);
}

void Application::SetActiveWindow(std::shared_ptr<WindowsWindow>& ActiveWindow)
{
	PlatformApplication->SetActiveWindow(ActiveWindow);
}

void Application::SetCapture(std::shared_ptr<WindowsWindow> Capture)
{
	PlatformApplication->SetCapture(Capture);
}

void Application::SetCursorPos(std::shared_ptr<WindowsWindow>& RelativeWindow, Int32 X, Int32 Y)
{
	PlatformApplication->SetCursorPos(RelativeWindow, X, Y);
}

ModifierKeyState Application::GetModifierKeyState() const
{
	return PlatformApplication->GetModifierKeyState();
}

std::shared_ptr<WindowsWindow> Application::GetWindow() const
{
	return Window;
}

std::shared_ptr<WindowsWindow> Application::GetActiveWindow() const
{
	return PlatformApplication->GetActiveWindow();
}

std::shared_ptr<WindowsWindow> Application::GetCapture() const
{
	return PlatformApplication->GetCapture();
}

void Application::GetCursorPos(std::shared_ptr<WindowsWindow>& RelativeWindow, Int32& OutX, Int32& OutY) const
{
	PlatformApplication->GetCursorPos(RelativeWindow, OutX, OutY);
}

Application* Application::Make()
{
	Instance = std::shared_ptr<Application>(new Application());
	if (Instance->Initialize())
	{
		return Instance.get();
	}
	else
	{
		return nullptr;
	}
}

Application* Application::Get()
{
	return Instance.get();
}

void Application::OnWindowResized(std::shared_ptr<WindowsWindow>& InWindow, Uint16 Width, Uint16 Height)
{
	UNREFERENCED_PARAMETER(InWindow);

	if (Renderer::Get())
	{
		Renderer::Get()->OnResize(Width, Height);
	}
}

void Application::OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState)
{
	UNREFERENCED_PARAMETER(ModierKeyState);

	InputManager::Get().RegisterKeyUp(KeyCode);

	if (GuiContext::Get())
	{
		GuiContext::Get()->OnKeyReleased(KeyCode);
	}
}

void Application::OnKeyPressed(EKey KeyCode, const ModifierKeyState& ModierKeyState)
{
	UNREFERENCED_PARAMETER(ModierKeyState);

	InputManager::Get().RegisterKeyDown(KeyCode);

	if (Renderer::Get())
	{
		Renderer::Get()->OnKeyPressed(KeyCode);
	}

	if (GuiContext::Get())
	{
		GuiContext::Get()->OnKeyPressed(KeyCode);
	}
}

void Application::OnMouseMove(Int32 X, Int32 Y)
{
	if (Renderer::Get())
	{
		Renderer::Get()->OnMouseMove(X, Y);
	}
}

void Application::OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState)
{
	UNREFERENCED_PARAMETER(ModierKeyState);

	std::shared_ptr<WindowsWindow> CaptureWindow = GetCapture();
	if (CaptureWindow)
	{
		SetCapture(nullptr);
	}

	if (GuiContext::Get())
	{
		GuiContext::Get()->OnMouseButtonReleased(Button);
	}
}

void Application::OnMouseButtonPressed(EMouseButton Button, const ModifierKeyState& ModierKeyState)
{
	UNREFERENCED_PARAMETER(ModierKeyState);

	std::shared_ptr<WindowsWindow> CaptureWindow = GetCapture();
	if (!CaptureWindow)
	{
		std::shared_ptr<WindowsWindow> ActiveWindow = GetActiveWindow();
		SetCapture(ActiveWindow);
	}

	if (GuiContext::Get())
	{
		GuiContext::Get()->OnMouseButtonPressed(Button);
	}
}

void Application::OnMouseScrolled(Float32 HorizontalDelta, Float32 VerticalDelta)
{
	if (GuiContext::Get())
	{
		GuiContext::Get()->OnMouseScrolled(HorizontalDelta, VerticalDelta);
	}
}

void Application::OnCharacterInput(Uint32 Character)
{
	if (GuiContext::Get())
	{
		GuiContext::Get()->OnCharacterInput(Character);
	}
}

bool Application::Initialize()
{
	// Application
	HINSTANCE InstanceHandle = static_cast<HINSTANCE>(GetModuleHandle(NULL));
	PlatformApplication = WindowsApplication::Make(InstanceHandle);
	if (PlatformApplication)
	{
		PlatformApplication->SetEventHandler(std::shared_ptr<EventHandler>(Instance));
	}
	else
	{
		return false;
	}

	// Window
	WindowProperties WindowProperties;
	WindowProperties.Title	= "DXR";
	WindowProperties.Width	= 1920;
	WindowProperties.Height = 1080;
	WindowProperties.Style	=	WINDOW_STYLE_FLAG_TITLED | WINDOW_STYLE_FLAG_CLOSABLE | 
								WINDOW_STYLE_FLAG_MINIMIZABLE | WINDOW_STYLE_FLAG_MAXIMIZABLE |
								WINDOW_STYLE_FLAG_RESIZEABLE;

	Window = PlatformApplication->MakeWindow(WindowProperties);
	if (Window)
	{
		Window->Show();
	}
	else
	{
		return false;
	}

	InitializeCursors();

	// Renderer
	Renderer* Renderer = Renderer::Make(GetWindow());
	if (!Renderer)
	{
		::MessageBox(0, "FAILED to create Renderer", "ERROR", MB_ICONERROR);
		return false;
	}

	// ImGui
	GuiContext* GUIContext = GuiContext::Make(Renderer->GetDevice());
	if (!GUIContext)
	{
		::MessageBox(0, "FAILED to create ImGuiContext", "ERROR", MB_ICONERROR);
		return false;
	}

	// Initialize Scene
	constexpr Float32	SphereOffset	= 1.25f;
	constexpr Uint32	SphereCountX	= 8;
	constexpr Float32	StartPositionX	= (-static_cast<Float32>(SphereCountX) * SphereOffset) / 2.0f;
	constexpr Uint32	SphereCountY	= 8;
	constexpr Float32	StartPositionY	= (-static_cast<Float32>(SphereCountY) * SphereOffset) / 2.0f;
	constexpr Float32	MetallicDelta	= 1.0f / SphereCountY;
	constexpr Float32	RoughnessDelta	= 1.0f / SphereCountX;
	
	Actor* NewActor = nullptr;
	RenderComponent* NewComponent = nullptr;
	CurrentScene = new Scene();

	// Create Spheres
	MeshData SphereMeshData = MeshFactory::CreateSphere(3);
	std::shared_ptr<Mesh> SphereMesh = Mesh::Make(Renderer->GetDevice().get(), SphereMeshData);

	// Create standard textures
	Byte Pixels[] = { 255, 0, 0, 255 };
	std::shared_ptr<D3D12Texture> BaseTexture = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromMemory(Renderer->GetDevice().get(), Pixels, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!BaseTexture)
	{
		return false;
	}
	else
	{
		BaseTexture->SetName("BaseTexture");
	}

	Pixels[0] = 127;
	Pixels[1] = 127;
	Pixels[2] = 255;

	std::shared_ptr<D3D12Texture> BaseNormal = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromMemory(Renderer->GetDevice().get(), Pixels, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!BaseNormal)
	{
		return false;
	}
	else
	{
		BaseNormal->SetName("BaseNormal");
	}

	XMFLOAT4X4 Matrix;
	MaterialProperties MatProperties;
	for (Uint32 y = 0; y < SphereCountY; y++)
	{
		for (Uint32 x = 0; x < SphereCountX; x++)
		{
			XMStoreFloat4x4(&Matrix, XMMatrixTranspose(XMMatrixTranslation(StartPositionX + (x * SphereOffset), StartPositionY + (y * SphereOffset), 0)));

			NewActor = new Actor();
			NewActor->SetTransform(Matrix);
			CurrentScene->AddActor(NewActor);

			NewComponent = new RenderComponent(NewActor);
			NewComponent->Mesh		= SphereMesh;
			NewComponent->Material	= std::make_shared<Material>(MatProperties);
			
			NewComponent->Material->AlbedoMap = BaseTexture;
			NewComponent->Material->NormalMap = BaseNormal;
			NewComponent->Material->Initialize(Renderer->GetDevice().get());

			NewActor->AddComponent(NewComponent);

			MatProperties.Roughness += RoughnessDelta;
		}

		MatProperties.Roughness	= 0.05f;
		MatProperties.Metallic	+= MetallicDelta;
	}

	// Create Other Meshes
	MeshData CubeMeshData = MeshFactory::CreateCube();
	
	NewActor = new Actor();
	CurrentScene->AddActor(NewActor);

	XMStoreFloat4x4(&Matrix, XMMatrixTranspose(XMMatrixTranslation(0.0f, 0.0f, -2.0f)));
	NewActor->SetTransform(Matrix);

	MatProperties.AO		= 1.0f;
	MatProperties.Metallic	= 0.0f;
	MatProperties.Roughness = 0.5f;

	NewComponent = new RenderComponent(NewActor);
	NewComponent->Mesh		= Mesh::Make(Renderer->GetDevice().get(), CubeMeshData);
	NewComponent->Material	= std::make_shared<Material>(MatProperties);

	std::shared_ptr<D3D12Texture> AlbedoMap = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().get(), "../Assets/Textures/RockySoil_Albedo.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!AlbedoMap)
	{
		return false;
	}
	else
	{
		AlbedoMap->SetName("AlbedoMap");
	}

	std::shared_ptr<D3D12Texture> NormalMap = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().get(), "../Assets/Textures/RockySoil_Normal.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!NormalMap)
	{
		return false;
	}
	else
	{
		NormalMap->SetName("NormalMap");
	}

	NewComponent->Material->AlbedoMap = AlbedoMap;
	NewComponent->Material->NormalMap = NormalMap;
	NewComponent->Material->Initialize(Renderer->GetDevice().get());

	NewActor->AddComponent(NewComponent);

	// Reset timer before starting
	Timer.Reset();

	return true;
}
