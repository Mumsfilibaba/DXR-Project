#include "Engine/EngineUI/Editor/EditorRendererSettingsWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"

#include "Core/CoreDefines.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/CString.h"

#include "ImGuiPlugin/ImGuiCore.h"

static constexpr float RendererSettingsLabelColumnWidth  = 310.0f;
static constexpr float RendererSettingsRevertColumnWidth = 28.0f;

template<typename ValueType>
static const ValueType* TryGetDefaultPtr(const TMap<FString, ValueType>& Defaults, const CHAR* CVarName, ValueType& OutValue)
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
            BoolDefaults.Add(FString(Name), CVar->GetBool());
        }
    };

    const auto CaptureInt = [&](const CHAR* Name)
    {
        if (IConsoleVariable* CVar = Console.FindConsoleVariable(Name))
        {
            IntDefaults.Add(FString(Name), CVar->GetInt());
        }
    };

    const auto CaptureFloat = [&](const CHAR* Name)
    {
        if (IConsoleVariable* CVar = Console.FindConsoleVariable(Name))
        {
            FloatDefaults.Add(FString(Name), CVar->GetFloat());
        }
    };

    // Deferred rendering
    CaptureBool("Renderer.Debug.DrawTiledLightning");
    CaptureBool("Renderer.BasePass.ClearAllTargets");
    CaptureBool("Renderer.Feature.PrePass");
    CaptureBool("Renderer.PrePass.DepthReduce");
    CaptureBool("Renderer.Feature.BasePass");

    // Shadows
    CaptureBool("Renderer.Feature.Shadows");
    CaptureBool("Renderer.Feature.SunShadows");

    // Cascaded shadow maps
    CaptureBool("Renderer.CSM.EnableSinglePassRendering");
    CaptureBool("Renderer.CSM.EnableGeometryShaderInstancing");
    CaptureBool("Renderer.CSM.RotateSamples");
    CaptureBool("Renderer.CSM.BlendCascades");
    CaptureBool("Renderer.CSM.TightFrustum");
    CaptureBool("Renderer.CSM.StableCascades");
    CaptureBool("Renderer.CSM.SelectCascadeFromProjection");

    CaptureInt("Renderer.CSM.FilterMode");
    CaptureInt("Renderer.CSM.FilterFunction");
    CaptureInt("Renderer.CSM.NumPoissonDiscSamples");
    CaptureInt("Renderer.CSM.CascadeSize");

    CaptureBool("Renderer.CSM.IGN.StableBetweenFrames");
    CaptureFloat("Renderer.CSM.PCF.FilterWorld");
    CaptureFloat("Renderer.CSM.PCF.MinFilterRadiusTexels");
    CaptureBool("Renderer.CSM.ShadowPancaking");
    CaptureFloat("Renderer.CSM.MaxShadowDistance");
    CaptureFloat("Renderer.CSM.MaxShadowDistanceFade");

    CaptureFloat("Renderer.CSM.PCSS.RadiusScale");
    CaptureFloat("Renderer.CSM.PCSS.BlockerSearchScale");
    CaptureFloat("Renderer.CSM.PCSS.MinFilterRadiusTexels");
    CaptureFloat("Renderer.CSM.PCSS.BlockerSamplingClump");
    CaptureFloat("Renderer.CSM.PCSS.MaxPenumbraWorld");
    CaptureFloat("Renderer.CSM.PCSS.MaxSearchDistanceWorld");
    CaptureFloat("Renderer.CSM.PCSS.MinFilterMaxAngularDiameter");
    CaptureFloat("Renderer.CSM.PCSS.BlockerSearchAngularDiameter");
    CaptureInt("Renderer.CSM.PCSS.NumBlockerSamples");

    // Point-lights
    CaptureBool("Renderer.Feature.PointLightShadows");
    CaptureBool("Renderer.PointLights.EnableSinglePassRendering");
    CaptureBool("Renderer.PointLights.EnableGeometryShaderInstancing");
    CaptureInt("Renderer.Shadows.PointLightShadowMapSize");

    // Skybox
    CaptureBool("Renderer.Feature.Skybox");
    CaptureBool("Renderer.Skybox.ClearBeforeSkybox");

    // SSAO
    CaptureBool("Renderer.Feature.SSAO");
    CaptureInt("Renderer.SSAO.KernelSize");
    CaptureFloat("Renderer.SSAO.Radius");
    CaptureFloat("Renderer.SSAO.Bias");

    // Anti-aliasing
    CaptureBool("Renderer.Feature.TemporalAA");
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
}

