#include "Editor.h"

#include "Rendering/DebugUI.h"
#include "Rendering/Renderer.h"

#include "Engine/EngineLoop.h"

#include "Application/Application.h"

#include "Scene/Scene.h"
#include "Scene/Lights/DirectionalLight.h"
#include "Scene/Lights/PointLight.h"
#include "Scene/Components/MeshComponent.h"

static Float MainMenuBarHeight = 0.0f;

static bool ShowRenderSettings	= false;
static bool ShowSceneGraph		= false;

/*
* Functions
*/
static void DrawMenu();
static void DrawDebugData();
static void DrawSideWindow();
static void DrawRenderSettings();
static void DrawSceneInfo();

/*
* DrawDebugData
*/
static void DrawDebugData()
{
	static std::string AdapterName = RenderingAPI::Get().GetAdapterName();

	const Double Delta = EngineLoop::GetDeltaTime().AsMilliSeconds();
	DebugUI::DrawDebugString("Adapter: " + AdapterName);
	DebugUI::DrawDebugString("Frametime: " + std::to_string(Delta) + " ms");
	DebugUI::DrawDebugString("FPS: " + std::to_string(static_cast<UInt32>(1000 / Delta)));
}

/*
* MenuBar
*/
static void DrawMenu()
{
	DebugUI::DrawUI([]
	{
		if (ImGui::BeginMainMenuBar())
		{
			// Set Size
			MainMenuBarHeight = ImGui::GetWindowHeight();

			// Menu
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Toggle Fullscreen"))
				{
					Application::Get().GetMainWindow()->ToggleFullscreen();
				}

				if (ImGui::MenuItem("Quit"))
				{
					EngineLoop::Exit();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				ImGui::MenuItem("Render Settings", NULL, &ShowRenderSettings);
				ImGui::MenuItem("SceneGraph", NULL, &ShowSceneGraph);

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	});
}

/*
* Draw SideWindow
*/

static void DrawSideWindow()
{
	DebugUI::DrawUI([]
	{
		constexpr UInt32 Width = 450;

		WindowShape WindowShape;
		Application::Get().GetMainWindow()->GetWindowShape(WindowShape);

		ImGui::SetNextWindowPos(ImVec2(0, MainMenuBarHeight));
		ImGui::SetNextWindowSize(ImVec2(Width, WindowShape.Height - MainMenuBarHeight));

		ImGui::Begin("Window", nullptr,
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoSavedSettings);

		if (ShowRenderSettings && ShowSceneGraph)
		{
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
		}
		else if (ShowRenderSettings)
		{
			DrawRenderSettings();
		}
		else if (ShowSceneGraph)
		{
			DrawSceneInfo();
		}

		ImGui::End();
	});
}

/*
* DrawRenderSettings
*/
static void DrawRenderSettings()
{
	ImGui::BeginChild("RendererInfo");

	WindowShape WindowShape;
	Application::Get().GetMainWindow()->GetWindowShape(WindowShape);

	ImGui::Spacing();
	ImGui::Text("Renderer Info");
	ImGui::Separator();

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

	Enabled = Renderer::Get()->IsFrustumCullEnabled();
	if (ImGui::Checkbox("Enable Frustum Culling", &Enabled))
	{
		Renderer::Get()->SetFrustumCullEnable(Enabled);
	}

	Enabled = Renderer::Get()->IsDrawAABBsEnabled();
	if (ImGui::Checkbox("Draw AABBs", &Enabled))
	{
		Renderer::Get()->SetDrawAABBsEnable(Enabled);
	}

	static const Char* AAItems[] =
	{
		"OFF",
		"FXAA",
	};

	static Int32 CurrentItem = 0;
	if (Renderer::Get()->IsFXAAEnabled())
	{
		CurrentItem = 1;
	}
	else
	{
		CurrentItem = 0;
	}

	if (ImGui::Combo("Anti-Aliasing", &CurrentItem, AAItems, IM_ARRAYSIZE(AAItems)))
	{
		if (CurrentItem == 0)
		{
			Renderer::Get()->SetFXAAEnable(false);
		}
		else if (CurrentItem == 1)
		{
			Renderer::Get()->SetFXAAEnable(true);
		}
	}

	ImGui::Spacing();
	ImGui::Text("Shadow Settings:");
	ImGui::Separator();

	static const Char* Items[] =
	{
		"8192x8192",
		"4096x4096",
		"3072x3072",
		"2048x2048",
		"1024x1024",
		"512x512",
		"256x256"
	};

	LightSettings Settings = Renderer::GetGlobalLightSettings();
	if (Settings.ShadowMapWidth == 8192)
	{
		CurrentItem = 0;
	}
	else if (Settings.ShadowMapWidth == 4096)
	{
		CurrentItem = 1;
	}
	else if (Settings.ShadowMapWidth == 3072)
	{
		CurrentItem = 2;
	}
	else if (Settings.ShadowMapWidth == 2048)
	{
		CurrentItem = 3;
	}
	else if (Settings.ShadowMapWidth == 1024)
	{
		CurrentItem = 4;
	}
	else if (Settings.ShadowMapWidth == 512)
	{
		CurrentItem = 5;
	}
	else if (Settings.ShadowMapWidth == 256)
	{
		CurrentItem = 6;
	}

	if (ImGui::Combo("Directional Light ShadowMap", &CurrentItem, Items, IM_ARRAYSIZE(Items)))
	{
		if (CurrentItem == 0)
		{
			Settings.ShadowMapWidth = 8192;
			Settings.ShadowMapHeight = 8192;
		}
		else if (CurrentItem == 1)
		{
			Settings.ShadowMapWidth = 4096;
			Settings.ShadowMapHeight = 4096;
		}
		else if (CurrentItem == 2)
		{
			Settings.ShadowMapWidth = 3072;
			Settings.ShadowMapHeight = 3072;
		}
		else if (CurrentItem == 3)
		{
			Settings.ShadowMapWidth = 2048;
			Settings.ShadowMapHeight = 2048;
		}
		else if (CurrentItem == 4)
		{
			Settings.ShadowMapWidth = 1024;
			Settings.ShadowMapHeight = 1024;
		}
		else if (CurrentItem == 5)
		{
			Settings.ShadowMapWidth = 512;
			Settings.ShadowMapHeight = 512;
		}
		else if (CurrentItem == 6)
		{
			Settings.ShadowMapWidth = 256;
			Settings.ShadowMapHeight = 256;
		}

		Renderer::SetGlobalLightSettings(Settings);
	}

	ImGui::Spacing();
	ImGui::Text("SSAO:");
	ImGui::Separator();

	Enabled = Renderer::Get()->IsSSAOEnabled();
	if (ImGui::Checkbox("Enabled ", &Enabled))
	{
		Renderer::Get()->SetSSAOEnable(Enabled);
	}

	Float Radius = Renderer::Get()->GetSSAORadius();
	if (ImGui::SliderFloat("Radius: ", &Radius, 0.05f, 2.0f, "%.3f"))
	{
		Renderer::Get()->SetSSAORadius(Radius);
	}

	ImGui::EndChild();
}

/*
* DrawSceneInfo
*/
static void DrawSceneInfo()
{
	constexpr Float Width = 400.0f;

	ImGui::Spacing();
	ImGui::Text("Current Scene");
	ImGui::Separator();

	ImGui::Indent();

	WindowShape WindowShape;
	Application::Get().GetMainWindow()->GetWindowShape(WindowShape);
	ImGui::BeginChild("SceneInfo", ImVec2(Width, Float(WindowShape.Height) - 100.0f));

	// Actors
	if (ImGui::TreeNode("Actors"))
	{
		for (Actor* Actor : Scene::GetCurrentScene()->GetActors())
		{
			if (ImGui::TreeNode(Actor->GetDebugName().c_str()))
			{
				// Transform
				if (ImGui::TreeNode("Transform"))
				{
					const XMFLOAT3& Position = Actor->GetTransform().GetPosition();

					Float Arr[3] = { Position.x, Position.y, Position.z };
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

						Float Arr[3] = { Color.x, Color.y, Color.z };
						if (ImGui::ColorEdit3("Albedo", Arr))
						{
							MComponent->Material->SetAlbedo(Arr[0], Arr[1], Arr[2]);
						}

						Float Roughness = MComponent->Material->GetMaterialProperties().Roughness;
						if (ImGui::SliderFloat("RoughnessMap", &Roughness, 0.01f, 1.0f, "%.2f"))
						{
							MComponent->Material->SetRoughness(Roughness);
						}

						Float Metallic = MComponent->Material->GetMaterialProperties().Metallic;
						if (ImGui::SliderFloat("Metallic", &Metallic, 0.01f, 1.0f, "%.2f"))
						{
							MComponent->Material->SetMetallic(Metallic);
						}

						Float AO = MComponent->Material->GetMaterialProperties().AO;
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
		for (Light* CurrentLight : Scene::GetCurrentScene()->GetLights())
		{
			if (IsSubClassOf<PointLight>(CurrentLight))
			{
				PointLight* CurrentPointLight = Cast<PointLight>(CurrentLight);
				if (ImGui::TreeNode("PointLight"))
				{
					ImGui::Text("Light Settings:");
					ImGui::Separator();

					const XMFLOAT3& Color = CurrentPointLight->GetColor();
					Float Arr[3] = { Color.x, Color.y, Color.z };
					if (ImGui::ColorEdit3("Color", Arr))
					{
						CurrentPointLight->SetColor(Arr[0], Arr[1], Arr[2]);
					}

					const XMFLOAT3& Position = CurrentPointLight->GetPosition();
					Float Arr2[3] = { Position.x, Position.y, Position.z };
					if (ImGui::DragFloat3("Position", Arr2, 0.5f))
					{
						CurrentPointLight->SetPosition(Arr2[0], Arr2[1], Arr2[2]);
					}

					Float Intensity = CurrentPointLight->GetIntensity();
					if (ImGui::SliderFloat("Intensity", &Intensity, 0.01f, 1000.0f, "%.2f"))
					{
						CurrentPointLight->SetIntensity(Intensity);
					}

					ImGui::Text("ShadowMap Settings:");
					ImGui::Separator();

					ImGui::PushItemWidth(100.0f);

					Float ShadowBias = CurrentPointLight->GetShadowBias();
					if (ImGui::SliderFloat("Shadow Bias", &ShadowBias, 0.0001f, 0.1f, "%.4f"))
					{
						CurrentPointLight->SetShadowBias(ShadowBias);
					}

					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("A Bias value used in lightning calculations\nwhen measuring the depth in a ShadowMap");
					}

					Float MaxShadowBias = CurrentPointLight->GetMaxShadowBias();
					if (ImGui::SliderFloat("Max Shadow Bias", &MaxShadowBias, 0.0001f, 0.1f, "%.4f"))
					{
						CurrentPointLight->SetMaxShadowBias(MaxShadowBias);
					}

					Float ShadowNearPlane = CurrentPointLight->GetShadowNearPlane();
					if (ImGui::SliderFloat("Shadow Near Plane", &ShadowNearPlane, 0.01f, 1.0f, "%0.2f"))
					{
						CurrentPointLight->SetShadowNearPlane(ShadowNearPlane);
					}

					Float ShadowFarPlane = CurrentPointLight->GetShadowFarPlane();
					if (ImGui::SliderFloat("Shadow Far Plane", &ShadowFarPlane, 1.0f, 100.0f, "%.1f"))
					{
						CurrentPointLight->SetShadowFarPlane(ShadowFarPlane);
					}

					ImGui::PopItemWidth();
					ImGui::TreePop();
				}
			}
			else if (IsSubClassOf<DirectionalLight>(CurrentLight))
			{
				DirectionalLight* CurrentDirectionalLight = Cast<DirectionalLight>(CurrentLight);
				if (ImGui::TreeNode("DirectionalLight"))
				{
					ImGui::Text("Light Settings:");
					ImGui::Separator();

					const XMFLOAT3& Color = CurrentDirectionalLight->GetColor();
					Float Arr[3] = { Color.x, Color.y, Color.z };
					if (ImGui::ColorEdit3("Color", Arr))
					{
						CurrentDirectionalLight->SetColor(Arr[0], Arr[1], Arr[2]);
					}

					const XMFLOAT3& Rotation = CurrentDirectionalLight->GetRotation();
					Float Arr2[3] =
					{
						XMConvertToDegrees(Rotation.x),
						XMConvertToDegrees(Rotation.y),
						XMConvertToDegrees(Rotation.z)
					};

					if (ImGui::DragFloat3("Rotation", Arr2, 1.0f, -90.0f, 90.0f, "%.2f"))
					{
						CurrentDirectionalLight->SetRotation(
							XMConvertToRadians(Arr2[0]),
							XMConvertToRadians(Arr2[1]),
							XMConvertToRadians(Arr2[2]));
					}

					XMFLOAT3 Direction	= CurrentDirectionalLight->GetDirection();
					Float* DirArr		= reinterpret_cast<Float*>(&Direction);
					ImGui::InputFloat3("Direction", DirArr, 2, ImGuiInputTextFlags_::ImGuiInputTextFlags_ReadOnly);

					Float Intensity = CurrentDirectionalLight->GetIntensity();
					if (ImGui::SliderFloat("Intensity", &Intensity, 0.01f, 1000.0f, "%.2f"))
					{
						CurrentDirectionalLight->SetIntensity(Intensity);
					}

					ImGui::Text("ShadowMap Settings:");
					ImGui::Separator();

					ImGui::PushItemWidth(200.0f);

					const XMFLOAT3& LookAt = CurrentDirectionalLight->GetLookAt();
					Float Arr3[3] = { LookAt.x, LookAt.y, LookAt.z };
					if (ImGui::DragFloat3("LookAt", Arr3, 0.5f))
					{
						CurrentDirectionalLight->SetLookAt(Arr3[0], Arr3[1], Arr3[2]);
					}

					XMFLOAT3 Position	= CurrentDirectionalLight->GetShadowMapPosition();
					Float* PosArr		= reinterpret_cast<Float*>(&Position);
					ImGui::InputFloat3("Position", PosArr, 2, ImGuiInputTextFlags_::ImGuiInputTextFlags_ReadOnly);

					ImGui::PopItemWidth();
					ImGui::PushItemWidth(100.0f);

					Float ShadowBias = CurrentDirectionalLight->GetShadowBias();
					if (ImGui::SliderFloat("Shadow Bias", &ShadowBias, 0.0001f, 0.1f, "%.4f"))
					{
						CurrentDirectionalLight->SetShadowBias(ShadowBias);
					}

					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("A Bias value used in lightning calculations\nwhen measuring the depth in a ShadowMap");
					}

					Float MaxShadowBias = CurrentDirectionalLight->GetMaxShadowBias();
					if (ImGui::SliderFloat("Max Shadow Bias", &MaxShadowBias, 0.0001f, 0.1f, "%.4f"))
					{
						CurrentDirectionalLight->SetMaxShadowBias(MaxShadowBias);
					}

					Float ShadowNearPlane = CurrentDirectionalLight->GetShadowNearPlane();
					if (ImGui::SliderFloat("Shadow Near Plane", &ShadowNearPlane, 0.01f, 1.0f, "%.2f"))
					{
						CurrentDirectionalLight->SetShadowNearPlane(ShadowNearPlane);
					}

					Float ShadowFarPlane = CurrentLight->GetShadowFarPlane();
					if (ImGui::SliderFloat("Shadow Far Plane", &ShadowFarPlane, 1.0f, 1000.0f, "%.1f"))
					{
						CurrentDirectionalLight->SetShadowFarPlane(ShadowFarPlane);
					}

					ImGui::PopItemWidth();
					ImGui::TreePop();
				}
			}
		}

		ImGui::TreePop();
	}

	ImGui::EndChild();
}

/*
* Tick
*/
void Editor::Tick()
{
	DrawMenu();

	if (ShowRenderSettings || ShowSceneGraph)
	{
		DrawSideWindow();
	}

	DrawDebugData();
}