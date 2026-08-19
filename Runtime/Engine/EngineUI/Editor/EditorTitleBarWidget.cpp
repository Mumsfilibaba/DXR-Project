#include "Core/CoreGlobals.h"
#include "Core/Misc/FrameProfiler.h"
#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/EngineUI/Editor/EditorTitleBarWidget.h"
#include "Engine/EngineUI/Editor/EditorOutputLogWidget.h"
#include "Engine/EngineUI/Editor/EditorSceneHierarchyWidget.h"
#include "Engine/EngineUI/Editor/EditorViewportWidget.h"
#include "Engine/EngineUI/Editor/EditorPropertiesWidget.h"
#include "Engine/EngineUI/Editor/EditorContentBrowserWidget.h"
#include "Engine/EngineUI/Editor/EditorRendererSettingsWidget.h"
#include "Engine/EngineUI/Editor/EditorGPUProfilerWidget.h"
#include "Engine/EngineUI/Editor/EditorFrameProfilerWidget.h"
#include "Engine/EngineUI/Editor/EditorRenderGraphWidget.h"
#include "Engine/EngineUI/Editor/EditorRHIInfoWidget.h"
#include "Engine/EngineUI/Editor/EditorStatsWidget.h"
#include "Engine/EngineUI/Editor/EditorAboutWidget.h"
#include "Application/Widgets/WindowWidget.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

enum class ECaptionButton : uint8
{
    Minimize,
    Maximize,
    Restore,
    Close,
};

constexpr ImU32 CAPTION_OVERLAY_HOVERED = IM_COL32(255, 255, 255, 15);
constexpr ImU32 CAPTION_OVERLAY_PRESSED = IM_COL32(255, 255, 255, 10);
constexpr ImU32 CAPTION_CLOSE_HOVERED   = IM_COL32(196, 43, 28, 255);
constexpr ImU32 CAPTION_CLOSE_PRESSED   = IM_COL32(196, 43, 28, 230);

// Only for the drawn fallback, since the real glyphs come at whatever size EditorFonts::SystemIcons was baked at.
constexpr float CAPTION_GLYPH_SIZE = 10.0f;

// Segoe Fluent Icons: E921 ChromeMinimize, E922 ChromeMaximize, E923 ChromeRestore, E8BB ChromeClose.
static const CHAR* GetCaptionGlyph(ECaptionButton Button)
{
    switch (Button)
    {
        case ECaptionButton::Minimize: return "\xEE\xA4\xA1";
        case ECaptionButton::Maximize: return "\xEE\xA4\xA2";
        case ECaptionButton::Restore:  return "\xEE\xA4\xA3";
        case ECaptionButton::Close:    return "\xEE\xA2\xBB";
    }

    return "";
}

// Used when neither system icon font is installed, so a missing font shows the right shape rather than tofu.
static void DrawCaptionGlyphShape(ImDrawList* DrawList, ECaptionButton Button, const ImVec2& Center, float Size, ImU32 Color)
{
    const float Half      = Size * 0.5f;
    const float Thickness = ImMax(1.0f, ImFloor(Size / 10.0f));

    const ImVec2 Min = ImVec2(ImFloor(Center.x - Half), ImFloor(Center.y - Half));
    const ImVec2 Max = ImVec2(Min.x + Size, Min.y + Size);

    switch (Button)
    {
        case ECaptionButton::Minimize:
        {
            const float MidY = ImFloor(Center.y);
            DrawList->AddLine(ImVec2(Min.x, MidY), ImVec2(Max.x, MidY), Color, Thickness);
            break;
        }

        case ECaptionButton::Maximize:
        {
            DrawList->AddRect(Min, Max, Color, 0.0f, ImDrawFlags_None, Thickness);
            break;
        }

        case ECaptionButton::Restore:
        {
            const float Offset = ImFloor(Size * 0.25f);
            DrawList->AddRect(ImVec2(Min.x, Min.y + Offset), ImVec2(Max.x - Offset, Max.y), Color, 0.0f, ImDrawFlags_None, Thickness);
            DrawList->AddRect(ImVec2(Min.x + Offset, Min.y), ImVec2(Max.x, Max.y - Offset), Color, 0.0f, ImDrawFlags_None, Thickness);
            break;
        }

        case ECaptionButton::Close:
        {
            DrawList->AddLine(Min, Max, Color, Thickness);
            DrawList->AddLine(ImVec2(Max.x, Min.y), ImVec2(Min.x, Max.y), Color, Thickness);
            break;
        }
    }
}

