#include "Engine/EngineUI/Editor/EditorSceneHierarchyWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/EditorEngine.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include <imgui.h>
#include <imgui_internal.h>

FEditorSceneHierarchyWidget::FEditorSceneHierarchyWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
	, RenamingActor(nullptr)
    , bVisible(true)
	, bRequestRenameFocus(false)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorSceneHierarchyWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }

	ActorSearchFilterBuffer.Fill(0);
	ActorRenameBuffer.Fill(0);
	ActorRenameBufferOriginal.Fill(0);
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
	const auto DrawFolderRow = [](const char* Label, const char* Type, const char* OpenKey, bool bDefaultOpen, float IndentPx) -> bool
	{
		ImGuiStyle& Style = ImGui::GetStyle();

		ImGui::PushID(OpenKey);

		ImGuiStorage* Storage = ImGui::GetStateStorage();

		const float SelectableRowHeight = ImGui::GetTextLineHeight() + Style.CellPadding.y * 2.0f;

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		const ImGuiSelectableFlags SelectableFlags =
			ImGuiSelectableFlags_SpanAllColumns | 
			ImGuiSelectableFlags_AllowItemOverlap;

		const ImGuiID OpenId = ImGui::GetID("Open");

		bool bOpen = Storage->GetBool(OpenId, bDefaultOpen);
		if (ImGui::Selectable("##Row", false, SelectableFlags, ImVec2(0.0f, SelectableRowHeight)))
		{
			bOpen = !bOpen;
			Storage->SetBool(OpenId, bOpen);
		}

		const ImVec2 RowMin       = ImGui::GetItemRectMin();
		const ImVec2 RowMax       = ImGui::GetItemRectMax();
		const float  RowHeight    = RowMax.y - RowMin.y;
		const float  TextHeight   = ImGui::GetTextLineHeight();
		const float  FontSize     = ImGui::GetFontSize();
		const float  TextY        = RowMin.y + (RowHeight - TextHeight) * 0.5f;
		const float  ArrowY       = RowMin.y + (RowHeight - FontSize) * 0.5f;
		const float  ArrowTextGap = 6.0f;
		const float  ArrowAdvance = FontSize;

		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, TextY));
		
		// TODO: Draw icon here perhaps?

		ImGui::TableSetColumnIndex(1);

		const ImVec2   CollumnPosition = ImGui::GetCursorScreenPos();
		const ImVec2   ArrowPosition   = ImVec2(CollumnPosition.x + IndentPx, ArrowY);
		const ImU32    ArrowColor      = ImGui::GetColorU32(ImGuiCol_Text);
		const ImGuiDir ArrowDirection  = bOpen ? ImGuiDir_Down : ImGuiDir_Right;

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		ImGui::RenderArrow(DrawList, ArrowPosition, ArrowColor, ArrowDirection, 1.0f);

		ImGui::SetCursorScreenPos(ImVec2(ArrowPosition.x + ArrowAdvance + ArrowTextGap, TextY));
		ImGui::TextUnformatted(Label);

		ImGui::TableSetColumnIndex(2);
		ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, TextY));
		ImGui::TextUnformatted(Type);

		ImGui::PopID();
		return bOpen;
	};

	const auto DrawLeafRow = [](const char* Label, const char* Type, bool bSelected, void* Id, float IndentPx, auto&& OnClick)
	{
		ImGuiStyle& Style = ImGui::GetStyle();

		const float SelectableRowHeight = ImGui::GetTextLineHeight() + Style.CellPadding.y * 2.0f;

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::PushID(Id);

		const ImGuiSelectableFlags SelectableFlags =
			ImGuiSelectableFlags_SpanAllColumns |
			ImGuiSelectableFlags_AllowItemOverlap;

		if (bSelected)
		{
			const ImU32 SelectedColor = IM_COL32(0x1d, 0x4f, 0x8b, 140);
			ImGui::PushStyleColor(ImGuiCol_Header, SelectedColor);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SelectedColor);
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, SelectedColor);
		}

		if (ImGui::Selectable("##Row", bSelected, SelectableFlags, ImVec2(0.0f, SelectableRowHeight)))
		{
			OnClick();
		}

		if (bSelected)
		{
			ImGui::PopStyleColor(3);
		}

		const ImVec2 RowMin       = ImGui::GetItemRectMin();
		const ImVec2 RowMax       = ImGui::GetItemRectMax();
		const float  RowHeight    = RowMax.y - RowMin.y;
		const float  TextHeight   = ImGui::GetTextLineHeight();
		const float  FontSize     = ImGui::GetFontSize();
		const float  TextY        = RowMin.y + (RowHeight - TextHeight) * 0.5f;
		const float  ArrowTextGap = 6.0f;
		const float  ArrowAdvance = FontSize;

		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, TextY));

		ImGui::TableSetColumnIndex(1);

		const ImVec2 Col1Pos = ImGui::GetCursorScreenPos();
		ImGui::SetCursorScreenPos(ImVec2(Col1Pos.x + IndentPx + ArrowAdvance + ArrowTextGap, TextY));
		ImGui::TextUnformatted(Label);

		ImGui::TableSetColumnIndex(2);
		ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, TextY));
		ImGui::TextUnformatted(Type);

		ImGui::PopID();
	};

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
	FLightProbe* SelectedLightProbe = EditorEngine->GetSelectedLightProbe();
	FLight*      SelectedLight      = EditorEngine->GetSelectedLight();
	FCamera*     SelectedCamera     = EditorEngine->GetSelectedCamera();

	// Pull camera (Currently only a single camera)
	FCamera* Camera = World->GetCamera();

	// Scene data
	const TArray<FActor*>&      Actors      = World->GetActors();
	const TArray<FLight*>&      Lights      = World->GetLights();
	const TArray<FLightProbe*>& LightProbes = World->GetLightProbes();

	const bool bHasActors   = !Actors.IsEmpty();
	const bool bHasLights   = !Lights.IsEmpty();
	const bool bHasProbes   = !LightProbes.IsEmpty();
	const bool bHasCameras  = Camera != nullptr;
	const bool bHasLighting = bHasLights || bHasProbes;

	// -------------------------------------------------------------------------------------------
	// Search Field (Actor Search)
	// -------------------------------------------------------------------------------------------

	ImGui::SetNextItemWidth(-1.0f);

	const float BorderRounding = 16.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, EditorStyleVars::InputFieldFramePadding);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, BorderRounding);

	ImGui::InputTextWithHint("##SceneHierarchySearch", "Search Actors", ActorSearchFilterBuffer.Data(), ActorSearchFilterBuffer.Size());
	
	ImGui::PopStyleVar(2);

	const ImVec2 ItemMin = ImGui::GetItemRectMin();
	const ImVec2 ItemMax = ImGui::GetItemRectMax();

	// Draw border if active
	const bool bIsInputFieldActive = ImGui::IsItemActive();
	if (bIsInputFieldActive)
	{
		const float BorderThickness = 2.0f;
		const ImU32 BorderColor     = IM_COL32(100, 136, 234, 255);

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		DrawList->AddRect(ItemMin, ItemMax, BorderColor, BorderRounding, 0, BorderThickness);
	}

	// -------------------------------------------------------------------------------------------
	// Divider between table and search field
	// -------------------------------------------------------------------------------------------

	ImGui::Dummy(ImVec2(0.0f, 2.0f));

	// -------------------------------------------------------------------------------------------
	// Actor Table
	// -------------------------------------------------------------------------------------------

	const ImGuiTableFlags TableFlags =
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_NoBordersInBody |
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchProp;

	ImGuiStyle& Style = ImGui::GetStyle();

	const float  ChildIndent    = Style.IndentSpacing;
	const ImVec4 BorderDarkGray = ImVec4(37.0f / 255.0f, 37.0f / 255.0f, 37.0f / 255.0f, 1.0f);

	ImGui::PushStyleColor(ImGuiCol_TableBorderLight, BorderDarkGray);
	ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, BorderDarkGray);

	const ImVec2 TableSize = ImVec2(0.0f, ImGui::GetContentRegionAvail().y);
	if (!ImGui::BeginTable("##SceneOutliner", 3, TableFlags, TableSize))
	{
		ImGui::PopStyleColor(2); // Border Colors
		return;
	}

	ImGui::TableSetupScrollFreeze(0, 1);

	// Custom header
	const float TypeColWidth = ImGui::CalcTextSize("DirectionalLight").x + Style.CellPadding.x * 2.0f + 12.0f;
	ImGui::TableSetupColumn("##Gutter", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_NoResize, 8.0f);
	ImGui::TableSetupColumn("Item Label", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, TypeColWidth);
	
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0, 10.0f));
	ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

	for (int32 Column = 0; Column < 3; Column++)
	{
		ImGui::TableSetColumnIndex(Column);
		ImGui::TableHeader(ImGui::TableGetColumnName(Column));
	}

	ImGui::PopStyleVar(); // CellPadding

	// Setup padding for the content in the table
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 4.0f));

	// Cameras
	if (bHasCameras)
	{
		const bool bCamerasOpen = DrawFolderRow("Cameras", "Folder", "CamerasFolder", true, 0.0f);
		if (bCamerasOpen)
		{
			const CHAR* Name = "Main Camera";

			bool bCameraFound = true;

			const CHAR* Search = ActorSearchFilterBuffer.Data();
			if (Search && Search[0] != '\0')
			{
				if (!FCString::Stristr(Name, Search))
				{
					bCameraFound = false;
				}
			}

			if (bCameraFound)
			{
				DrawLeafRow("Main Camera", "Camera", Camera == SelectedCamera, (void*)Camera, ChildIndent, [&]()
				{
					EditorEngine->SetSelectedCamera(Camera);
				});
			}
		}
	}

	// Actors folder
	if (bHasActors)
	{
		const bool bActorsOpen = DrawFolderRow("Actors", "Folder", "ActorsFolder", true, 0.0f);
		if (bActorsOpen)
		{
			for (FActor* Actor : Actors)
			{
				if (!Actor)
				{
					continue;
				}

				const FString& Name = Actor->GetName();
				
				const CHAR* Search = ActorSearchFilterBuffer.Data();
				if (Search && Search[0] != '\0')
				{
					if (Name.IsEmpty() || !FCString::Stristr(*Name, Search))
					{
						continue;
					}
				}

				DrawActorRow(Actor, "Actor", Actor == SelectedActor, ChildIndent);
			}
		}
	}

	// Lighting folder
	if (bHasLighting)
	{
		const bool bLightingOpen = DrawFolderRow("Lighting", "Folder", "LightingFolder", true, 0.0f);
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

					const CHAR* Search = ActorSearchFilterBuffer.Data();
					if (Search && Search[0] != '\0')
					{
						if (!FCString::Stristr(Label, Search))
						{
							continue;
						}
					}

					DrawLeafRow(Label, TypeLabel, Light == SelectedLight, (void*)Light, ChildIndent, [&]()
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

					const CHAR* Search = ActorSearchFilterBuffer.Data();
					if (Search && Search[0] != '\0')
					{
						if (!FCString::Stristr(Label, Search))
						{
							continue;
						}
					}

					DrawLeafRow(Label, "LightProbe", Probe == SelectedLightProbe, (void*)Probe, ChildIndent, [&]()
					{
						EditorEngine->SetSelectedLightProbe(Probe);
					});
				}
			}
		}
	}

	ImGui::PopStyleColor(2); // Border Colors
	ImGui::PopStyleVar(); // CellPadding

	ImGui::EndTable();
}

