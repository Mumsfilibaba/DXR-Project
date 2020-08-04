#include "Application.h"
#include "InputManager.h"

#include "Rendering/Renderer.h"
#include "Rendering/GuiContext.h"
#include "Rendering/TextureFactory.h"

#include "Scene/MeshComponent.h"
#include "Scene/Scene.h"
#include "Scene/PointLight.h"

#include "Windows/WindowsConsoleOutput.h"

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
	
	Timer.Tick();
	const Float32 Delta = static_cast<Float32>(Timer.GetDeltaTime().AsSeconds());
	const Float32 RotationSpeed = 45.0f;

	Float32 Speed = 1.0f;
	if (InputManager::Get().IsKeyDown(EKey::KEY_LEFT_SHIFT))
	{
		Speed = 4.0f;
	}

	if (InputManager::Get().IsKeyDown(EKey::KEY_RIGHT))
	{
		CurrentCamera->Rotate(0.0f, XMConvertToRadians(RotationSpeed * Delta), 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_LEFT))
	{
		CurrentCamera->Rotate(0.0f, XMConvertToRadians(-RotationSpeed * Delta), 0.0f);
	}

	if (InputManager::Get().IsKeyDown(EKey::KEY_UP))
	{
		CurrentCamera->Rotate(XMConvertToRadians(-RotationSpeed * Delta), 0.0f, 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_DOWN))
	{
		CurrentCamera->Rotate(XMConvertToRadians(RotationSpeed * Delta), 0.0f, 0.0f);
	}

	if (InputManager::Get().IsKeyDown(EKey::KEY_W))
	{
		CurrentCamera->Move(0.0f, 0.0f, Speed * Delta);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_S))
	{
		CurrentCamera->Move(0.0f, 0.0f, -Speed * Delta);
	}

	if (InputManager::Get().IsKeyDown(EKey::KEY_A))
	{
		CurrentCamera->Move(Speed * Delta, 0.0f, 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_D))
	{
		CurrentCamera->Move(-Speed * Delta, 0.0f, 0.0f);
	}

	if (InputManager::Get().IsKeyDown(EKey::KEY_Q))
	{
		CurrentCamera->Move(0.0f, Speed * Delta, 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_E))
	{
		CurrentCamera->Move(0.0f, -Speed * Delta, 0.0f);
	}

	CurrentCamera->UpdateMatrices();

	GuiContext::Get()->BeginFrame();

	DrawDebugData();
	DrawSideWindow();

	GuiContext::Get()->EndFrame();

	Renderer::Get()->Tick(*CurrentScene);

	return ShouldExit;
}

void Application::DrawDebugData()
{
	constexpr Uint32 Width = 300;
	
	WindowShape WindowShape;
	Window->GetWindowShape(WindowShape);

	ImGui::SetNextWindowPos(ImVec2(static_cast<Float32>(WindowShape.Width - Width), 5.0f));
	ImGui::SetNextWindowSize(ImVec2(Width, WindowShape.Height));

	ImGui::Begin("DebugWindow", nullptr,
		ImGuiWindowFlags_NoBackground | 
		ImGuiWindowFlags_NoTitleBar | 
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoDecoration | 
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoSavedSettings);

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));

	static std::string AdapterName = Renderer::Get()->GetDevice()->GetAdapterName();

	const Float64 Delta = Timer.GetDeltaTime().AsMilliSeconds();
	ImGui::Text("Adapter: %s", AdapterName.c_str());
	ImGui::Text("Frametime: %.4f ms", Delta);
	ImGui::Text("FPS: %d", static_cast<Uint32>(1000 / Delta));

	ImGui::PopStyleColor();

	ImGui::End();
}