static bool DrawCaptionButton(const CHAR* Id, ECaptionButton Button, const ImVec2& Size, bool bWindowActive)
{
    const ImVec2 Min = ImGui::GetCursorScreenPos();
    const ImVec2 Max = ImVec2(Min.x + Size.x, Min.y + Size.y);

    const bool bPressed = ImGui::InvisibleButton(Id, Size);
    const bool bHovered = ImGui::IsItemHovered();
    const bool bHeld    = ImGui::IsItemActive();

    ImU32 BackplateColor = 0;
    ImU32 GlyphColor     = ImGui::GetColorU32(ImGuiCol_Text, bWindowActive ? 1.0f : 0.5f);

    if (Button == ECaptionButton::Close)
    {
        if (bHeld)
        {
            BackplateColor = CAPTION_CLOSE_PRESSED;
        }
        else if (bHovered)
        {
            BackplateColor = CAPTION_CLOSE_HOVERED;
        }

        if (bHovered || bHeld)
        {
            GlyphColor = IM_COL32_WHITE;
        }
    }
    else
    {
        if (bHeld)
        {
            BackplateColor = CAPTION_OVERLAY_PRESSED;
        }
        else if (bHovered)
        {
            BackplateColor = CAPTION_OVERLAY_HOVERED;
        }
    }

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    if (BackplateColor != 0)
    {
        DrawList->AddRectFilled(Min, Max, BackplateColor);
    }

    const ImVec2 Center = ImVec2((Min.x + Max.x) * 0.5f, (Min.y + Max.y) * 0.5f);

    if (ImFont* IconFont = EditorFonts::SystemIcons)
    {
        const CHAR*  Glyph     = GetCaptionGlyph(Button);
        const ImVec2 GlyphSpan = IconFont->CalcTextSizeA(IconFont->FontSize, FLT_MAX, 0.0f, Glyph);

        DrawList->AddText(IconFont, IconFont->FontSize, ImVec2(ImFloor(Center.x - GlyphSpan.x * 0.5f), ImFloor(Center.y - GlyphSpan.y * 0.5f)), GlyphColor, Glyph);
    }
    else
    {
        DrawCaptionGlyphShape(DrawList, Button, Center, CAPTION_GLYPH_SIZE * ImGui::GetIO().FontGlobalScale, GlyphColor);
    }

    return bPressed;
}

// The OS decides how tall its own chrome is, and the traffic lights on macOS will not re-centre themselves in a
// taller strip, so the strip is sized to the measurement rather than the other way around. A window without a
// custom title bar, and a macOS window in fullscreen, both report zero and leave the fallback height in place.
static FWindowTitleBarMetrics QueryTitleBarMetrics(const TSharedPtr<FWindowWidget>& Window)
{
    FWindowTitleBarMetrics Metrics;
    if (Window && (Window->GetStyle() & EWindowStyleFlags::CustomTitleBar) != EWindowStyleFlags::None)
    {
        Metrics = Window->GetTitleBarMetrics();
    }

    if (Metrics.Height > 0.0f)
    {
        EditorStyleVars::MainMenuBarHeight = Metrics.Height;
    }

    return Metrics;
}

void FEditorTitleBarWidget::RefreshMenuBarHeight(FEditorEngine* InEditorEngine)
{
    QueryTitleBarMetrics(InEditorEngine ? InEditorEngine->GetEngineWindow() : nullptr);
}

