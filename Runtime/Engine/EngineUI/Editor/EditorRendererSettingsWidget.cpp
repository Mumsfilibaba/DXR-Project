#include "Engine/EngineUI/Editor/EditorRendererSettingsWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Core/CoreDefines.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Templates/CString.h"
#include "RHI/RHI.h"
#include "ImGuiPlugin/ImGuiCore.h"

static constexpr float RendererSettingsLabelColumnWidth  = 310.0f;
static constexpr float RendererSettingsRevertColumnWidth = 28.0f;

template<typename ValueType>
static const ValueType* TryGetDefaultPtr(const TMap<FString, ValueType>& Defaults, const CHAR_T* CVarName, ValueType& OutValue)
{
    if (const ValueType* Found = Defaults.Find(FString(CVarName)))
    {
        OutValue = *Found;
        return &OutValue;
    }

    return nullptr;
}

FEditorRendererSettingsWidget::FEditorRendererSettingsWidget()
    : ImGuiDelegateHandle()
    , BoolDefaults()
    , IntDefaults()
    , FloatDefaults()
    , CVarCache()
    , bVisible(true)
    , bDefaultsCaptured(false)
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

    TRACE_SCOPE(TEXT("Renderer Settings"));

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, EditorStyleVars::PropertiesItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, EditorStyleVars::PropertiesWindowPadding);

    if (EditorWidgets::BeginEditorWindow(TEXT("Renderer Settings"), &bVisible))
    {
        DrawWindow();
    }

    EditorWidgets::EndEditorWindow();

    ImGui::PopStyleVar(2);
}

IConsoleVariable* FEditorRendererSettingsWidget::GetCachedConsoleVariable(const CHAR_T* InName)
{
    if (IConsoleVariable** Cached = CVarCache.Find(InName))
    {
        return *Cached;
    }

    IConsoleVariable* Variable = FConsoleManager::Get().FindConsoleVariable(InName);
    if (Variable)
    {
        CVarCache.Add(InName, Variable);
    }

    return Variable;
}

void FEditorRendererSettingsWidget::CaptureDefaultsIfNeeded()
{
    if (bDefaultsCaptured)
    {
        return;
    }

    bDefaultsCaptured = true;

    FConsoleManager& Console = FConsoleManager::Get();

    const auto CaptureBool = [&](const CHAR_T* Name)
    {
        if (IConsoleVariable* CVar = Console.FindConsoleVariable(Name))
        {
            BoolDefaults.Add(FString(Name), CVar->GetBool());
        }
    };

    const auto CaptureInt = [&](const CHAR_T* Name)
    {
        if (IConsoleVariable* CVar = Console.FindConsoleVariable(Name))
        {
            IntDefaults.Add(FString(Name), CVar->GetInt());
        }
    };

    const auto CaptureFloat = [&](const CHAR_T* Name)
    {
        if (IConsoleVariable* CVar = Console.FindConsoleVariable(Name))
        {
            FloatDefaults.Add(FString(Name), CVar->GetFloat());
        }
    };

    // Deferred rendering
    CaptureBool(TEXT("Renderer.Debug.DrawTiledLightning"));
    CaptureBool(TEXT("Renderer.BasePass.ClearAllTargets"));
    CaptureBool(TEXT("Renderer.Feature.PrePass"));
    CaptureBool(TEXT("Renderer.Feature.BasePass"));

    // Skybox
    CaptureBool(TEXT("Renderer.Feature.Skybox"));
    CaptureBool(TEXT("Renderer.Skybox.ClearBeforeSkybox"));

    // SSAO
    CaptureBool(TEXT("Renderer.Feature.SSAO"));
    CaptureInt(TEXT("Renderer.SSAO.KernelSize"));
    CaptureFloat(TEXT("Renderer.SSAO.Radius"));
    CaptureFloat(TEXT("Renderer.SSAO.Bias"));

    // Ray tracing
    CaptureBool(TEXT("Renderer.Feature.RayTracing"));
    CaptureBool(TEXT("Renderer.RayTracing.EnableLocalShaderBindings"));
    CaptureBool(TEXT("Renderer.RayTracing.InlineReflections"));
    CaptureBool(TEXT("Renderer.RayTracing.SER"));
    CaptureBool(TEXT("Renderer.RayTracing.Compaction"));
    CaptureBool(TEXT("Renderer.RayTracing.ASCache"));

    // Ray traced reflections
    CaptureFloat(TEXT("Renderer.Reflections.IndirectSpecularStrength"));
    CaptureBool(TEXT("Renderer.RayTracing.Reflections.Enable"));
    CaptureBool(TEXT("Renderer.RayTracing.Reflections.HalfRes"));
    CaptureFloat(TEXT("Renderer.RayTracing.Reflections.MaxRayDistance"));
    CaptureFloat(TEXT("Renderer.RayTracing.Reflections.MirrorRoughnessThreshold"));
    CaptureFloat(TEXT("Renderer.RayTracing.Reflections.RayBias"));
    CaptureBool(TEXT("Renderer.RayTracing.Reflections.Denoise"));
    CaptureFloat(TEXT("Renderer.RayTracing.Reflections.TemporalAlpha"));
    CaptureFloat(TEXT("Renderer.RayTracing.Reflections.MaxRadiance"));
    CaptureFloat(TEXT("Renderer.RayTracing.Reflections.HistoryClampGamma"));
    CaptureFloat(TEXT("Renderer.RayTracing.Reflections.MaxHistoryLength"));
    CaptureInt(TEXT("Renderer.RayTracing.Reflections.NeighborhoodRadius"));
    CaptureFloat(TEXT("Renderer.RayTracing.Reflections.CameraMotionMaxHistory"));
    CaptureInt(TEXT("Renderer.RayTracing.Reflections.AtrousIterations"));
    CaptureFloat(TEXT("Renderer.RayTracing.Reflections.AtrousPhiColor"));
    CaptureInt(TEXT("Renderer.RayTracing.Reflections.Sampler"));

    // Anti-aliasing
    CaptureBool(TEXT("Renderer.Feature.TemporalAntiAliasing"));
    CaptureBool(TEXT("Renderer.TemporalAntiAliasing.HardwareJitter"));
    CaptureBool(TEXT("Renderer.Feature.FXAA"));
    CaptureBool(TEXT("Renderer.Debug.FXAADebug"));

    // Display / culling / debug
    CaptureBool(TEXT("Renderer.Feature.VerticalSync"));
    CaptureBool(TEXT("Renderer.Feature.FrustumCulling"));
    CaptureBool(TEXT("Renderer.Debug.DrawAABBs"));
    CaptureBool(TEXT("Renderer.Debug.DrawPointLights"));
    CaptureBool(TEXT("Renderer.Debug.LightProbes"));

    // Tonemapping
    CaptureInt(TEXT("Renderer.Tonemapping.Function"));
    CaptureFloat(TEXT("Renderer.Tonemapping.EV100"));
    CaptureFloat(TEXT("Renderer.Tonemapping.ReinhardIntensity"));

    // Shadows
    CaptureBool(TEXT("Renderer.Feature.Shadows"));
    CaptureBool(TEXT("Renderer.Feature.SunShadows"));
    CaptureBool(TEXT("Renderer.Feature.ShadowMask"));
    CaptureBool(TEXT("Renderer.Feature.PointLightShadows"));
    CaptureBool(TEXT("Renderer.CSM.TightFrustum"));

    // CSM
    CaptureInt(TEXT("Renderer.CSM.CascadeSize"));
    CaptureBool(TEXT("Renderer.CSM.StableCascades"));
    CaptureBool(TEXT("Renderer.CSM.EnableSinglePassRendering"));
    CaptureBool(TEXT("Renderer.CSM.EnableGeometryShaderInstancing"));
    CaptureBool(TEXT("Renderer.CSM.EnableViewInstancing"));
    CaptureBool(TEXT("Renderer.CSM.EnableDepthClipping"));
    CaptureBool(TEXT("Renderer.CSM.BlendCascades"));
    CaptureBool(TEXT("Renderer.CSM.SelectCascadeFromProjection"));
    CaptureInt(TEXT("Renderer.CSM.FilterMode"));
    CaptureInt(TEXT("Renderer.CSM.FilterFunction"));
    CaptureInt(TEXT("Renderer.CSM.FilterSize"));
    CaptureInt(TEXT("Renderer.CSM.MaxFilterSize"));
    CaptureInt(TEXT("Renderer.CSM.NumPoissonDiscSamples"));
    CaptureBool(TEXT("Renderer.CSM.RotateSamples"));

    // Point-light shadows
    CaptureInt(TEXT("Renderer.Shadows.PointLightShadowMapSize"));
    CaptureBool(TEXT("Renderer.PointLights.EnableSinglePassRendering"));
    CaptureBool(TEXT("Renderer.PointLights.EnableGeometryShaderInstancing"));
}

