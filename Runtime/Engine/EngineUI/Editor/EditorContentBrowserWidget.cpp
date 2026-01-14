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
		// -----------------------------------------------------------------------------------------
		// Colors
		// -----------------------------------------------------------------------------------------
		const ImVec4 PanelBg             = ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);
		const ImVec4 RightBg             = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
		const ImVec4 RowHoverBg          = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
		const ImVec4 SearchBg            = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);
		const ImVec4 SearchTextColor     = ImVec4(77.0f / 255.0f, 77.0f / 255.0f, 77.0f / 255.0f, 1.0f);
		const ImVec4 NameTextColor       = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);
		const ImVec4 MutedTextColor      = ImVec4(122.0f / 255.0f, 122.0f / 255.0f, 122.0f / 255.0f, 1.0f);
		const ImU32  FolderActiveColor   = IM_COL32(0, 112, 224, 255);
		const ImU32  FolderInactiveColor = IM_COL32(64, 87, 111, 255);
		const ImU32  FolderHoverColor    = IM_COL32(56, 56, 56, 255);
		const ImU32  TileSelectedColor   = IM_COL32(0, 112, 224, 255);
		const ImU32  TileHoverColor      = IM_COL32(47, 47, 47, 255);
		const ImU32  TileIdleColor       = IM_COL32(31, 31, 31, 255);

		const auto DrawSearchField = [&](const char* InId, const char* InHint, TStaticArray<CHAR, 256>& InOutBuffer)
		{
			ImGui::SetNextItemWidth(-1.0f);

			const ImVec2 BasePadding = EditorStyleVars::InputFieldFramePadding;
			const float  IconGapPx   = 6.0f;
			const float  IconSizePx  = 16.0f;
			const float  PaddedX     = BasePadding.x + IconSizePx + IconGapPx;

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(PaddedX, BasePadding.y));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, EditorStyleVars::InputFieldBorderRounding);

			const ImVec4 SearchBg        = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);
			const ImVec4 SearchTextColor = ImVec4(77.0f / 255.0f, 77.0f / 255.0f, 77.0f / 255.0f, 1.0f);

			ImGui::PushStyleColor(ImGuiCol_FrameBg, SearchBg);
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, SearchBg);
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, SearchBg);
			ImGui::PushStyleColor(ImGuiCol_Text, SearchTextColor);
			ImGui::PushStyleColor(ImGuiCol_TextDisabled, SearchTextColor);

			ImGui::InputTextWithHint(InId, InHint, InOutBuffer.Data(), InOutBuffer.Size());

			// Search icon
			if (EditorIcons::SearchIcon)
			{
				const ImVec2 ItemMin = ImGui::GetItemRectMin();
				const ImVec2 ItemMax = ImGui::GetItemRectMax();
				const float  ItemH   = ItemMax.y - ItemMin.y;
				const ImVec2 IconMin = ImVec2(ItemMin.x + BasePadding.x, ItemMin.y + (ItemH - IconSizePx) * 0.5f);
				const ImVec2 IconMax = ImVec2(IconMin.x + IconSizePx, IconMin.y + IconSizePx);

				ImGui::GetWindowDrawList()->AddImage(
					EditorIcons::SearchIcon,
					IconMin, IconMax,
					ImVec2(0, 0), ImVec2(1, 1),
					IM_COL32(255, 255, 255, 255));
			}

			ImGui::PopStyleColor(5);
			ImGui::PopStyleVar(2);

			// Custom border
			{
				const ImU32 BorderNormal  = IM_COL32(51, 51, 51, 255);
				const ImU32 BorderHovered = IM_COL32(74, 74, 74, 255);
				const ImU32 BorderActive  = IM_COL32(9, 92, 176, 255);

				const ImVec2 ItemMin = ImGui::GetItemRectMin();
				const ImVec2 ItemMax = ImGui::GetItemRectMax();

				const bool bActive  = ImGui::IsItemActive();
				const bool bHovered = ImGui::IsItemHovered();

				const ImU32 BorderColor = bActive ? BorderActive : (bHovered ? BorderHovered : BorderNormal);

				ImGui::GetWindowDrawList()->AddRect(
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

			// -------------------------------------------------------------------------------------
			// LEFT: Folder panel
			// -------------------------------------------------------------------------------------
			ImGui::TableSetColumnIndex(0);

			ImGui::PushStyleColor(ImGuiCol_ChildBg, PanelBg);

			if (ImGui::BeginChild("##CB_Folders", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar))
			{
				DrawSearchField("##CB_FolderSearch", "Search Folders", FolderSearchBuffer);
				ImGui::Dummy(ImVec2(0.0f, 6.0f));

				struct FFolderRow
				{
					const char* Label;
					int32       Indent;
					bool        bIsRoot;
				};

				const FFolderRow Folders[] =
				{
					{ "Content",   0,  true  },
					{ "Materials", 18, false },
					{ "Meshes",    18, false },
					{ "Textures",  18, false },
					{ "Scenes",    18, false },
				};

				ImGuiStyle& Style = ImGui::GetStyle();

				ImGuiStorage* Storage = ImGui::GetStateStorage();
				const ImGuiID RootOpenId = ImGui::GetID("##CB_RootOpen");
				bool bRootOpen = Storage->GetBool(RootOpenId, true);

				const float RowHeightPx      = 20.0f;
				const float EdgePadPx        = 2.0f;
				const float ArrowGapPx       = 4.0f;
				const float IconGapPx        = 4.0f;
				const float FolderIconSizePx = 16.0f;

				const bool bWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

				for (int32 i = 0; i < (int32)(sizeof(Folders) / sizeof(Folders[0])); ++i)
				{
					const bool bIsRoot = Folders[i].bIsRoot;
					if (!bIsRoot && !bRootOpen)
					{
						continue;
					}

					const bool  bSelected     = (SelectedFolderIndex == i);
					const ImU32 SelectedColor = (bWindowFocused && bSelectionActiveInBrowser) ? FolderActiveColor : FolderInactiveColor;

					ImGui::PushID(i);

					if (bSelected)
					{
						ImGui::PushStyleColor(ImGuiCol_Header, SelectedColor);
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SelectedColor);
						ImGui::PushStyleColor(ImGuiCol_HeaderActive, FolderActiveColor);
					}
					else
					{
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered, FolderHoverColor);
						ImGui::PushStyleColor(ImGuiCol_HeaderActive, FolderActiveColor);
					}

					const ImGuiSelectableFlags SelFlags =
						ImGuiSelectableFlags_SpanAllColumns |
						ImGuiSelectableFlags_AllowItemOverlap;

					if (ImGui::Selectable("##FolderRow", bSelected, SelFlags, ImVec2(0.0f, RowHeightPx)))
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

					const ImVec2 RowMin     = ImGui::GetItemRectMin();
					const ImVec2 RowMax     = ImGui::GetItemRectMax();
					const float  H          = RowMax.y - RowMin.y;
					const float  FontSize   = ImGui::GetFontSize();
					const float  TextHeight = ImGui::GetTextLineHeight();
					const float  TextY      = RowMin.y + (H - TextHeight) * 0.5f;
					const float  ArrowY     = RowMin.y + (H - FontSize) * 0.5f;

					const float IndentPx = (float)Folders[i].Indent;

					ImDrawList* DL = ImGui::GetWindowDrawList();

					const ImVec2 ArrowPos = ImVec2(RowMin.x + EdgePadPx + IndentPx, ArrowY);

					float X = ArrowPos.x;

					if (bIsRoot)
					{
						const ImGuiDir Dir = bRootOpen ? ImGuiDir_Down : ImGuiDir_Right;
						ImGui::RenderArrow(DL, ArrowPos, IM_COL32(220, 220, 220, 255), Dir, 1.0f);
						X += FontSize + ArrowGapPx;
					}
					else
					{
						X += FontSize + ArrowGapPx;
					}

					ImTextureID FolderIcon = (bIsRoot && bRootOpen) ? EditorIcons::FolderOpenSmallIcon : EditorIcons::FolderSmallIcon;
					if (FolderIcon)
					{
						const float  IconY   = RowMin.y + (H - FolderIconSizePx) * 0.5f;
						const ImVec2 IconMin = ImVec2(X, IconY);
						const ImVec2 IconMax = ImVec2(IconMin.x + FolderIconSizePx, IconMin.y + FolderIconSizePx);
						DL->AddImage(FolderIcon, IconMin, IconMax);

						X = IconMax.x + IconGapPx;
					}

					DL->AddText(ImVec2(X, TextY), ImGui::GetColorU32(NameTextColor), Folders[i].Label);

					ImGui::PopID();
				}
			}

			ImGui::EndChild();
			ImGui::PopStyleColor(); // ChildBg

			// -------------------------------------------------------------------------------------
			// RIGHT: Content panel
			// -------------------------------------------------------------------------------------
			ImGui::TableSetColumnIndex(1);

			ImGui::PushStyleColor(ImGuiCol_ChildBg, RightBg);
			if (ImGui::BeginChild("##CB_Content", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar))
			{
				DrawSearchField("##CB_AssetSearch", "Search Assets", AssetSearchBuffer);

				ImGui::Dummy(ImVec2(0.0f, 6.0f));

				if (ImGui::BeginChild("##CB_GridScroll", ImVec2(0, 0), false, 0))
				{
					struct FItem
					{
						const char* Label;
						bool        bFolder;
					};

					const FItem Items[] =
					{
						{ "Meshes",          true  },
						{ "Materials",       true  },
						{ "Textures",        true  },
						{ "Crate_01.asset",  false },
						{ "Door.asset",      false },
						{ "Wood.asset",      false },
						{ "Stone.asset",     false },
						{ "Metal.asset",     false },
					};

					const float TileW      = 132.0f;
					const float TileH      = 158.0f;
					const float LabelAreaH = 40.0f;
					const float CornerR    = 6.0f;
					const float IconPadPx  = 4.0f;
					const float CellW      = TileW + ImGui::GetStyle().ItemSpacing.x;
					const float AvailX     = ImGui::GetContentRegionAvail().x;

					int32 ColumnCount = (int32)(AvailX / CellW);
					if (ColumnCount < 1) ColumnCount = 1;

					if (ImGui::BeginTable("##CB_AssetGrid", ColumnCount, ImGuiTableFlags_SizingFixedFit))
					{
						for (int32 i = 0; i < (int32)(sizeof(Items) / sizeof(Items[0])); ++i)
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
								SelectedItemIndex         = i;
								bSelectionActiveInBrowser = false;
							}

							ImDrawList* DL = ImGui::GetWindowDrawList();

							const ImU32 Bg = bSelected ? TileSelectedColor : (bHovered ? TileHoverColor : TileIdleColor);
							DL->AddRectFilled(TileStart, TileEnd, Bg, CornerR);

							ImTextureID Icon = Items[i].bFolder ? EditorIcons::FolderIcon : EditorIcons::DocumentIcon;
							if (!Icon && Items[i].bFolder)
							{
								Icon = EditorIcons::FolderSmallIcon;
							}

							if (Icon)
							{
								const float  IconAreaH = TileH - LabelAreaH;
								const float  MaxIconSz = Math::Min((TileW - IconPadPx * 2.0f), (IconAreaH - IconPadPx * 2.0f));
								const float  IconSz    = Math::Max(1.0f, MaxIconSz);
								const ImVec2 IconMin   = ImVec2(TileStart.x + (TileW - IconSz) * 0.5f, TileStart.y + (IconAreaH - IconSz) * 0.5f);
								const ImVec2 IconMax   = ImVec2(IconMin.x + IconSz, IconMin.y + IconSz);

								DL->AddImage(Icon, IconMin, IconMax);
							}

							{
								const ImVec2 LabelMin = ImVec2(TileStart.x + 8.0f, TileEnd.y - LabelAreaH + 6.0f);
								const ImVec2 LabelMax = ImVec2(TileEnd.x - 8.0f, TileEnd.y - 6.0f);

								ImGui::PushStyleColor(ImGuiCol_Text, NameTextColor);
								ImGui::RenderTextClipped(LabelMin, LabelMax, Items[i].Label, nullptr, nullptr, ImVec2(0.5f, 0.0f));
								ImGui::PopStyleColor();
							}

							if (bHovered)
							{
								const ImVec4 TooltipBg     = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
								const ImVec4 TooltipBorder = ImVec4(71.0f / 255.0f, 71.0f / 255.0f, 71.0f / 255.0f, 1.0f);
								const ImVec4 TextWhite     = ImVec4(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 1.0f);
								const ImVec4 TextGrey      = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);

								ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
								ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
								ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
								ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 2.0f);

								ImGui::PushStyleColor(ImGuiCol_PopupBg, TooltipBg);
								ImGui::PushStyleColor(ImGuiCol_Border, TooltipBorder);
								ImGui::PushStyleColor(ImGuiCol_Separator, TooltipBorder);

								ImGui::BeginTooltip();

								ImGui::PushStyleColor(ImGuiCol_Text, TextWhite);
								ImGui::TextUnformatted(Items[i].Label);
								ImGui::PopStyleColor();

								ImGui::Spacing();

								const bool bIsFolder = Items[i].bFolder;

								ImTextureID TypeIcon = bIsFolder
									? (EditorIcons::FolderSmallIcon ? EditorIcons::FolderSmallIcon : EditorIcons::FolderIcon)
									: (EditorIcons::DocumentSmallIcon ? EditorIcons::DocumentSmallIcon : EditorIcons::DocumentIcon);

								if (TypeIcon)
								{
									const float IconSizePx = 16.0f;
									ImGui::Image(TypeIcon, ImVec2(IconSizePx, IconSizePx));
									ImGui::SameLine();
								}

								ImGui::PushStyleColor(ImGuiCol_Text, TextGrey);
								ImGui::TextUnformatted(bIsFolder ? "Folder" : "Document");
								ImGui::PopStyleColor();

								ImGui::Separator();

								char PathBuf[512];
								FCString::Snprintf(PathBuf, sizeof(PathBuf), "Content/%s", Items[i].Label);

								ImGui::PushStyleColor(ImGuiCol_Text, TextGrey);
								ImGui::TextUnformatted("Path:");
								ImGui::PopStyleColor();

								ImGui::SameLine();

								ImGui::PushStyleColor(ImGuiCol_Text, TextWhite);
								ImGui::TextUnformatted(PathBuf);
								ImGui::PopStyleColor();

								ImGui::EndTooltip();

								ImGui::PopStyleColor(3);
								ImGui::PopStyleVar(4);
							}

							ImGui::PopID();
						}

						ImGui::EndTable();
					}

					if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered())
					{
						SelectedItemIndex = -1;
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