FEditorTitleBarWidget::FEditorTitleBarWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , Regions()
    , ClientOrigin(0.0f, 0.0f)
{
}

FEditorTitleBarWidget::~FEditorTitleBarWidget()
{
    EditorEngine = nullptr;
}

void FEditorTitleBarWidget::Draw()
{
    TRACE_SCOPE("Title Bar");

    TSharedPtr<FWindowWidget> Window = EditorEngine ? EditorEngine->GetEngineWindow() : nullptr;

    const FWindowTitleBarMetrics Metrics         = QueryTitleBarMetrics(Window);
    const bool                   bCustomTitleBar = Window && (Window->GetStyle() & EWindowStyleFlags::CustomTitleBar) != EWindowStyleFlags::None;

    const ImGuiWindowFlags TitleBarFlags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings;

    const ImGuiStyle& Style = ImGui::GetStyle();

    const ImVec4 TitleBarBg   = Style.Colors[ImGuiCol_MenuBarBg];
    const ImU32  HoveredColor = EditorStyleVars::ButtonBgHovered;
    const ImU32  PressedColor = EditorStyleVars::ButtonBgSelected;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, TitleBarBg);
    ImGui::PushStyleColor(ImGuiCol_Button, TitleBarBg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HoveredColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, PressedColor);

    Regions      = FWindowTitleBarRegions();
    ClientOrigin = ImGui::GetMainViewport()->Pos;

    if (ImGui::BeginChild("##EditorTitleBar", ImVec2(0.0f, EditorStyleVars::MainMenuBarHeight), ImGuiChildFlags_None, TitleBarFlags))
    {
        const ImVec2 StripMin = ImGui::GetWindowPos();
        const ImVec2 StripMax(StripMin.x + ImGui::GetWindowSize().x, StripMin.y + ImGui::GetWindowSize().y);

        ImGui::SetCursorPosY(0.0f);

        // Steps past the buttons the OS draws for itself, which is the traffic lights on macOS and nothing on Windows.
        if (Metrics.LeadingInset > 0.0f)
        {
            ImGui::SetCursorPosX(Metrics.LeadingInset);
        }

        DrawMenuButtons();

        if (bCustomTitleBar && !FPlatformApplicationMisc::DoesPlatformDrawCustomTitleBarCaptionButtons())
        {
            DrawCaptionButtons(Metrics, Window);
        }

        // Only where the OS still has chrome for the strip to stand in for. Zero height means macOS fullscreen,
        // where there is no window to drag and no zoom to toggle, so the strip is a plain menu bar.
        if (Metrics.Height > 0.0f)
        {
            Regions.CaptionRect = FWindowRect(
                static_cast<int32>(StripMin.x - ClientOrigin.x),
                static_cast<int32>(StripMin.y - ClientOrigin.y),
                static_cast<int32>(StripMax.x - ClientOrigin.x),
                static_cast<int32>(StripMax.y - ClientOrigin.y));
        }
    }

    ImGui::EndChild();

    ImGui::PopStyleColor(4); // ChildBg + Button + ButtonHovered + ButtonActive
    ImGui::PopStyleVar(3);   // WindowPadding + ItemSpacing + FrameRounding

    if (Window)
    {
        Window->SetTitleBarRegions(Regions);
    }
}

void FEditorTitleBarWidget::AddInteractiveRect(const ImVec2& Min, const ImVec2& Max)
{
    Regions.InteractiveRects.Emplace(
        static_cast<int32>(Min.x - ClientOrigin.x),
        static_cast<int32>(Min.y - ClientOrigin.y),
        static_cast<int32>(Max.x - ClientOrigin.x),
        static_cast<int32>(Max.y - ClientOrigin.y));
}

