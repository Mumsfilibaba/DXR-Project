#include "Engine/EngineUI/Editor/EditorContentBrowserWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include <imgui.h>
#include <imgui_internal.h>

FEditorContentBrowserWidget::FEditorContentBrowserWidget()
    : ImGuiDelegateHandle()
    , bVisible(true)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorContentBrowserWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }

	FolderSearchBuffer.Fill(0);
	AssetSearchBuffer.Fill(0);
}

FEditorContentBrowserWidget::~FEditorContentBrowserWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FEditorContentBrowserWidget::Draw()
{
	if (!bVisible)
	{
		return;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, EditorStyleVars::SceneHierarchyItemSpacing);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, EditorStyleVars::SceneHierarchyWindowPadding);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f));

	const ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoFocusOnAppearing;
	if (ImGui::Begin("Content Browser", &bVisible, WindowFlags))
	{
		// Colors
		const ImVec4 PanelBg               = ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);
		const ImVec4 RowHoverBg            = ImVec4(47.0f / 255.0f, 47.0f / 255.0f, 47.0f / 255.0f, 1.0f);
		const ImVec4 SearchBg              = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);
		const ImVec4 SearchTextColor       = ImVec4(77.0f / 255.0f, 77.0f / 255.0f, 77.0f / 255.0f, 1.0f);
		const ImVec4 NameTextColor         = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);
		const ImVec4 MutedTextColor        = ImVec4(122.0f / 255.0f, 122.0f / 255.0f, 122.0f / 255.0f, 1.0f);
		const ImU32  BorderNormal          = IM_COL32(51, 51, 51, 255);
		const ImU32  BorderHovered         = IM_COL32(74, 74, 74, 255);
		const ImU32  BorderActive          = IM_COL32(9, 92, 176, 255);
		const ImU32  SelectedActiveColor   = IM_COL32(0, 112, 224, 255);
		const ImU32  SelectedInactiveColor = IM_COL32(64, 87, 111, 255);

		const auto DrawSearchField = [&](const char* InId, const char* InHint, TStaticArray<CHAR, 256>& InOutBuffer)
		{
			ImGui::SetNextItemWidth(-1.0f);

			const ImVec2 BasePadding = EditorStyleVars::InputFieldFramePadding;
			const float  IconGapPx   = 6.0f;
			const float  IconSizePx  = 16.0f;
			const float  PaddedX     = BasePadding.x + IconSizePx + IconGapPx;

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(PaddedX, BasePadding.y));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, EditorStyleVars::InputFieldBorderRounding);

			ImGui::PushStyleColor(ImGuiCol_FrameBg, SearchBg);
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, SearchBg);
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, SearchBg);
			ImGui::PushStyleColor(ImGuiCol_Text, SearchTextColor);
			ImGui::PushStyleColor(ImGuiCol_TextDisabled, SearchTextColor);

			ImGui::InputTextWithHint(InId, InHint, InOutBuffer.Data(), InOutBuffer.Size());

			if (EditorIcons::SearchIcon)
			{
				const ImVec2 ItemMin = ImGui::GetItemRectMin();
				const ImVec2 ItemMax = ImGui::GetItemRectMax();
				const float  ItemH   = ItemMax.y - ItemMin.y;
				const ImVec2 IconMin = ImVec2(ItemMin.x + BasePadding.x, ItemMin.y + (ItemH - IconSizePx) * 0.5f);
				const ImVec2 IconMax = ImVec2(IconMin.x + IconSizePx, IconMin.y + IconSizePx);

				ImDrawList* DrawList = ImGui::GetWindowDrawList();
				DrawList->AddImage(EditorIcons::SearchIcon, IconMin, IconMax, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
			}

			ImGui::PopStyleColor(5);
			ImGui::PopStyleVar(2);

			{
				const ImVec2 ItemMin = ImGui::GetItemRectMin();
				const ImVec2 ItemMax = ImGui::GetItemRectMax();

				const bool bActive  = ImGui::IsItemActive();
				const bool bHovered = ImGui::IsItemHovered();

				const ImU32 BorderColor = bActive ? BorderActive : (bHovered ? BorderHovered : BorderNormal);

				ImDrawList* DrawList = ImGui::GetWindowDrawList();
				DrawList->AddRect(
					ItemMin,
					ItemMax,
					BorderColor,
					EditorStyleVars::InputFieldBorderRounding,
					0,
					EditorStyleVars::InputFieldBorderThickness);
			}
		};

		const ImGuiTableFlags LayoutFlags =
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_NoPadOuterX |
			ImGuiTableFlags_BordersInnerV;

		if (ImGui::BeginTable("##ContentBrowserLayout", 2, LayoutFlags))
		{
			ImGui::TableSetupColumn("##Folders", ImGuiTableColumnFlags_WidthFixed, 260.0f);
			ImGui::TableSetupColumn("##Content", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			ImGui::PushStyleColor(ImGuiCol_ChildBg, PanelBg);
			if (ImGui::BeginChild("##CB_Folders", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar))
			{
				DrawSearchField("##CB_FolderSearch", "Search Folders", FolderSearchBuffer);
				ImGui::Spacing();

				struct FFolderRow 
				{ 
					const char* Label; 
					int32       Indent; 
					bool        bIsRoot; 
				};

				const FFolderRow Folders[] =
				{
					{ "Content",   0, true  },
					{ "Materials", 18, false },
					{ "Meshes",    18, false },
					{ "Textures",  18, false },
					{ "Scenes",    18, false },
				};

				ImGuiStyle& Style = ImGui::GetStyle();
				const float RowHeight = ImGui::GetTextLineHeight() + Style.FramePadding.y * 2.0f;

				ImGuiStorage* Storage = ImGui::GetStateStorage();
				const ImGuiID RootOpenId = ImGui::GetID("##CB_RootOpen");

				bool bRootOpen = Storage->GetBool(RootOpenId, true);
				for (int32 i = 0; i < (int32)(sizeof(Folders) / sizeof(Folders[0])); ++i)
				{
					const bool bIsRoot = Folders[i].bIsRoot;
					if (!bIsRoot && !bRootOpen)
					{
						continue;
					}

					const bool bSelected      = (SelectedFolderIndex == i);
					const bool bWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

					const ImU32 SelColor = (bWindowFocused && bSelectionActiveInBrowser) ? SelectedActiveColor : SelectedInactiveColor;

					ImGui::PushID(i);

					if (bSelected)
					{
						ImGui::PushStyleColor(ImGuiCol_Header, SelColor);
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SelColor);
						ImGui::PushStyleColor(ImGuiCol_HeaderActive, SelColor);
					}
					else
					{
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered, RowHoverBg);
						ImGui::PushStyleColor(ImGuiCol_HeaderActive, RowHoverBg);
					}

					const ImGuiSelectableFlags SelFlags =
						ImGuiSelectableFlags_SpanAllColumns |
						ImGuiSelectableFlags_AllowItemOverlap;

					if (ImGui::Selectable("##FolderRow", bSelected, SelFlags, ImVec2(0.0f, RowHeight)))
					{
						bSelectionActiveInBrowser = true;
						SelectedFolderIndex       = i;

						if (bIsRoot)
						{
							bRootOpen = !bRootOpen;
							Storage->SetBool(RootOpenId, bRootOpen);
						}
					}

					if (bSelected)
					{
						ImGui::PopStyleColor(3);
					}
					else
					{
						ImGui::PopStyleColor(2);
					}

					const ImVec2 RowMin = ImGui::GetItemRectMin();
					const ImVec2 RowMax = ImGui::GetItemRectMax();
					const float  H      = RowMax.y - RowMin.y;

					const float FontSize         = ImGui::GetFontSize();
					const float TextHeight       = ImGui::GetTextLineHeight();
					const float TextY            = RowMin.y + (H - TextHeight) * 0.5f;
					const float ArrowY           = RowMin.y + (H - FontSize) * 0.5f;
					const float ArrowTextGap     = 6.0f;
					const float ArrowAdvance     = FontSize;
					const float FolderIconGapPx  = 6.0f;
					const float FolderIconSizePx = ImMin(16.0f, ImMax(1.0f, H - 6.0f));
					const float IndentPx         = static_cast<float>(Folders[i].Indent);

					ImGui::PushStyleColor(ImGuiCol_Text, NameTextColor);

					ImVec2 Cursor = ImGui::GetCursorScreenPos();

					const ImVec2 ArrowPos = ImVec2(RowMin.x + Style.FramePadding.x + IndentPx, ArrowY);
					float LabelX = ArrowPos.x;

					ImDrawList* DrawList = ImGui::GetWindowDrawList();
					if (bIsRoot)
					{
						const ImGuiDir Dir = bRootOpen ? ImGuiDir_Down : ImGuiDir_Right;
						ImGui::RenderArrow(DrawList, ArrowPos, ImGui::GetColorU32(ImGuiCol_Text), Dir, 1.0f);
						LabelX = ArrowPos.x + ArrowAdvance + ArrowTextGap;
					}
					else
					{
						LabelX = ArrowPos.x + ArrowAdvance + ArrowTextGap;
					}

					if (EditorIcons::FolderSmallIcon || EditorIcons::FolderOpenSmallIcon)
					{
						ImTextureID FolderIcon = (bIsRoot && bRootOpen) ? EditorIcons::FolderOpenSmallIcon : EditorIcons::FolderSmallIcon;
						if (FolderIcon)
						{
							const float  IconY   = RowMin.y + (H - FolderIconSizePx) * 0.5f;
							const ImVec2 IconMin = ImVec2(LabelX, IconY);
							const ImVec2 IconMax = ImVec2(IconMin.x + FolderIconSizePx, IconMin.y + FolderIconSizePx);
							
							DrawList->AddImage(FolderIcon, IconMin, IconMax);
							LabelX = IconMax.x + FolderIconGapPx;
						}
					}

					DrawList->AddText(ImVec2(LabelX, TextY), ImGui::GetColorU32(ImGuiCol_Text), Folders[i].Label);
					ImGui::PopStyleColor();

					ImGui::PopID();
				}
			}

			ImGui::EndChild();
			ImGui::PopStyleColor(); // ChildBg

			ImGui::TableSetColumnIndex(1);

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f));

			if (ImGui::BeginChild("##CB_Content", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar))
			{
				{
					// Breadcrumb (placeholder)
					ImGui::PushStyleColor(ImGuiCol_Text, MutedTextColor);
					ImGui::TextUnformatted("Path:");
					ImGui::PopStyleColor();

					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, NameTextColor);
					ImGui::TextUnformatted("/Game");
					ImGui::PopStyleColor();

					// Search on same line aligned to right
					const float SearchWidth = 320.0f;
					const float RightEdgeX  = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
					
					ImGui::SameLine();
					ImGui::SetCursorScreenPos(ImVec2(RightEdgeX - SearchWidth, ImGui::GetCursorScreenPos().y - ImGui::GetTextLineHeight() - ImGui::GetStyle().ItemSpacing.y));

					ImGui::SetNextItemWidth(SearchWidth);

					DrawSearchField("##CB_AssetSearch", "Search Assets", AssetSearchBuffer);
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				if (ImGui::BeginChild("##CB_GridScroll", ImVec2(0, 0), false, 0))
				{
					struct FItem 
					{ 
						const char* Label; 
						bool        bFolder; 
					};

					const FItem Items[] =
					{
						{ "Meshes",    true  },
						{ "Materials", true  },
						{ "Textures",  true  },
						{ "Crate_01.asset", false },
						{ "Door.asset",     false },
						{ "Wood.asset",     false },
						{ "Stone.asset",    false },
						{ "Metal.asset",    false },
					};

					const float TileW  = 110.0f;
					const float TileH  = 110.0f;
					const float LabelH = ImGui::GetTextLineHeight() * 2.0f;
					const float CellW  = TileW + ImGui::GetStyle().ItemSpacing.x;
					const float AvailX = ImGui::GetContentRegionAvail().x;

					int32 ColumnCount = static_cast<int32>(AvailX / CellW);
					if (ColumnCount < 1)
					{
						ColumnCount = 1;
					}

					if (ImGui::BeginTable("##CB_AssetGrid", ColumnCount, ImGuiTableFlags_SizingFixedFit))
					{
						const bool bWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

						const ImU32 SelColor = (bWindowFocused && bSelectionActiveInBrowser) ? SelectedActiveColor : SelectedInactiveColor;
						for (int32 i = 0; i < static_cast<int32>(sizeof(Items) / sizeof(Items[0])); ++i)
						{
							ImGui::TableNextColumn();

							ImGui::PushID(i);

							const bool bSelected = (SelectedItemIndex == i);

							const ImVec2 TileStart = ImGui::GetCursorScreenPos();
							const ImVec2 TileEnd   = ImVec2(TileStart.x + TileW, TileStart.y + TileH);

							ImGui::InvisibleButton("##TileBtn", ImVec2(TileW, TileH));
							
							const bool bHovered = ImGui::IsItemHovered();
							const bool bPressed = ImGui::IsItemClicked();

							if (bPressed)
							{
								bSelectionActiveInBrowser = true;
								SelectedItemIndex         = i;
							}

							ImDrawList* DL = ImGui::GetWindowDrawList();

							const ImU32 BgIdle  = IM_COL32(31, 31, 31, 255);
							const ImU32 BgHover = IM_COL32(47, 47, 47, 255);
							const ImU32 BgSel   = bSelected ? SelColor : (bHovered ? BgHover : BgIdle);

							const float Rounding = 6.0f;
							DL->AddRectFilled(TileStart, TileEnd, BgSel, Rounding);

							ImTextureID Icon = nullptr;
							if (Items[i].bFolder)
							{
								Icon = EditorIcons::FolderIcon ? EditorIcons::FolderIcon : EditorIcons::FolderSmallIcon;
							}
							else
							{
								Icon = EditorIcons::FolderIcon ? EditorIcons::FolderIcon : EditorIcons::FolderSmallIcon;
							}

							if (Icon)
							{
								const float IconSize = 52.0f;
								const ImVec2 Center  = ImVec2((TileStart.x + TileEnd.x) * 0.5f, (TileStart.y + TileEnd.y) * 0.5f);
								const ImVec2 IconMin = ImVec2(Center.x - IconSize * 0.5f, Center.y - IconSize * 0.5f);
								const ImVec2 IconMax = ImVec2(IconMin.x + IconSize, IconMin.y + IconSize);
								DL->AddImage(Icon, IconMin, IconMax);
							}

							const ImU32 Outline = bSelected ? IM_COL32(255, 255, 255, 60) : IM_COL32(0, 0, 0, 0);
							if (Outline != 0)
							{
								DL->AddRect(TileStart, TileEnd, Outline, Rounding, 0, 1.0f);
							}

							ImGui::PushStyleColor(ImGuiCol_Text, NameTextColor);
							ImGui::TextWrapped("%s", Items[i].Label);
							ImGui::PopStyleColor();

							ImGui::Dummy(ImVec2(0.0f, ImMax(0.0f, LabelH - ImGui::GetTextLineHeight())));

							ImGui::PopID();
						}

						ImGui::EndTable();
					}
				}

				ImGui::EndChild();
			}

			ImGui::EndChild();
			ImGui::PopStyleColor(); // ChildBg

			ImGui::EndTable();
		}
	}

	ImGui::End();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}
