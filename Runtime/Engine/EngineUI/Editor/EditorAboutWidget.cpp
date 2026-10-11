#include "Core/Containers/StaticArray.h"
#include "Core/Templates/CString.h"
#include "Core/Misc/BuildInfo.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/EngineUI/Editor/EditorAboutWidget.h"

static constexpr float AboutLabelColumnWidth  = 160.0f;
static constexpr float AboutRevertColumnWidth = 28.0f;

static const CHAR_T* GetDirtySuffix()
{
    return BuildInfo::IsWorkingTreeDirty() ? TEXT(" (modified)") : TEXT("");
}

static void CopyRowToClipboard(const CHAR_T* Label, const CHAR_T* ValueText)
{
    FString Text;
    Text.AppendPrintf(TEXT("%s: %s"), Label, ValueText ? ValueText : TEXT(""));
    ImGui::SetClipboardText(StringCast<ANSICHAR>(Text).Get());
}

static void DrawSectionHeader(const CHAR_T* Label)
{
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextBorderSize, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.1f, 0.5f));
    ImGui::SeparatorText(StringCast<ANSICHAR>(Label).Get());
    ImGui::PopStyleVar(2);
}

FEditorAboutWidget::FEditorAboutWidget()
    : ImGuiDelegateHandle()
    , bVisible(false)
    , ContextRowLabel()
    , ContextRowValue()
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorAboutWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorAboutWidget::~FEditorAboutWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDrawDelegate(ImGuiDelegateHandle);
    }
}

void FEditorAboutWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    TRACE_SCOPE(TEXT("About"));

    ImGui::SetNextWindowSize(EditorWidgets::ScaleEditorWindowSize(ImVec2(520.0f, 540.0f)), ImGuiCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, EditorStyleVars::PropertiesItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, EditorStyleVars::PropertiesWindowPadding);

    if (EditorWidgets::BeginEditorWindow(TEXT("About"), &bVisible))
    {
        // The popup opens on release, so retarget then. A release that lands outside any row clears the previous target.
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            ContextRowLabel.Clear();
            ContextRowValue.Clear();
        }

        DrawTitle();
        DrawSourceInfo();
        DrawBuildInfo();
        DrawGraphicsInfo();

        DrawContextMenu();
    }

    EditorWidgets::EndEditorWindow();

    ImGui::PopStyleVar(2);
}

void FEditorAboutWidget::DrawTitle()
{
    TStaticArray<CHAR_T, 128> TitleText{};
    CString::Snprintf(TitleText.Data(), static_cast<int32>(TitleText.Size()), TEXT("%s %s"), BuildInfo::GetEngineName(), BuildInfo::GetVersionString());

    ImFont* TitleFont = EditorFonts::SegoeUI_22 ? EditorFonts::SegoeUI_22 : EditorFonts::DefaultFont;
    ImGui::PushFont(TitleFont);

    const float Offset = (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(StringCast<ANSICHAR>(TitleText.Data()).Get()).x) * 0.5f;
    if (Offset > 0.0f)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Offset);
    }

    ImGui::TextUnformatted(StringCast<ANSICHAR>(TitleText.Data()).Get());
    ImGui::PopFont();
}

void FEditorAboutWidget::DrawSourceInfo()
{
    DrawSectionHeader(TEXT("Source"));

    if (EditorWidgets::BeginPropertyTable(TEXT("##AboutSourceTable"), AboutLabelColumnWidth, AboutRevertColumnWidth))
    {
        DrawProperty(TEXT("Branch"), BuildInfo::GetBranch());

        TStaticArray<CHAR_T, 64> CommitText{};
        CString::Snprintf(CommitText.Data(), static_cast<int32>(CommitText.Size()), TEXT("%s%s"), BuildInfo::GetCommit(), GetDirtySuffix());
        DrawProperty(TEXT("Commit"), CommitText.Data());

        DrawProperty(TEXT("Commit Date"), BuildInfo::GetCommitDate());

        EditorWidgets::EndPropertyTable();
    }
}

void FEditorAboutWidget::DrawBuildInfo()
{
    DrawSectionHeader(TEXT("Build"));

    if (EditorWidgets::BeginPropertyTable(TEXT("##AboutBuildTable"), AboutLabelColumnWidth, AboutRevertColumnWidth))
    {
        TStaticArray<CHAR_T, 64> ConfigurationText{};
        CString::Snprintf(ConfigurationText.Data(), static_cast<int32>(ConfigurationText.Size()), TEXT("%s (%s)"), BuildInfo::GetConfigurationName(), BuildInfo::GetLinkageName());
        DrawProperty(TEXT("Configuration"), ConfigurationText.Data());

        TStaticArray<CHAR_T, 64> PlatformText{};
        CString::Snprintf(PlatformText.Data(), static_cast<int32>(PlatformText.Size()), TEXT("%s %s"), BuildInfo::GetPlatformName(), BuildInfo::GetArchitectureName());
        DrawProperty(TEXT("Platform"), PlatformText.Data());

        DrawProperty(TEXT("Compiler"), BuildInfo::GetCompilerName());
        DrawProperty(TEXT("Compiled"), BuildInfo::GetCompileTimestamp());

        EditorWidgets::EndPropertyTable();
    }
}

void FEditorAboutWidget::DrawGraphicsInfo()
{
    DrawSectionHeader(TEXT("Graphics"));

    FRHIDevice* Device = RHI::Device;
    if (!Device)
    {
        return;
    }

    if (EditorWidgets::BeginPropertyTable(TEXT("##AboutGraphicsTable"), AboutLabelColumnWidth, AboutRevertColumnWidth))
    {
        DrawProperty(TEXT("Backend"), ToString(Device->GetRHIType()));

        // The Metal backend does not report an adapter name yet
        const FString AdapterName = Device->GetAdapterName();
        DrawProperty(TEXT("Adapter"), AdapterName.IsEmpty() ? TEXT("Unknown") : *AdapterName);

        EditorWidgets::EndPropertyTable();
    }
}

void FEditorAboutWidget::DrawContextMenu()
{
    if (EditorWidgets::BeginPopupContextWindow(TEXT("AboutContextMenu")))
    {
        const bool bHasRow = !ContextRowLabel.IsEmpty();
        EditorWidgets::MenuLabeledSeparator(bHasRow ? *ContextRowLabel : TEXT("Common"));

        if (EditorWidgets::MenuItem(TEXT("Copy"), EDITOR_SHORTCUT_MOD TEXT("+C"), false, bHasRow))
        {
            CopyRowToClipboard(*ContextRowLabel, *ContextRowValue);
        }

        EditorWidgets::EndPopupContext();
    }
}

void FEditorAboutWidget::DrawProperty(const CHAR_T* Label, const CHAR_T* ValueText)
{
    if (!EditorWidgets::DrawTextProperty(Label, ValueText))
    {
        return;
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        ContextRowLabel = Label;
        ContextRowValue = ValueText ? ValueText : TEXT("");
    }

    const ImGuiIO& IO = ImGui::GetIO();
    if (IO.KeyCtrl && !IO.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_C, false))
    {
        ImGui::SetNextFrameWantCaptureKeyboard(true);
        CopyRowToClipboard(Label, ValueText);
    }
}
