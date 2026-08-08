#include "Engine/EngineUI/Editor/EditorRendererSettingsWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Core/CoreDefines.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/CString.h"
#include "RHI/RHI.h"
#include "ImGuiPlugin/ImGuiCore.h"

static constexpr float RendererSettingsLabelColumnWidth  = 310.0f;
static constexpr float RendererSettingsRevertColumnWidth = 28.0f;

template<typename ValueType>
static const ValueType* TryGetDefaultPtr(const TMap<String, ValueType>& Defaults, const CHAR* CVarName, ValueType& OutValue)
{
    if (const ValueType* Found = Defaults.Find(String(CVarName)))
    {
        OutValue = *Found;
        return &OutValue;
    }

    return nullptr;
}

FEditorRendererSettingsWidget::FEditorRendererSettingsWidget()
    : ImGuiDelegateHandle()
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorRendererSettingsWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorRendererSettingsWidget::~FEditorRendererSettingsWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDrawDelegate(ImGuiDelegateHandle);
    }
}

void FEditorRendererSettingsWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, EditorStyleVars::PropertiesItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, EditorStyleVars::PropertiesWindowPadding);

    if (ImGui::Begin("Renderer Settings", &bVisible))
    {
        DrawWindow();
    }

    ImGui::End();

    ImGui::PopStyleVar(2);
}

void FEditorRendererSettingsWidget::CaptureDefaultsIfNeeded()
{
    if (bDefaultsCaptured)
    {
        return;
    }

    bDefaultsCaptured = true;

    auto& Console = FConsoleManager::Get();

    const auto CaptureBool = [&](const CHAR* Name)
    {
        if (IConsoleVariable* CVar = Console.FindConsoleVariable(Name))
        {
            BoolDefaults.Add(String(Name), CVar->GetBool());
        }
    };

    const auto CaptureInt = [&](const CHAR* Name)
    {
        if (IConsoleVariable* CVar = Console.FindConsoleVariable(Name))
        {
            IntDefaults.Add(String(Name), CVar->GetInt());
        }
    };

    const auto CaptureFloat = [&](const CHAR* Name)
    {
        if (IConsoleVariable* CVar = Console.FindConsoleVariable(Name))
        {
            FloatDefaults.Add(String(Name), CVar->GetFloat());
        }
    };

    // Deferred rendering
    CaptureBool("Renderer.Debug.DrawTiledLightning");
    CaptureBool("Renderer.BasePass.ClearAllTargets");
    CaptureBool("Renderer.Feature.PrePass");
    CaptureBool("Renderer.Feature.BasePass");

    // Skybox
    CaptureBool("Renderer.Feature.Skybox");
    CaptureBool("Renderer.Skybox.ClearBeforeSkybox");

    // SSAO
    CaptureBool("Renderer.Feature.SSAO");
    CaptureInt("Renderer.SSAO.KernelSize");
    CaptureFloat("Renderer.SSAO.Radius");
    CaptureFloat("Renderer.SSAO.Bias");

    // Ray tracing
    CaptureBool("Renderer.Feature.RayTracing");
    CaptureBool("Renderer.RayTracing.EnableLocalShaderBindings");
    CaptureBool("Renderer.RayTracing.InlineReflections");
    CaptureBool("Renderer.RayTracing.SER");
    CaptureBool("Renderer.RayTracing.Compaction");
    CaptureBool("Renderer.RayTracing.ASCache");

    // Ray traced reflections
    CaptureFloat("Renderer.Reflections.IndirectSpecularStrength");
    CaptureBool("Renderer.RayTracing.Reflections.Enable");
    CaptureBool("Renderer.RayTracing.Reflections.HalfRes");
    CaptureFloat("Renderer.RayTracing.Reflections.MaxRayDistance");
    CaptureFloat("Renderer.RayTracing.Reflections.MirrorRoughnessThreshold");
    CaptureFloat("Renderer.RayTracing.Reflections.RayBias");
    CaptureBool("Renderer.RayTracing.Reflections.Denoise");
    CaptureFloat("Renderer.RayTracing.Reflections.TemporalAlpha");
    CaptureFloat("Renderer.RayTracing.Reflections.MaxRadiance");
    CaptureFloat("Renderer.RayTracing.Reflections.HistoryClampGamma");
    CaptureFloat("Renderer.RayTracing.Reflections.MaxHistoryLength");
    CaptureInt("Renderer.RayTracing.Reflections.NeighborhoodRadius");
    CaptureFloat("Renderer.RayTracing.Reflections.CameraMotionMaxHistory");
    CaptureInt("Renderer.RayTracing.Reflections.AtrousIterations");
    CaptureFloat("Renderer.RayTracing.Reflections.AtrousPhiColor");
    CaptureInt("Renderer.RayTracing.Reflections.Sampler");

    // Anti-aliasing
    CaptureBool("Renderer.Feature.TemporalAntiAliasing");
    CaptureBool("Renderer.TemporalAntiAliasing.HardwareJitter");
    CaptureBool("Renderer.Feature.FXAA");
    CaptureBool("Renderer.Debug.FXAADebug");

    // Display / culling / debug
    CaptureBool("Renderer.Feature.VerticalSync");
    CaptureBool("Renderer.Feature.FrustumCulling");
    CaptureBool("Renderer.Debug.DrawAABBs");
    CaptureBool("Renderer.Debug.DrawPointLights");
    CaptureBool("Renderer.Debug.LightProbes");

    // Tonemapping
    CaptureInt("Renderer.Tonemapping.Function");
    CaptureFloat("Renderer.Tonemapping.EV100");
    CaptureFloat("Renderer.Tonemapping.ReinhardIntensity");

    // Shadows
    CaptureBool("Renderer.Feature.Shadows");
    CaptureBool("Renderer.Feature.SunShadows");
    CaptureBool("Renderer.Feature.ShadowMask");
    CaptureBool("Renderer.Feature.PointLightShadows");
    CaptureBool("Renderer.CSM.TightFrustum");

    // CSM
    CaptureInt("Renderer.CSM.CascadeSize");
    CaptureBool("Renderer.CSM.StableCascades");
    CaptureBool("Renderer.CSM.EnableSinglePassRendering");
    CaptureBool("Renderer.CSM.EnableGeometryShaderInstancing");
    CaptureBool("Renderer.CSM.EnableViewInstancing");
    CaptureBool("Renderer.CSM.EnableDepthClipping");
    CaptureBool("Renderer.CSM.BlendCascades");
    CaptureBool("Renderer.CSM.SelectCascadeFromProjection");
    CaptureInt("Renderer.CSM.FilterMode");
    CaptureInt("Renderer.CSM.FilterFunction");
    CaptureInt("Renderer.CSM.FilterSize");
    CaptureInt("Renderer.CSM.MaxFilterSize");
    CaptureInt("Renderer.CSM.NumPoissonDiscSamples");
    CaptureBool("Renderer.CSM.RotateSamples");

    // Point-light shadows
    CaptureInt("Renderer.Shadows.PointLightShadowMapSize");
    CaptureBool("Renderer.PointLights.EnableSinglePassRendering");
    CaptureBool("Renderer.PointLights.EnableGeometryShaderInstancing");
}