void FEditorRendererSettingsWidget::DrawWindow()
{
    CaptureDefaultsIfNeeded();

    const auto DrawLabelWithSeperator = [](const CHAR_T* InLabel)
    {
        static constexpr int32 LabelLength = 256;
        TStaticArray<CHAR_T, LabelLength> Label{};
        CString::Snprintf(Label.Data(), static_cast<int32>(Label.Size()), TEXT("%s"), InLabel);

        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextBorderSize, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.1f, 0.5f));
        ImGui::SeparatorText(StringCast<ANSICHAR>(Label.Data()).Get());
        ImGui::PopStyleVar(2);
    };

    const auto DrawCollapsingHeader = [](const CHAR_T* Label, ImGuiTreeNodeFlags Flags, const bool bDrawBottomBorder, const bool bFullWidthBorder = true)
    {
        ImGuiStyle& Style = ImGui::GetStyle();

        const ImVec4 HeaderBg = ImVec4(47.0f / 255.0f, 47.0f / 255.0f, 47.0f / 255.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Header, HeaderBg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HeaderBg);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, HeaderBg);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, EditorStyleVars::PropertiesCollapsingFrameRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, EditorStyleVars::PropertiesCollapsingHeaderItemSpacing);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, EditorStyleVars::PropertiesCollapsingFramePadding);

        const bool bResult = ImGui::CollapsingHeader(StringCast<ANSICHAR>(Label).Get(), Flags);

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

                const FEditorIcon& ArrowIcon = bResult ? EditorIcons::CollapseArrowDown : EditorIcons::CollapseArrowRight;
                EditorWidgets::DrawIcon(Window->DrawList, ArrowIcon, IconMin, IconMax, Tint);
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

    DrawLabelWithSeperator(TEXT("Renderer Settings"));

    if (DrawCollapsingHeader(TEXT("Deferred Rendering"), ImGuiTreeNodeFlags_None, true))
    {
        DrawDeferredRenderingSettings();
    }

    if (DrawCollapsingHeader(TEXT("Shadows"), ImGuiTreeNodeFlags_None, true))
    {
        DrawShadowSettings();

        ImGui::Indent();

        if (DrawCollapsingHeader(TEXT("Cascaded Shadow Maps"), ImGuiTreeNodeFlags_None, true, false))
        {
            DrawCascadedShadowSettings();
        }

        if (DrawCollapsingHeader(TEXT("Point-light Shadow Maps"), ImGuiTreeNodeFlags_None, true))
        {
            DrawPointLightShadowSettings();
        }

        ImGui::Unindent();
    }

    if (DrawCollapsingHeader(TEXT("Skybox"), ImGuiTreeNodeFlags_None, true))
    {
        DrawSkyboxSettings();
    }

    if (DrawCollapsingHeader(TEXT("SSAO"), ImGuiTreeNodeFlags_None, true))
    {
        DrawSSAOSettings();
    }

    if (DrawCollapsingHeader(TEXT("Ray Tracing"), ImGuiTreeNodeFlags_None, true))
    {
        DrawRayTracingSettings();

        ImGui::Indent();

        if (DrawCollapsingHeader(TEXT("Reflections"), ImGuiTreeNodeFlags_None, true))
        {
            DrawRayTracingReflectionsSettings();
        }

        ImGui::Unindent();
    }

    if (DrawCollapsingHeader(TEXT("Temporal Anti-aliasing (TAA)"), ImGuiTreeNodeFlags_None, true))
    {
        DrawTAASettings();
    }

    if (DrawCollapsingHeader(TEXT("FXAA"), ImGuiTreeNodeFlags_None, true))
    {
        DrawFXAASettings();
    }

    if (DrawCollapsingHeader(TEXT("Tonemapping"), ImGuiTreeNodeFlags_None, true))
    {
        DrawTonemappingSettings();
    }

    if (DrawCollapsingHeader(TEXT("Display"), ImGuiTreeNodeFlags_None, true))
    {
        DrawDisplaySettings();
    }

    if (DrawCollapsingHeader(TEXT("Culling"), ImGuiTreeNodeFlags_None, true))
    {
        DrawCullingSettings();
    }

    if (DrawCollapsingHeader(TEXT("Debug"), ImGuiTreeNodeFlags_None, false))
    {
        DrawDebugSettings();
    }
}