void FEditorRendererSettingsWidget::DrawWindow()
{
    CaptureDefaultsIfNeeded();

    const auto DrawLabelWithSeperator = [](const CHAR* InLabel)
    {
        static constexpr int32 LabelLength = 256;
        TStaticArray<CHAR, LabelLength> Label{};
        FCString::Snprintf(Label.Data(), static_cast<int32>(Label.Size()), "%s", InLabel);

        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextBorderSize, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.1f, 0.5f));
        ImGui::SeparatorText(Label.Data());
        ImGui::PopStyleVar(2);
    };

    const auto DrawCollapsingHeader = [](const CHAR* Label, ImGuiTreeNodeFlags Flags, const bool bDrawBottomBorder)
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
            // Draw custom icon instead of the ImGui arrow
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

        // Only add the bottom border when collapsed (same as before) AND when it's not the last header
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
                    Window->DrawList->AddLine(ImVec2(HeaderMin.x, Y), ImVec2(HeaderMax.x, Y), Color, Thickness);
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
    }

    if (DrawCollapsingHeader("Cascaded Shadow Maps", ImGuiTreeNodeFlags_None, true))
    {
        DrawCascadedShadowSettings();
    }

    if (DrawCollapsingHeader("Point-light Shadow Maps", ImGuiTreeNodeFlags_None, true))
    {
        DrawPointLightShadowSettings();
    }

    if (DrawCollapsingHeader("Skybox", ImGuiTreeNodeFlags_None, true))
    {
        DrawSkyboxSettings();
    }

    if (DrawCollapsingHeader("SSAO", ImGuiTreeNodeFlags_None, true))
    {
        DrawSSAOSettings();
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

    // Draw tile-debug
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

    // Clear all targets
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

    // Enable pre-pass
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

    // Enable pre-pass depth-reduce
    if (IConsoleVariable* CVarEnablePrePassDepthReduce = FConsoleManager::Get().FindConsoleVariable("Renderer.PrePass.DepthReduce"))
    {
        bool bValue  = CVarEnablePrePassDepthReduce->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.PrePass.DepthReduce", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable pre-pass depth-reduce", bValue, RevertPtr))
        {
            CVarEnablePrePassDepthReduce->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    // Enable base-pass
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

    if (IConsoleVariable* CVarEnableShadows = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.Shadows"))
    {
        bool bValue  = CVarEnableShadows->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.Shadows", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable shadows", bValue, RevertPtr))
        {
            CVarEnableShadows->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    // Enable sun shadows
    if (IConsoleVariable* CVarEnableSunShadows = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.SunShadows"))
    {
        bool bValue  = CVarEnableSunShadows->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.SunShadows", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable sun shadows", bValue, RevertPtr))
        {
            CVarEnableSunShadows->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
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

    int32 ActiveFilterMode = 0;
    int32 ActiveFilterFunctionRawValue = 1;

    // Enable single-pass shadow map rendering
    if (IConsoleVariable* CVarEnableSinglePassRendering = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.EnableSinglePassRendering"))
    {
        bool bValue  = CVarEnableSinglePassRendering->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.EnableSinglePassRendering", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable single-pass cascade rendering", bValue, RevertPtr))
        {
            CVarEnableSinglePassRendering->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    // Enable geometry-shader instancing
    if (IConsoleVariable* CVarEnableGeometryShaderInstancing = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.EnableGeometryShaderInstancing"))
    {
        bool bValue  = CVarEnableGeometryShaderInstancing->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.EnableGeometryShaderInstancing", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable geometry-shader instancing", bValue, RevertPtr))
        {
            CVarEnableGeometryShaderInstancing->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    // Rotate samples
    if (IConsoleVariable* CVarEnableRotateSamples = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.RotateSamples"))
    {
        bool bValue  = CVarEnableRotateSamples->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.RotateSamples", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable rotate samples", bValue, RevertPtr))
        {
            CVarEnableRotateSamples->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    // Blend cascades
    if (IConsoleVariable* CVarEnableBlendCascades = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.BlendCascades"))
    {
        bool bValue  = CVarEnableBlendCascades->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.BlendCascades", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Blend cascades", bValue, RevertPtr))
        {
            CVarEnableBlendCascades->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    // Enable Tight Frustum
    if (IConsoleVariable* CVarCSMTightFrustum = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.TightFrustum"))
    {
        bool bValue  = CVarCSMTightFrustum->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.TightFrustum", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable Tight Frustum", bValue, RevertPtr))
        {
            CVarCSMTightFrustum->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    // Enable Stable Cascades
    if (IConsoleVariable* CVarCSMStableCascades = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.StableCascades"))
    {
        bool bValue  = CVarCSMStableCascades->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.StableCascades", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable Stable Cascades", bValue, RevertPtr))
        {
            CVarCSMStableCascades->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    // Adaptive Split Range
    if (IConsoleVariable* CVarAdaptiveSplitRange = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.AdaptiveSplitRange"))
    {
        bool bValue  = CVarAdaptiveSplitRange->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.AdaptiveSplitRange", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Adaptive Split Range", bValue, RevertPtr))
        {
            CVarAdaptiveSplitRange->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    // Max shadow distance
    if (IConsoleVariable* CVarMaxShadowDistance = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.MaxShadowDistance"))
    {
        float Value  = CVarMaxShadowDistance->GetFloat();
        float Value0 = 0.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.CSM.MaxShadowDistance", Value0);
        if (EditorWidgets::DrawFloatProperty("Max Shadow Distance", Value, 1.0f, 0.0f, 100000.0f, "%.1f", true, RevertPtr))
        {
            CVarMaxShadowDistance->SetAsFloat(Value, EConsoleVariableFlags::SetByCode);
        }
    }

    // Max shadow distance fade
    if (IConsoleVariable* CVarMaxShadowDistanceFade = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.MaxShadowDistanceFade"))
    {
        float Value  = CVarMaxShadowDistanceFade->GetFloat();
        float Value0 = 50.0f;

        const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.CSM.MaxShadowDistanceFade", Value0);
        if (EditorWidgets::DrawFloatProperty("Max Shadow Distance Fade", Value, 1.0f, 0.0f, 100000.0f, "%.1f", true, RevertPtr))
        {
            CVarMaxShadowDistanceFade->SetAsFloat(Value, EConsoleVariableFlags::SetByCode);
        }
    }

    // Shadow pancaking
    if (IConsoleVariable* CVarShadowPancaking = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.ShadowPancaking"))
    {
        bool bValue  = CVarShadowPancaking->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.ShadowPancaking", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Enable Shadow Pancaking", bValue, RevertPtr))
        {
            CVarShadowPancaking->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    // Select cascade from projection
    if (IConsoleVariable* CVarSelectCascadeFromProjection = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.SelectCascadeFromProjection"))
    {
        bool bValue  = CVarSelectCascadeFromProjection->GetBool();
        bool bValue0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.SelectCascadeFromProjection", bValue0);
        if (EditorWidgets::DrawCheckboxProperty("Select cascade from projection", bValue, RevertPtr))
        {
            CVarSelectCascadeFromProjection->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
        }
    }

    // Filter mode
    if (IConsoleVariable* CVarFilterMode = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.FilterMode"))
    {
        static const CHAR* const Items[] =
        {
            "Percentage Closer Filtering (PCF)",
            "Percentage Closer Soft Shadows (PCSS)",
        };

        constexpr int32 ItemCount = static_cast<int32>(ARRAY_COUNT(Items));

        int32 FilterMode  = Math::Clamp<int32>(CVarFilterMode->GetInt(), 0, 1);
        int32 FilterMode0 = 0;

        const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.CSM.FilterMode", FilterMode0);

        ActiveFilterMode = FilterMode;
        if (EditorWidgets::DrawComboProperty("Filter mode", FilterMode, Items, ItemCount, RevertPtr))
        {
            CVarFilterMode->SetAsInt(FilterMode, EConsoleVariableFlags::SetByCode);
            ActiveFilterMode = FilterMode;
        }
    }

    // Filter distribution
    if (IConsoleVariable* CVarFilterFunction = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.FilterFunction"))
    {
        static const CHAR* const Items[] =
        {
            "Poisson Disk",
            "Vogel Disk",
            "Interleaved Gradient Noise"
        };

        constexpr int32 ItemCount = static_cast<int32>(ARRAY_COUNT(Items));

        const int32 RawValue = Math::Clamp<int32>(CVarFilterFunction->GetInt(), 0, 3);
        ActiveFilterFunctionRawValue = RawValue;
        int32 UiIndex = (RawValue >= 3) ? 2 : ((RawValue >= 2) ? 1 : 0); // 0/1 => Poisson, 2 => Vogel, 3 => IGN

        int32 UiIndex0 = 0;
        const int32* DefaultPtr = IntDefaults.Find(FString("Renderer.CSM.FilterFunction"));
        const int32* RevertPtr = nullptr;
        if (DefaultPtr)
        {
            const int32 DefaultRaw = Math::Clamp<int32>(*DefaultPtr, 0, 3);
            UiIndex0  = (DefaultRaw >= 3) ? 2 : ((DefaultRaw >= 2) ? 1 : 0);
            RevertPtr = &UiIndex0;
        }

        if (EditorWidgets::DrawComboProperty("Filter Distribution", UiIndex, Items, ItemCount, RevertPtr))
        {
            const int32 NewRawValue = (UiIndex == 2) ? 3 : ((UiIndex == 1) ? 2 : 1);
            CVarFilterFunction->SetAsInt(NewRawValue, EConsoleVariableFlags::SetByCode);
            ActiveFilterFunctionRawValue = NewRawValue;
        }
    }

    // IGN: Stable noise between frames
    if (ActiveFilterFunctionRawValue >= 3)
    {
        if (IConsoleVariable* CVarStableIgn = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.IGN.StableBetweenFrames"))
        {
            bool bStable  = CVarStableIgn->GetBool();
            bool bStable0 = false;

            const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.CSM.IGN.StableBetweenFrames", bStable0);
            if (EditorWidgets::DrawCheckboxProperty("Stable noise between frames", bStable, RevertPtr))
            {
                CVarStableIgn->SetAsBool(bStable, EConsoleVariableFlags::SetByCode);
            }
        }
    }

    // PCF: Filter size (world)
    if (ActiveFilterMode == 0)
    {
        if (IConsoleVariable* CVarPCFFilterWorld = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCF.FilterWorld"))
        {
            float FilterWorld  = Math::Clamp<float>(CVarPCFFilterWorld->GetFloat(), 0.0f, 1.0f);
            float FilterWorld0 = 0.0f;

            const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.CSM.PCF.FilterWorld", FilterWorld0);
            if (EditorWidgets::DrawFloatProperty("PCF filter size (world)", FilterWorld, 0.01f, 0.0f, 1.0f, "%.2f", true, RevertPtr))
            {
                CVarPCFFilterWorld->SetAsFloat(FilterWorld, EConsoleVariableFlags::SetByCode);
            }
        }

        if (IConsoleVariable* CVarPCFMinFilterRadius = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCF.MinFilterRadiusTexels"))
        {
            float MinFilterRadius  = Math::Clamp<float>(CVarPCFMinFilterRadius->GetFloat(), 0.0f, 8.0f);
            float MinFilterRadius0 = 0.0f;

            const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.CSM.PCF.MinFilterRadiusTexels", MinFilterRadius0);
            if (EditorWidgets::DrawFloatProperty("PCF min filter (texels)", MinFilterRadius, 0.01f, 0.0f, 8.0f, "%.2f", true, RevertPtr))
            {
                CVarPCFMinFilterRadius->SetAsFloat(MinFilterRadius, EConsoleVariableFlags::SetByCode);
            }
        }
    }

    // Sample count
    if (IConsoleVariable* CVarNumPoissonDiscSamples = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.NumPoissonDiscSamples"))
    {
        const bool bIsPoisson = (ActiveFilterFunctionRawValue <= 1);

        if (bIsPoisson)
        {
            static const CHAR* const Items[] = { "16", "32", "64", "128" };
            static const int32 Samples[]     = { 16, 32, 64, 128 };
            constexpr int32 ItemCount = static_cast<int32>(ARRAY_COUNT(Items));

            const auto GetIndexForSamples = [](int32 NumSamples)
            {
                if (NumSamples >= 128) { return 3; }
                if (NumSamples >= 64)  { return 2; }
                if (NumSamples >= 32)  { return 1; }
                return 0;
            };

            int32 ItemIndex  = GetIndexForSamples(CVarNumPoissonDiscSamples->GetInt());
            int32 ItemIndex0 = 0;

            const int32* DefaultSamplesPtr = IntDefaults.Find(FString("Renderer.CSM.NumPoissonDiscSamples"));
            const int32* RevertPtr = nullptr;

            if (DefaultSamplesPtr)
            {
                ItemIndex0 = GetIndexForSamples(*DefaultSamplesPtr);
                RevertPtr  = &ItemIndex0;
            }

            if (EditorWidgets::DrawComboProperty("Sample count", ItemIndex, Items, ItemCount, RevertPtr))
            {
                const int32 NewNumSamples = Samples[Math::Clamp<int32>(ItemIndex, 0, ItemCount - 1)];
                CVarNumPoissonDiscSamples->SetAsInt(NewNumSamples, EConsoleVariableFlags::SetByCode);
            }
        }
        else
        {
            int32 NumSamples  = Math::Clamp<int32>(CVarNumPoissonDiscSamples->GetInt(), 1, 256);
            int32 NumSamples0 = 0;

            const int32* RevertPtr = TryGetDefaultPtr(IntDefaults, "Renderer.CSM.NumPoissonDiscSamples", NumSamples0);
            if (EditorWidgets::DrawIntProperty("Sample count", NumSamples, 1.0f, 1, 256, "%d", true, RevertPtr))
            {
                CVarNumPoissonDiscSamples->SetAsInt(NumSamples, EConsoleVariableFlags::SetByCode);
            }
        }
    }

    if (ActiveFilterMode == 1)
    {
        // PCSS: Penumbra scale
        if (IConsoleVariable* CVarRadiusScale = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCSS.RadiusScale"))
        {
            float RadiusScale  = Math::Clamp<float>(CVarRadiusScale->GetFloat(), 0.0f, 10.0f);
            float RadiusScale0 = 0.0f;

            const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.CSM.PCSS.RadiusScale", RadiusScale0);
            if (EditorWidgets::DrawFloatProperty("Penumbra scale", RadiusScale, 0.01f, 0.0f, 10.0f, "%.2f", true, RevertPtr))
            {
                CVarRadiusScale->SetAsFloat(RadiusScale, EConsoleVariableFlags::SetByCode);
            }
        }

        // PCSS: Blocker search scale
        if (IConsoleVariable* CVarBlockerSearchScale = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCSS.BlockerSearchScale"))
        {
            float BlockerSearchScale  = Math::Clamp<float>(CVarBlockerSearchScale->GetFloat(), 0.0f, 4.0f);
            float BlockerSearchScale0 = 0.0f;

            const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.CSM.PCSS.BlockerSearchScale", BlockerSearchScale0);
            if (EditorWidgets::DrawFloatProperty("Blocker search scale", BlockerSearchScale, 0.01f, 0.0f, 4.0f, "%.2f", true, RevertPtr))
            {
                CVarBlockerSearchScale->SetAsFloat(BlockerSearchScale, EConsoleVariableFlags::SetByCode);
            }
        }

        // PCSS: Min filter radius
        if (IConsoleVariable* CVarMinFilterRadius = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCSS.MinFilterRadiusTexels"))
        {
            float MinFilterRadius  = Math::Clamp<float>(CVarMinFilterRadius->GetFloat(), 0.0f, 8.0f);
            float MinFilterRadius0 = 0.0f;

            const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.CSM.PCSS.MinFilterRadiusTexels", MinFilterRadius0);
            if (EditorWidgets::DrawFloatProperty("Min filter (texels)", MinFilterRadius, 0.01f, 0.0f, 8.0f, "%.2f", true, RevertPtr))
            {
                CVarMinFilterRadius->SetAsFloat(MinFilterRadius, EConsoleVariableFlags::SetByCode);
            }
        }

        // PCSS: Min filter max angular diameter
        if (IConsoleVariable* CVarMinFilterMaxAngular = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCSS.MinFilterMaxAngularDiameter"))
        {
            float MinFilterMaxAngular  = Math::Clamp<float>(CVarMinFilterMaxAngular->GetFloat(), 0.0f, 6.0f);
            float MinFilterMaxAngular0 = 0.0f;

            const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.CSM.PCSS.MinFilterMaxAngularDiameter", MinFilterMaxAngular0);
            if (EditorWidgets::DrawFloatProperty("Min filter max angular diameter (deg)", MinFilterMaxAngular, 0.01f, 0.0f, 6.0f, "%.2f", true, RevertPtr))
            {
                CVarMinFilterMaxAngular->SetAsFloat(MinFilterMaxAngular, EConsoleVariableFlags::SetByCode);
            }
        }

        // PCSS: Max filter size (world)
        if (IConsoleVariable* CVarMaxPenumbraWorld = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCSS.MaxPenumbraWorld"))
        {
            float MaxPenumbraWorld  = Math::Clamp<float>(CVarMaxPenumbraWorld->GetFloat(), 0.0f, 10.0f);
            float MaxPenumbraWorld0 = 0.0f;

            const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.CSM.PCSS.MaxPenumbraWorld", MaxPenumbraWorld0);
            if (EditorWidgets::DrawFloatProperty("Max filter size (world)", MaxPenumbraWorld, 0.01f, 0.0f, 10.0f, "%.2f", true, RevertPtr))
            {
                CVarMaxPenumbraWorld->SetAsFloat(MaxPenumbraWorld, EConsoleVariableFlags::SetByCode);
            }
        }

        // PCSS: Max sampling distance (world)
        if (IConsoleVariable* CVarMaxSearchDistanceWorld = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCSS.MaxSearchDistanceWorld"))
        {
            float MaxSearchDistanceWorld  = Math::Clamp<float>(CVarMaxSearchDistanceWorld->GetFloat(), 0.0f, 1000.0f);
            float MaxSearchDistanceWorld0 = 0.0f;

            const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.CSM.PCSS.MaxSearchDistanceWorld", MaxSearchDistanceWorld0);
            if (EditorWidgets::DrawFloatProperty("Max sampling distance (world)", MaxSearchDistanceWorld, 0.01f, 0.0f, 1000.0f, "%.2f", true, RevertPtr))
            {
                CVarMaxSearchDistanceWorld->SetAsFloat(MaxSearchDistanceWorld, EConsoleVariableFlags::SetByCode);
            }
        }

        // PCSS: Blocker search angular diameter
        if (IConsoleVariable* CVarBlockerSearchAngular = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCSS.BlockerSearchAngularDiameter"))
        {
            float BlockerSearchAngular  = Math::Clamp<float>(CVarBlockerSearchAngular->GetFloat(), 0.0f, 6.0f);
            float BlockerSearchAngular0 = 0.0f;

            const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.CSM.PCSS.BlockerSearchAngularDiameter", BlockerSearchAngular0);
            if (EditorWidgets::DrawFloatProperty("Blocker search angular diameter (deg)", BlockerSearchAngular, 0.01f, 0.0f, 6.0f, "%.2f", true, RevertPtr))
            {
                CVarBlockerSearchAngular->SetAsFloat(BlockerSearchAngular, EConsoleVariableFlags::SetByCode);
            }
        }

        // PCSS: Blocker sampling clump
        if (IConsoleVariable* CVarBlockerSamplingClump = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCSS.BlockerSamplingClump"))
        {
            float BlockerSamplingClump  = Math::Clamp<float>(CVarBlockerSamplingClump->GetFloat(), 0.0f, 6.0f);
            float BlockerSamplingClump0 = 0.0f;

            const float* RevertPtr = TryGetDefaultPtr(FloatDefaults, "Renderer.CSM.PCSS.BlockerSamplingClump", BlockerSamplingClump0);
            if (EditorWidgets::DrawFloatProperty("Blocker sampling clump (exp)", BlockerSamplingClump, 0.01f, 0.0f, 6.0f, "%.2f", true, RevertPtr))
            {
                CVarBlockerSamplingClump->SetAsFloat(BlockerSamplingClump, EConsoleVariableFlags::SetByCode);
            }
        }

        // PCSS: Blocker sample count
        if (IConsoleVariable* CVarNumBlockerSamples = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCSS.NumBlockerSamples"))
        {
            static const CHAR* const Items[] = { "16", "32", "64", "128" };
            static const int32 Samples[]     = { 16, 32, 64, 128 };

            constexpr int32 ItemCount = static_cast<int32>(ARRAY_COUNT(Items));

            const auto GetIndexForSamples = [](int32 NumSamples)
            {
                if (NumSamples >= 128)
                {
                    return 3;
                }

                if (NumSamples >= 64)
                {
                    return 2;
                }

                if (NumSamples >= 32)
                {
                    return 1;
                }

                return 0;
            };

            int32 ItemIndex  = GetIndexForSamples(CVarNumBlockerSamples->GetInt());
            int32 ItemIndex0 = 0;

            const int32* DefaultSamplesPtr = IntDefaults.Find(FString("Renderer.CSM.PCSS.NumBlockerSamples"));
            const int32* RevertPtr = nullptr;

            if (DefaultSamplesPtr)
            {
                ItemIndex0 = GetIndexForSamples(*DefaultSamplesPtr);
                RevertPtr  = &ItemIndex0;
            }

            if (EditorWidgets::DrawComboProperty("Blocker sample count", ItemIndex, Items, ItemCount, RevertPtr))
            {
                const int32 NewNumSamples = Samples[Math::Clamp<int32>(ItemIndex, 0, ItemCount - 1)];
                CVarNumBlockerSamples->SetAsInt(NewNumSamples, EConsoleVariableFlags::SetByCode);
            }
        }
    }

    // Cascade size
    if (IConsoleVariable* CVarCascadeSize = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.CascadeSize"))
    {
        static const CHAR* const Items[] = { "1024", "2048", "4096", "8192" };
        static const int32 Sizes[]       = { 1024, 2048, 4096, 8192 };

        constexpr int32 ItemCount = static_cast<int32>(ARRAY_COUNT(Items));

        const auto GetIndexForSize = [](int32 InCascadeSize)
        {
            if (InCascadeSize >= 8192)
            {
                return 3;
            }

            if (InCascadeSize >= 4096)
            {
                return 2;
            }

            if (InCascadeSize >= 2048)
            {
                return 1;
            }

            return 0;
        };

        int32 ItemIndex  = GetIndexForSize(CVarCascadeSize->GetInt());
        int32 ItemIndex0 = 0;

        const int32* DefaultSizePtr = IntDefaults.Find(FString("Renderer.CSM.CascadeSize"));
        const int32* RevertPtr      = nullptr;

        if (DefaultSizePtr)
        {
            ItemIndex0 = GetIndexForSize(*DefaultSizePtr);
            RevertPtr  = &ItemIndex0;
        }

        if (EditorWidgets::DrawComboProperty("Cascade size", ItemIndex, Items, ItemCount, RevertPtr))
        {
            const int32 NewCascadeSize = Sizes[Math::Clamp<int32>(ItemIndex, 0, ItemCount - 1)];
            CVarCascadeSize->SetAsInt(NewCascadeSize, EConsoleVariableFlags::SetByCode);
        }
    }

    EditorWidgets::EndPropertyTable();
}

void FEditorRendererSettingsWidget::DrawPointLightShadowSettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsPointLights", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    // Enable point-light shadows
    if (IConsoleVariable* CVarEnablePointLightShadows = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.PointLightShadows"))
    {
        bool bEnablePointLightShadows  = CVarEnablePointLightShadows->GetBool();
        bool bEnablePointLightShadows0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.PointLightShadows", bEnablePointLightShadows0);
        if (EditorWidgets::DrawCheckboxProperty("Enable point-light shadows", bEnablePointLightShadows, RevertPtr))
        {
            CVarEnablePointLightShadows->SetAsBool(bEnablePointLightShadows, EConsoleVariableFlags::SetByCode);
        }
    }

    // Enable single-pass shadow-map generation
    if (IConsoleVariable* CVarEnableSinglePassRendering = FConsoleManager::Get().FindConsoleVariable("Renderer.PointLights.EnableSinglePassRendering"))
    {
        bool bEnableSinglePassRendering  = CVarEnableSinglePassRendering->GetBool();
        bool bEnableSinglePassRendering0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.PointLights.EnableSinglePassRendering", bEnableSinglePassRendering0);
        if (EditorWidgets::DrawCheckboxProperty("Enable single-pass cube-map generation", bEnableSinglePassRendering, RevertPtr))
        {
            CVarEnableSinglePassRendering->SetAsBool(bEnableSinglePassRendering, EConsoleVariableFlags::SetByCode);
        }
    }

    // Enable geometry-shader instancing
    if (IConsoleVariable* CVarEnableGeometryShaderInstancing = FConsoleManager::Get().FindConsoleVariable("Renderer.PointLights.EnableGeometryShaderInstancing"))
    {
        bool bEnableGeometryShaderInstancing  = CVarEnableGeometryShaderInstancing->GetBool();
        bool bEnableGeometryShaderInstancing0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.PointLights.EnableGeometryShaderInstancing", bEnableGeometryShaderInstancing0);
        if (EditorWidgets::DrawCheckboxProperty("Enable geometry-shader instancing", bEnableGeometryShaderInstancing, RevertPtr))
        {
            CVarEnableGeometryShaderInstancing->SetAsBool(bEnableGeometryShaderInstancing, EConsoleVariableFlags::SetByCode);
        }
    }

    // Point-light shadow-map size
    if (IConsoleVariable* CVarPointLightShadowMapSize = FConsoleManager::Get().FindConsoleVariable("Renderer.Shadows.PointLightShadowMapSize"))
    {
        static const CHAR* const Items[] = { "128", "256", "512", "1024" };
        static const int32 Sizes[]       = { 128, 256, 512, 1024 };

        constexpr int32 ItemCount = static_cast<int32>(ARRAY_COUNT(Items));

        const auto GetIndexForSize = [](int32 InShadowMapSize)
        {
            if (InShadowMapSize >= 1024)
            {
                return 3;
            }

            if (InShadowMapSize >= 512)
            {
                return 2;
            }

            if (InShadowMapSize >= 256)
            {
                return 1;
            }

            return 0;
        };

        int32 ItemIndex  = GetIndexForSize(CVarPointLightShadowMapSize->GetInt());
        int32 ItemIndex0 = 0;

        const int32* DefaultSizePtr = IntDefaults.Find(FString("Renderer.Shadows.PointLightShadowMapSize"));
        const int32* RevertPtr      = nullptr;

        if (DefaultSizePtr)
        {
            ItemIndex0 = GetIndexForSize(*DefaultSizePtr);
            RevertPtr  = &ItemIndex0;
        }

        if (EditorWidgets::DrawComboProperty("Shadow-map size", ItemIndex, Items, ItemCount, RevertPtr))
        {
            const int32 NewSize = Sizes[Math::Clamp<int32>(ItemIndex, 0, ItemCount - 1)];
            CVarPointLightShadowMapSize->SetAsInt(NewSize, EConsoleVariableFlags::SetByCode);
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

    // Enable Skybox
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

    // Enable clear before skybox
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

    // Enable SSAO
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

    // Kernel-size
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

    // Radius
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

    // Bias
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

void FEditorRendererSettingsWidget::DrawTAASettings()
{
    if (!EditorWidgets::BeginPropertyTable("##RendererSettingsTAA", RendererSettingsLabelColumnWidth, RendererSettingsRevertColumnWidth))
    {
        return;
    }

    // Enable TemporalAA
    if (IConsoleVariable* CVarEnableTemporalAA = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.TemporalAA"))
    {
        bool bEnableTemporalAA  = CVarEnableTemporalAA->GetBool();
        bool bEnableTemporalAA0 = false;

        const bool* RevertPtr = TryGetDefaultPtr(BoolDefaults, "Renderer.Feature.TemporalAA", bEnableTemporalAA0);
        if (EditorWidgets::DrawCheckboxProperty("Enable TemporalAA", bEnableTemporalAA, RevertPtr))
        {
            CVarEnableTemporalAA->SetAsBool(bEnableTemporalAA, EConsoleVariableFlags::SetByCode);
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

    // Enable FXAA
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

    // Enable FXAA debug
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

    // Enable VSync
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

    // Enable frustum-culling
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

    // Enable debug-draw AABBs
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

    // Enable debug-draw point-lights
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

    // Enable debug-draw light-probes
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

    // Tonemapping Function
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

    // Exposure (EV100)
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

    // ReinhardIntensity
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