void FEditorRendererSettingsWidget::DrawWindow()
{
    CaptureDefaultsIfNeeded();

    const auto DrawLabelWithSeperator = [](const CHAR* InLabel)
    {
        static constexpr int32 LabelLength = 256;
        TStaticArray<CHAR, LabelLength> Label{};
        CString::Snprintf(Label.Data(), static_cast<int32>(Label.Size()), "%s", InLabel);

        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextBorderSize, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.1f, 0.5f));
        ImGui::SeparatorText(Label.Data());
        ImGui::PopStyleVar(2);
    };

    const auto DrawCollapsingHeader = [](const CHAR* Label, ImGuiTreeNodeFlags Flags, const bool bDrawBottomBorder, const bool bFullWidthBorder = true)
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

        const ImVec2 HeaderMin = ImGui::GetItemRectMin();
        const ImVec2 HeaderMax = ImGui::GetItemRectMax();

        {
            ImGuiWindow* Window = ImGui::GetCurrentWindow();
            if (Window && !Window->SkipItems)
            {
                const float IconSize = 16.0f;
                const float Alpha01  = Math::Clamp(Style.Alpha, 0.0f, 1.0f);
                const int32 Alpha255 = static_cast<int32>(Alpha01 * 255.0f);
                const ImU32 Tint     = IM_COL32(101, 101, 101, Alpha255);
                const ImU32 CoverCol = ImGui::ColorConvertFloat4ToU32(HeaderBg);

                const float CoverWidth = Style.FramePadding.x + IconSize + 6.0f;
                Window->DrawList->AddRectFilled(HeaderMin, ImVec2(HeaderMin.x + CoverWidth, HeaderMax.y), CoverCol);

                const float  IconY   = HeaderMin.y + ((HeaderMax.y - HeaderMin.y) - IconSize) * 0.5f;
                const ImVec2 IconMin = ImVec2(HeaderMin.x + Style.FramePadding.x, IconY);
                const ImVec2 IconMax = ImVec2(IconMin.x + IconSize, IconMin.y + IconSize);

                ImTextureID ArrowIcon = bResult ? EditorIcons::CollapseArrowDown : EditorIcons::CollapseArrowRight;
                if (ArrowIcon)
                {
                    Window->DrawList->AddImage(ArrowIcon, IconMin, IconMax, ImVec2(0, 0), ImVec2(1, 1), Tint);
                }
            }
        }

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(3);

        if (!bResult)
        {
            ImGuiWindow* Window = ImGui::GetCurrentWindow();
            if (Window && !Window->SkipItems)
            {
                const float Thickness = 2.0f;
                const float Y         = Math::Ceil(HeaderMax.y) - 0.5f;
                const float Alpha01   = Math::Clamp(Style.Alpha, 0.0f, 1.0f);
                const int32 Alpha255  = static_cast<int32>(Alpha01 * 255.0f);
                const ImU32 Color     = IM_COL32(26, 26, 26, Alpha255);

                if (bDrawBottomBorder)
                {
                    const float LineMinX = bFullWidthBorder ? Window->WorkRect.Min.x : HeaderMin.x;
                    const float LineMaxX = bFullWidthBorder ? Window->WorkRect.Max.x : HeaderMax.x;
                    Window->DrawList->AddLine(ImVec2(LineMinX, Y), ImVec2(LineMaxX, Y), Color, Thickness);
                }

                const ImVec2 Cursor = ImGui::GetCursorScreenPos();
                ImGui::SetCursorScreenPos(ImVec2(Cursor.x, HeaderMax.y + Thickness * 0.5f));
            }
        }

        return bResult;
    };

    DrawLabelWithSeperator("Renderer Settings");

    if (DrawCollapsingHeader("Deferred Rendering", ImGuiTreeNodeFlags_None, true))
    {
        DrawDeferredRenderingSettings();
    }

    if (DrawCollapsingHeader("Shadows", ImGuiTreeNodeFlags_None, true))
    {
        DrawShadowSettings();

        ImGui::Indent();

        if (DrawCollapsingHeader("Cascaded Shadow Maps", ImGuiTreeNodeFlags_None, true, false))
        {
            DrawCascadedShadowSettings();
        }

        if (DrawCollapsingHeader("Point-light Shadow Maps", ImGuiTreeNodeFlags_None, true))
        {
            DrawPointLightShadowSettings();
        }

        ImGui::Unindent();
    }

    if (DrawCollapsingHeader("Skybox", ImGuiTreeNodeFlags_None, true))
    {
        DrawSkyboxSettings();
    }

    if (DrawCollapsingHeader("SSAO", ImGuiTreeNodeFlags_None, true))
    {
        DrawSSAOSettings();
    }

    if (DrawCollapsingHeader("Ray Tracing", ImGuiTreeNodeFlags_None, true))
    {
        DrawRayTracingSettings();

        ImGui::Indent();

        if (DrawCollapsingHeader("Reflections", ImGuiTreeNodeFlags_None, true))
        {
            DrawRayTracingReflectionsSettings();
        }

        ImGui::Unindent();
    }

    if (DrawCollapsingHeader("Temporal Anti-aliasing (TAA)", ImGuiTreeNodeFlags_None, true))
    {
        DrawTAASettings();
    }

    if (DrawCollapsingHeader("FXAA", ImGuiTreeNodeFlags_None, true))
    {
        DrawFXAASettings();
    }

    if (DrawCollapsingHeader("Tonemapping", ImGuiTreeNodeFlags_None, true))
    {
        DrawTonemappingSettings();
    }

    if (DrawCollapsingHeader("Display", ImGuiTreeNodeFlags_None, true))
    {
        DrawDisplaySettings();
    }

    if (DrawCollapsingHeader("Culling", ImGuiTreeNodeFlags_None, true))
    {
        DrawCullingSettings();
    }

    if (DrawCollapsingHeader("Debug", ImGuiTreeNodeFlags_None, false))
    {
        DrawDebugSettings();
    }
}