void FEditorSceneHierarchyWidget::DrawActorRow(FActor* Actor, const char* Type, const bool bSelected, float IndentPx)
{
	if (!Actor)
	{
		return;
	}

	const auto BeginActorRename = [this](FActor* InActor)
	{
		RenamingActor       = InActor;
		bRequestRenameFocus = true;

		ActorRenameBuffer.Fill(0);
		ActorRenameBufferOriginal.Fill(0);

		const FString& Name = InActor->GetName();
		if (!Name.IsEmpty())
		{
			FCString::Strncpy(ActorRenameBuffer.Data(), *Name, ActorRenameBuffer.Size());
			FCString::Strncpy(ActorRenameBufferOriginal.Data(), *Name, ActorRenameBufferOriginal.Size());
		}
	};

	const auto CancelActorRename = [this]()
	{
		RenamingActor       = nullptr;
		bRequestRenameFocus = false;
	};

	const auto CommitActorRename = [this]()
	{
		if (RenamingActor)
		{
			RenamingActor->SetName(FString(ActorRenameBuffer.Data()));
			RenamingActor = nullptr;
		}

		bRequestRenameFocus = false;
	};

	const ImU32 RowBlue_Selected = IM_COL32(0x1d, 0x4f, 0x8b, 140);
	const ImU32 RowBlue_Rename   = IM_COL32(0x3f, 0x7b, 0xb6, 160);
	const ImU32 BorderBlueRename = IM_COL32(0x3f, 0x7b, 0xb6, 255);

	ImGuiStyle& Style = ImGui::GetStyle();

	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);

	ImGui::PushID(Actor);

	const float SelectableRowHeight = ImGui::GetTextLineHeight() + Style.CellPadding.y * 2.0f;

	bool bIsRenamingThis = (RenamingActor == Actor);
	if (bIsRenamingThis && !bSelected)
	{
		CommitActorRename();
		bIsRenamingThis = false;
	}

	const ImGuiSelectableFlags SelectableFlags =
		ImGuiSelectableFlags_SpanAllColumns |
		ImGuiSelectableFlags_AllowItemOverlap;

	const ImU32 RowBlue = bIsRenamingThis ? RowBlue_Rename : RowBlue_Selected;
	if (bSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_Header, RowBlue);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, RowBlue);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, RowBlue);
	}

	const bool bRowPressed = ImGui::Selectable("##Row", bSelected, SelectableFlags, ImVec2(0.0f, SelectableRowHeight));
	if (bSelected)
	{
		ImGui::PopStyleColor(3);
	}

	const ImVec2 RowMin       = ImGui::GetItemRectMin();
	const ImVec2 RowMax       = ImGui::GetItemRectMax();
	const float  RowHeight    = RowMax.y - RowMin.y;
	const float  TextHeight   = ImGui::GetTextLineHeight();
	const float  TextY        = RowMin.y + (RowHeight - TextHeight) * 0.5f;
	const float  FontSize     = ImGui::GetFontSize();
	const float  ArrowAdvance = FontSize;
	const float  ArrowTextGap = 6.0f;

	// Column 0: Empty
	ImGui::TableSetColumnIndex(0);
	ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, TextY));

	// Column 1: Label
	ImGui::TableSetColumnIndex(1);

	const ImVec2 Column1Pos   = ImGui::GetCursorScreenPos();
	const float  Column1Width = ImGui::GetContentRegionAvail().x;
	const float  Column1MinX  = Column1Pos.x;
	const float  Column1MaxX  = Column1Pos.x + Column1Width;
	const float  LabelStartX  = Column1Pos.x + IndentPx + ArrowAdvance + ArrowTextGap;
	const ImVec2 MousePos     = ImGui::GetIO().MousePos;

	const bool bClickedInLabelColumn = (MousePos.x >= Column1MinX && MousePos.x <= Column1MaxX);
	if (bRowPressed)
	{
		if (!bSelected)
		{
			EditorEngine->SetSelectedActor(Actor);
			CancelActorRename();
		}
		else
		{
			if (bClickedInLabelColumn && !bIsRenamingThis)
			{
				BeginActorRename(Actor);
				bIsRenamingThis = true;
			}
		}
	}

	const float RenameFramePadY = Style.CellPadding.y;
	if (bIsRenamingThis)
	{
		const float InputY     = TextY - RenameFramePadY;
		const float InputX     = LabelStartX - Style.FramePadding.x;
		const float InputWidth = (Column1MaxX - InputX) - 2.0f;

		ImGui::SetCursorScreenPos(ImVec2(InputX, InputY));
		ImGui::SetNextItemWidth(InputWidth > 0.0f ? InputWidth : 0.0f);

		const float BorderRounding  = 16.0f;
		const float BorderThickness = 2.0f;

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, BorderRounding);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Style.FramePadding.x, RenameFramePadY));

		if (bRequestRenameFocus)
		{
			ImGui::SetKeyboardFocusHere();
			bRequestRenameFocus = false;
		}

		const ImGuiInputTextFlags InputFlags =
			ImGuiInputTextFlags_EnterReturnsTrue |
			ImGuiInputTextFlags_AutoSelectAll;

		const bool bEnter = ImGui::InputText("##RenameActor", ActorRenameBuffer.Data(), ActorRenameBuffer.Size(), InputFlags);

		ImGui::PopStyleVar(2);

		if (ImGui::IsItemActive())
		{
			const ImVec2 ItemMin = ImGui::GetItemRectMin();
			const ImVec2 ItemMax = ImGui::GetItemRectMax();

			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			DrawList->AddRect(ItemMin, ItemMax, BorderBlueRename, BorderRounding, 0, BorderThickness);
		}

		if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			FCString::Strncpy(ActorRenameBuffer.Data(), ActorRenameBufferOriginal.Data(), ActorRenameBuffer.Size());
			CancelActorRename();
		}
		else if (bEnter || ImGui::IsItemDeactivatedAfterEdit())
		{
			CommitActorRename();
		}
		else if (ImGui::IsItemDeactivated())
		{
			CancelActorRename();
		}
	}
	else
	{
		ImGui::SetCursorScreenPos(ImVec2(LabelStartX, TextY));

		const FString& Name = Actor->GetName();
		ImGui::TextUnformatted(Name.IsEmpty() ? "Actor" : *Name);
	}

	// Column 2: type baseline compensation when InputText was used in this row.
	ImGui::TableSetColumnIndex(2);

	const float TypeLabelX            = ImGui::GetCursorScreenPos().x;
	const float BaselineCompensationY = bIsRenamingThis ? RenameFramePadY : 0.0f;

	ImGui::SetCursorScreenPos(ImVec2(TypeLabelX, TextY - BaselineCompensationY));
	ImGui::TextUnformatted(Type);

	ImGui::PopID();
}

