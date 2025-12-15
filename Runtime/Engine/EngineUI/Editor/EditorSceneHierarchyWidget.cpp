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

	SearchFilterBuf.Fill(0);
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

		if (EditorEngine)
		{
			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
			{
				EditorEngine->ClearSelection();
			}
		}
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

	// Scene data
	FCamera* Camera = World->GetCamera();

	const TArray<FActor*>&      Actors      = World->GetActors();
	const TArray<FLight*>&      Lights      = World->GetLights();
	const TArray<FLightProbe*>& LightProbes = World->GetLightProbes();

	const bool bHasActors   = !Actors.IsEmpty();
	const bool bHasLights   = !Lights.IsEmpty();
	const bool bHasProbes   = !LightProbes.IsEmpty();
	const bool bHasCameras   = Camera != nullptr;
	const bool bHasLighting = bHasLights || bHasProbes;

	const auto DrawLeafRow = [](const char* Label, const char* Type, const bool bSelected, void* Id, auto&& OnClick)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::PushID(Id);

		ImGuiTreeNodeFlags Flags =
			ImGuiTreeNodeFlags_Leaf |
			ImGuiTreeNodeFlags_NoTreePushOnOpen |
			ImGuiTreeNodeFlags_SpanFullWidth;

		if (bSelected)
		{
			Flags |= ImGuiTreeNodeFlags_Selected;
		}

		ImGui::TreeNodeEx(Label, Flags);

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			OnClick();
		}

		ImGui::TableSetColumnIndex(1);
		ImGui::TextUnformatted(Type);

		ImGui::PopID();
	};

	const auto DrawFolderRowBegin = [](const char* Label, const char* Type, void* Id, bool bDefaultOpen = true) -> bool
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::PushID(Id);

		ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_SpanFullWidth;
		if (bDefaultOpen)
		{
			Flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}

		const bool bOpen = ImGui::TreeNodeEx(Label, Flags);

		ImGui::TableSetColumnIndex(1);
		ImGui::TextUnformatted(Type);

		return bOpen;
	};

	const auto DrawFolderRowEnd = [&](bool bWasOpen)
	{
		if (bWasOpen)
		{
			ImGui::TreePop();
		}

		ImGui::PopID();
	};

	// -------------------------------------------------------------------------------------------
	// Search Field (Actor Search)
	// -------------------------------------------------------------------------------------------

	ImGui::SetNextItemWidth(-1.0f);

	const float BorderRounding = 16.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, BorderRounding);
	ImGui::InputTextWithHint("##SceneHierarchySearch", "Search Actors", SearchFilterBuf.Data(), SearchFilterBuf.Size());
	ImGui::PopStyleVar();

	const ImVec2 ItemMin = ImGui::GetItemRectMin();
	const ImVec2 ItemMax = ImGui::GetItemRectMax();

	// Draw border if active
	const bool bIsInputFieldActive = ImGui::IsItemActive();
	if (bIsInputFieldActive)
	{
		const float BorderThickness = 2.0f;
		const ImU32 BorderColor = IM_COL32(100, 136, 234, 255);

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		DrawList->AddRect(ItemMin, ItemMax, BorderColor, BorderRounding, 0, BorderThickness);
	}

	// Separator between search and table
	ImGui::Separator();

	// -------------------------------------------------------------------------------------------
	// Actor Table
	// -------------------------------------------------------------------------------------------

	const ImGuiTableFlags TableFlags =
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_BordersInnerV |
		ImGuiTableFlags_BordersOuterH |
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchProp;

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10.0f, 6.0f));

	const ImVec2 TableSize = ImVec2(0.0f, ImGui::GetContentRegionAvail().y);
	if (!ImGui::BeginTable("##SceneOutliner", 2, TableFlags, TableSize))
	{
		ImGui::PopStyleVar();
		return;
	}

	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Item Label", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
	ImGui::TableHeadersRow();

	// Cameras
	if (bHasCameras)
	{
		const bool bCamerasOpen = DrawFolderRowBegin("Cameras", "Folder", (void*)"CamerasFolder", true);
		if (bCamerasOpen)
		{
			const CHAR* Name = "Main Camera";

			bool bCameraFound = true;

			const CHAR* Search = SearchFilterBuf.Data();
			if (Search && Search[0] != '\0')
			{
				if (!FCString::Stristr(Name, Search))
				{
					bCameraFound = false;
				}
			}

			if (bCameraFound)
			{
				DrawLeafRow(Name, "Camera", Camera == SelectedCamera, (void*)Camera, [&]()
				{
					EditorEngine->SetSelectedCamera(Camera);
				});
			}
		}

		DrawFolderRowEnd(bCamerasOpen);
	}

	// Actors folder
	if (bHasActors)
	{
		const bool bActorsOpen = DrawFolderRowBegin("Actors", "Folder", (void*)"ActorsFolder", true);
		if (bActorsOpen)
		{
			for (FActor* Actor : Actors)
			{
				if (!Actor)
				{
					continue;
				}

				const FString& Name = Actor->GetName();
				
				const CHAR* Search = SearchFilterBuf.Data();
				if (Search && Search[0] != '\0')
				{
					if (Name.IsEmpty() || !FCString::Stristr(*Name, Search))
					{
						continue;
					}
				}

				const char* Label = Name.IsEmpty() ? "Actor" : *Name;
				DrawLeafRow(Label, "Actor", Actor == SelectedActor, (void*)Actor, [&]()
				{
					EditorEngine->SetSelectedActor(Actor);
				});
			}
		}

		DrawFolderRowEnd(bActorsOpen);
	}

	// Lighting folder
	if (bHasLighting)
	{
		const bool bLightingOpen = DrawFolderRowBegin("Lighting", "Folder", (void*)"LightingFolder", true);
		if (bLightingOpen)
		{
			// Lights
			if (bHasLights)
			{
				int32 LightIndex = 0;
				for (FLight* Light : Lights)
				{
					if (!Light)
					{
						continue;
					}

					const char* TypeLabel = "Light";
					if (Cast<FPointLight>(Light))
					{
						TypeLabel = "PointLight";
					}
					else if (Cast<FDirectionalLight>(Light))
					{
						TypeLabel = "DirectionalLight";
					}

					constexpr uint32 LabelLength = 256;
					char Label[LabelLength];
					FCString::Snprintf(Label, LabelLength, "%s %d", TypeLabel, LightIndex++);

					const CHAR* Search = SearchFilterBuf.Data();
					if (Search && Search[0] != '\0')
					{
						if (!FCString::Stristr(Label, Search))
						{
							continue;
						}
					}

					DrawLeafRow(Label, TypeLabel, (Light == SelectedLight), (void*)Light, [&]()
					{
						EditorEngine->SetSelectedLight(Light);
					});
				}
			}

			// Light Probes
			if (bHasProbes)
			{
				int32 ProbeIndex = 0;
				for (FLightProbe* Probe : LightProbes)
				{
					if (!Probe)
					{
						continue;
					}

					constexpr uint32 LabelLength = 256;
					char Label[LabelLength];
					FCString::Snprintf(Label, LabelLength, "LightProbe %d", ProbeIndex++);

					const CHAR* Search = SearchFilterBuf.Data();
					if (Search && Search[0] != '\0')
					{
						if (!FCString::Stristr(Label, Search))
						{
							continue;
						}
					}

					DrawLeafRow(Label, "LightProbe", (Probe == SelectedLightProbe), (void*)Probe, [&]()
					{
						EditorEngine->SetSelectedLightProbe(Probe);
					});
				}
			}
		}

		DrawFolderRowEnd(bLightingOpen);
	}

	ImGui::EndTable();
	ImGui::PopStyleVar();
}

#if 0
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
#endif