void FEditorRendererSettingsWidget::DrawDeferredRenderingSettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsDeferred", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarDrawTiledLightning = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.DrawTiledLightning"))
    {
        bool bValue  = CVarDrawTiledLightning->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Debug.DrawTiledLightning", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Draw tile-debug", bValue, RevertPtr))
        {
            CVarDrawTiledLightning->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarClearAllTargets = FConsoleManager::Get().FindConsoleVariable("Renderer.BasePass.ClearAllTargets"))
    {
        bool bValue  = CVarClearAllTargets->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.BasePass.ClearAllTargets", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Clear all targets", bValue, RevertPtr))
        {
            CVarClearAllTargets->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarEnablePrePass = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.PrePass"))
    {
        bool bValue  = CVarEnablePrePass->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.PrePass", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable pre-pass", bValue, RevertPtr))
        {
            CVarEnablePrePass->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarEnableBasePass = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.BasePass"))
    {
        bool bValue  = CVarEnableBasePass->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.BasePass", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable base-pass", bValue, RevertPtr))
        {
            CVarEnableBasePass->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawShadowSettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsShadows", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.Shadows"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.Shadows", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable shadows", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawCascadedShadowSettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsCSM", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.SunShadows"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.SunShadows", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable sun shadows", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.ShadowMask"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.ShadowMask", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable shadow mask", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.TightFrustum"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.TightFrustum", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Tight frustum (depth reduce)", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.CascadeSize"))
    {
        static const CHAR* CascadeResItems[] = { "512", "1024", "2048", "4096" };
        static const int32 CascadeResValues[] = { 512, 1024, 2048, 4096 };
        static constexpr int32 CascadeResCount = 4;

        const int32 RawValue = CVar->GetInt();
        int32 ComboIndex = 1;
        for (int32 i = 0; i < CascadeResCount; ++i)
        {
            if (CascadeResValues[i] == RawValue)
            {
                ComboIndex = i;
                break;
            }
        }

        int32 ComboIndex0 = 0;
        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.CSM.CascadeSize", ComboIndex0);
        int32 RevertComboIndex = 1;
        if (RevertPtr)
        {
            for (int32 i = 0; i < CascadeResCount; ++i)
            {
                if (CascadeResValues[i] == *RevertPtr)
                {
                    RevertComboIndex = i;
                    break;
                }
            }
        }

        const int32* RevertComboPtr = RevertPtr ? &RevertComboIndex : nullptr;
        if (EditorWidgets::DrawComboProperty("Cascade resolution", ComboIndex, CascadeResItems, CascadeResCount, RevertComboPtr))
        {
            CVar->SetAsInt(CascadeResValues[ComboIndex], EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.StableCascades"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.StableCascades", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Stable cascades", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.EnableSinglePassRendering"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.EnableSinglePassRendering", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Single-pass rendering", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.EnableGeometryShaderInstancing"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.EnableGeometryShaderInstancing", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Geometry shader instancing", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.EnableViewInstancing"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.EnableViewInstancing", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("View instancing", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.EnableDepthClipping"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.EnableDepthClipping", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Depth clipping", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.BlendCascades"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.BlendCascades", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Blend cascades", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.SelectCascadeFromProjection"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.SelectCascadeFromProjection", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Select cascade from projection", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.FilterMode"))
    {
        static const CHAR* const FilterModeItems[] = { "PCF", "PCSS" };
        constexpr int32 FilterModeCount = static_cast<int32>(ARRAY_COUNT(FilterModeItems));

        int32 Value  = Math::Clamp<int32>(CVar->GetInt(), 0, FilterModeCount - 1);
        int32 Value0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.CSM.FilterMode", Value0);
        if (EditorWidgets::DrawComboProperty("Filter mode", Value, FilterModeItems, FilterModeCount, RevertPtr))
        {
            CVar->SetAsInt(Value, EConsoleVariableFlags::SetByCode);
        }
    }

    int32 CurrentFilterFunction = 0;
    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.FilterFunction"))
    {
        static const CHAR* const FilterFuncItems[] = { "Grid", "Poisson Disk", "Vogel Disk" };
        constexpr int32 FilterFuncCount = static_cast<int32>(ARRAY_COUNT(FilterFuncItems));

        int32 Value  = Math::Clamp<int32>(CVar->GetInt(), 0, FilterFuncCount - 1);
        CurrentFilterFunction = Value;
        int32 Value0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.CSM.FilterFunction", Value0);
        if (EditorWidgets::DrawComboProperty("Filter function", Value, FilterFuncItems, FilterFuncCount, RevertPtr))
        {
            CVar->SetAsInt(Value, EConsoleVariableFlags::SetByCode);
            CurrentFilterFunction = Value;

            if (Value == 0)
            {
                if (IConsoleVariable* FilterSizeCVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.FilterSize"))
                {
                    FilterSizeCVar->SetAsInt(9, EConsoleVariableFlags::SetByCode);
                }
            }
        }
    }

    const bool bIsGridFunction = (CurrentFilterFunction == 0);

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.FilterSize"))
    {
        int32 Value  = Math::Clamp<int32>(CVar->GetInt(), 1, 2048);
        int32 Value0 = 0;

        const CHAR* FilterSizeLabel = bIsGridFunction ? "Grid size" : "Filter radius";
        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.CSM.FilterSize", Value0);
        if (EditorWidgets::DrawIntProperty(FilterSizeLabel, Value, 1.0f, 1, 2048, "%d", true, RevertPtr))
        {
            CVar->SetAsInt(Value, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.MaxFilterSize"))
    {
        int32 Value  = Math::Clamp<int32>(CVar->GetInt(), 1, 2048);
        int32 Value0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.CSM.MaxFilterSize", Value0);
        if (EditorWidgets::DrawIntProperty("Max filter size", Value, 1.0f, 1, 2048, "%d", true, RevertPtr))
        {
            CVar->SetAsInt(Value, EConsoleVariableFlags::SetByCode);
        }
    }

    const bool bIsVogelFunction = (CurrentFilterFunction == 2);

    if (!bIsGridFunction)
    {
        if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.NumPoissonDiscSamples"))
        {
            if (bIsVogelFunction)
            {
                int32 Value  = Math::Clamp<int32>(CVar->GetInt(), 4, 128);
                int32 Value0 = 0;

                const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.CSM.NumPoissonDiscSamples", Value0);
                if (EditorWidgets::DrawIntProperty("Sample count", Value, 1.0f, 4, 128, "%d", true, RevertPtr))
                {
                    CVar->SetAsInt(Value, EConsoleVariableFlags::SetByCode);
                }
            }
            else
            {
                static const CHAR* SampleCountItems[] = { "16", "32", "64", "128" };
                static const int32 SampleCountValues[] = { 16, 32, 64, 128 };
                static constexpr int32 SampleCountCount = 4;

                const int32 RawValue = CVar->GetInt();
                int32 ComboIndex = 0;
                for (int32 i = 0; i < SampleCountCount; ++i)
                {
                    if (SampleCountValues[i] == RawValue)
                    {
                        ComboIndex = i;
                        break;
                    }
                }

                int32 ComboIndex0 = 0;
                const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.CSM.NumPoissonDiscSamples", ComboIndex0);
                int32 RevertComboIndex = 0;
                if (RevertPtr)
                {
                    for (int32 i = 0; i < SampleCountCount; ++i)
                    {
                        if (SampleCountValues[i] == *RevertPtr)
                        {
                            RevertComboIndex = i;
                            break;
                        }
                    }
                }

                const int32* RevertComboPtr = RevertPtr ? &RevertComboIndex : nullptr;
                if (EditorWidgets::DrawComboProperty("Sample count", ComboIndex, SampleCountItems, SampleCountCount, RevertComboPtr))
                {
                    CVar->SetAsInt(SampleCountValues[ComboIndex], EConsoleVariableFlags::SetByCode);
                }
            }
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.RotateSamples"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.RotateSamples", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Rotate samples", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawPointLightShadowSettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsPointLightShadows", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.PointLightShadows"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.PointLightShadows", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable point-light shadows", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.Shadows.PointLightShadowMapSize"))
    {
        static const CHAR* PointResItems[] = { "128", "256", "512", "1024" };
        static const int32 PointResValues[] = { 128, 256, 512, 1024 };
        static constexpr int32 PointResCount = 4;

        const int32 RawValue = CVar->GetInt();
        int32 ComboIndex = 1;
        for (int32 i = 0; i < PointResCount; ++i)
        {
            if (PointResValues[i] == RawValue)
            {
                ComboIndex = i;
                break;
            }
        }

        int32 ComboIndex0 = 0;
        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.Shadows.PointLightShadowMapSize", ComboIndex0);
        int32 RevertComboIndex = 1;
        if (RevertPtr)
        {
            for (int32 i = 0; i < PointResCount; ++i)
            {
                if (PointResValues[i] == *RevertPtr)
                {
                    RevertComboIndex = i;
                    break;
                }
            }
        }

        const int32* RevertComboPtr = RevertPtr ? &RevertComboIndex : nullptr;
        if (EditorWidgets::DrawComboProperty("Shadow map resolution", ComboIndex, PointResItems, PointResCount, RevertComboPtr))
        {
            CVar->SetAsInt(PointResValues[ComboIndex], EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.PointLights.EnableSinglePassRendering"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.PointLights.EnableSinglePassRendering", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Single-pass rendering", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("Renderer.PointLights.EnableGeometryShaderInstancing"))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.PointLights.EnableGeometryShaderInstancing", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Geometry shader instancing", bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawSkyboxSettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsSkybox", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarEnableSkybox = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.Skybox"))
    {
        bool bEnableSkybox  = CVarEnableSkybox->GetBool();
        bool bEnableSkybox0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.Skybox", bEnableSkybox0);
        if (EditorWidgets::DrawCheckboxProperty("Enable Skybox", bEnableSkybox, RevertPtr))
        {
            CVarEnableSkybox->SetAsBool(bEnableSkybox, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarEnableClearBeforeSkybox = FConsoleManager::Get().FindConsoleVariable("Renderer.Skybox.ClearBeforeSkybox"))
    {
        bool bEnableClearBeforeSkybox  = CVarEnableClearBeforeSkybox->GetBool();
        bool bEnableClearBeforeSkybox0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Skybox.ClearBeforeSkybox", bEnableClearBeforeSkybox0);
        if (EditorWidgets::DrawCheckboxProperty("Clear before skybox", bEnableClearBeforeSkybox, RevertPtr))
        {
            CVarEnableClearBeforeSkybox->SetAsBool(bEnableClearBeforeSkybox, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawSSAOSettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsSSAO", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarEnableSSAO = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.SSAO"))
    {
        bool bEnableSSAO  = CVarEnableSSAO->GetBool();
        bool bEnableSSAO0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.SSAO", bEnableSSAO0);
        if (EditorWidgets::DrawCheckboxProperty("Enable SSAO", bEnableSSAO, RevertPtr))
        {
            CVarEnableSSAO->SetAsBool(bEnableSSAO, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarKernelSize = FConsoleManager::Get().FindConsoleVariable("Renderer.SSAO.KernelSize"))
    {
        int32 KernelSize  = Math::Clamp<int32>(CVarKernelSize->GetInt(), 1, 128);
        int32 KernelSize0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.SSAO.KernelSize", KernelSize0);
        if (EditorWidgets::DrawIntProperty("Kernel size", KernelSize, 1.0f, 1, 128, "%d", true, RevertPtr))
        {
            CVarKernelSize->SetAsInt(KernelSize, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarRadius = FConsoleManager::Get().FindConsoleVariable("Renderer.SSAO.Radius"))
    {
        float Radius  = Math::Clamp<float>(CVarRadius->GetFloat(), 0.01f, 1.0f);
        float Radius0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.SSAO.Radius", Radius0);
        if (EditorWidgets::DrawFloatProperty("Radius", Radius, 0.01f, 0.01f, 1.0f, "%.2f", true, RevertPtr))
        {
            CVarRadius->SetAsFloat(Radius, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarBias = FConsoleManager::Get().FindConsoleVariable("Renderer.SSAO.Bias"))
    {
        float Bias  = Math::Clamp<float>(CVarBias->GetFloat(), 0.01f, 1.0f);
        float Bias0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.SSAO.Bias", Bias0);
        if (EditorWidgets::DrawFloatProperty("Bias", Bias, 0.01f, 0.01f, 1.0f, "%.2f", true, RevertPtr))
        {
            CVarBias->SetAsFloat(Bias, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawRayTracingSettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsRayTracing", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    EditorWidgets::DrawTextProperty("Hardware support", RHI::bSupportsRayTracing ? "Supported" : "Unsupported");

    if (IConsoleVariable* CVarEnableRayTracing = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.RayTracing"))
    {
        bool bEnableRayTracing  = CVarEnableRayTracing->GetBool();
        bool bEnableRayTracing0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.RayTracing", bEnableRayTracing0);
        if (EditorWidgets::DrawCheckboxProperty("Enable ray tracing", bEnableRayTracing, RevertPtr, RHI::bSupportsRayTracing))
        {
            CVarEnableRayTracing->SetAsBool(bEnableRayTracing, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarLocalShaderBindings = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.EnableLocalShaderBindings"))
    {
        bool bLocalShaderBindings  = CVarLocalShaderBindings->GetBool();
        bool bLocalShaderBindings0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.RayTracing.EnableLocalShaderBindings", bLocalShaderBindings0);
        if (EditorWidgets::DrawCheckboxProperty("Use local shader bindings", bLocalShaderBindings, RevertPtr, RHI::bSupportsShaderBindingTableDescriptors))
        {
            CVarLocalShaderBindings->SetAsBool(bLocalShaderBindings, EConsoleVariableFlags::SetByCode);
        }
    }

    bool bInlineReflections = false;
    if (IConsoleVariable* CVarInlineReflections = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.InlineReflections"))
    {
        bInlineReflections = CVarInlineReflections->GetBool();

        bool bInlineReflections0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.RayTracing.InlineReflections", bInlineReflections0);
        if (EditorWidgets::DrawCheckboxProperty("Inline reflections (RayQuery)", bInlineReflections, RevertPtr, RHI::bSupportsInlineRayTracing))
        {
            CVarInlineReflections->SetAsBool(bInlineReflections, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarSER = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.SER"))
    {
        bool bSER  = CVarSER->GetBool();
        bool bSER0 = false;

        // The inline path takes priority over SER, so SER has no effect while inline reflections are enabled
        const bool  bSEREnabled = RHI::bSupportsShaderExecutionReordering && !bInlineReflections;
        const bool* RevertPtr   = TryGetDefaultPtr(BoolDefaults, "Renderer.RayTracing.SER", bSER0);
        if (EditorWidgets::DrawCheckboxProperty("Shader Execution Reordering", bSER, RevertPtr, bSEREnabled))
        {
            CVarSER->SetAsBool(bSER, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarCompaction = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Compaction"))
    {
        bool bCompaction  = CVarCompaction->GetBool();
        bool bCompaction0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.RayTracing.Compaction", bCompaction0);
        if (EditorWidgets::DrawCheckboxProperty("BLAS compaction", bCompaction, RevertPtr, RHI::bSupportsRayTracing))
        {
            CVarCompaction->SetAsBool(bCompaction, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarASCache = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.ASCache"))
    {
        bool bASCache  = CVarASCache->GetBool();
        bool bASCache0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.RayTracing.ASCache", bASCache0);
        if (EditorWidgets::DrawCheckboxProperty("Acceleration-structure disk cache", bASCache, RevertPtr, RHI::bSupportsRayTracing))
        {
            CVarASCache->SetAsBool(bASCache, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawRayTracingReflectionsSettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsRayTracingReflections", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    bool bRayTracingActive = false;
    if (IConsoleVariable* CVarEnableRayTracing = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.RayTracing"))
    {
        bRayTracingActive = CVarEnableRayTracing->GetBool() && RHI::bSupportsRayTracing;
    }

    bool bReflectionsEnabled = false;
    if (IConsoleVariable* CVarEnableReflections = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.Enable"))
    {
        bReflectionsEnabled = CVarEnableReflections->GetBool();

        bool bEnableReflections0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.RayTracing.Reflections.Enable", bEnableReflections0);
        if (EditorWidgets::DrawCheckboxProperty("Enable", bReflectionsEnabled, RevertPtr, bRayTracingActive))
        {
            CVarEnableReflections->SetAsBool(bReflectionsEnabled, EConsoleVariableFlags::SetByCode);
        }
    }

    const bool bReflectionsActive = bRayTracingActive && bReflectionsEnabled;

    if (IConsoleVariable* CVarIndirectSpecularStrength = FConsoleManager::Get().FindConsoleVariable("Renderer.Reflections.IndirectSpecularStrength"))
    {
        float IndirectSpecularStrength  = Math::Clamp<float>(CVarIndirectSpecularStrength->GetFloat(), 0.0f, 4.0f);
        float IndirectSpecularStrength0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.Reflections.IndirectSpecularStrength", IndirectSpecularStrength0);
        if (EditorWidgets::DrawFloatProperty("Indirect specular strength", IndirectSpecularStrength, 0.01f, 0.0f, 4.0f, "%.2f", true, RevertPtr, bReflectionsActive))
        {
            CVarIndirectSpecularStrength->SetAsFloat(IndirectSpecularStrength, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarHalfRes = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.HalfRes"))
    {
        bool bHalfRes  = CVarHalfRes->GetBool();
        bool bHalfRes0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.RayTracing.Reflections.HalfRes", bHalfRes0);
        if (EditorWidgets::DrawCheckboxProperty("Half resolution", bHalfRes, RevertPtr, bReflectionsActive))
        {
            CVarHalfRes->SetAsBool(bHalfRes, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarMaxRayDistance = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.MaxRayDistance"))
    {
        float MaxRayDistance  = Math::Clamp<float>(CVarMaxRayDistance->GetFloat(), 1.0f, 100000.0f);
        float MaxRayDistance0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.RayTracing.Reflections.MaxRayDistance", MaxRayDistance0);
        if (EditorWidgets::DrawFloatProperty("Max ray distance", MaxRayDistance, 10.0f, 1.0f, 100000.0f, "%.0f", true, RevertPtr, bReflectionsActive))
        {
            CVarMaxRayDistance->SetAsFloat(MaxRayDistance, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarMirrorRoughnessThreshold = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.MirrorRoughnessThreshold"))
    {
        float MirrorRoughnessThreshold  = Math::Clamp<float>(CVarMirrorRoughnessThreshold->GetFloat(), 0.0f, 1.0f);
        float MirrorRoughnessThreshold0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.RayTracing.Reflections.MirrorRoughnessThreshold", MirrorRoughnessThreshold0);
        if (EditorWidgets::DrawFloatProperty("Mirror roughness threshold", MirrorRoughnessThreshold, 0.001f, 0.0f, 1.0f, "%.3f", true, RevertPtr, bReflectionsActive))
        {
            CVarMirrorRoughnessThreshold->SetAsFloat(MirrorRoughnessThreshold, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarRayBias = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.RayBias"))
    {
        float RayBias  = Math::Clamp<float>(CVarRayBias->GetFloat(), 0.0f, 1.0f);
        float RayBias0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.RayTracing.Reflections.RayBias", RayBias0);
        if (EditorWidgets::DrawFloatProperty("Ray bias", RayBias, 0.001f, 0.0f, 1.0f, "%.3f", true, RevertPtr, bReflectionsActive))
        {
            CVarRayBias->SetAsFloat(RayBias, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarSampler = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.Sampler"))
    {
        // Order matches REFLECTION_SAMPLER_* in Shaders/Reflections/ReflectionSampling.hlsli.
        static const CHAR* const Items[] =
        {
            "White noise", "Halton", "Blue noise"
        };

        constexpr int32 ItemCount = static_cast<int32>(ARRAY_COUNT(Items));

        int32 Sampler  = Math::Clamp<int32>(CVarSampler->GetInt(), 0, ItemCount - 1);
        int32 Sampler0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.RayTracing.Reflections.Sampler", Sampler0);
        if (EditorWidgets::DrawComboProperty("GGX sampler", Sampler, Items, ItemCount, RevertPtr, bReflectionsActive))
        {
            CVarSampler->SetAsInt(Sampler, EConsoleVariableFlags::SetByCode);
        }
    }

    bool bDenoise = false;
    if (IConsoleVariable* CVarDenoise = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.Denoise"))
    {
        bDenoise = CVarDenoise->GetBool();

        bool bDenoise0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.RayTracing.Reflections.Denoise", bDenoise0);
        if (EditorWidgets::DrawCheckboxProperty("Enable denoiser", bDenoise, RevertPtr, bReflectionsActive))
        {
            CVarDenoise->SetAsBool(bDenoise, EConsoleVariableFlags::SetByCode);
        }
    }

    const bool bDenoiserActive = bReflectionsActive && bDenoise;

    if (IConsoleVariable* CVarTemporalAlpha = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.TemporalAlpha"))
    {
        float TemporalAlpha  = Math::Clamp<float>(CVarTemporalAlpha->GetFloat(), 0.01f, 1.0f);
        float TemporalAlpha0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.RayTracing.Reflections.TemporalAlpha", TemporalAlpha0);
        if (EditorWidgets::DrawFloatProperty("Temporal alpha", TemporalAlpha, 0.01f, 0.01f, 1.0f, "%.2f", true, RevertPtr, bDenoiserActive))
        {
            CVarTemporalAlpha->SetAsFloat(TemporalAlpha, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarMaxRadiance = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.MaxRadiance"))
    {
        float MaxRadiance  = Math::Clamp<float>(CVarMaxRadiance->GetFloat(), 0.0f, 100.0f);
        float MaxRadiance0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.RayTracing.Reflections.MaxRadiance", MaxRadiance0);
        if (EditorWidgets::DrawFloatProperty("Max radiance (firefly clamp)", MaxRadiance, 0.1f, 0.0f, 100.0f, "%.2f", true, RevertPtr, bDenoiserActive))
        {
            CVarMaxRadiance->SetAsFloat(MaxRadiance, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarHistoryClampGamma = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.HistoryClampGamma"))
    {
        float HistoryClampGamma  = Math::Clamp<float>(CVarHistoryClampGamma->GetFloat(), 0.0f, 10.0f);
        float HistoryClampGamma0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.RayTracing.Reflections.HistoryClampGamma", HistoryClampGamma0);
        if (EditorWidgets::DrawFloatProperty("History clamp gamma", HistoryClampGamma, 0.01f, 0.0f, 10.0f, "%.2f", true, RevertPtr, bDenoiserActive))
        {
            CVarHistoryClampGamma->SetAsFloat(HistoryClampGamma, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarMaxHistoryLength = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.MaxHistoryLength"))
    {
        float MaxHistoryLength  = Math::Clamp<float>(CVarMaxHistoryLength->GetFloat(), 1.0f, 128.0f);
        float MaxHistoryLength0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.RayTracing.Reflections.MaxHistoryLength", MaxHistoryLength0);
        if (EditorWidgets::DrawFloatProperty("Max history length", MaxHistoryLength, 1.0f, 1.0f, 128.0f, "%.0f", true, RevertPtr, bDenoiserActive))
        {
            CVarMaxHistoryLength->SetAsFloat(MaxHistoryLength, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarNeighborhoodRadius = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.NeighborhoodRadius"))
    {
        int32 NeighborhoodRadius  = Math::Clamp<int32>(CVarNeighborhoodRadius->GetInt(), 0, 8);
        int32 NeighborhoodRadius0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.RayTracing.Reflections.NeighborhoodRadius", NeighborhoodRadius0);
        if (EditorWidgets::DrawIntProperty("Neighborhood radius", NeighborhoodRadius, 1.0f, 0, 8, "%d", true, RevertPtr, bDenoiserActive))
        {
            CVarNeighborhoodRadius->SetAsInt(NeighborhoodRadius, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarCameraMotionMaxHistory = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.CameraMotionMaxHistory"))
    {
        float CameraMotionMaxHistory  = Math::Clamp<float>(CVarCameraMotionMaxHistory->GetFloat(), 0.0f, 64.0f);
        float CameraMotionMaxHistory0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.RayTracing.Reflections.CameraMotionMaxHistory", CameraMotionMaxHistory0);
        if (EditorWidgets::DrawFloatProperty("Camera-motion max history", CameraMotionMaxHistory, 0.5f, 0.0f, 64.0f, "%.1f", true, RevertPtr, bDenoiserActive))
        {
            CVarCameraMotionMaxHistory->SetAsFloat(CameraMotionMaxHistory, EConsoleVariableFlags::SetByCode);
        }
    }

    int32 AtrousIterations = 0;
    if (IConsoleVariable* CVarAtrousIterations = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.AtrousIterations"))
    {
        AtrousIterations = Math::Clamp<int32>(CVarAtrousIterations->GetInt(), 0, 8);

        int32 AtrousIterations0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.RayTracing.Reflections.AtrousIterations", AtrousIterations0);
        if (EditorWidgets::DrawIntProperty("A-trous iterations", AtrousIterations, 1.0f, 0, 8, "%d", true, RevertPtr, bDenoiserActive))
        {
            CVarAtrousIterations->SetAsInt(AtrousIterations, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarAtrousPhiColor = FConsoleManager::Get().FindConsoleVariable("Renderer.RayTracing.Reflections.AtrousPhiColor"))
    {
        float AtrousPhiColor  = Math::Clamp<float>(CVarAtrousPhiColor->GetFloat(), 0.1f, 32.0f);
        float AtrousPhiColor0 = 0.0f;

        const bool   bSpatialActive = bDenoiserActive && (AtrousIterations > 0);
        const float* RevertPtr      = TryGetDefaultPtr(FloatDefaults, "Renderer.RayTracing.Reflections.AtrousPhiColor", AtrousPhiColor0);
        if (EditorWidgets::DrawFloatProperty("A-trous color phi", AtrousPhiColor, 0.1f, 0.1f, 32.0f, "%.2f", true, RevertPtr, bSpatialActive))
        {
            CVarAtrousPhiColor->SetAsFloat(AtrousPhiColor, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawTAASettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsTAA", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    bool bIsTAAEnabled = false;
    if (IConsoleVariable* CVarEnableTemporalAntiAliasing = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.TemporalAntiAliasing"))
    {
        bool bEnableTAA  = CVarEnableTemporalAntiAliasing->GetBool();
        bool bEnableTAA0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.TemporalAntiAliasing", bEnableTAA0);
        if (EditorWidgets::DrawCheckboxProperty("Enabled", bEnableTAA, RevertPtr))
        {
            CVarEnableTemporalAntiAliasing->SetAsBool(bEnableTAA, EConsoleVariableFlags::SetByCode);
        }

        bIsTAAEnabled = bEnableTAA;
    }

    if (IConsoleVariable* CVarHardwareJitter = FConsoleManager::Get().FindConsoleVariable("Renderer.TemporalAntiAliasing.HardwareJitter"))
    {
        const bool bSupported = RHI::bSupportsProgrammableSamplePositions && IsSampleCountSupported(RHI::SupportedSamplePositionSampleCounts, RHI_SAMPLE_COUNT_1);

        bool bHardwareJitter  = CVarHardwareJitter->GetBool();
        bool bHardwareJitter0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.TemporalAntiAliasing.HardwareJitter", bHardwareJitter0);
        if (EditorWidgets::DrawCheckboxProperty("Hardware jitter", bHardwareJitter, RevertPtr, bIsTAAEnabled && bSupported))
        {
            CVarHardwareJitter->SetAsBool(bHardwareJitter, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawFXAASettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsFXAA", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarEnableFXAA = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.FXAA"))
    {
        bool bEnableFXAA  = CVarEnableFXAA->GetBool();
        bool bEnableFXAA0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.FXAA", bEnableFXAA0);
        if (EditorWidgets::DrawCheckboxProperty("Enable FXAA", bEnableFXAA, RevertPtr))
        {
            CVarEnableFXAA->SetAsBool(bEnableFXAA, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarEnableFXAADebug = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.FXAADebug"))
    {
        bool bEnableFXAADebug  = CVarEnableFXAADebug->GetBool();
        bool bEnableFXAADebug0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Debug.FXAADebug", bEnableFXAADebug0);
        if (EditorWidgets::DrawCheckboxProperty("Enable FXAA debug", bEnableFXAADebug, RevertPtr))
        {
            CVarEnableFXAADebug->SetAsBool(bEnableFXAADebug, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawDisplaySettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsDisplay", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarEnableVSync = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.VerticalSync"))
    {
        bool bEnableVSync  = CVarEnableVSync->GetBool();
        bool bEnableVSync0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.VerticalSync", bEnableVSync0);
        if (EditorWidgets::DrawCheckboxProperty("Enable VSync", bEnableVSync, RevertPtr))
        {
            CVarEnableVSync->SetAsBool(bEnableVSync, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawCullingSettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsCulling", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarEnableFrustumCulling = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.FrustumCulling"))
    {
        bool bEnableFrustumCulling  = CVarEnableFrustumCulling->GetBool();
        bool bEnableFrustumCulling0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.FrustumCulling", bEnableFrustumCulling0);
        if (EditorWidgets::DrawCheckboxProperty("Enable frustum-culling", bEnableFrustumCulling, RevertPtr))
        {
            CVarEnableFrustumCulling->SetAsBool(bEnableFrustumCulling, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawDebugSettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsDebug", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarEnableDebugDrawAABBs = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.DrawAABBs"))
    {
        bool bEnableDebugDrawAABBs  = CVarEnableDebugDrawAABBs->GetBool();
        bool bEnableDebugDrawAABBs0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Debug.DrawAABBs", bEnableDebugDrawAABBs0);
        if (EditorWidgets::DrawCheckboxProperty("Enable debug-draw AABBs", bEnableDebugDrawAABBs, RevertPtr))
        {
            CVarEnableDebugDrawAABBs->SetAsBool(bEnableDebugDrawAABBs, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarEnableDebugDrawPointLights = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.DrawPointLights"))
    {
        bool bEnableDebugDrawPointLights  = CVarEnableDebugDrawPointLights->GetBool();
        bool bEnableDebugDrawPointLights0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Debug.DrawPointLights", bEnableDebugDrawPointLights0);
        if (EditorWidgets::DrawCheckboxProperty("Enable debug-draw point-lights", bEnableDebugDrawPointLights, RevertPtr))
        {
            CVarEnableDebugDrawPointLights->SetAsBool(bEnableDebugDrawPointLights, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarEnableDebugDrawLightProbes = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.LightProbes"))
    {
        bool bEnableDebugDrawLightProbes  = CVarEnableDebugDrawLightProbes->GetBool();
        bool bEnableDebugDrawLightProbes0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Debug.LightProbes", bEnableDebugDrawLightProbes0);
        if (EditorWidgets::DrawCheckboxProperty("Enable debug-draw light-probes", bEnableDebugDrawLightProbes, RevertPtr))
        {
            CVarEnableDebugDrawLightProbes->SetAsBool(bEnableDebugDrawLightProbes, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawTonemappingSettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsTonemapping", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    bool  bHasTonemappingFunction = false;
    int32 TonemappingFunction     = 0;

    if (IConsoleVariable* CVarTonemappingFunction = FConsoleManager::Get().FindConsoleVariable("Renderer.Tonemapping.Function"))
    {
        static const CHAR* const Items[] =
        {
            "Default", "ACES", "Reinhard", "Uncharted 2"
        };

        constexpr int32 ItemCount = static_cast<int32>(ARRAY_COUNT(Items));

        TonemappingFunction     = Math::Clamp<int32>(CVarTonemappingFunction->GetInt(), 0, 3);
        bHasTonemappingFunction = true;

        int32 TonemappingFunction0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.Tonemapping.Function", TonemappingFunction0);
        if (EditorWidgets::DrawComboProperty("Tonemapping function", TonemappingFunction, Items, ItemCount, RevertPtr))
        {
            CVarTonemappingFunction->SetAsInt(TonemappingFunction, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarTonemappingEV100 = FConsoleManager::Get().FindConsoleVariable("Renderer.Tonemapping.EV100"))
    {
        float ExposureEV100  = Math::Clamp<float>(CVarTonemappingEV100->GetFloat(), -10.0f, 20.0f);
        float ExposureEV1000 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.Tonemapping.EV100", ExposureEV1000);
        if (EditorWidgets::DrawFloatProperty("Exposure (EV100)", ExposureEV100, 0.1f, -10.0f, 20.0f, "%.2f", true, RevertPtr))
        {
            CVarTonemappingEV100->SetAsFloat(ExposureEV100, EConsoleVariableFlags::SetByCode);
        }
    }

    const bool bIsReinhard = bHasTonemappingFunction && (TonemappingFunction == 2);
    if (bIsReinhard)
    {
        if (IConsoleVariable* CVarTonemappingReinhardIntensity = FConsoleManager::Get().FindConsoleVariable("Renderer.Tonemapping.ReinhardIntensity"))
        {
            float ReinhardIntensity  = Math::Clamp<float>(CVarTonemappingReinhardIntensity->GetFloat(), 0.1f, 10.0f);
            float ReinhardIntensity0 = 0.0f;

            const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.Tonemapping.ReinhardIntensity", ReinhardIntensity0);
            if (EditorWidgets::DrawFloatProperty("Reinhard intensity", ReinhardIntensity, 0.01f, 0.1f, 10.0f, "%.2f", true, RevertPtr))
            {
                CVarTonemappingReinhardIntensity->SetAsFloat(ReinhardIntensity, EConsoleVariableFlags::SetByCode);
            }
        }
    }

    EditorWidgets::EndPropertyTable();
}
