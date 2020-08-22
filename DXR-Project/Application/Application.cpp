#include "Application.h"
#include "Input.h"

#include "Events/EventQueue.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Events/WindowEvent.h"

// TODO: Mayebe should handle this in a different way
#include "EngineLoop.h"

#include "Rendering/Renderer.h"
#include "Rendering/DebugUI.h"
#include "Rendering/TextureFactory.h"

#include "Scene/Scene.h"
#include "Scene/PointLight.h"
#include "Scene/DirectionalLight.h"
#include "Scene/Components/MeshComponent.h"

#include "Windows/WindowsConsoleOutput.h"

TSharedPtr<Application> Application::Instance = nullptr;

Application::Application()
{
}

Application::~Application()
{
}

void Application::Release()
{
	DebugUI::Release();

	SAFEDELETE(CurrentScene);
	SAFEDELETE(PlatformApplication);
}

void Application::Tick()
{
	// Tick OS
	if (!PlatformApplication->Tick())
	{
		EngineLoop::Exit();
	}
	
	// Run app
	const Float32 Delta = static_cast<Float32>(EngineLoop::GetDeltaTime().AsSeconds());
	const Float32 RotationSpeed = 45.0f;

	Float32 Speed = 1.0f;
	if (Input::IsKeyDown(EKey::KEY_LEFT_SHIFT))
	{
		Speed = 4.0f;
	}

	if (Input::IsKeyDown(EKey::KEY_RIGHT))
	{
		CurrentCamera->Rotate(0.0f, XMConvertToRadians(RotationSpeed * Delta), 0.0f);
	}
	else if (Input::IsKeyDown(EKey::KEY_LEFT))
	{
		CurrentCamera->Rotate(0.0f, XMConvertToRadians(-RotationSpeed * Delta), 0.0f);
	}

	if (Input::IsKeyDown(EKey::KEY_UP))
	{
		CurrentCamera->Rotate(XMConvertToRadians(-RotationSpeed * Delta), 0.0f, 0.0f);
	}
	else if (Input::IsKeyDown(EKey::KEY_DOWN))
	{
		CurrentCamera->Rotate(XMConvertToRadians(RotationSpeed * Delta), 0.0f, 0.0f);
	}

	if (Input::IsKeyDown(EKey::KEY_W))
	{
		CurrentCamera->Move(0.0f, 0.0f, Speed * Delta);
	}
	else if (Input::IsKeyDown(EKey::KEY_S))
	{
		CurrentCamera->Move(0.0f, 0.0f, -Speed * Delta);
	}

	if (Input::IsKeyDown(EKey::KEY_A))
	{
		CurrentCamera->Move(Speed * Delta, 0.0f, 0.0f);
	}
	else if (Input::IsKeyDown(EKey::KEY_D))
	{
		CurrentCamera->Move(-Speed * Delta, 0.0f, 0.0f);
	}

	if (Input::IsKeyDown(EKey::KEY_Q))
	{
		CurrentCamera->Move(0.0f, Speed * Delta, 0.0f);
	}
	else if (Input::IsKeyDown(EKey::KEY_E))
	{
		CurrentCamera->Move(0.0f, -Speed * Delta, 0.0f);
	}

	CurrentCamera->UpdateMatrices();

	DrawDebugData();
	DrawSideWindow();

	Renderer::Get()->Tick(*CurrentScene);
}

void Application::DrawDebugData()
{
	static std::string AdapterName = Renderer::Get()->GetDevice()->GetAdapterName();

	const Float64 Delta = EngineLoop::GetDeltaTime().AsMilliSeconds();
	DebugUI::DrawDebugString("Adapter: "	+ AdapterName);
	DebugUI::DrawDebugString("Frametime: "	+ std::to_string(Delta) + " ms");
	DebugUI::DrawDebugString("FPS: "		+ std::to_string(static_cast<Uint32>(1000 / Delta)));
}