void FEditorTitleBarWidget::DrawCaptionButtons(const FWindowTitleBarMetrics& Metrics, const TSharedPtr<FWindowWidget>& Window)
{
    if (Metrics.CaptionButtonWidth <= 0.0f)
    {
        return;
    }

    const ImVec2 ButtonSize(Metrics.CaptionButtonWidth, ImGui::GetWindowSize().y);
    const bool   bIsMaximized = Window->IsMaximized();
    const bool   bIsActive    = Window->IsActive();

    // Flush to the trailing corner with no gap, exactly where the system buttons would have been.
    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - (ButtonSize.x * 3.0f), 0.0f));

    if (DrawCaptionButton("##CaptionMinimize", ECaptionButton::Minimize, ButtonSize, bIsActive))
    {
        Window->Minimize();
    }

    AddInteractiveRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    ImGui::SameLine(0.0f, 0.0f);

    const ECaptionButton MaximizeGlyph = bIsMaximized ? ECaptionButton::Restore : ECaptionButton::Maximize;
    if (DrawCaptionButton("##CaptionMaximize", MaximizeGlyph, ButtonSize, bIsActive))
    {
        if (bIsMaximized)
        {
            Window->Restore();
        }
        else
        {
            Window->Maximize();
        }
    }

    // Windows resolves this one to HTMAXBUTTON so the Snap Layouts flyout can appear, which means the OS
    // maximizes the window and the click above never arrives. Both rects are published for that reason.
    const ImVec2 MaximizeMin = ImGui::GetItemRectMin();
    const ImVec2 MaximizeMax = ImGui::GetItemRectMax();

    AddInteractiveRect(MaximizeMin, MaximizeMax);

    Regions.MaximizeButtonRect = FWindowRect(
        static_cast<int32>(MaximizeMin.x - ClientOrigin.x),
        static_cast<int32>(MaximizeMin.y - ClientOrigin.y),
        static_cast<int32>(MaximizeMax.x - ClientOrigin.x),
        static_cast<int32>(MaximizeMax.y - ClientOrigin.y));

    ImGui::SameLine(0.0f, 0.0f);

    if (DrawCaptionButton("##CaptionClose", ECaptionButton::Close, ButtonSize, bIsActive))
    {
        RequestEngineExit("Title bar close button");
    }

    AddInteractiveRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