void FEditorRendererSettingsWidget::DrawDeferredRenderingSettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsDeferred"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarDrawTiledLightning = GetCachedConsoleVariable(TEXT("Renderer.Debug.DrawTiledLightning")))
    {
        bool bValue  = CVarDrawTiledLightning->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Debug.DrawTiledLightning"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Draw tile-debug"), bValue, RevertPtr))
        {
            CVarDrawTiledLightning->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarClearAllTargets = GetCachedConsoleVariable(TEXT("Renderer.BasePass.ClearAllTargets")))
    {
        bool bValue  = CVarClearAllTargets->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.BasePass.ClearAllTargets"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Clear all targets"), bValue, RevertPtr))
        {
            CVarClearAllTargets->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarEnablePrePass = GetCachedConsoleVariable(TEXT("Renderer.Feature.PrePass")))
    {
        bool bValue  = CVarEnablePrePass->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Feature.PrePass"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable pre-pass"), bValue, RevertPtr))
        {
            CVarEnablePrePass->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarEnableBasePass = GetCachedConsoleVariable(TEXT("Renderer.Feature.BasePass")))
    {
        bool bValue  = CVarEnableBasePass->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Feature.BasePass"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable base-pass"), bValue, RevertPtr))
        {
            CVarEnableBasePass->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawShadowSettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsShadows"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.Feature.Shadows")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Feature.Shadows"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable shadows"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawCascadedShadowSettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsCSM"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.Feature.SunShadows")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Feature.SunShadows"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable sun shadows"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.Feature.ShadowMask")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Feature.ShadowMask"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable shadow mask"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.TightFrustum")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.CSM.TightFrustum"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Tight frustum (depth reduce)"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.CascadeSize")))
    {
        static const CHAR_T* CascadeResItems[]  = { TEXT("512"), TEXT("1024"), TEXT("2048"), TEXT("4096") };
        static const int32   CascadeResValues[] = { 512, 1024, 2048, 4096 };

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
        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, TEXT("Renderer.CSM.CascadeSize"), ComboIndex0);
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
        if (EditorWidgets::DrawComboProperty(TEXT("Cascade resolution"), ComboIndex, CascadeResItems, CascadeResCount, RevertComboPtr))
        {
            CVar->SetAsInt(CascadeResValues[ComboIndex], EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.StableCascades")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.CSM.StableCascades"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Stable cascades"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.EnableSinglePassRendering")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.CSM.EnableSinglePassRendering"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Single-pass rendering"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.EnableGeometryShaderInstancing")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.CSM.EnableGeometryShaderInstancing"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Geometry shader instancing"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.EnableViewInstancing")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.CSM.EnableViewInstancing"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("View instancing"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.EnableDepthClipping")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.CSM.EnableDepthClipping"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Depth clipping"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.BlendCascades")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.CSM.BlendCascades"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Blend cascades"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.SelectCascadeFromProjection")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.CSM.SelectCascadeFromProjection"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Select cascade from projection"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.FilterMode")))
    {
        static const CHAR_T* const FilterModeItems[] = { TEXT("PCF"), TEXT("PCSS") };
        constexpr int32 FilterModeCount = static_cast<int32>(ARRAY_COUNT(FilterModeItems));

        int32 Value  = Math::Clamp<int32>(CVar->GetInt(), 0, FilterModeCount - 1);
        int32 Value0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, TEXT("Renderer.CSM.FilterMode"), Value0);
        if (EditorWidgets::DrawComboProperty(TEXT("Filter mode"), Value, FilterModeItems, FilterModeCount, RevertPtr))
        {
            CVar->SetAsInt(Value, EConsoleVariableFlags::SetByCode);
        }
    }

    int32 CurrentFilterFunction = 0;
    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.FilterFunction")))
    {
        static const CHAR_T* const FilterFuncItems[] = { TEXT("Grid"), TEXT("Poisson Disk"), TEXT("Vogel Disk") };
        constexpr int32 FilterFuncCount = static_cast<int32>(ARRAY_COUNT(FilterFuncItems));

        int32 Value  = Math::Clamp<int32>(CVar->GetInt(), 0, FilterFuncCount - 1);
        CurrentFilterFunction = Value;
        int32 Value0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, TEXT("Renderer.CSM.FilterFunction"), Value0);
        if (EditorWidgets::DrawComboProperty(TEXT("Filter function"), Value, FilterFuncItems, FilterFuncCount, RevertPtr))
        {
            CVar->SetAsInt(Value, EConsoleVariableFlags::SetByCode);
            CurrentFilterFunction = Value;

            if (Value == 0)
            {
                if (IConsoleVariable* FilterSizeCVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.FilterSize")))
                {
                    FilterSizeCVar->SetAsInt(9, EConsoleVariableFlags::SetByCode);
                }
            }
        }
    }

    const bool bIsGridFunction = (CurrentFilterFunction == 0);

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.FilterSize")))
    {
        int32 Value  = Math::Clamp<int32>(CVar->GetInt(), 1, 2048);
        int32 Value0 = 0;

        const CHAR_T* FilterSizeLabel = bIsGridFunction ? TEXT("Grid size") : TEXT("Filter radius");
        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, TEXT("Renderer.CSM.FilterSize"), Value0);
        if (EditorWidgets::DrawIntProperty(FilterSizeLabel, Value, 1.0f, 1, 2048, TEXT("%d"), true, RevertPtr))
        {
            CVar->SetAsInt(Value, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.MaxFilterSize")))
    {
        int32 Value  = Math::Clamp<int32>(CVar->GetInt(), 1, 2048);
        int32 Value0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, TEXT("Renderer.CSM.MaxFilterSize"), Value0);
        if (EditorWidgets::DrawIntProperty(TEXT("Max filter size"), Value, 1.0f, 1, 2048, TEXT("%d"), true, RevertPtr))
        {
            CVar->SetAsInt(Value, EConsoleVariableFlags::SetByCode);
        }
    }

    const bool bIsVogelFunction = (CurrentFilterFunction == 2);

    if (!bIsGridFunction)
    {
        if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.NumPoissonDiscSamples")))
        {
            if (bIsVogelFunction)
            {
                int32 Value  = Math::Clamp<int32>(CVar->GetInt(), 4, 128);
                int32 Value0 = 0;

                const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, TEXT("Renderer.CSM.NumPoissonDiscSamples"), Value0);
                if (EditorWidgets::DrawIntProperty(TEXT("Sample count"), Value, 1.0f, 4, 128, TEXT("%d"), true, RevertPtr))
                {
                    CVar->SetAsInt(Value, EConsoleVariableFlags::SetByCode);
                }
            }
            else
            {
                static const CHAR_T* SampleCountItems[] = { TEXT("16"), TEXT("32"), TEXT("64"), TEXT("128") };
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
                const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, TEXT("Renderer.CSM.NumPoissonDiscSamples"), ComboIndex0);
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
                if (EditorWidgets::DrawComboProperty(TEXT("Sample count"), ComboIndex, SampleCountItems, SampleCountCount, RevertComboPtr))
                {
                    CVar->SetAsInt(SampleCountValues[ComboIndex], EConsoleVariableFlags::SetByCode);
                }
            }
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.CSM.RotateSamples")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.CSM.RotateSamples"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Rotate samples"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawPointLightShadowSettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsPointLightShadows"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.Feature.PointLightShadows")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Feature.PointLightShadows"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable point-light shadows"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.Shadows.PointLightShadowMapSize")))
    {
        static const CHAR_T* PointResItems[]  = { TEXT("128"), TEXT("256"), TEXT("512"), TEXT("1024") };
        static const int32   PointResValues[] = { 128, 256, 512, 1024 };

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
        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, TEXT("Renderer.Shadows.PointLightShadowMapSize"), ComboIndex0);
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
        if (EditorWidgets::DrawComboProperty(TEXT("Shadow map resolution"), ComboIndex, PointResItems, PointResCount, RevertComboPtr))
        {
            CVar->SetAsInt(PointResValues[ComboIndex], EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.PointLights.EnableSinglePassRendering")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.PointLights.EnableSinglePassRendering"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Single-pass rendering"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVar = GetCachedConsoleVariable(TEXT("Renderer.PointLights.EnableGeometryShaderInstancing")))
    {
        bool bValue  = CVar->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.PointLights.EnableGeometryShaderInstancing"), bValue0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Geometry shader instancing"), bValue, RevertPtr))
        {
            CVar->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawSkyboxSettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsSkybox"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarEnableSkybox = GetCachedConsoleVariable(TEXT("Renderer.Feature.Skybox")))
    {
        bool bEnableSkybox  = CVarEnableSkybox->GetBool();
        bool bEnableSkybox0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Feature.Skybox"), bEnableSkybox0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable Skybox"), bEnableSkybox, RevertPtr))
        {
            CVarEnableSkybox->SetAsBool(bEnableSkybox, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarEnableClearBeforeSkybox = GetCachedConsoleVariable(TEXT("Renderer.Skybox.ClearBeforeSkybox")))
    {
        bool bEnableClearBeforeSkybox  = CVarEnableClearBeforeSkybox->GetBool();
        bool bEnableClearBeforeSkybox0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Skybox.ClearBeforeSkybox"), bEnableClearBeforeSkybox0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Clear before skybox"), bEnableClearBeforeSkybox, RevertPtr))
        {
            CVarEnableClearBeforeSkybox->SetAsBool(bEnableClearBeforeSkybox, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawSSAOSettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsSSAO"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarEnableSSAO = GetCachedConsoleVariable(TEXT("Renderer.Feature.SSAO")))
    {
        bool bEnableSSAO  = CVarEnableSSAO->GetBool();
        bool bEnableSSAO0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Feature.SSAO"), bEnableSSAO0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable SSAO"), bEnableSSAO, RevertPtr))
        {
            CVarEnableSSAO->SetAsBool(bEnableSSAO, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarKernelSize = GetCachedConsoleVariable(TEXT("Renderer.SSAO.KernelSize")))
    {
        int32 KernelSize  = Math::Clamp<int32>(CVarKernelSize->GetInt(), 1, 128);
        int32 KernelSize0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, TEXT("Renderer.SSAO.KernelSize"), KernelSize0);
        if (EditorWidgets::DrawIntProperty(TEXT("Kernel size"), KernelSize, 1.0f, 1, 128, TEXT("%d"), true, RevertPtr))
        {
            CVarKernelSize->SetAsInt(KernelSize, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarRadius = GetCachedConsoleVariable(TEXT("Renderer.SSAO.Radius")))
    {
        float Radius  = Math::Clamp<float>(CVarRadius->GetFloat(), 0.01f, 1.0f);
        float Radius0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.SSAO.Radius"), Radius0);
        if (EditorWidgets::DrawFloatProperty(TEXT("Radius"), Radius, 0.01f, 0.01f, 1.0f, TEXT("%.2f"), true, RevertPtr))
        {
            CVarRadius->SetAsFloat(Radius, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarBias = GetCachedConsoleVariable(TEXT("Renderer.SSAO.Bias")))
    {
        float Bias  = Math::Clamp<float>(CVarBias->GetFloat(), 0.01f, 1.0f);
        float Bias0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.SSAO.Bias"), Bias0);
        if (EditorWidgets::DrawFloatProperty(TEXT("Bias"), Bias, 0.01f, 0.01f, 1.0f, TEXT("%.2f"), true, RevertPtr))
        {
            CVarBias->SetAsFloat(Bias, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawRayTracingSettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsRayTracing"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    const CHAR_T* HardwareSupport = RHI::bSupportsRayTracingPipeline
        ? (RHI::bSupportsInlineRayTracing ? TEXT("Pipeline and inline") : TEXT("Pipeline only"))
        : (RHI::bSupportsRayTracing ? TEXT("Inline only") : TEXT("Unsupported"));

    EditorWidgets::DrawTextProperty(TEXT("Hardware support"), HardwareSupport);

    if (IConsoleVariable* CVarEnableRayTracing = GetCachedConsoleVariable(TEXT("Renderer.Feature.RayTracing")))
    {
        bool bEnableRayTracing  = CVarEnableRayTracing->GetBool();
        bool bEnableRayTracing0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Feature.RayTracing"), bEnableRayTracing0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable ray tracing"), bEnableRayTracing, RevertPtr, RHI::bSupportsRayTracing))
        {
            CVarEnableRayTracing->SetAsBool(bEnableRayTracing, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarLocalShaderBindings = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.EnableLocalShaderBindings")))
    {
        bool bLocalShaderBindings  = CVarLocalShaderBindings->GetBool();
        bool bLocalShaderBindings0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.RayTracing.EnableLocalShaderBindings"), bLocalShaderBindings0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Use local shader bindings"), bLocalShaderBindings, RevertPtr, RHI::bSupportsShaderBindingTableDescriptors))
        {
            CVarLocalShaderBindings->SetAsBool(bLocalShaderBindings, EConsoleVariableFlags::SetByCode);
        }
    }

    bool bInlineReflections = false;
    if (IConsoleVariable* CVarInlineReflections = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.InlineReflections")))
    {
        bInlineReflections = CVarInlineReflections->GetBool();

        bool bInlineReflections0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.RayTracing.InlineReflections"), bInlineReflections0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Inline reflections (RayQuery)"), bInlineReflections, RevertPtr, RHI::bSupportsInlineRayTracing))
        {
            CVarInlineReflections->SetAsBool(bInlineReflections, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarSER = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.SER")))
    {
        bool bSER  = CVarSER->GetBool();
        bool bSER0 = false;

        // The inline path takes priority over SER, so SER has no effect while inline reflections are enabled
        const bool  bSEREnabled = RHI::bSupportsShaderExecutionReordering && !bInlineReflections;
        const bool* RevertPtr   = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.RayTracing.SER"), bSER0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Shader Execution Reordering"), bSER, RevertPtr, bSEREnabled))
        {
            CVarSER->SetAsBool(bSER, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarCompaction = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Compaction")))
    {
        bool bCompaction  = CVarCompaction->GetBool();
        bool bCompaction0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.RayTracing.Compaction"), bCompaction0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("BLAS compaction"), bCompaction, RevertPtr, RHI::bSupportsRayTracing))
        {
            CVarCompaction->SetAsBool(bCompaction, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarASCache = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.ASCache")))
    {
        bool bASCache  = CVarASCache->GetBool();
        bool bASCache0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.RayTracing.ASCache"), bASCache0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Acceleration-structure disk cache"), bASCache, RevertPtr, RHI::bSupportsRayTracing))
        {
            CVarASCache->SetAsBool(bASCache, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawRayTracingReflectionsSettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsRayTracingReflections"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    bool bRayTracingActive = false;
    if (IConsoleVariable* CVarEnableRayTracing = GetCachedConsoleVariable(TEXT("Renderer.Feature.RayTracing")))
    {
        bRayTracingActive = CVarEnableRayTracing->GetBool() && RHI::bSupportsRayTracing;
    }

    bool bReflectionsEnabled = false;
    if (IConsoleVariable* CVarEnableReflections = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.Enable")))
    {
        bReflectionsEnabled = CVarEnableReflections->GetBool();

        bool bEnableReflections0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.RayTracing.Reflections.Enable"), bEnableReflections0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable"), bReflectionsEnabled, RevertPtr, bRayTracingActive))
        {
            CVarEnableReflections->SetAsBool(bReflectionsEnabled, EConsoleVariableFlags::SetByCode);
        }
    }

    const bool bReflectionsActive = bRayTracingActive && bReflectionsEnabled;

    if (IConsoleVariable* CVarIndirectSpecularStrength = GetCachedConsoleVariable(TEXT("Renderer.Reflections.IndirectSpecularStrength")))
    {
        float IndirectSpecularStrength  = Math::Clamp<float>(CVarIndirectSpecularStrength->GetFloat(), 0.0f, 4.0f);
        float IndirectSpecularStrength0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.Reflections.IndirectSpecularStrength"), IndirectSpecularStrength0);
        if (EditorWidgets::DrawFloatProperty(TEXT("Indirect specular strength"), IndirectSpecularStrength, 0.01f, 0.0f, 4.0f, TEXT("%.2f"), true, RevertPtr, bReflectionsActive))
        {
            CVarIndirectSpecularStrength->SetAsFloat(IndirectSpecularStrength, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarHalfRes = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.HalfRes")))
    {
        bool bHalfRes  = CVarHalfRes->GetBool();
        bool bHalfRes0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.RayTracing.Reflections.HalfRes"), bHalfRes0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Half resolution"), bHalfRes, RevertPtr, bReflectionsActive))
        {
            CVarHalfRes->SetAsBool(bHalfRes, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarMaxRayDistance = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.MaxRayDistance")))
    {
        float MaxRayDistance  = Math::Clamp<float>(CVarMaxRayDistance->GetFloat(), 1.0f, 100000.0f);
        float MaxRayDistance0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.RayTracing.Reflections.MaxRayDistance"), MaxRayDistance0);
        if (EditorWidgets::DrawFloatProperty(TEXT("Max ray distance"), MaxRayDistance, 10.0f, 1.0f, 100000.0f, TEXT("%.0f"), true, RevertPtr, bReflectionsActive))
        {
            CVarMaxRayDistance->SetAsFloat(MaxRayDistance, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarMirrorRoughnessThreshold = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.MirrorRoughnessThreshold")))
    {
        float MirrorRoughnessThreshold  = Math::Clamp<float>(CVarMirrorRoughnessThreshold->GetFloat(), 0.0f, 1.0f);
        float MirrorRoughnessThreshold0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.RayTracing.Reflections.MirrorRoughnessThreshold"), MirrorRoughnessThreshold0);
        if (EditorWidgets::DrawFloatProperty(TEXT("Mirror roughness threshold"), MirrorRoughnessThreshold, 0.001f, 0.0f, 1.0f, TEXT("%.3f"), true, RevertPtr, bReflectionsActive))
        {
            CVarMirrorRoughnessThreshold->SetAsFloat(MirrorRoughnessThreshold, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarRayBias = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.RayBias")))
    {
        float RayBias  = Math::Clamp<float>(CVarRayBias->GetFloat(), 0.0f, 1.0f);
        float RayBias0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.RayTracing.Reflections.RayBias"), RayBias0);
        if (EditorWidgets::DrawFloatProperty(TEXT("Ray bias"), RayBias, 0.001f, 0.0f, 1.0f, TEXT("%.3f"), true, RevertPtr, bReflectionsActive))
        {
            CVarRayBias->SetAsFloat(RayBias, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarSampler = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.Sampler")))
    {
        // Order matches REFLECTION_SAMPLER_* in Shaders/Reflections/ReflectionSampling.hlsli.
        static const CHAR_T* const Items[] =
        {
            TEXT("White noise"), TEXT("Halton"), TEXT("Blue noise")
        };

        constexpr int32 ItemCount = static_cast<int32>(ARRAY_COUNT(Items));

        int32 Sampler  = Math::Clamp<int32>(CVarSampler->GetInt(), 0, ItemCount - 1);
        int32 Sampler0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, TEXT("Renderer.RayTracing.Reflections.Sampler"), Sampler0);
        if (EditorWidgets::DrawComboProperty(TEXT("GGX sampler"), Sampler, Items, ItemCount, RevertPtr, bReflectionsActive))
        {
            CVarSampler->SetAsInt(Sampler, EConsoleVariableFlags::SetByCode);
        }
    }

    bool bDenoise = false;
    if (IConsoleVariable* CVarDenoise = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.Denoise")))
    {
        bDenoise = CVarDenoise->GetBool();

        bool bDenoise0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.RayTracing.Reflections.Denoise"), bDenoise0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable denoiser"), bDenoise, RevertPtr, bReflectionsActive))
        {
            CVarDenoise->SetAsBool(bDenoise, EConsoleVariableFlags::SetByCode);
        }
    }

    const bool bDenoiserActive = bReflectionsActive && bDenoise;

    if (IConsoleVariable* CVarTemporalAlpha = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.TemporalAlpha")))
    {
        float TemporalAlpha  = Math::Clamp<float>(CVarTemporalAlpha->GetFloat(), 0.01f, 1.0f);
        float TemporalAlpha0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.RayTracing.Reflections.TemporalAlpha"), TemporalAlpha0);
        if (EditorWidgets::DrawFloatProperty(TEXT("Temporal alpha"), TemporalAlpha, 0.01f, 0.01f, 1.0f, TEXT("%.2f"), true, RevertPtr, bDenoiserActive))
        {
            CVarTemporalAlpha->SetAsFloat(TemporalAlpha, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarMaxRadiance = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.MaxRadiance")))
    {
        float MaxRadiance  = Math::Clamp<float>(CVarMaxRadiance->GetFloat(), 0.0f, 100.0f);
        float MaxRadiance0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.RayTracing.Reflections.MaxRadiance"), MaxRadiance0);
        if (EditorWidgets::DrawFloatProperty(TEXT("Max radiance (firefly clamp)"), MaxRadiance, 0.1f, 0.0f, 100.0f, TEXT("%.2f"), true, RevertPtr, bDenoiserActive))
        {
            CVarMaxRadiance->SetAsFloat(MaxRadiance, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarHistoryClampGamma = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.HistoryClampGamma")))
    {
        float HistoryClampGamma  = Math::Clamp<float>(CVarHistoryClampGamma->GetFloat(), 0.0f, 10.0f);
        float HistoryClampGamma0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.RayTracing.Reflections.HistoryClampGamma"), HistoryClampGamma0);
        if (EditorWidgets::DrawFloatProperty(TEXT("History clamp gamma"), HistoryClampGamma, 0.01f, 0.0f, 10.0f, TEXT("%.2f"), true, RevertPtr, bDenoiserActive))
        {
            CVarHistoryClampGamma->SetAsFloat(HistoryClampGamma, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarMaxHistoryLength = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.MaxHistoryLength")))
    {
        float MaxHistoryLength  = Math::Clamp<float>(CVarMaxHistoryLength->GetFloat(), 1.0f, 128.0f);
        float MaxHistoryLength0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.RayTracing.Reflections.MaxHistoryLength"), MaxHistoryLength0);
        if (EditorWidgets::DrawFloatProperty(TEXT("Max history length"), MaxHistoryLength, 1.0f, 1.0f, 128.0f, TEXT("%.0f"), true, RevertPtr, bDenoiserActive))
        {
            CVarMaxHistoryLength->SetAsFloat(MaxHistoryLength, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarNeighborhoodRadius = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.NeighborhoodRadius")))
    {
        int32 NeighborhoodRadius  = Math::Clamp<int32>(CVarNeighborhoodRadius->GetInt(), 0, 8);
        int32 NeighborhoodRadius0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, TEXT("Renderer.RayTracing.Reflections.NeighborhoodRadius"), NeighborhoodRadius0);
        if (EditorWidgets::DrawIntProperty(TEXT("Neighborhood radius"), NeighborhoodRadius, 1.0f, 0, 8, TEXT("%d"), true, RevertPtr, bDenoiserActive))
        {
            CVarNeighborhoodRadius->SetAsInt(NeighborhoodRadius, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarCameraMotionMaxHistory = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.CameraMotionMaxHistory")))
    {
        float CameraMotionMaxHistory  = Math::Clamp<float>(CVarCameraMotionMaxHistory->GetFloat(), 0.0f, 64.0f);
        float CameraMotionMaxHistory0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.RayTracing.Reflections.CameraMotionMaxHistory"), CameraMotionMaxHistory0);
        if (EditorWidgets::DrawFloatProperty(TEXT("Camera-motion max history"), CameraMotionMaxHistory, 0.5f, 0.0f, 64.0f, TEXT("%.1f"), true, RevertPtr, bDenoiserActive))
        {
            CVarCameraMotionMaxHistory->SetAsFloat(CameraMotionMaxHistory, EConsoleVariableFlags::SetByCode);
        }
    }

    int32 AtrousIterations = 0;
    if (IConsoleVariable* CVarAtrousIterations = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.AtrousIterations")))
    {
        AtrousIterations = Math::Clamp<int32>(CVarAtrousIterations->GetInt(), 0, 8);

        int32 AtrousIterations0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, TEXT("Renderer.RayTracing.Reflections.AtrousIterations"), AtrousIterations0);
        if (EditorWidgets::DrawIntProperty(TEXT("A-trous iterations"), AtrousIterations, 1.0f, 0, 8, TEXT("%d"), true, RevertPtr, bDenoiserActive))
        {
            CVarAtrousIterations->SetAsInt(AtrousIterations, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarAtrousPhiColor = GetCachedConsoleVariable(TEXT("Renderer.RayTracing.Reflections.AtrousPhiColor")))
    {
        float AtrousPhiColor  = Math::Clamp<float>(CVarAtrousPhiColor->GetFloat(), 0.1f, 32.0f);
        float AtrousPhiColor0 = 0.0f;

        const bool   bSpatialActive = bDenoiserActive && (AtrousIterations > 0);
        const float* RevertPtr      = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.RayTracing.Reflections.AtrousPhiColor"), AtrousPhiColor0);
        if (EditorWidgets::DrawFloatProperty(TEXT("A-trous color phi"), AtrousPhiColor, 0.1f, 0.1f, 32.0f, TEXT("%.2f"), true, RevertPtr, bSpatialActive))
        {
            CVarAtrousPhiColor->SetAsFloat(AtrousPhiColor, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawTAASettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsTAA"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    bool bIsTAAEnabled = false;
    if (IConsoleVariable* CVarEnableTemporalAntiAliasing = GetCachedConsoleVariable(TEXT("Renderer.Feature.TemporalAntiAliasing")))
    {
        bool bEnableTAA  = CVarEnableTemporalAntiAliasing->GetBool();
        bool bEnableTAA0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Feature.TemporalAntiAliasing"), bEnableTAA0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enabled"), bEnableTAA, RevertPtr))
        {
            CVarEnableTemporalAntiAliasing->SetAsBool(bEnableTAA, EConsoleVariableFlags::SetByCode);
        }

        bIsTAAEnabled = bEnableTAA;
    }

    if (IConsoleVariable* CVarHardwareJitter = GetCachedConsoleVariable(TEXT("Renderer.TemporalAntiAliasing.HardwareJitter")))
    {
        const bool bSupported = RHI::bSupportsProgrammableSamplePositions && IsSampleCountSupported(RHI::SupportedSamplePositionSampleCounts, RHI_SAMPLE_COUNT_1);

        bool bHardwareJitter  = CVarHardwareJitter->GetBool();
        bool bHardwareJitter0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.TemporalAntiAliasing.HardwareJitter"), bHardwareJitter0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Hardware jitter"), bHardwareJitter, RevertPtr, bIsTAAEnabled && bSupported))
        {
            CVarHardwareJitter->SetAsBool(bHardwareJitter, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawFXAASettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsFXAA"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarEnableFXAA = GetCachedConsoleVariable(TEXT("Renderer.Feature.FXAA")))
    {
        bool bEnableFXAA  = CVarEnableFXAA->GetBool();
        bool bEnableFXAA0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Feature.FXAA"), bEnableFXAA0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable FXAA"), bEnableFXAA, RevertPtr))
        {
            CVarEnableFXAA->SetAsBool(bEnableFXAA, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarEnableFXAADebug = GetCachedConsoleVariable(TEXT("Renderer.Debug.FXAADebug")))
    {
        bool bEnableFXAADebug  = CVarEnableFXAADebug->GetBool();
        bool bEnableFXAADebug0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Debug.FXAADebug"), bEnableFXAADebug0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable FXAA debug"), bEnableFXAADebug, RevertPtr))
        {
            CVarEnableFXAADebug->SetAsBool(bEnableFXAADebug, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawDisplaySettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsDisplay"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarEnableVSync = GetCachedConsoleVariable(TEXT("Renderer.Feature.VerticalSync")))
    {
        bool bEnableVSync  = CVarEnableVSync->GetBool();
        bool bEnableVSync0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Feature.VerticalSync"), bEnableVSync0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable VSync"), bEnableVSync, RevertPtr))
        {
            CVarEnableVSync->SetAsBool(bEnableVSync, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawCullingSettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsCulling"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarEnableFrustumCulling = GetCachedConsoleVariable(TEXT("Renderer.Feature.FrustumCulling")))
    {
        bool bEnableFrustumCulling  = CVarEnableFrustumCulling->GetBool();
        bool bEnableFrustumCulling0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Feature.FrustumCulling"), bEnableFrustumCulling0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable frustum-culling"), bEnableFrustumCulling, RevertPtr))
        {
            CVarEnableFrustumCulling->SetAsBool(bEnableFrustumCulling, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawDebugSettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsDebug"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    if (IConsoleVariable* CVarEnableDebugDrawAABBs = GetCachedConsoleVariable(TEXT("Renderer.Debug.DrawAABBs")))
    {
        bool bEnableDebugDrawAABBs  = CVarEnableDebugDrawAABBs->GetBool();
        bool bEnableDebugDrawAABBs0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Debug.DrawAABBs"), bEnableDebugDrawAABBs0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable debug-draw AABBs"), bEnableDebugDrawAABBs, RevertPtr))
        {
            CVarEnableDebugDrawAABBs->SetAsBool(bEnableDebugDrawAABBs, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarEnableDebugDrawPointLights = GetCachedConsoleVariable(TEXT("Renderer.Debug.DrawPointLights")))
    {
        bool bEnableDebugDrawPointLights  = CVarEnableDebugDrawPointLights->GetBool();
        bool bEnableDebugDrawPointLights0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Debug.DrawPointLights"), bEnableDebugDrawPointLights0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable debug-draw point-lights"), bEnableDebugDrawPointLights, RevertPtr))
        {
            CVarEnableDebugDrawPointLights->SetAsBool(bEnableDebugDrawPointLights, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarEnableDebugDrawLightProbes = GetCachedConsoleVariable(TEXT("Renderer.Debug.LightProbes")))
    {
        bool bEnableDebugDrawLightProbes  = CVarEnableDebugDrawLightProbes->GetBool();
        bool bEnableDebugDrawLightProbes0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, TEXT("Renderer.Debug.LightProbes"), bEnableDebugDrawLightProbes0);
        if (EditorWidgets::DrawCheckboxProperty(TEXT("Enable debug-draw light-probes"), bEnableDebugDrawLightProbes, RevertPtr))
        {
            CVarEnableDebugDrawLightProbes->SetAsBool(bEnableDebugDrawLightProbes, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawTonemappingSettings()
{
    if (!EditorWidgets::BeginPropertyTable(TEXT("##RendererSettingsTonemapping"), RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    bool  bHasTonemappingFunction = false;
    int32 TonemappingFunction     = 0;

    if (IConsoleVariable* CVarTonemappingFunction = GetCachedConsoleVariable(TEXT("Renderer.Tonemapping.Function")))
    {
        static const CHAR_T* const Items[] =
        {
            TEXT("Default"), TEXT("ACES"), TEXT("Reinhard"), TEXT("Uncharted 2")
        };

        constexpr int32 ItemCount = static_cast<int32>(ARRAY_COUNT(Items));

        TonemappingFunction     = Math::Clamp<int32>(CVarTonemappingFunction->GetInt(), 0, 3);
        bHasTonemappingFunction = true;

        int32 TonemappingFunction0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, TEXT("Renderer.Tonemapping.Function"), TonemappingFunction0);
        if (EditorWidgets::DrawComboProperty(TEXT("Tonemapping function"), TonemappingFunction, Items, ItemCount, RevertPtr))
        {
            CVarTonemappingFunction->SetAsInt(TonemappingFunction, EConsoleVariableFlags::SetByCode);
        }
    }

    if (IConsoleVariable* CVarTonemappingEV100 = GetCachedConsoleVariable(TEXT("Renderer.Tonemapping.EV100")))
    {
        float ExposureEV100  = Math::Clamp<float>(CVarTonemappingEV100->GetFloat(), -10.0f, 20.0f);
        float ExposureEV1000 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.Tonemapping.EV100"), ExposureEV1000);
        if (EditorWidgets::DrawFloatProperty(TEXT("Exposure (EV100)"), ExposureEV100, 0.1f, -10.0f, 20.0f, TEXT("%.2f"), true, RevertPtr))
        {
            CVarTonemappingEV100->SetAsFloat(ExposureEV100, EConsoleVariableFlags::SetByCode);
        }
    }

    const bool bIsReinhard = bHasTonemappingFunction && (TonemappingFunction == 2);
    if (bIsReinhard)
    {
        if (IConsoleVariable* CVarTonemappingReinhardIntensity = GetCachedConsoleVariable(TEXT("Renderer.Tonemapping.ReinhardIntensity")))
        {
            float ReinhardIntensity  = Math::Clamp<float>(CVarTonemappingReinhardIntensity->GetFloat(), 0.1f, 10.0f);
            float ReinhardIntensity0 = 0.0f;

            const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, TEXT("Renderer.Tonemapping.ReinhardIntensity"), ReinhardIntensity0);
            if (EditorWidgets::DrawFloatProperty(TEXT("Reinhard intensity"), ReinhardIntensity, 0.01f, 0.1f, 10.0f, TEXT("%.2f"), true, RevertPtr))
            {
                CVarTonemappingReinhardIntensity->SetAsFloat(ReinhardIntensity, EConsoleVariableFlags::SetByCode);
            }
        }
    }

    EditorWidgets::EndPropertyTable();
}
