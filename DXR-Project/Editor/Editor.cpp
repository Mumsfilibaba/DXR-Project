#include "Editor.h"

#include "Rendering/DebugUI.h"
#include "Rendering/Renderer.h"

#include "Engine/EngineLoop.h"
#include "Engine/EngineGlobals.h"

#include "Application/Application.h"

#include "Scene/Scene.h"
#include "Scene/Lights/DirectionalLight.h"
#include "Scene/Lights/PointLight.h"
#include "Scene/Components/MeshComponent.h"

#include <imgui_internal.h>

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
* Helpers
*/

static void DrawFloat3Control(const std::string& Label, XMFLOAT3& Value, Float ResetValue = 0.0f, Float ColumnWidth = 100.0f, Float Speed = 0.01f)
{
	ImGui::PushID(Label.c_str());
	ImGui::Columns(2);

	// Text
	ImGui::SetColumnWidth(0, ColumnWidth);
	ImGui::Text(Label.c_str());
	ImGui::NextColumn();

	// Drag Floats
	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

	Float	LineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2	ButtonSize = ImVec2(LineHeight + 3.0f, LineHeight);
	
	// X
	ImGui::PushStyleColor(ImGuiCol_Button,			ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,	ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,	ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
	if (ImGui::Button("X", ButtonSize))
	{
		Value.x = ResetValue;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##X", &Value.x, Speed);
	ImGui::PopItemWidth();
	ImGui::SameLine();

	// Y
	ImGui::PushStyleColor(ImGuiCol_Button,			ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,	ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,	ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
	if (ImGui::Button("Y", ButtonSize))
	{
		Value.y = ResetValue;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##Y", &Value.y, Speed);
	ImGui::PopItemWidth();
	ImGui::SameLine();

	// Z
	ImGui::PushStyleColor(ImGuiCol_Button,			ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,	ImVec4(0.2f, 0.35f, 0.9f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,	ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
	if (ImGui::Button("Z", ButtonSize))
	{
		Value.z = ResetValue;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##Z", &Value.z, Speed);
	ImGui::PopItemWidth();

	// Reset
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::Columns(1);
	ImGui::PopID();
}

/*
* DrawDebugData
*/

static void DrawDebugData()
{
	static std::string AdapterName = RenderingAPI::GetAdapterName();

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
		constexpr UInt32 Width = 500;

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

	bool Enabled = GlobalRenderer->IsPrePassEnabled();
	if (ImGui::Checkbox("Enable Z-PrePass", &Enabled))
	{
		GlobalRenderer->SetPrePassEnable(Enabled);
	}

	Enabled = GlobalRenderer->IsVerticalSyncEnabled();
	if (ImGui::Checkbox("Enable VSync", &Enabled))
	{
		GlobalRenderer->SetVerticalSyncEnable(Enabled);
	}

	Enabled = GlobalRenderer->IsFrustumCullEnabled();
	if (ImGui::Checkbox("Enable Frustum Culling", &Enabled))
	{
		GlobalRenderer->SetFrustumCullEnable(Enabled);
	}

	Enabled = GlobalRenderer->IsDrawAABBsEnabled();
	if (ImGui::Checkbox("Draw AABBs", &Enabled))
	{
		GlobalRenderer->SetDrawAABBsEnable(Enabled);
	}

	static const Char* AAItems[] =
	{
		"OFF",
		"FXAA",
	};

	static Int32 CurrentItem = 0;
	if (GlobalRenderer->IsFXAAEnabled())
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
			GlobalRenderer->SetFXAAEnable(false);
		}
		else if (CurrentItem == 1)
		{
			GlobalRenderer->SetFXAAEnable(true);
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

	LightSettings Settings = GlobalRenderer->GetLightSettings();
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

		GlobalRenderer->SetLightSettings(Settings);
	}

	ImGui::Spacing();
	ImGui::Text("SSAO:");
	ImGui::Separator();

	ImGui::Columns(2);

	// Text
	ImGui::SetColumnWidth(0, 100.0f);

	Enabled = GlobalRenderer->IsSSAOEnabled();
	ImGui::Text("Enabled: ");
	ImGui::NextColumn();

	if (ImGui::Checkbox("##Enabled", &Enabled))
	{
		GlobalRenderer->SetSSAOEnable(Enabled);
	}

	ImGui::NextColumn();
	ImGui::Text("Radius: ");
	ImGui::NextColumn();

	Float Radius = GlobalRenderer->GetSSAORadius();
	if (ImGui::SliderFloat("##Radius", &Radius, 0.05f, 5.0f, "%.3f"))
	{
		GlobalRenderer->SetSSAORadius(Radius);
	}

	ImGui::NextColumn();
	ImGui::Text("Bias: ");
	ImGui::NextColumn();

	Float Bias = GlobalRenderer->GetSSAOBias();
	if (ImGui::SliderFloat("##Bias", &Bias, 0.0f, 0.5f, "%.3f"))
	{
		GlobalRenderer->SetSSAOBias(Bias);
	}

	ImGui::NextColumn();
	ImGui::Text("KernelSize: ");
	ImGui::NextColumn();

	Int32 KernelSize = GlobalRenderer->GetSSAOKernelSize();
	if (ImGui::SliderInt("##KernelSize", &KernelSize, 4, 64))
	{
		GlobalRenderer->SetSSAOKernelSize(KernelSize);
	}

	ImGui::Columns(1);
	ImGui::EndChild();
}

/*
* DrawSceneInfo
*/

static void DrawSceneInfo()
{
	constexpr Float Width = 450.0f;

	ImGui::Spacing();
	ImGui::Text("Current Scene");
	ImGui::Separator();

	WindowShape WindowShape;
	Application::Get().GetMainWindow()->GetWindowShape(WindowShape);
	ImGui::BeginChild("SceneInfo", ImVec2(Width, Float(WindowShape.Height) - 100.0f));

	// Actors
	if (ImGui::TreeNode("Actors"))
	{
		for (Actor* Actor : Scene::GetCurrentScene()->GetActors())
		{
			if (ImGui::TreeNode(Actor->GetName().c_str()))
			{
				// Transform
				if (ImGui::TreeNode("Transform"))
				{
					// Transform
					XMFLOAT3 Translation = Actor->GetTransform().GetTranslation();
					DrawFloat3Control("Translation", Translation);
					Actor->GetTransform().SetTranslation(Translation);

					// Rotation
					XMFLOAT3 Rotation = Actor->GetTransform().GetRotation();
					Rotation = XMFLOAT3(
						XMConvertToDegrees(Rotation.x),
						XMConvertToDegrees(Rotation.y),
						XMConvertToDegrees(Rotation.z));
					
					DrawFloat3Control("Rotation", Rotation, 0.0f, 100.0f, 1.0f);

					Rotation = XMFLOAT3(
						XMConvertToRadians(Rotation.x),
						XMConvertToRadians(Rotation.y),
						XMConvertToRadians(Rotation.z));

					Actor->GetTransform().SetRotation(Rotation);

					// Scale
					XMFLOAT3 Scale0 = Actor->GetTransform().GetScale();
					XMFLOAT3 Scale1 = Scale0;
					DrawFloat3Control("Scale", Scale0, 1.0f);

					ImGui::SameLine();

					static bool Uniform = false;
					ImGui::Checkbox("##Uniform", &Uniform);
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Enable Uniform Scaling");
					}

					if (Uniform)
					{
						if (Scale1.x != Scale0.x)
						{
							Scale0.y = Scale0.x;
							Scale0.z = Scale0.x;
						}
						else if (Scale1.y != Scale0.y)
						{
							Scale0.x = Scale0.y;
							Scale0.z = Scale0.y;
						}
						else if (Scale1.z != Scale0.z)
						{
							Scale0.x = Scale0.z;
							Scale0.y = Scale0.z;
						}
					}

					Actor->GetTransform().SetScale(Scale0);

					ImGui::TreePop();
				}

				// MeshComponent
				MeshComponent* MComponent = Actor->GetComponentOfType<MeshComponent>();
				if (MComponent)
				{
					if (ImGui::TreeNode("MeshComponent"))
					{
						ImGui::Columns(2);
						ImGui::SetColumnWidth(0, 100.0f);

						// Albedo
						ImGui::Text("Albedo");
						ImGui::NextColumn();

						const XMFLOAT3& Color = MComponent->Material->GetMaterialProperties().Albedo;
						Float Arr[3] = { Color.x, Color.y, Color.z };
						if (ImGui::ColorEdit3("##Albedo", Arr))
						{
							MComponent->Material->SetAlbedo(Arr[0], Arr[1], Arr[2]);
						}

						// Roughness
						ImGui::NextColumn();
						ImGui::Text("Roughness");
						ImGui::NextColumn();

						Float Roughness = MComponent->Material->GetMaterialProperties().Roughness;
						if (ImGui::SliderFloat("##Roughness", &Roughness, 0.01f, 1.0f, "%.2f"))
						{
							MComponent->Material->SetRoughness(Roughness);
						}

						// Metallic
						ImGui::NextColumn();
						ImGui::Text("Metallic");
						ImGui::NextColumn();

						Float Metallic = MComponent->Material->GetMaterialProperties().Metallic;
						if (ImGui::SliderFloat("##Metallic", &Metallic, 0.01f, 1.0f, "%.2f"))
						{
							MComponent->Material->SetMetallic(Metallic);
						}

						// AO
						ImGui::NextColumn();
						ImGui::Text("AO");
						ImGui::NextColumn();

						Float AO = MComponent->Material->GetMaterialProperties().AO;
						if (ImGui::SliderFloat("##AO", &AO, 0.01f, 1.0f, "%.2f"))
						{
							MComponent->Material->SetAmbientOcclusion(AO);
						}

						ImGui::Columns(1);
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
					const Float ColumnWidth = 150.0f;

					// Transform
					if (ImGui::TreeNode("Transform"))
					{
						XMFLOAT3 Translation = CurrentPointLight->GetPosition();
						DrawFloat3Control("Translation", Translation, 0.0f, ColumnWidth);
						CurrentPointLight->SetPosition(Translation);
						
						ImGui::TreePop();
					}

					// Color
					if (ImGui::TreeNode("Light Settings"))
					{
						ImGui::Columns(2);
						ImGui::SetColumnWidth(0, ColumnWidth);

						ImGui::Text("Color");
						ImGui::NextColumn();

						const XMFLOAT3& Color = CurrentPointLight->GetColor();
						Float Arr[3] = { Color.x, Color.y, Color.z };
						if (ImGui::ColorEdit3("##Color", Arr))
						{
							CurrentPointLight->SetColor(Arr[0], Arr[1], Arr[2]);
						}

						ImGui::NextColumn();
						ImGui::Text("Intensity");
						ImGui::NextColumn();

						Float Intensity = CurrentPointLight->GetIntensity();
						if (ImGui::SliderFloat("##Intensity", &Intensity, 0.01f, 1000.0f, "%.2f"))
						{
							CurrentPointLight->SetIntensity(Intensity);
						}

						ImGui::Columns(1);
						ImGui::TreePop();
					}

					// Shadow Settings
					if (ImGui::TreeNode("Shadows"))
					{
						ImGui::Columns(2);
						ImGui::SetColumnWidth(0, ColumnWidth);

						// Bias
						ImGui::Text("Shadow Bias");
						ImGui::NextColumn();

						Float ShadowBias = CurrentPointLight->GetShadowBias();
						if (ImGui::SliderFloat("##ShadowBias", &ShadowBias, 0.0001f, 0.1f, "%.4f"))
						{
							CurrentPointLight->SetShadowBias(ShadowBias);
						}

						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("A Bias value used in lightning calculations\nwhen measuring the depth in a ShadowMap");
						}

						// Max Shadow Bias
						ImGui::NextColumn();
						ImGui::Text("Max Shadow Bias");
						ImGui::NextColumn();

						Float MaxShadowBias = CurrentPointLight->GetMaxShadowBias();
						if (ImGui::SliderFloat("##MaxShadowBias", &MaxShadowBias, 0.0001f, 0.1f, "%.4f"))
						{
							CurrentPointLight->SetMaxShadowBias(MaxShadowBias);
						}

						// Shadow Near Plane
						ImGui::NextColumn();
						ImGui::Text("Shadow Near Plane");
						ImGui::NextColumn();

						Float ShadowNearPlane = CurrentPointLight->GetShadowNearPlane();
						if (ImGui::SliderFloat("##ShadowNearPlane", &ShadowNearPlane, 0.01f, 1.0f, "%0.2f"))
						{
							CurrentPointLight->SetShadowNearPlane(ShadowNearPlane);
						}

						// Shadow Far Plane
						ImGui::NextColumn();
						ImGui::Text("Shadow Far Plane");
						ImGui::NextColumn();

						Float ShadowFarPlane = CurrentPointLight->GetShadowFarPlane();
						if (ImGui::SliderFloat("##ShadowFarPlane", &ShadowFarPlane, 1.0f, 100.0f, "%.1f"))
						{
							CurrentPointLight->SetShadowFarPlane(ShadowFarPlane);
						}

						ImGui::Columns(1);
						ImGui::TreePop();
					}

					ImGui::TreePop();
				}
			}
			else if (IsSubClassOf<DirectionalLight>(CurrentLight))
			{
				DirectionalLight* CurrentDirectionalLight = Cast<DirectionalLight>(CurrentLight);
				if (ImGui::TreeNode("DirectionalLight"))
				{
					const Float ColumnWidth = 150.0f;

					// Color
					if (ImGui::TreeNode("Light Settings"))
					{
						ImGui::Columns(2);
						ImGui::SetColumnWidth(0, ColumnWidth);

						ImGui::Text("Color");
						ImGui::NextColumn();

						const XMFLOAT3& Color = CurrentDirectionalLight->GetColor();
						Float Arr[3] = { Color.x, Color.y, Color.z };
						if (ImGui::ColorEdit3("##Color", Arr))
						{
							CurrentDirectionalLight->SetColor(Arr[0], Arr[1], Arr[2]);
						}

						ImGui::NextColumn();
						ImGui::Text("Intensity");
						ImGui::NextColumn();

						Float Intensity = CurrentDirectionalLight->GetIntensity();
						if (ImGui::SliderFloat("##Intensity", &Intensity, 0.01f, 1000.0f, "%.2f"))
						{
							CurrentDirectionalLight->SetIntensity(Intensity);
						}

						ImGui::Columns(1);
						ImGui::TreePop();
					}

					// Transform
					if (ImGui::TreeNode("Transform"))
					{
						XMFLOAT3 Rotation = CurrentDirectionalLight->GetRotation();
						Rotation = XMFLOAT3(
							XMConvertToDegrees(Rotation.x),
							XMConvertToDegrees(Rotation.y),
							XMConvertToDegrees(Rotation.z)
						);

						DrawFloat3Control("Rotation", Rotation, 0.0f, ColumnWidth, 1.0f);

						Rotation = XMFLOAT3(
							XMConvertToRadians(Rotation.x),
							XMConvertToRadians(Rotation.y),
							XMConvertToRadians(Rotation.z)
						);

						CurrentDirectionalLight->SetRotation(Rotation);

						ImGui::Columns(2);
						ImGui::SetColumnWidth(0, ColumnWidth);

						ImGui::Text("Direction");
						ImGui::NextColumn();

						XMFLOAT3 Direction = CurrentDirectionalLight->GetDirection();
						Float* DirArr = reinterpret_cast<Float*>(&Direction);
						ImGui::InputFloat3("##Direction", DirArr, 2, ImGuiInputTextFlags_::ImGuiInputTextFlags_ReadOnly);

						ImGui::Columns(1);
						ImGui::TreePop();
					}

					// Shadow Settings
					if (ImGui::TreeNode("Shadow Settings"))
					{
						XMFLOAT3 LookAt = CurrentDirectionalLight->GetLookAt();
						DrawFloat3Control("LookAt", LookAt, 0.0f, ColumnWidth);
						CurrentDirectionalLight->SetLookAt(LookAt);

						ImGui::Columns(2);
						ImGui::SetColumnWidth(0, ColumnWidth);

						// Read only translation
						ImGui::Text("Translation");
						ImGui::NextColumn();

						XMFLOAT3 Position = CurrentDirectionalLight->GetShadowMapPosition();
						Float* PosArr = reinterpret_cast<Float*>(&Position);
						ImGui::InputFloat3("##Translation", PosArr, 2, ImGuiInputTextFlags_::ImGuiInputTextFlags_ReadOnly);

						// Shadow Bias
						ImGui::NextColumn();
						ImGui::Text("Shadow Bias");
						ImGui::NextColumn();

						Float ShadowBias = CurrentDirectionalLight->GetShadowBias();
						if (ImGui::SliderFloat("##ShadowBias", &ShadowBias, 0.0001f, 0.1f, "%.4f"))
						{
							CurrentDirectionalLight->SetShadowBias(ShadowBias);
						}

						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("A Bias value used in lightning calculations\nwhen measuring the depth in a ShadowMap");
						}

						// Max Shadow Bias
						ImGui::NextColumn();
						ImGui::Text("Max Shadow Bias");
						ImGui::NextColumn();

						Float MaxShadowBias = CurrentDirectionalLight->GetMaxShadowBias();
						if (ImGui::SliderFloat("##MaxShadowBias", &MaxShadowBias, 0.0001f, 0.1f, "%.4f"))
						{
							CurrentDirectionalLight->SetMaxShadowBias(MaxShadowBias);
						}

						// Shadow Near Plane
						ImGui::NextColumn();
						ImGui::Text("Shadow Near Plane");
						ImGui::NextColumn();

						Float ShadowNearPlane = CurrentDirectionalLight->GetShadowNearPlane();
						if (ImGui::SliderFloat("##ShadowNearPlane", &ShadowNearPlane, 0.01f, 1.0f, "%.2f"))
						{
							CurrentDirectionalLight->SetShadowNearPlane(ShadowNearPlane);
						}

						// Shadow Far Plane 
						ImGui::NextColumn();
						ImGui::Text("Shadow Far Plane");
						ImGui::NextColumn();

						Float ShadowFarPlane = CurrentLight->GetShadowFarPlane();
						if (ImGui::SliderFloat("##ShadowFarPlane", &ShadowFarPlane, 1.0f, 1000.0f, "%.1f"))
						{
							CurrentDirectionalLight->SetShadowFarPlane(ShadowFarPlane);
						}

						ImGui::Columns(1);
						ImGui::TreePop();
					}

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