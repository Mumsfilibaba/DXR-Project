#include "Core/Containers/StaticArray.h"
#include "Core/Templates/CString.h"
#include "Core/Misc/BuildInfo.h"
#include "RHI/RHI.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/EngineUI/Editor/EditorAboutWidget.h"

static constexpr float AboutLabelColumnWidth  = 160.0f;
static constexpr float AboutRevertColumnWidth = 28.0f;

static const CHAR* GetDirtySuffix()
{
    return BuildInfo::IsWorkingTreeDirty() ? " (modified)" : "";
}

static void CopyRowToClipboard(const CHAR* Label, const CHAR* ValueText)
{
    String Text;
    Text.AppendPrintf("%s: %s", Label, ValueText ? ValueText : "");
    ImGui::SetClipboardText(*Text);
}

static void DrawSectionHeader(const CHAR* Label)
{
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextBorderSize, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.1f, 0.5f));
    ImGui::SeparatorText(Label);
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

    const ImVec2 FrameBufferScale = ImGuiExtensions::GetDisplayFramebufferScale();
    const float  Scale            = FrameBufferScale.x;

    ImGui::SetNextWindowSize(ImVec2(520.0f * Scale, 540.0f * Scale), ImGuiCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, EditorStyleVars::PropertiesItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, EditorStyleVars::PropertiesWindowPadding);

    if (ImGui::Begin("About", &bVisible, ImGuiWindowFlags_NoCollapse))
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

    ImGui::End();

    ImGui::PopStyleVar(2);
}

void FEditorAboutWidget::DrawTitle()
{
    TStaticArray<CHAR, 128> TitleText{};
    CString::Snprintf(TitleText.Data(), static_cast<int32>(TitleText.Size()), "%s %s", BuildInfo::GetEngineName(), BuildInfo::GetVersionString());

    ImFont* TitleFont = EditorFonts::SegoeUI_22 ? EditorFonts::SegoeUI_22 : EditorFonts::DefaultFont;
    ImGui::PushFont(TitleFont);

    const float Offset = (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(TitleText.Data()).x) * 0.5f;
    if (Offset > 0.0f)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Offset);
    }

    ImGui::TextUnformatted(TitleText.Data());
    ImGui::PopFont();
}

void FEditorAboutWidget::DrawSourceInfo()
{
    DrawSectionHeader("Source");

    if (EditorWidgets::BeginPropertyTable("##AboutSourceTable", AboutLabelColumnWidth, AboutRevertColumnWidth))
    {
        DrawProperty("Branch", BuildInfo::GetBranch());

        TStaticArray<CHAR, 64> CommitText{};
        CString::Snprintf(CommitText.Data(), static_cast<int32>(CommitText.Size()), "%s%s", BuildInfo::GetCommit(), GetDirtySuffix());
        DrawProperty("Commit", CommitText.Data());

        DrawProperty("Commit Date", BuildInfo::GetCommitDate());

        EditorWidgets::EndPropertyTable();
    }
}

void FEditorAboutWidget::DrawBuildInfo()
{
    DrawSectionHeader("Build");

    if (EditorWidgets::BeginPropertyTable("##AboutBuildTable", AboutLabelColumnWidth, AboutRevertColumnWidth))
    {
        TStaticArray<CHAR, 64> ConfigurationText{};
        CString::Snprintf(ConfigurationText.Data(), static_cast<int32>(ConfigurationText.Size()), "%s (%s)", BuildInfo::GetConfigurationName(), BuildInfo::GetLinkageName());
        DrawProperty("Configuration", ConfigurationText.Data());

        TStaticArray<CHAR, 64> PlatformText{};
        CString::Snprintf(PlatformText.Data(), static_cast<int32>(PlatformText.Size()), "%s %s", BuildInfo::GetPlatformName(), BuildInfo::GetArchitectureName());
        DrawProperty("Platform", PlatformText.Data());

        DrawProperty("Compiler", BuildInfo::GetCompilerName());
        DrawProperty("Compiled", BuildInfo::GetCompileTimestamp());

        EditorWidgets::EndPropertyTable();
    }
}

void FEditorAboutWidget::DrawGraphicsInfo()
{
    DrawSectionHeader("Graphics");

    FRHIDevice* Device = RHI::Device;
    if (!Device)
    {
        return;
    }

    if (EditorWidgets::BeginPropertyTable("##AboutGraphicsTable", AboutLabelColumnWidth, AboutRevertColumnWidth))
    {
        DrawProperty("Backend", ToString(Device->GetRHIType()));

        // The Metal backend does not report an adapter name yet
        const String AdapterName = Device->GetAdapterName();
        DrawProperty("Adapter", AdapterName.IsEmpty() ? "Unknown" : *AdapterName);

        EditorWidgets::EndPropertyTable();
    }
}

void FEditorAboutWidget::DrawContextMenu()
{
    if (EditorWidgets::BeginPopupContextWindow("AboutContextMenu"))
    {
        const bool bHasRow = !ContextRowLabel.IsEmpty();
        EditorWidgets::MenuLabeledSeparator(bHasRow ? *ContextRowLabel : "Common");

        if (EditorWidgets::MenuItem("Copy", EDITOR_SHORTCUT_MOD "+C", false, bHasRow))
        {
            CopyRowToClipboard(*ContextRowLabel, *ContextRowValue);
        }

        EditorWidgets::EndPopupContext();
    }
}

void FEditorAboutWidget::DrawProperty(const CHAR* Label, const CHAR* ValueText)
{
    if (!EditorWidgets::DrawTextProperty(Label, ValueText))
    {
        return;
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        ContextRowLabel = Label;
        ContextRowValue = ValueText ? ValueText : "";
    }

    const ImGuiIO& IO = ImGui::GetIO();
    if (IO.KeyCtrl && !IO.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_C, false))
    {
        ImGui::SetNextFrameWantCaptureKeyboard(true);
        CopyRowToClipboard(Label, ValueText);
    }
}