void Application::DrawSideWindow()
{
	constexpr Uint32 Width = 450;

	WindowShape WindowShape;
	Window->GetWindowShape(WindowShape);

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(Width, WindowShape.Height));

	ImGui::Begin("Window", nullptr, 
		ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoTitleBar | 
		ImGuiWindowFlags_NoSavedSettings);

	ImGuiTabBarFlags TabBarFlags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("Menu", TabBarFlags))
	{
		if (ImGui::BeginTabItem("Renderer"))
		{
			DrawRenderSettings();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Scene"))
		{
			DrawSceneInfo();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

void Application::DrawRenderSettings()
{
	ImGui::BeginChild("RendererInfo");

	WindowShape WindowShape;
	Window->GetWindowShape(WindowShape);

	ImGui::Text("Renderer Info");

	ImGui::Indent();
	ImGui::Text("Resolution: %d x %d", WindowShape.Width, WindowShape.Height);

	ImGui::EndChild();
}

void Application::DrawSceneInfo()
{
	ImGui::Text("Current Scene");
	ImGui::Indent();

	ImGui::BeginChild("SceneInfo");

	// Actors
	if (ImGui::TreeNode("Actors"))
	{
		for (Actor* Actor : CurrentScene->GetActors())
		{
			if (ImGui::TreeNode(Actor->GetDebugName().c_str()))
			{
				// Transform
				if (ImGui::TreeNode("Transform"))
				{
					const XMFLOAT3& Position = Actor->GetTransform().GetPosition();

					Float32 Arr[3] = { Position.x, Position.y, Position.z };
					if (ImGui::DragFloat3("Position", Arr, 0.5f))
					{
						Actor->GetTransform().SetPosition(Arr[0], Arr[1], Arr[2]);
					}

					ImGui::TreePop();
				}

				// MeshComponent
				MeshComponent* MComponent = Actor->GetComponentOfType<MeshComponent>();
				if (MComponent)
				{
					if (ImGui::TreeNode("MeshComponent"))
					{
						const XMFLOAT3& Color = MComponent->Material->GetMaterialProperties().Albedo;

						Float32 Arr[3] = { Color.x, Color.y, Color.z };
						if (ImGui::ColorEdit3("Albedo", Arr))
						{
							MComponent->Material->SetAlbedo(Arr[0], Arr[1], Arr[2]);
						}

						Float32 Roughness = MComponent->Material->GetMaterialProperties().Roughness;
						if (ImGui::SliderFloat("Roughness", &Roughness, 0.01f, 1.0f, "%.2f"))
						{
							MComponent->Material->SetRoughness(Roughness);
						}

						Float32 Metallic = MComponent->Material->GetMaterialProperties().Metallic;
						if (ImGui::SliderFloat("Metallic", &Metallic, 0.01f, 1.0f, "%.2f"))
						{
							MComponent->Material->SetMetallic(Metallic);
						}

						Float32 AO = MComponent->Material->GetMaterialProperties().AO;
						if (ImGui::SliderFloat("Ambient Occlusion", &AO, 0.01f, 1.0f, "%.2f"))
						{
							MComponent->Material->SetAmbientOcclusion(AO);
						}

						ImGui::TreePop();
					}
				}

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	// Lights
	if (ImGui::TreeNode("Lights"))
	{
		for (Light* CurrentLight : CurrentScene->GetLights())
		{
			if (IsOfType<PointLight>(CurrentLight))
			{
				if (ImGui::TreeNode("PointLight"))
				{
					const XMFLOAT3& Color = CurrentLight->GetColor();

					Float32 Arr[3] = { Color.x, Color.y, Color.z };
					if (ImGui::ColorEdit3("Color", Arr))
					{
						CurrentLight->SetColor(Arr[0], Arr[1], Arr[2]);
					}

					const XMFLOAT3& Position = static_cast<PointLight*>(CurrentLight)->GetPosition();

					Float32 Arr2[3] = { Position.x, Position.y, Position.z };
					if (ImGui::DragFloat3("Position", Arr2, 0.5f))
					{
						static_cast<PointLight*>(CurrentLight)->SetPosition(Arr2[0], Arr2[1], Arr2[2]);
					}

					Float32 Intensity = CurrentLight->GetIntensity();
					if (ImGui::SliderFloat("Intensity", &Intensity, 0.01f, 10000.0f, "%.2f"))
					{
						CurrentLight->SetIntensity(Intensity);
					}

					ImGui::TreePop();
				}
			}
		}

		ImGui::TreePop();
	}

	ImGui::EndChild();
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

	if (GuiContext::Get())
	{
		GuiContext::Get()->OnKeyPressed(KeyCode);
	}
}

void Application::OnMouseMove(Int32 X, Int32 Y)
{
	UNREFERENCED_PARAMETER(X);
	UNREFERENCED_PARAMETER(Y);
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

	// Cursors
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
	MeshComponent* NewComponent = nullptr;
	CurrentScene = Scene::LoadFromFile("../Assets/Scenes/Sponza/Sponza.obj", Renderer->GetDevice().get());

	// Create Spheres
	MeshData SphereMeshData = MeshFactory::CreateSphere(3);
	std::shared_ptr<Mesh> SphereMesh = Mesh::Make(Renderer->GetDevice().get(), SphereMeshData);

	// Create standard textures
	Byte Pixels[] = { 255, 255, 255, 255 };
	std::shared_ptr<D3D12Texture> BaseTexture = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromMemory(Renderer->GetDevice().get(), Pixels, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!BaseTexture)
	{
		return false;
	}
	else
	{
		BaseTexture->SetDebugName("BaseTexture");
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
		BaseNormal->SetDebugName("BaseNormal");
	}

	Pixels[0] = 255;
	Pixels[1] = 255;
	Pixels[2] = 255;

	std::shared_ptr<D3D12Texture> WhiteTexture = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromMemory(Renderer->GetDevice().get(), Pixels, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!WhiteTexture)
	{
		return false;
	}
	else
	{
		WhiteTexture->SetDebugName("WhiteTexture");
	}

	MaterialProperties MatProperties;
	Uint32 SphereIndex = 0;
	for (Uint32 y = 0; y < SphereCountY; y++)
	{
		for (Uint32 x = 0; x < SphereCountX; x++)
		{
			NewActor = new Actor();
			NewActor->GetTransform().SetPosition(StartPositionX + (x * SphereOffset), 8.0f + StartPositionY + (y * SphereOffset), 0.0f);

			NewActor->SetDebugName("Sphere[" + std::to_string(SphereIndex) + "]");
			SphereIndex++;

			CurrentScene->AddActor(NewActor);

			NewComponent = new MeshComponent(NewActor);
			NewComponent->Mesh		= SphereMesh;
			NewComponent->Material	= std::make_shared<Material>(MatProperties);
			
			NewComponent->Material->AlbedoMap	= BaseTexture;
			NewComponent->Material->NormalMap	= BaseNormal;
			NewComponent->Material->Roughness	= WhiteTexture;
			NewComponent->Material->Height		= WhiteTexture;
			NewComponent->Material->AO			= WhiteTexture;
			NewComponent->Material->Metallic	= WhiteTexture;
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

	NewActor->SetDebugName("Cube");
	NewActor->GetTransform().SetPosition(0.0f, 2.0f, -2.0f);

	MatProperties.AO		= 1.0f;
	MatProperties.Metallic	= 1.0f;
	MatProperties.Roughness = 1.0f;

	NewComponent = new MeshComponent(NewActor);
	NewComponent->Mesh		= Mesh::Make(Renderer->GetDevice().get(), CubeMeshData);
	NewComponent->Material	= std::make_shared<Material>(MatProperties);

	std::shared_ptr<D3D12Texture> AlbedoMap = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().get(), "../Assets/Textures/Gate_Albedo.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!AlbedoMap)
	{
		return false;
	}
	else
	{
		AlbedoMap->SetDebugName("AlbedoMap");
	}

	std::shared_ptr<D3D12Texture> NormalMap = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().get(), "../Assets/Textures/Gate_Normal.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!NormalMap)
	{
		return false;
	}
	else
	{
		NormalMap->SetDebugName("NormalMap");
	}

	std::shared_ptr<D3D12Texture> AOMap = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().get(), "../Assets/Textures/Gate_AO.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!AOMap)
	{
		return false;
	}
	else
	{
		AOMap->SetDebugName("AOMap");
	}

	std::shared_ptr<D3D12Texture> RoughnessMap = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().get(), "../Assets/Textures/Gate_Roughness.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!RoughnessMap)
	{
		return false;
	}
	else
	{
		RoughnessMap->SetDebugName("RoughnessMap");
	}

	std::shared_ptr<D3D12Texture> HeightMap = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().get(), "../Assets/Textures/Gate_Height.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!HeightMap)
	{
		return false;
	}
	else
	{
		HeightMap->SetDebugName("HeightMap");
	}

	std::shared_ptr<D3D12Texture> MetallicMap = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().get(), "../Assets/Textures/Gate_Metallic.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!MetallicMap)
	{
		return false;
	}
	else
	{
		MetallicMap->SetDebugName("MetallicMap");
	}

	NewComponent->Material->AlbedoMap	= AlbedoMap;
	NewComponent->Material->NormalMap	= NormalMap;
	NewComponent->Material->Roughness	= RoughnessMap;
	NewComponent->Material->Height		= HeightMap;
	NewComponent->Material->AO			= AOMap;
	NewComponent->Material->Metallic	= MetallicMap;
	NewComponent->Material->Initialize(Renderer->GetDevice().get());

	NewActor->AddComponent(NewComponent);

	CurrentCamera = new Camera();
	CurrentScene->AddCamera(CurrentCamera);

	// Add lightsource
	PointLight* Light = new PointLight();
	Light->SetPosition(0.0f, 10.0f, -10.0f);
	Light->SetColor(1.0f, 1.0f, 1.0f);
	Light->SetIntensity(400.0f);
	CurrentScene->AddLight(Light);

	// Reset timer before starting
	Timer.Reset();

	return true;
}