void Application::DrawSideWindow()
{
	DebugUI::DrawUI([]
		{
			constexpr Uint32 Width = 450;

			WindowShape WindowShape;
			Application::Get()->GetWindow()->GetWindowShape(WindowShape);

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
					Application::Get()->DrawRenderSettings();
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Scene"))
				{
					Application::Get()->DrawSceneInfo();
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			ImGui::End();
		});
}

void Application::DrawRenderSettings()
{
	ImGui::BeginChild("RendererInfo");

	WindowShape WindowShape;
	Window->GetWindowShape(WindowShape);

	ImGui::Text("Renderer Info");

	ImGui::Indent();
	ImGui::Text("Resolution: %d x %d", WindowShape.Width, WindowShape.Height);

	bool Enabled = Renderer::Get()->IsPrePassEnabled();
	if (ImGui::Checkbox("Enable Z-PrePass", &Enabled))
	{
		Renderer::Get()->SetPrePassEnable(Enabled);
	}

	Enabled = Renderer::Get()->IsVerticalSyncEnabled();
	if (ImGui::Checkbox("Enable VSync", &Enabled))
	{
		Renderer::Get()->SetVerticalSyncEnable(Enabled);
	}

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
						if (ImGui::SliderFloat("RoughnessMap", &Roughness, 0.01f, 1.0f, "%.2f"))
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
			if (IsSubClassOf<PointLight>(CurrentLight))
			{
				if (ImGui::TreeNode("PointLight"))
				{
					const XMFLOAT3& Color = CurrentLight->GetColor();

					Float32 Arr[3] = { Color.x, Color.y, Color.z };
					if (ImGui::ColorEdit3("Color", Arr))
					{
						CurrentLight->SetColor(Arr[0], Arr[1], Arr[2]);
					}

					const XMFLOAT3& Position = Cast<PointLight>(CurrentLight)->GetPosition();

					Float32 Arr2[3] = { Position.x, Position.y, Position.z };
					if (ImGui::DragFloat3("Position", Arr2, 0.5f))
					{
						Cast<PointLight>(CurrentLight)->SetPosition(Arr2[0], Arr2[1], Arr2[2]);
					}

					Float32 Intensity = CurrentLight->GetIntensity();
					if (ImGui::SliderFloat("Intensity", &Intensity, 0.01f, 10000.0f, "%.2f"))
					{
						CurrentLight->SetIntensity(Intensity);
					}

					ImGui::TreePop();
				}
			}
			else if (IsSubClassOf<DirectionalLight>(CurrentLight))
			{
				if (ImGui::TreeNode("DirectionalLight"))
				{
					const XMFLOAT3& Color = CurrentLight->GetColor();

					Float32 Arr[3] = { Color.x, Color.y, Color.z };
					if (ImGui::ColorEdit3("Color", Arr))
					{
						CurrentLight->SetColor(Arr[0], Arr[1], Arr[2]);
					}

					const XMFLOAT3& Direction = Cast<DirectionalLight>(CurrentLight)->GetDirection();

					Float32 Arr2[3] = { Direction.x, Direction.y, Direction.z };
					if (ImGui::DragFloat3("Direction", Arr2, 0.5f))
					{
						Cast<DirectionalLight>(CurrentLight)->SetDirection(Arr2[0], Arr2[1], Arr2[2]);
					}

					const XMFLOAT3& Position = Cast<DirectionalLight>(CurrentLight)->GetShadowMapPosition();

					Float32 Arr3[3] = { Position.x, Position.y, Position.z };
					if (ImGui::DragFloat3("ShadowMap Position", Arr3, 0.5f))
					{
						Cast<DirectionalLight>(CurrentLight)->SetShadowMapPosition(Arr3[0], Arr3[1], Arr3[2]);
					}

					Float32 ShadowBias = Cast<DirectionalLight>(CurrentLight)->GetShadowBias();
					if (ImGui::SliderFloat("Shadow Bias", &ShadowBias, 0.0001f, 0.1f, "%.4f"))
					{
						Cast<DirectionalLight>(CurrentLight)->SetShadowBias(ShadowBias);
					}

					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("A Bias value used in lightning calculations\nwhen measuring the depth in a ShadowMap");
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

void Application::SetCursor(TSharedPtr<WindowsCursor> Cursor)
{
	PlatformApplication->SetCursor(Cursor);
}

void Application::SetActiveWindow(TSharedPtr<WindowsWindow>& ActiveWindow)
{
	PlatformApplication->SetActiveWindow(ActiveWindow);
}

void Application::SetCapture(TSharedPtr<WindowsWindow> Capture)
{
	PlatformApplication->SetCapture(Capture);
}

void Application::SetCursorPos(TSharedPtr<WindowsWindow>& RelativeWindow, Int32 X, Int32 Y)
{
	PlatformApplication->SetCursorPos(RelativeWindow, X, Y);
}

ModifierKeyState Application::GetModifierKeyState() const
{
	return PlatformApplication->GetModifierKeyState();
}

TSharedPtr<WindowsWindow> Application::GetWindow() const
{
	return Window;
}

TSharedPtr<WindowsWindow> Application::GetActiveWindow() const
{
	return PlatformApplication->GetActiveWindow();
}

TSharedPtr<WindowsWindow> Application::GetCapture() const
{
	return PlatformApplication->GetCapture();
}

void Application::GetCursorPos(TSharedPtr<WindowsWindow>& RelativeWindow, Int32& OutX, Int32& OutY) const
{
	PlatformApplication->GetCursorPos(RelativeWindow, OutX, OutY);
}

Application* Application::Make()
{
	Instance = TSharedPtr<Application>(new Application());
	if (Instance->Initialize())
	{
		return Instance.Get();
	}
	else
	{
		return nullptr;
	}
}

Application* Application::Get()
{
	return Instance.Get();
}

void Application::OnWindowResized(TSharedPtr<WindowsWindow>& InWindow, Uint16 Width, Uint16 Height)
{
	UNREFERENCED_PARAMETER(InWindow);

	WindowResizeEvent Event(InWindow, Width, Height);
	EventQueue::SendEvent(Event);

	if (Renderer::Get())
	{
		Renderer::Get()->OnResize(Width, Height);
	}
}

void Application::OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState)
{
	UNREFERENCED_PARAMETER(ModierKeyState);

	Input::RegisterKeyUp(KeyCode);

	KeyReleasedEvent Event(KeyCode, ModierKeyState);
	EventQueue::SendEvent(Event);
}

void Application::OnKeyPressed(EKey KeyCode, const ModifierKeyState& ModierKeyState)
{
	UNREFERENCED_PARAMETER(ModierKeyState);

	Input::RegisterKeyDown(KeyCode);

	KeyPressedEvent Event(KeyCode, ModierKeyState);
	EventQueue::SendEvent(Event);
}

void Application::OnMouseMove(Int32 X, Int32 Y)
{
	MouseMovedEvent Event(X, Y);
	EventQueue::SendEvent(Event);
}

void Application::OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState)
{
	UNREFERENCED_PARAMETER(ModierKeyState);

	TSharedPtr<WindowsWindow> CaptureWindow = GetCapture();
	if (CaptureWindow)
	{
		SetCapture(nullptr);
	}

	MouseReleasedEvent Event(Button, ModierKeyState);
	EventQueue::SendEvent(Event);
}

void Application::OnMouseButtonPressed(EMouseButton Button, const ModifierKeyState& ModierKeyState)
{
	UNREFERENCED_PARAMETER(ModierKeyState);

	TSharedPtr<WindowsWindow> CaptureWindow = GetCapture();
	if (!CaptureWindow)
	{
		TSharedPtr<WindowsWindow> ActiveWindow = GetActiveWindow();
		SetCapture(ActiveWindow);
	}

	MousePressedEvent Event(Button, ModierKeyState);
	EventQueue::SendEvent(Event);
}

void Application::OnMouseScrolled(Float32 HorizontalDelta, Float32 VerticalDelta)
{
	MouseScrolledEvent Event(HorizontalDelta, VerticalDelta);
	EventQueue::SendEvent(Event);
}

void Application::OnCharacterInput(Uint32 Character)
{
	KeyTypedEvent Event(Character);
	EventQueue::SendEvent(Event);
}

bool Application::Initialize()
{
	// Application
	HINSTANCE InstanceHandle = static_cast<HINSTANCE>(GetModuleHandle(NULL));
	PlatformApplication = WindowsApplication::Make(InstanceHandle);
	if (PlatformApplication)
	{
		PlatformApplication->SetEventHandler(TSharedPtr<ApplicationEventHandler>(Instance));
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
	if (!DebugUI::Initialize(Renderer->GetDevice()))
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
	CurrentScene = Scene::LoadFromFile("../Assets/Scenes/Sponza/Sponza.obj", Renderer->GetDevice().Get());

	// Create Spheres
	MeshData SphereMeshData = MeshFactory::CreateSphere(3);
	TSharedPtr<Mesh> SphereMesh = Mesh::Make(Renderer->GetDevice().Get(), SphereMeshData);

	// Create standard textures
	Byte Pixels[] = { 255, 255, 255, 255 };
	TSharedPtr<D3D12Texture> BaseTexture = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromMemory(Renderer->GetDevice().Get(), Pixels, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM));
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

	TSharedPtr<D3D12Texture> BaseNormal = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromMemory(Renderer->GetDevice().Get(), Pixels, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM));
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

	TSharedPtr<D3D12Texture> WhiteTexture = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromMemory(Renderer->GetDevice().Get(), Pixels, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM));
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
			NewComponent->Material	= MakeShared<Material>(MatProperties);
			
			NewComponent->Material->AlbedoMap	= BaseTexture;
			NewComponent->Material->NormalMap	= BaseNormal;
			NewComponent->Material->RoughnessMap	= WhiteTexture;
			NewComponent->Material->HeightMap	= WhiteTexture;
			NewComponent->Material->AOMap		= WhiteTexture;
			NewComponent->Material->MetallicMap = WhiteTexture;
			NewComponent->Material->Initialize(Renderer->GetDevice().Get());

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
	NewComponent->Mesh		= Mesh::Make(Renderer->GetDevice().Get(), CubeMeshData);
	NewComponent->Material	= MakeShared<Material>(MatProperties);

	TSharedPtr<D3D12Texture> AlbedoMap = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().Get(), "../Assets/Textures/Gate_Albedo.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!AlbedoMap)
	{
		return false;
	}
	else
	{
		AlbedoMap->SetDebugName("AlbedoMap");
	}

	TSharedPtr<D3D12Texture> NormalMap = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().Get(), "../Assets/Textures/Gate_Normal.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!NormalMap)
	{
		return false;
	}
	else
	{
		NormalMap->SetDebugName("NormalMap");
	}

	TSharedPtr<D3D12Texture> AOMap = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().Get(), "../Assets/Textures/Gate_AO.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!AOMap)
	{
		return false;
	}
	else
	{
		AOMap->SetDebugName("AOMap");
	}

	TSharedPtr<D3D12Texture> RoughnessMap = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().Get(), "../Assets/Textures/Gate_Roughness.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!RoughnessMap)
	{
		return false;
	}
	else
	{
		RoughnessMap->SetDebugName("RoughnessMap");
	}

	TSharedPtr<D3D12Texture> HeightMap = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().Get(), "../Assets/Textures/Gate_Height.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!HeightMap)
	{
		return false;
	}
	else
	{
		HeightMap->SetDebugName("HeightMap");
	}

	TSharedPtr<D3D12Texture> MetallicMap = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromFile(Renderer->GetDevice().Get(), "../Assets/Textures/Gate_Metallic.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!MetallicMap)
	{
		return false;
	}
	else
	{
		MetallicMap->SetDebugName("MetallicMap");
	}

	NewComponent->Material->AlbedoMap		= AlbedoMap;
	NewComponent->Material->NormalMap		= NormalMap;
	NewComponent->Material->RoughnessMap	= RoughnessMap;
	NewComponent->Material->HeightMap		= HeightMap;
	NewComponent->Material->AOMap			= AOMap;
	NewComponent->Material->MetallicMap		= MetallicMap;
	NewComponent->Material->Initialize(Renderer->GetDevice().Get());
	NewActor->AddComponent(NewComponent);

	CurrentCamera = new Camera();
	CurrentScene->AddCamera(CurrentCamera);

	// Add PointLight- Source
	PointLight* Light0 = new PointLight();
	Light0->SetPosition(0.0f, 10.0f, -10.0f);
	Light0->SetColor(1.0f, 1.0f, 1.0f);
	Light0->SetIntensity(400.0f);
	CurrentScene->AddLight(Light0);

	// Add DirectionalLight- Source
	DirectionalLight* Light1 = new DirectionalLight();
	Light1->SetDirection(0.0f, -1.0f, 0.0f);
	Light1->SetShadowMapPosition(0.0f, 25.0f, 0.0f);
	Light1->SetShadowBias(0.0005f);
	Light1->SetColor(1.0f, 1.0f, 1.0f);
	Light1->SetIntensity(10.0f);
	CurrentScene->AddLight(Light1);

	return true;
}