void FEditorTitleBarWidget::DrawMenuButtons()
{
    const CHAR* PopupFile    = "##ToolbarPopup_File";
    const CHAR* PopupEdit    = "##ToolbarPopup_Edit";
    const CHAR* PopupWindows = "##ToolbarPopup_Windows";
    const CHAR* PopupHelp    = "##ToolbarPopup_Help";

    const bool bAnyPopupOpen =
        ImGui::IsPopupOpen(PopupFile, ImGuiPopupFlags_None) ||
        ImGui::IsPopupOpen(PopupEdit, ImGuiPopupFlags_None) ||
        ImGui::IsPopupOpen(PopupWindows, ImGuiPopupFlags_None) ||
        ImGui::IsPopupOpen(PopupHelp, ImGuiPopupFlags_None);

    // File
    {
        FPopupAnchor FileAnchor;
        EditorWidgets::MenuButton("File", PopupFile, bAnyPopupOpen, EditorStyleVars::MainMenuBarHeight, FileAnchor);

        AddInteractiveRect(FileAnchor.Min, FileAnchor.Max);

        if (EditorWidgets::BeginMenuPopup(PopupFile, FileAnchor))
        {
            EditorWidgets::MenuLabeledSeparator("Open");
            EditorWidgets::MenuItem("New Level", "Ctrl+N");
            EditorWidgets::MenuItem("Open Level", "Ctrl+O");
            EditorWidgets::MenuLabeledSeparator("Save");
            EditorWidgets::MenuItem("Save All", "Ctrl+Shift+S");
            EditorWidgets::MenuLabeledSeparator("Exit");
            EditorWidgets::MenuItem("Exit");
            EditorWidgets::EndMenuPopup();
        }
    }

    ImGui::SameLine(0.0f, 0.0f);

    // Edit
    {
        FPopupAnchor EditAnchor;
        EditorWidgets::MenuButton("Edit", PopupEdit, bAnyPopupOpen, EditorStyleVars::MainMenuBarHeight, EditAnchor);

        AddInteractiveRect(EditAnchor.Min, EditAnchor.Max);

        if (EditorWidgets::BeginMenuPopup(PopupEdit, EditAnchor))
        {
            EditorWidgets::MenuLabeledSeparator("Settings");
            EditorWidgets::MenuItem("Project Settings");
            EditorWidgets::MenuItem("Editor Preferences");
            EditorWidgets::EndMenuPopup();
        }
    }

    ImGui::SameLine(0.0f, 0.0f);

    // Windows
    {
        FPopupAnchor WindowsAnchor;
        EditorWidgets::MenuButton("Windows", PopupWindows, bAnyPopupOpen, EditorStyleVars::MainMenuBarHeight, WindowsAnchor);

        AddInteractiveRect(WindowsAnchor.Min, WindowsAnchor.Max);

        if (EditorWidgets::BeginMenuPopup(PopupWindows, WindowsAnchor))
        {
            EditorWidgets::MenuLabeledSeparator("Windows");

            if (EditorEngine)
            {
                if (FEditorOutputLogWidget* LogWidget = EditorEngine->GetOutputLogWidget().Get())
                {
                    bool bVisible = LogWidget->IsVisible();
                    if (EditorWidgets::MenuItem("Output Log", nullptr, bVisible))
                    {
                        LogWidget->SetVisible(!bVisible);
                    }
                }
                else
                {
                    EditorWidgets::MenuItem("Output Log", nullptr, false, false);
                }

                if (FEditorViewportWidget* EditorWidget = EditorEngine->GetEditorViewportWidget().Get())
                {
                    bool bVisible = EditorWidget->IsVisible();
                    if (EditorWidgets::MenuItem("Viewport", nullptr, bVisible))
                    {
                        EditorWidget->SetVisible(!bVisible);
                    }
                }
                else
                {
                    EditorWidgets::MenuItem("Viewport", nullptr, false, false);
                }

                if (FEditorSceneHierarchyWidget* SceneHierarchyWidget = EditorEngine->GetSceneHierarchyWidget().Get())
                {
                    bool bVisible = SceneHierarchyWidget->IsVisible();
                    if (EditorWidgets::MenuItem("Scene Hierarchy", nullptr, bVisible))
                    {
                        SceneHierarchyWidget->SetVisible(!bVisible);
                    }
                }
                else
                {
                    EditorWidgets::MenuItem("Scene Hierarchy", nullptr, false, false);
                }

                if (FEditorPropertiesWidget* PropertiesWidget = EditorEngine->GetPropertiesWidget().Get())
                {
                    bool bVisible = PropertiesWidget->IsVisible();
                    if (EditorWidgets::MenuItem("Properties", nullptr, bVisible))
                    {
                        PropertiesWidget->SetVisible(!bVisible);
                    }
                }
                else
                {
                    EditorWidgets::MenuItem("Properties", nullptr, false, false);
                }

                if (FEditorRendererSettingsWidget* RendererSettingsWidget = EditorEngine->GetRendererSettingsWidget().Get())
                {
                    bool bVisible = RendererSettingsWidget->IsVisible();
                    if (EditorWidgets::MenuItem("Renderer Settings", nullptr, bVisible))
                    {
                        RendererSettingsWidget->SetVisible(!bVisible);
                    }
                }
                else
                {
                    EditorWidgets::MenuItem("Renderer Settings", nullptr, false, false);
                }

                if (FEditorContentBrowserWidget* ContentBrowserWidget = EditorEngine->GetContentBrowserWidget().Get())
                {
                    bool bVisible = ContentBrowserWidget->IsVisible();
                    if (EditorWidgets::MenuItem("Content Browser", nullptr, bVisible))
                    {
                        ContentBrowserWidget->SetVisible(!bVisible);
                    }
                }
                else
                {
                    EditorWidgets::MenuItem("Content Browser", nullptr, false, false);
                }

                if (FEditorGPUProfilerWidget* GPUProfilerWidget = EditorEngine->GetGPUProfilerWidget().Get())
                {
                    bool bVisible = GPUProfilerWidget->IsVisible();
                    if (EditorWidgets::MenuItem("GPU Profiler", nullptr, bVisible))
                    {
                        GPUProfilerWidget->SetVisible(!bVisible);
                    }
                }
                else
                {
                    EditorWidgets::MenuItem("GPU Profiler", nullptr, false, false);
                }

                if (FEditorFrameProfilerWidget* FrameProfilerWidget = EditorEngine->GetFrameProfilerWidget().Get())
                {
                    bool bVisible = FrameProfilerWidget->IsVisible();
                    if (EditorWidgets::MenuItem("Frame Profiler", nullptr, bVisible))
                    {
                        FrameProfilerWidget->SetVisible(!bVisible);
                    }
                }
                else
                {
                    EditorWidgets::MenuItem("Frame Profiler", nullptr, false, false);
                }

                if (FEditorRenderGraphWidget* RenderGraphWidget = EditorEngine->GetRenderGraphWidget().Get())
                {
                    bool bVisible = RenderGraphWidget->IsVisible();
                    if (EditorWidgets::MenuItem("Render Graph", nullptr, bVisible))
                    {
                        RenderGraphWidget->SetVisible(!bVisible);
                    }
                }
                else
                {
                    EditorWidgets::MenuItem("Render Graph", nullptr, false, false);
                }

                if (FEditorRHIInfoWidget* RHIInfoWidget = EditorEngine->GetRHIInfoWidget().Get())
                {
                    bool bVisible = RHIInfoWidget->IsVisible();
                    if (EditorWidgets::MenuItem("RHI Info", nullptr, bVisible))
                    {
                        RHIInfoWidget->SetVisible(!bVisible);
                    }
                }
                else
                {
                    EditorWidgets::MenuItem("RHI Info", nullptr, false, false);
                }

                if (FEditorStatsWidget* StatsWidget = EditorEngine->GetStatsWidget().Get())
                {
                    bool bVisible = StatsWidget->IsVisible();
                    if (EditorWidgets::MenuItem("Engine Stats", nullptr, bVisible))
                    {
                        StatsWidget->SetVisible(!bVisible);
                    }
                }
                else
                {
                    EditorWidgets::MenuItem("Engine Stats", nullptr, false, false);
                }

                if (FEditorAboutWidget* AboutWidget = EditorEngine->GetAboutWidget().Get())
                {
                    bool bVisible = AboutWidget->IsVisible();
                    if (EditorWidgets::MenuItem("About", nullptr, bVisible))
                    {
                        AboutWidget->SetVisible(!bVisible);
                    }
                }
                else
                {
                    EditorWidgets::MenuItem("About", nullptr, false, false);
                }
            }

            EditorWidgets::EndMenuPopup();
        }
    }

    ImGui::SameLine(0.0f, 0.0f);

    // Help
    {
        FPopupAnchor HelpAnchor;
        EditorWidgets::MenuButton("Help", PopupHelp, bAnyPopupOpen, EditorStyleVars::MainMenuBarHeight, HelpAnchor);

        AddInteractiveRect(HelpAnchor.Min, HelpAnchor.Max);

        if (EditorWidgets::BeginMenuPopup(PopupHelp, HelpAnchor))
        {
            EditorWidgets::MenuLabeledSeparator("About");

            if (FEditorAboutWidget* AboutWidget = EditorEngine ? EditorEngine->GetAboutWidget().Get() : nullptr)
            {
                if (EditorWidgets::MenuItem("About"))
                {
                    AboutWidget->SetVisible(true);
                }
            }
            else
            {
                EditorWidgets::MenuItem("About", nullptr, false, false);
            }

            EditorWidgets::EndMenuPopup();
        }
    }
}
