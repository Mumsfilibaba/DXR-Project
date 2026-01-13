#include "Engine/EditorEngine.h"
#include "Engine/World/Components/StaticMeshComponent.h"
#include "Engine/EngineUI/Editor/EditorPropertiesWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include <imgui.h>
#include <imgui_internal.h>

FEditorPropertiesWidget::FEditorPropertiesWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , ImGuiDelegateHandle()
    , bVisible(true)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorPropertiesWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorPropertiesWidget::~FEditorPropertiesWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FEditorPropertiesWidget::Draw()
{
	if (!bVisible)
	{
		return;
	}

	if (!EditorEngine)
	{
		return;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, EditorStyleVars::PropertiesItemSpacing);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, EditorStyleVars::PropertiesWindowPadding);

	if (ImGui::Begin("Properties", &bVisible))
	{
		DrawWindowContents();
	}
	
	ImGui::End();

	ImGui::PopStyleVar(2);
}

void FEditorPropertiesWidget::DrawWindowContents()
{
	FActor*      SelectedActor      = EditorEngine->GetSelectedActor();
	FLight*      SelectedLight      = EditorEngine->GetSelectedLight();
	FCamera*     SelectedCamera     = EditorEngine->GetSelectedCamera();
	FLightProbe* SelectedLightProbe = EditorEngine->GetSelectedLightProbe();

	if (!SelectedActor && !SelectedLight && !SelectedCamera && !SelectedLightProbe)
	{
		ImGui::TextDisabled("No selection");
		return;
	}

	const auto DrawLabelWithSeperator = [](const char* InLabel)
	{
		static constexpr uint32 LabelLength = 256;
		char Label[LabelLength];
		FCString::Snprintf(Label, LabelLength, "%s", InLabel);

		ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextBorderSize, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.1f, 0.5f));
		ImGui::SeparatorText(Label);
		ImGui::PopStyleVar(2);
	};

	const auto DrawCollapsingHeader = [](const char* Label, ImGuiTreeNodeFlags Flags)
	{
		ImGuiStyle& Style = ImGui::GetStyle();

		const ImVec4 HeaderBg = ImVec4(47.0f / 255.0f, 47.0f / 255.0f, 47.0f / 255.0f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Header, HeaderBg);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HeaderBg);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, HeaderBg);

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, EditorStyleVars::PropertiesCollapsingFrameRounding);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, EditorStyleVars::PropertiesCollapsingHeaderItemSpacing);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, EditorStyleVars::PropertiesCollapsingFramePadding);

		const bool bResult = ImGui::CollapsingHeader(Label, Flags);

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(3);

		if (bResult)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(Style.ItemSpacing.x, 0.0f));
			ImGui::PopStyleVar();
		}

		return bResult;
	};

	static constexpr float LabelColumnWidth  = 128.0f;
	static constexpr float RevertColumnWidth = 28.0f;

	// ------------------------------------------------------------
	// Actor properties
	// ------------------------------------------------------------
	if (SelectedActor)
	{
		ImGui::PushID(SelectedActor);

		const FString& ActorName = SelectedActor->GetName();
		DrawLabelWithSeperator(ActorName.IsEmpty() ? "Actor" : *ActorName);

		if (DrawCollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (EditorWidgets::BeginPropertyTable("##ActorTransformTable", LabelColumnWidth, RevertColumnWidth))
			{
				// Translation
				{
					FVector3 Translation = SelectedActor->GetTransform().GetTranslation();
					const FVector3 TranslationRevert = FVector3(0.0f, 0.0f, 0.0f);

					if (EditorWidgets::DrawFloat3Control("Translation", Translation, 0.0f, &TranslationRevert, EVector3ControlType::Position))
					{
						SelectedActor->GetTransform().SetTranslation(Translation);
					}
				}

				// Rotation (degrees UI)
				{
					FVector3 Rotation = SelectedActor->GetTransform().GetRotation();
					Rotation = FVector3::RadiansToDegrees(Rotation);

					const FVector3 RotationRevert = FVector3(0.0f, 0.0f, 0.0f);

					if (EditorWidgets::DrawFloat3Control("Rotation", Rotation, 0.0f, &RotationRevert, EVector3ControlType::RotationDegrees))
					{
						const FVector3 Radians = FVector3::DegreesToRadians(Rotation);
						SelectedActor->GetTransform().SetRotation(Radians);
					}
				}

				// Scale
				{
					FVector3 Scale = SelectedActor->GetTransform().GetScale();
					const FVector3 ScaleRevert = FVector3(1.0f, 1.0f, 1.0f);

					const bool bScaleChanged = EditorWidgets::DrawFloat3Control("Scale", Scale, 1.0f, &ScaleRevert, EVector3ControlType::Scale);
					if (bScaleChanged)
					{
						SelectedActor->GetTransform().SetScale(Scale);
					}
				}

				EditorWidgets::EndPropertyTable();
			}
		}

		// MeshComponent
		if (FStaticMeshComponent* MeshComponent = SelectedActor->GetComponentOfType<FStaticMeshComponent>())
		{
			if (DrawCollapsingHeader("MeshComponent", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (EditorWidgets::BeginPropertyTable("##MeshComponentMaterialTable", LabelColumnWidth, RevertColumnWidth))
				{
					FMaterialInfo MaterialInfo = MeshComponent->GetMaterial()->GetMaterialInfo();
					const FMaterialInfo MaterialOriginal = MaterialInfo;

					// Albedo
					{
						FFloatColor Albedo = MaterialInfo.Albedo;
						const FFloatColor Albedo0 = FFloatColor::White;

						if (EditorWidgets::DrawColor3Property("Albedo", Albedo, Albedo0))
						{
							MeshComponent->GetMaterial()->SetAlbedo(Albedo);
						}
					}

					// Roughness
					{
						float Roughness = MaterialInfo.Roughness;
						const float Roughness0 = 0.0f;

						if (EditorWidgets::DrawFloatProperty("Roughness", Roughness, 0.01f, 0.01f, 1.0f, "%.2f", true, &Roughness0))
						{
							MeshComponent->GetMaterial()->SetRoughness(Roughness);
						}
					}

					// Metallic
					{
						float Metallic = MaterialInfo.Metallic;
						const float Metallic0 = 0.0f;

						if (EditorWidgets::DrawFloatProperty("Metallic", Metallic, 0.01f, 0.01f, 1.0f, "%.2f", true, &Metallic0))
						{
							MeshComponent->GetMaterial()->SetMetallic(Metallic);
						}
					}

					// AO
					{
						float AO = MaterialInfo.AmbientOcclusion;
						const float AO0 = 1.0f;
						
						if (EditorWidgets::DrawFloatProperty("AO", AO, 0.01f, 0.01f, 1.0f, "%.2f", true, &AO0))
						{
							MeshComponent->GetMaterial()->SetAmbientOcclusion(AO);
						}
					}

					EditorWidgets::EndPropertyTable();
				}
			}
		}

		ImGui::PopID();
		return;
	}

	// ------------------------------------------------------------
	// Light properties
	// ------------------------------------------------------------
	if (SelectedLight)
	{
		ImGui::PushID(SelectedLight);

		// Point light
		if (FPointLight* Point = Cast<FPointLight>(SelectedLight))
		{
			DrawLabelWithSeperator("PointLight");

			if (DrawCollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (EditorWidgets::BeginPropertyTable("##PointLightSettingsTable", LabelColumnWidth, RevertColumnWidth))
				{
					FVector3 Color = Point->GetColor();
					const FVector3 Color0 = FVector3(1.0f, 1.0f, 1.0f);
					
					if (EditorWidgets::DrawColor3Property("Color", Color, Color0))
					{
						Point->SetColor(Color);
					}

					float Intensity = Point->GetIntensity();
					const float Intensity0 = 1.0f;
					
					if (EditorWidgets::DrawFloatProperty("Intensity", Intensity, 0.01f, 0.01f, 1000.0f, "%.2f", true, &Intensity0))
					{
						Point->SetIntensity(Intensity);
					}

					EditorWidgets::EndPropertyTable();
				}
			}

			if (DrawCollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (EditorWidgets::BeginPropertyTable("##PointLightTransformTable", LabelColumnWidth, RevertColumnWidth))
				{
					FVector3 Translation = Point->GetPosition();
					const FVector3 Translation0 = FVector3(0.0f, 0.0f, 0.0f);
					
					if (EditorWidgets::DrawFloat3Control("Translation", Translation, 0.0f, &Translation0, EVector3ControlType::Position))
					{
						Point->SetPosition(Translation);
					}

					EditorWidgets::EndPropertyTable();
				}
			}

			if (DrawCollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (EditorWidgets::BeginPropertyTable("##PointLightShadowsTable", LabelColumnWidth, RevertColumnWidth))
				{
					float ShadowBias = Point->GetShadowBias();
					const float ShadowBias0 = 0.005f;
					
					if (EditorWidgets::DrawFloatProperty("Shadow-bias", ShadowBias, 0.0001f, 0.0001f, 0.1f, "%.4f", true, &ShadowBias0))
					{
						Point->SetShadowBias(ShadowBias);
					}

					float ShadowNearPlane = Point->GetShadowNearPlane();
					const float ShadowNearPlane0 = 1.0f;
					
					if (EditorWidgets::DrawFloatProperty("Shadow near-plane", ShadowNearPlane, 0.01f, 0.01f, 1.0f, "%.2f", true, &ShadowNearPlane0))
					{
						Point->SetShadowNearPlane(ShadowNearPlane);
					}

					float ShadowFarPlane = Point->GetShadowFarPlane();
					const float ShadowFarPlane0 = 30.0f;

					if (EditorWidgets::DrawFloatProperty("Shadow far-plane", ShadowFarPlane, 1.0f, 1.0f, 100.0f, "%.1f", true, &ShadowFarPlane0))
					{
						Point->SetShadowFarPlane(ShadowFarPlane);
					}

					EditorWidgets::EndPropertyTable();
				}
			}
		}
		else if (FDirectionalLight* Dir = Cast<FDirectionalLight>(SelectedLight))
		{
			DrawLabelWithSeperator("DirectionalLight");

			if (DrawCollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (EditorWidgets::BeginPropertyTable("##DirLightSettingsTable", LabelColumnWidth, RevertColumnWidth))
				{
					FVector3 Color = Dir->GetColor();
					const FVector3 Color0 = FVector3(1.0f, 1.0f, 1.0f);

					if (EditorWidgets::DrawColor3Property("Color", Color, Color0))
					{
						Dir->SetColor(Color);
					}

					float Intensity = Dir->GetIntensity();
					const float Intensity0 = 1.0f;

					if (EditorWidgets::DrawFloatProperty("Intensity", Intensity, 0.01f, 0.01f, 1000.0f, "%.2f", true, &Intensity0))
					{
						Dir->SetIntensity(Intensity);
					}

					EditorWidgets::EndPropertyTable();
				}
			}

			if (DrawCollapsingHeader("Direction", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (EditorWidgets::BeginPropertyTable("##DirLightDirectionTable", LabelColumnWidth, RevertColumnWidth))
				{
					FVector3 Rotation = Dir->GetRotation();
					
					float RotationTheta = Math::RadiansToDegrees(Rotation.X);
					float RotationPhi   = Math::RadiansToDegrees(Rotation.Y);

					const float RotationTheta0 = 0.0f;
					const float RotationPhi0   = 0.0f;

					bool bSetRotation = false;
					bSetRotation |= EditorWidgets::DrawFloatProperty("Rotation theta (degrees)", RotationTheta, 0.25f, -90.0f, 90.0f, "%.2f", true, &RotationTheta0);
					bSetRotation |= EditorWidgets::DrawFloatProperty("Rotation phi (degrees)", RotationPhi, 0.25f, 0.0f, 360.0f, "%.2f", true, &RotationPhi0);

					if (bSetRotation)
					{
						Rotation.X = Math::DegreesToRadians(RotationTheta);
						Rotation.Y = Math::DegreesToRadians(RotationPhi);
						Dir->SetRotation(Rotation);
					}

					const FVector3 Direction = Dir->GetDirectionVector();
					EditorWidgets::DrawReadOnlyFloat3Property("Direction", Direction);

					EditorWidgets::EndPropertyTable();
				}
			}

			if (DrawCollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (EditorWidgets::BeginPropertyTable("##DirLightShadowsTable", LabelColumnWidth, RevertColumnWidth))
				{
					float ShadowBias = Dir->GetShadowBias();
					const float ShadowBias0 = 0.005f;
					
					if (EditorWidgets::DrawFloatProperty("Shadow-bias", ShadowBias, 0.0001f, 0.0001f, 0.1f, "%.4f", true, &ShadowBias0))
					{
						Dir->SetShadowBias(ShadowBias);
					}

					float Lambda = Dir->GetCascadeSplitLambda();
					const float Lambda0 = 0.95f;
					
					if (EditorWidgets::DrawFloatProperty("Cascade Split Lambda", Lambda, 0.01f, 0.0f, 1.0f, "%.2f", true, &Lambda0))
					{
						Dir->SetCascadeSplitLambda(Lambda);
					}

					float Offset = Dir->GetShadowPositionOffset();
					const float Offset0 = 200.0f;
					
					if (EditorWidgets::DrawFloatProperty("Cascade Position Offset", Offset, 1.0f, 0.0f, 1000.0f, "%.1f", true, &Offset0))
					{
						Dir->SetShadowPositionOffset(Offset);
					}

					float ShadowNearPlane = Dir->GetShadowNearPlane();
					const float ShadowNearPlane0 = 120.0f;
					
					if (EditorWidgets::DrawFloatProperty("Shadow near-plane", ShadowNearPlane, 1.0f, 0.0f, 1000.0f, "%.1f", true, &ShadowNearPlane0))
					{
						Dir->SetShadowNearPlane(ShadowNearPlane);
					}

					float ShadowFarPlane = Dir->GetShadowFarPlane();
					const float ShadowFarPlane0 = 250.0f;
					
					if (EditorWidgets::DrawFloatProperty("Shadow far-plane", ShadowFarPlane, 1.0f, 0.0f, 1000.0f, "%.1f", true, &ShadowFarPlane0))
					{
						Dir->SetShadowFarPlane(ShadowFarPlane);
					}

					float LightArea = Dir->GetLightArea();
					const float LightArea0 = 0.05f;

					if (EditorWidgets::DrawFloatProperty("Light area", LightArea, 0.01f, 0.0f, 1.0f, "%.2f", true, &LightArea0))
					{
						Dir->SetLightArea(LightArea);
					}

					EditorWidgets::EndPropertyTable();
				}
			}
		}
		else
		{
			ImGui::TextDisabled("Unknown light type");
		}

		ImGui::PopID();
		return;
	}

	// ------------------------------------------------------------
	// Camera properties
	// ------------------------------------------------------------
	if (SelectedCamera)
	{
		ImGui::PushID(SelectedCamera);

		DrawLabelWithSeperator("Camera");

		if (DrawCollapsingHeader("Projection", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (EditorWidgets::BeginPropertyTable("##CameraProjectionTable", LabelColumnWidth, RevertColumnWidth))
			{
				{
					char ViewportText[64];
					FCString::Snprintf(ViewportText, 64, "%.1f x %.1f", SelectedCamera->GetWidth(), SelectedCamera->GetHeight());
					EditorWidgets::DrawTextProperty("Viewport size", ViewportText);
				}

				{
					float FieldOfView = SelectedCamera->GetFieldOfView();
					const float FieldOfView0 = 60.0f;
					
					if (EditorWidgets::DrawFloatProperty("Field Of View", FieldOfView, 0.1f, 40.0f, 120.0f, "%.1f", true, &FieldOfView0))
					{
						SelectedCamera->SetFieldOfView(FieldOfView);
					}
				}

				EditorWidgets::EndPropertyTable();
			}
		}

		if (DrawCollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (EditorWidgets::BeginPropertyTable("##CameraTransformTable", LabelColumnWidth, RevertColumnWidth))
			{
				FVector3 Position = SelectedCamera->GetPosition();
				const FVector3 Position0 = FVector3(0.0f, 0.0f, 0.0f);
				
				if (EditorWidgets::DrawFloat3Control("Position", Position, 0.0f, &Position0, EVector3ControlType::Position))
				{
					SelectedCamera->SetPosition(Position.X, Position.Y, Position.Z);
				}

				FVector3 Rotation = SelectedCamera->GetRotation();
				Rotation = FVector3::RadiansToDegrees(Rotation);
				
				const FVector3 Rotation0 = FVector3(0.0f, 0.0f, 0.0f);
				if (EditorWidgets::DrawFloat3Control("Rotation", Rotation, 0.0f, &Rotation0, EVector3ControlType::RotationDegrees))
				{
					const FVector3 Radians = FVector3::DegreesToRadians(Rotation);
					SelectedCamera->SetRotation(Radians.X, Radians.Y, Radians.Z);
				}

				EditorWidgets::EndPropertyTable();
			}
		}

		ImGui::PopID();
		return;
	}

	// ------------------------------------------------------------
	// Light-Probe properties
	// ------------------------------------------------------------
	if (SelectedLightProbe)
	{
		ImGui::PushID(SelectedLightProbe);

		DrawLabelWithSeperator("Light Probe");

		if (DrawCollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (EditorWidgets::BeginPropertyTable("##ProbeTransformTable", LabelColumnWidth, RevertColumnWidth))
			{
				FVector3 Position = SelectedLightProbe->GetPosition();
				const FVector3 Position0 = FVector3(0.0f, 0.0f, 0.0f);
				
				if (EditorWidgets::DrawFloat3Control("Position", Position, 0.0f, &Position0, EVector3ControlType::Position))
				{
					SelectedLightProbe->SetPosition(Position);
				}

				EditorWidgets::EndPropertyTable();
			}
		}

		if (DrawCollapsingHeader("Box Projection", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (EditorWidgets::BeginPropertyTable("##ProbeBoxProjectionTable", LabelColumnWidth, RevertColumnWidth))
			{
				FVector3 BoxExtent = SelectedLightProbe->GetBoxExtents();
				const FVector3 BoxExtent0 = FVector3(0.0f, 0.0f, 0.0f);
				
				if (EditorWidgets::DrawFloat3Control("Box Extent", BoxExtent, 0.0f, &BoxExtent0, EVector3ControlType::Position))
				{
					SelectedLightProbe->SetBoxExtent(BoxExtent);
				}

				FVector3 BoxOffset = SelectedLightProbe->GetBoxOffset();
				const FVector3 BoxOffset0 = FVector3(0.0f, 0.0f, 0.0f);
				
				if (EditorWidgets::DrawFloat3Control("Box Origin", BoxOffset, 0.0f, &BoxOffset0, EVector3ControlType::Position))
				{
					SelectedLightProbe->SetBoxOffset(BoxOffset);
				}

				bool bBoxProjection = SelectedLightProbe->GetBoxProjection();
				const bool bBoxProjection0 = false;
				
				if (EditorWidgets::DrawCheckboxProperty("Enable Box-Projection", bBoxProjection, &bBoxProjection0))
				{
					SelectedLightProbe->SetBoxProjection(bBoxProjection);
				}

				EditorWidgets::EndPropertyTable();
			}
		}

		ImGui::PopID();
		return;
	}
}
