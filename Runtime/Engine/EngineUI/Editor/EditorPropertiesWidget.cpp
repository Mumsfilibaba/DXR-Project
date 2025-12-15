#include "Engine/EngineUI/Editor/EditorPropertiesWidget.h"
#include "Engine/EditorEngine.h"
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

	if (!ImGui::Begin("Properties", &bVisible))
	{
		ImGui::End();
		return;
	}

	FActor*      SelectedActor      = EditorEngine->GetSelectedActor();
	FLight*      SelectedLight      = EditorEngine->GetSelectedLight();
	FCamera*     SelectedCamera     = EditorEngine->GetSelectedCamera();
	FLightProbe* SelectedLightProbe = EditorEngine->GetSelectedLightProbe();

	if (!SelectedActor && !SelectedLight && !SelectedCamera && !SelectedLightProbe)
	{
		ImGui::TextDisabled("No selection");
		ImGui::End();
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

	// Actor properties
	if (SelectedActor)
	{
		ImGui::PushID(SelectedActor);

		const FString& ActorName = SelectedActor->GetName();
		DrawLabelWithSeperator(ActorName.IsEmpty() ? "Actor" : *ActorName);

		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// Translation
			FVector3 Translation = SelectedActor->GetTransform().GetTranslation();
			ImGuiExtensions::DrawFloat3Control("Translation", Translation);
			SelectedActor->GetTransform().SetTranslation(Translation);

			// Rotation (degrees UI)
			FVector3 Rotation = SelectedActor->GetTransform().GetRotation();
			Rotation = FVector3::RadiansToDegrees(Rotation);
			ImGuiExtensions::DrawFloat3Control("Rotation", Rotation, 0.0f, 100.0f, 1.0f);
			Rotation = FVector3::DegreesToRadians(Rotation);
			SelectedActor->GetTransform().SetRotation(Rotation);

			// Scale + optional uniform scaling toggle
			static bool bUniformScale = false;

			FVector3 Scale0 = SelectedActor->GetTransform().GetScale();
			FVector3 Scale1 = Scale0;

			ImGuiExtensions::DrawFloat3Control("Scale", Scale0, 1.0f);

			ImGui::SameLine();
			ImGui::Checkbox("##UniformScale", &bUniformScale);

			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Enable Uniform Scaling");
			}

			if (bUniformScale)
			{
				if (Scale1.X != Scale0.X) 
				{
					Scale0.Y = Scale0.X; 
					Scale0.Z = Scale0.X; 
				}
				else if (Scale1.Y != Scale0.Y) 
				{
					Scale0.X = Scale0.Y; 
					Scale0.Z = Scale0.Y;
				}
				else if (Scale1.Z != Scale0.Z)
				{
					Scale0.X = Scale0.Z;
					Scale0.Y = Scale0.Z;
				}
			}

			SelectedActor->GetTransform().SetScale(Scale0);
		}

		// MeshComponent (moved here from hierarchy) :contentReference[oaicite:5]{index=5}
		if (FStaticMeshComponent* MeshComponent = SelectedActor->GetComponentOfType<FStaticMeshComponent>())
		{
			if (ImGui::CollapsingHeader("MeshComponent", ImGuiTreeNodeFlags_DefaultOpen))
			{
				const float ColumnWidth = 200.0f;

				ImGui::Columns(2, nullptr, false);
				ImGui::SetColumnWidth(0, ColumnWidth);

				FMaterialInfo MaterialInfo = MeshComponent->GetMaterial()->GetMaterialInfo();

				// Albedo
				ImGui::Text("Albedo");
				ImGui::NextColumn();

				if (ImGuiExtensions::DrawColorEdit3("##Albedo", MaterialInfo.Albedo))
				{
					MeshComponent->GetMaterial()->SetAlbedo(MaterialInfo.Albedo);
				}

				// Roughness
				ImGui::NextColumn();
				ImGui::Text("Roughness");
				ImGui::NextColumn();

				if (ImGui::SliderFloat("##Roughness", &MaterialInfo.Roughness, 0.01f, 1.0f, "%.2f"))
				{
					MeshComponent->GetMaterial()->SetRoughness(MaterialInfo.Roughness);
				}

				// Metallic
				ImGui::NextColumn();
				ImGui::Text("Metallic");
				ImGui::NextColumn();
				
				if (ImGui::SliderFloat("##Metallic", &MaterialInfo.Metallic, 0.01f, 1.0f, "%.2f"))
				{
					MeshComponent->GetMaterial()->SetMetallic(MaterialInfo.Metallic);
				}

				// AO
				ImGui::NextColumn();
				ImGui::Text("AO");
				ImGui::NextColumn();

				if (ImGui::SliderFloat("##AO", &MaterialInfo.AmbientOcclusion, 0.01f, 1.0f, "%.2f"))
				{
					MeshComponent->GetMaterial()->SetAmbientOcclusion(MaterialInfo.AmbientOcclusion);
				}

				ImGui::Columns(1);
			}
		}

		ImGui::PopID();
		ImGui::End();
		return;
	}
	// Light Properties
	else if (SelectedLight) 
	{
		ImGui::PushID(SelectedLight);

		const float ColumnWidth = 200.0f;

		// Point light
		if (FPointLight* Point = Cast<FPointLight>(SelectedLight))
		{
			DrawLabelWithSeperator("PointLight");

			// PointLight
			ImGui::SeparatorText("Settings");

			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, ColumnWidth);

			ImGui::Text("Color");
			ImGui::NextColumn();

			FVector3 Color = Point->GetColor();
			if (ImGuiExtensions::DrawColorEdit3("##Color", Color))
			{
				Point->SetColor(Color);
			}

			ImGui::NextColumn();
			ImGui::Text("Intensity");
			ImGui::NextColumn();

			float Intensity = Point->GetIntensity();
			if (ImGui::SliderFloat("##Intensity", &Intensity, 0.01f, 1000.0f, "%.2f"))
			{
				Point->SetIntensity(Intensity);
			}

			ImGui::Columns(1);

			ImGui::SeparatorText("Transform");

			FVector3 Translation = Point->GetPosition();
			ImGuiExtensions::DrawFloat3Control("Translation", Translation, 0.0f, ColumnWidth);
			Point->SetPosition(Translation);

			ImGui::SeparatorText("Shadows");

			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, ColumnWidth);

			ImGui::Text("Shadow-bias");
			ImGui::NextColumn();
			
			float ShadowBias = Point->GetShadowBias();
			if (ImGui::SliderFloat("##ShadowBias", &ShadowBias, 0.0001f, 0.1f, "%.4f"))
			{
				Point->SetShadowBias(ShadowBias);
			}

			ImGui::NextColumn();
			ImGui::Text("Shadow near-plane");
			ImGui::NextColumn();
			
			float ShadowNearPlane = Point->GetShadowNearPlane();
			if (ImGui::SliderFloat("##ShadowNearPlane", &ShadowNearPlane, 0.01f, 1.0f, "%0.2f"))
			{
				Point->SetShadowNearPlane(ShadowNearPlane);
			}

			ImGui::NextColumn();
			ImGui::Text("Shadow far-plane");
			ImGui::NextColumn();
			
			float ShadowFarPlane = Point->GetShadowFarPlane();
			if (ImGui::SliderFloat("##ShadowFarPlane", &ShadowFarPlane, 1.0f, 100.0f, "%.1f"))
			{
				Point->SetShadowFarPlane(ShadowFarPlane);
			}

			ImGui::Columns(1);
		}
		else if (FDirectionalLight* Dir = Cast<FDirectionalLight>(SelectedLight))
		{
			DrawLabelWithSeperator("DirectionalLight");

			// Directional light
			ImGui::SeparatorText("Settings");

			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, ColumnWidth);

			ImGui::Text("Color");
			ImGui::NextColumn();

			FVector3 Color = Dir->GetColor();
			if (ImGuiExtensions::DrawColorEdit3("##Color", Color))
			{
				Dir->SetColor(Color);
			}

			ImGui::NextColumn();
			ImGui::Text("Intensity");
			ImGui::NextColumn();

			float Intensity = Dir->GetIntensity();
			if (ImGui::SliderFloat("##Intensity", &Intensity, 0.01f, 1000.0f, "%.2f"))
			{
				Dir->SetIntensity(Intensity);
			}

			ImGui::Columns(1);

			ImGui::SeparatorText("Direction");

			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, ColumnWidth);

			bool bSetRotation = false;
			FVector3 Rotation = Dir->GetRotation();

			ImGui::Text("Rotation theta (degrees)");
			ImGui::NextColumn();

			float RotationTheta = Math::RadiansToDegrees(Rotation.X);
			if (ImGui::SliderFloat("##RotationTheta", &RotationTheta, -90.0f, 90.0f, "%.2f"))
			{
				bSetRotation = true;
			}

			ImGui::NextColumn();
			ImGui::Text("Rotation phi (degrees)");
			ImGui::NextColumn();

			float RotationPhi = Math::RadiansToDegrees(Rotation.Y);
			if (ImGui::SliderFloat("##RotationPhi", &RotationPhi, 0.0f, 360.0f, "%.2f"))
			{
				bSetRotation = true;
			}

			if (bSetRotation)
			{
				Rotation.X = Math::DegreesToRadians(RotationTheta);
				Rotation.Y = Math::DegreesToRadians(RotationPhi);
				Dir->SetRotation(Rotation);
			}

			ImGui::NextColumn();
			ImGui::Text("Direction");
			ImGui::NextColumn();
			FVector3 Direction = Dir->GetDirectionVector();
			ImGui::InputFloat3("##Direction", Direction.XYZ, "%.3f", ImGuiInputTextFlags_ReadOnly);

			ImGui::Columns(1);

			ImGui::SeparatorText("Shadows");

			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, ColumnWidth);

			ImGui::Text("Shadow-bias");
			ImGui::NextColumn();

			float ShadowBias = Dir->GetShadowBias();
			if (ImGui::SliderFloat("##ShadowBias", &ShadowBias, 0.0001f, 0.1f, "%.4f"))
			{
				Dir->SetShadowBias(ShadowBias);
			}

			ImGui::NextColumn();
			ImGui::Text("Cascade Split Lambda");
			ImGui::NextColumn();
			
			float Lambda = Dir->GetCascadeSplitLambda();
			if (ImGui::SliderFloat("##CascadeSplitLambda", &Lambda, 0.0f, 1.0f, "%.2f"))
			{
				Dir->SetCascadeSplitLambda(Lambda);
			}

			ImGui::NextColumn();
			ImGui::Text("Cascade Position Offset");
			ImGui::NextColumn();

			float Offset = Dir->GetShadowPositionOffset();
			if (ImGui::SliderFloat("##CascadePositionOffset", &Offset, 0.0f, 1000.0f, "%.1f"))
			{
				Dir->SetShadowPositionOffset(Offset);
			}

			ImGui::NextColumn();
			ImGui::Text("Shadow near-plane");
			ImGui::NextColumn();

			float ShadowNearPlane = Dir->GetShadowNearPlane();
			if (ImGui::SliderFloat("##ShadowNearPlane", &ShadowNearPlane, 0.0f, 1000.0f, "%.1f"))
			{
				Dir->SetShadowNearPlane(ShadowNearPlane);
			}

			ImGui::NextColumn();
			ImGui::Text("Shadow far-plane");
			ImGui::NextColumn();

			float ShadowFarPlane = Dir->GetShadowFarPlane();
			if (ImGui::SliderFloat("##ShadowFarPlane", &ShadowFarPlane, 0.0f, 1000.0f, "%.1f"))
			{
				Dir->SetShadowFarPlane(ShadowFarPlane);
			}

			ImGui::NextColumn();
			ImGui::Text("Light area");
			ImGui::NextColumn();
			
			float LightArea = Dir->GetLightArea();
			if (ImGui::SliderFloat("##LightArea", &LightArea, 0.0f, 1.0f, "%.2f"))
			{
				Dir->SetLightArea(LightArea);
			}

			ImGui::Columns(1);
		}
		else
		{
			ImGui::TextDisabled("Unknown light type");
		}

		ImGui::PopID();
		ImGui::End();
		return;
	}
	// Camera Properties
	else if (SelectedCamera)
	{
		ImGui::PushID(SelectedCamera);

		DrawLabelWithSeperator("Camera");

		if (ImGui::CollapsingHeader("Projection", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Viewport size: %.1f x %.1f", SelectedCamera->GetWidth(), SelectedCamera->GetHeight());

			float FieldOfView = SelectedCamera->GetFieldOfView();
			if (ImGui::SliderFloat("Field Of View", &FieldOfView, 40.0f, 120.0f, "%.1f Degrees"))
			{
				SelectedCamera->SetFieldOfView(FieldOfView);
			}
		}

		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			FVector3 Position = SelectedCamera->GetPosition();
			ImGuiExtensions::DrawFloat3Control("Position", Position);
			SelectedCamera->SetPosition(Position.X, Position.Y, Position.Z);

			FVector3 Rotation = SelectedCamera->GetRotation();
			Rotation = FVector3::RadiansToDegrees(Rotation);
			ImGuiExtensions::DrawFloat3Control("Rotation", Rotation, 0.0f, 100.0f, 1.0f);
			Rotation = FVector3::DegreesToRadians(Rotation);
			SelectedCamera->SetRotation(Rotation.X, Rotation.Y, Rotation.Z);
		}

		ImGui::PopID();
		ImGui::End();
		return;
	}
	// Light-Probe Properties
	else if (SelectedLightProbe)
	{
		ImGui::PushID(SelectedLightProbe);

		DrawLabelWithSeperator("Light Probe");

		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			FVector3 Position = SelectedLightProbe->GetPosition();
			ImGuiExtensions::DrawFloat3Control("Position", Position);
			SelectedLightProbe->SetPosition(Position);
		}

		if (ImGui::CollapsingHeader("Box Projection", ImGuiTreeNodeFlags_DefaultOpen))
		{
			FVector3 BoxExtent = SelectedLightProbe->GetBoxExtents();
			ImGuiExtensions::DrawFloat3Control("Box Extent", BoxExtent);
			SelectedLightProbe->SetBoxExtent(BoxExtent);

			FVector3 BoxOffset = SelectedLightProbe->GetBoxOffset();
			ImGuiExtensions::DrawFloat3Control("Box Origin", BoxOffset);
			SelectedLightProbe->SetBoxOffset(BoxOffset);

			bool bBoxProjection = SelectedLightProbe->GetBoxProjection();
			if (ImGui::Checkbox("Enable Box-Projection", &bBoxProjection))
			{
				SelectedLightProbe->SetBoxProjection(bBoxProjection);
			}
		}

		ImGui::PopID();
		ImGui::End();
		return;
	}

	ImGui::End();
}
