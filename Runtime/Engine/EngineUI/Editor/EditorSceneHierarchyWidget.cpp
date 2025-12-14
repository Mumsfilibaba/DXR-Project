#include "Engine/EngineUI/Editor/EditorSceneHierarchyWidget.h"
#include "Engine/EditorEngine.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include <imgui.h>

FEditorSceneHierarchyWidget::FEditorSceneHierarchyWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , ImGuiDelegateHandle()
    , bVisible(true)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorSceneHierarchyWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorSceneHierarchyWidget::~FEditorSceneHierarchyWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FEditorSceneHierarchyWidget::Draw()
{
	if (!bVisible)
	{
		return;
	}

	const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoFocusOnAppearing;
	if (ImGui::Begin("Scene Hierarchy", &bVisible, Flags))
	{
		DrawSceneInfo();
	}

	ImGui::End();
}

void FEditorSceneHierarchyWidget::DrawSceneInfo()
{
	if (!EditorEngine)
	{
		ImGui::TextDisabled("No EditorEngine");
		return;
	}

	FWorld* World = EditorEngine->GetWorld();
	if (!World)
	{
		ImGui::TextDisabled("No World");
		return;
	}

	// Pull current selection
	FActor*      SelectedActor      = EditorEngine->GetSelectedActor();
	FLight*      SelectedLight      = EditorEngine->GetSelectedLight();
	FCamera*     SelectedCamera     = EditorEngine->GetSelectedCamera();
	FLightProbe* SelectedLightProbe = EditorEngine->GetSelectedLightProbe();

	// Camera
	if (FCamera* Camera = World->GetCamera())
	{
		if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const bool bIsSelected = (Camera == SelectedCamera);
			if (ImGui::Selectable("Main Camera", bIsSelected))
			{
				EditorEngine->SetSelectedCamera(Camera);
			}
			ImGui::TreePop();
		}
	}

	// Light Probes
	const TArray<FLightProbe*>& LightProbes = World->GetLightProbes();
	if (!LightProbes.IsEmpty())
	{
		if (ImGui::TreeNodeEx("Light Probes", ImGuiTreeNodeFlags_DefaultOpen))
		{
			int32 ProbeIndex = 0;
			for (FLightProbe* Probe : LightProbes)
			{
				if (!Probe)
				{
					continue;
				}

				const bool bIsSelected = (Probe == SelectedLightProbe);

				ImGui::PushID(Probe);

				static constexpr uint32 LabelLength = 64;
				char Label[LabelLength];
				FCString::Snprintf(Label, LabelLength, "LightProbe %d", ProbeIndex++);

				if (ImGui::Selectable(Label, bIsSelected))
				{
					EditorEngine->SetSelectedLightProbe(Probe);
				}

				ImGui::PopID();
			}

			ImGui::TreePop();
		}
	}

	// Actors
    const TArray<FActor*>& Actors = World->GetActors();
    if (!Actors.IsEmpty())
    {
	    if (ImGui::TreeNodeEx("Actors", ImGuiTreeNodeFlags_DefaultOpen))
	    {
		    for (FActor* Actor : Actors)
		    {
			    if (!Actor)
			    {
				    continue;
			    }

			    const bool bIsSelected = (Actor == SelectedActor);

			    ImGui::PushID(Actor);

			    const FString& Name = Actor->GetName();
			    const char* Label = Name.IsEmpty() ? "Actor" : *Name;
			    if (ImGui::Selectable(Label, bIsSelected))
			    {
				    EditorEngine->SetSelectedActor(Actor);
			    }

			    ImGui::PopID();
		    }

		    ImGui::TreePop();
	    }
    }

	// Lights
    const TArray<FLight*>& Lights = World->GetLights();
    if (!Lights.IsEmpty())
    {
		if (ImGui::TreeNodeEx("Lights", ImGuiTreeNodeFlags_DefaultOpen))
		{
			int32 LightIndex = 0;
			for (FLight* Light : Lights)
			{
				if (!Light)
				{
					continue;
				}

				const bool bIsSelected = (Light == SelectedLight);

				ImGui::PushID(Light);

				const char* TypeLabel = "Light";
				if (Cast<FPointLight>(Light))
				{
					TypeLabel = "PointLight";
				}
				else if (Cast<FDirectionalLight>(Light))
				{
					TypeLabel = "DirectionalLight";
				}

				static constexpr uint32 LabelLength = 256;
				char Label[LabelLength];
				FCString::Snprintf(Label, LabelLength, "%s %d", TypeLabel, LightIndex++);

				if (ImGui::Selectable(Label, bIsSelected))
				{
					EditorEngine->SetSelectedLight(Light);
				}

				ImGui::PopID();
			}

			ImGui::TreePop();
		}
    }
}
