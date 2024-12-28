#include "Core/Misc/ConsoleManager.h"
#include "Engine/Engine.h"
#include "Renderer/Widgets/RendererSettingsWidget.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"

static TAutoConsoleVariable<bool> CVarDrawSettingsWindow(
    "Renderer.DrawSettingsWindow",
    "Enables the Debug RenderTarget-viewer",
    false);

// Same column size for all different types
static constexpr float ColumnWidth = 260.0f;

FRendererSettingsWidget::FRendererSettingsWidget()
    : ImGuiDelegateHandle()
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FRendererSettingsWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FRendererSettingsWidget::~FRendererSettingsWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FRendererSettingsWidget::Draw()
{
    bool bDrawSettingsWindow = CVarDrawSettingsWindow.GetValue();
    if (bDrawSettingsWindow)
    {
        const uint32 WindowWidth  = GEngine->GetEngineWindow()->GetWidth();
        const uint32 WindowHeight = GEngine->GetEngineWindow()->GetHeight();

        const float Width  = FMath::Max(WindowWidth * 0.3f, 400.0f);
        const float Height = WindowHeight * 0.7f;

        ImGui::SetNextWindowPos(ImVec2(float(WindowWidth) * 0.5f, float(WindowHeight) * 0.175f), ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Appearing);

        const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::Begin("Renderer Settings", &bDrawSettingsWindow, Flags))
        {
            // Deferred Rendering Settings
            if (ImGui::CollapsingHeader("Deferred Rendering", ImGuiTreeNodeFlags_None))
            {
                DrawDeferredRenderingSettings();
            }

            // Shadows
            if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_None))
            {
                DrawShadowSettings();
            }

            // Cascaded Shadows
            if (ImGui::CollapsingHeader("Cascaded Shadow Maps", ImGuiTreeNodeFlags_None))
            {
                DrawCascadedShadowSettings();
            }

            // Point-light shadows
            if (ImGui::CollapsingHeader("Point-light Shadow Maps", ImGuiTreeNodeFlags_None))
            {
                DrawPointLightShadowSettings();
            }

            // Skybox
            if (ImGui::CollapsingHeader("Skybox", ImGuiTreeNodeFlags_None))
            {
                DrawSkyboxSettings();
            }

            // Screen-Space Occlusion Settings
            if (ImGui::CollapsingHeader("SSAO", ImGuiTreeNodeFlags_None))
            {
                DrawSSAOSettings();
            }

            // Temporal AA
            if (ImGui::CollapsingHeader("Temporal AA", ImGuiTreeNodeFlags_None))
            {
                DrawTAASettings();
            }

            // FXAA
            if (ImGui::CollapsingHeader("FXAA", ImGuiTreeNodeFlags_None))
            {
                DrawFXAASettings();
            }

            // Other
            if (ImGui::CollapsingHeader("Other", ImGuiTreeNodeFlags_None))
            {
                DrawOtherSettings();
            }

            // Debug
            if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_None))
            {
                DrawDebugSettings();
            }
        }

        ImGui::End();

        CVarDrawSettingsWindow->SetAsBool(bDrawSettingsWindow, EConsoleVariableFlags::SetByCode);
    }
}

void FRendererSettingsWidget::DrawDeferredRenderingSettings()
{
    // Setup the columns
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, ColumnWidth);

    // Draw tile-debug
    if (IConsoleVariable* CVarDrawTiledLightning = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.DrawTiledLightning"))
    {
        ImGui::Text("Draw tile-debug");
        ImGui::NextColumn();

        bool bDrawTiledLightning = CVarDrawTiledLightning->GetBool();
        if (ImGui::Checkbox("##DrawTiledLightning", &bDrawTiledLightning))
        {
            CVarDrawTiledLightning->SetAsBool(bDrawTiledLightning, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Clear all targets
    if (IConsoleVariable* CVarClearAllTargets = FConsoleManager::Get().FindConsoleVariable("Renderer.BasePass.ClearAllTargets"))
    {
        ImGui::Text("Clear all targets");
        ImGui::NextColumn();

        bool bClearAllTargets = CVarClearAllTargets->GetBool();
        if (ImGui::Checkbox("##ClearAllTargets", &bClearAllTargets))
        {
            CVarClearAllTargets->SetAsBool(bClearAllTargets, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Enable pre-pass
    if (IConsoleVariable* CVarEnablePrePass = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.PrePass"))
    {
        ImGui::Text("Enable pre-pass");
        ImGui::NextColumn();

        bool bEnablePrePass = CVarEnablePrePass->GetBool();
        if (ImGui::Checkbox("##EnablePrePass", &bEnablePrePass))
        {
            CVarEnablePrePass->SetAsBool(bEnablePrePass, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Enable pre-pass depth-reduce
    if (IConsoleVariable* CVarEnablePrePassDepthReduce = FConsoleManager::Get().FindConsoleVariable("Renderer.PrePass.DepthReduce"))
    {
        ImGui::Text("Enable pre-pass depth-reduce");
        ImGui::NextColumn();

        bool bEnablePrePassDepthReduce = CVarEnablePrePassDepthReduce->GetBool();
        if (ImGui::Checkbox("##EnablePrePassDepthReduce", &bEnablePrePassDepthReduce))
        {
            CVarEnablePrePassDepthReduce->SetAsBool(bEnablePrePassDepthReduce, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Enable base-pass
    if (IConsoleVariable* CVarEnableBasePass = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.BasePass"))
    {
        ImGui::Text("Enable base-pass");
        ImGui::NextColumn();

        bool bEnableBasePass = CVarEnableBasePass->GetBool();
        if (ImGui::Checkbox("##EnableBasePass", &bEnableBasePass))
        {
            CVarEnableBasePass->SetAsBool(bEnableBasePass, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Reset the columns
    ImGui::Columns(1);
}

void FRendererSettingsWidget::DrawShadowSettings()
{
    // Setup the columns
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, ColumnWidth);

    // Enable shadows
    if (IConsoleVariable* CVarEnableShadows = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.Shadows"))
    {
        ImGui::Text("Enable shadows");
        ImGui::NextColumn();

        bool bEnableShadows = CVarEnableShadows->GetBool();
        if (ImGui::Checkbox("##EnableShadows", &bEnableShadows))
        {
            CVarEnableShadows->SetAsBool(bEnableShadows, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Reset the columns
    ImGui::Columns(1);
}

void FRendererSettingsWidget::DrawCascadedShadowSettings()
{
    // Setup the columns
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, ColumnWidth);

    // Draw cascades
    if (IConsoleVariable* CVarDrawCascades = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.DrawCascades"))
    {
        ImGui::Text("Draw cascades");
        ImGui::NextColumn();

        bool bDrawCascades = CVarDrawCascades->GetBool();
        if (ImGui::Checkbox("##DrawCascades", &bDrawCascades))
        {
            CVarDrawCascades->SetAsBool(bDrawCascades, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Enable shadow-mask
    if (IConsoleVariable* CVarEnableShadowMask = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.ShadowMask"))
    {
        ImGui::Text("Enable shadow-mask");
        ImGui::NextColumn();

        bool bEnableShadowMask = CVarEnableShadowMask->GetBool();
        if (ImGui::Checkbox("##EnableShadowMask", &bEnableShadowMask))
        {
            CVarEnableShadowMask->SetAsBool(bEnableShadowMask, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Enable sun-shadows
    if (IConsoleVariable* CVarEnableSunShadows = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.SunShadows"))
    {
        ImGui::Text("Enable sun-shadows");
        ImGui::NextColumn();

        bool bEnableSunShadows = CVarEnableSunShadows->GetBool();
        if (ImGui::Checkbox("##EnableSunShadows", &bEnableSunShadows))
        {
            CVarEnableSunShadows->SetAsBool(bEnableSunShadows, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Enable geometry-shader instancing
    if (IConsoleVariable* CVarEnableGeometryShaderInstancing = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.EnableGeometryShaderInstancing"))
    {
        ImGui::Text("Enable geometry-shader instancing");
        ImGui::NextColumn();

        bool bEnableGeometryShaderInstancing = CVarEnableGeometryShaderInstancing->GetBool();
        if (ImGui::Checkbox("##EnableGeometryShaderInstancing", &bEnableGeometryShaderInstancing))
        {
            CVarEnableGeometryShaderInstancing->SetAsBool(bEnableGeometryShaderInstancing, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Enable view instancing
    if (IConsoleVariable* CVarEnableViewInstancing = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.EnableViewInstancing"))
    {
        ImGui::Text("Enable view instancing");
        ImGui::NextColumn();

        bool bEnableViewInstancing = CVarEnableViewInstancing->GetBool();
        if (ImGui::Checkbox("##EnableViewInstancing", &bEnableViewInstancing))
        {
            CVarEnableViewInstancing->SetAsBool(bEnableViewInstancing, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Rotate samples
    if (IConsoleVariable* CVarEnableRotateSamples = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.RotateSamples"))
    {
        ImGui::Text("Enable rotate samples");
        ImGui::NextColumn();

        bool bEnableRotateSamples = CVarEnableRotateSamples->GetBool();
        if (ImGui::Checkbox("##RotateSamples", &bEnableRotateSamples))
        {
            CVarEnableRotateSamples->SetAsBool(bEnableRotateSamples, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Blend cascades
    if (IConsoleVariable* CVarEnableBlendCascades = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.BlendCascades"))
    {
        ImGui::Text("Enable blend cascades");
        ImGui::NextColumn();

        bool bEnableBlendCascades = CVarEnableBlendCascades->GetBool();
        if (ImGui::Checkbox("##BlendCascades", &bEnableBlendCascades))
        {
            CVarEnableBlendCascades->SetAsBool(bEnableBlendCascades, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Select cascade from projection
    if (IConsoleVariable* CVarSelectCascadeFromProjection = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.SelectCascadeFromProjection"))
    {
        ImGui::Text("Select cascade from projection");
        ImGui::NextColumn();

        bool bSelectCascadeFromProjection = CVarSelectCascadeFromProjection->GetBool();
        if (ImGui::Checkbox("##SelectCascadeFromProjection", &bSelectCascadeFromProjection))
        {
            CVarSelectCascadeFromProjection->SetAsBool(bSelectCascadeFromProjection, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Filter function
    if (IConsoleVariable* CVarFilterFunction = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.FilterFunction"))
    {
        ImGui::Text("Enable view instancing");
        ImGui::NextColumn();

        int32 FilterFunction = CVarFilterFunction->GetInt();

        const char* Items[] = 
        {
            "Grid PCF",
            "Poisson Disc PCF"
        };

        constexpr uint32 ItemSize = ARRAY_COUNT(Items);
        if (ImGui::Combo("##FilterFunction", &FilterFunction, Items, ItemSize))
        {
            CVarFilterFunction->SetAsInt(FilterFunction, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Filter size
    if (IConsoleVariable* CVarFilterSize = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.FilterSize"))
    {
        ImGui::Text("Filter size");
        ImGui::NextColumn();

        int32 FilterSize = CVarFilterSize->GetInt();
        if (ImGui::SliderInt("##FilterSize", &FilterSize, 16, 256, "%d"))
        {
            CVarFilterSize->SetAsInt(FilterSize, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Max filter size
    if (IConsoleVariable* CVarMaxFilterSize = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.MaxFilterSize"))
    {
        ImGui::Text("Max filter-size");
        ImGui::NextColumn();

        int32 MaxFilterSize = CVarMaxFilterSize->GetInt();
        if (ImGui::SliderInt("##MaxFilterSize", &MaxFilterSize, 256, 1024, "%d"))
        {
            CVarMaxFilterSize->SetAsInt(MaxFilterSize, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Num poisson-disc samples
    if (IConsoleVariable* CVarNumPoissonDiscSamples = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.NumPoissonDiscSamples"))
    {
        ImGui::Text("Num poisson-disc samples");
        ImGui::NextColumn();

        const char* Items[] =
        {
            "16", "32", "64", "128"
        };

        const int32 Samples[] = 
        {
            16, 32, 64, 128,
        };

        const int32 NumPoissonDiscSamples = CVarNumPoissonDiscSamples->GetInt();

        int32 ItemIndex = [](int32 NumSamples)
        {
            if (NumSamples >= 128)
            {
                return 3;
            }
            else if (NumSamples >= 64)
            {
                return 2;
            }
            else if (NumSamples >= 32)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }(NumPoissonDiscSamples);

        constexpr int32 ItemCount = ARRAY_COUNT(Items);
        if (ImGui::Combo("##NumPoissonDiscSamples", &ItemIndex, Items, ItemCount))
        {
            const int32 NewNumPoissonDiscSamples = Samples[ItemIndex];
            CVarNumPoissonDiscSamples->SetAsInt(NewNumPoissonDiscSamples, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Cascade size
    if (IConsoleVariable* CVarCascadeSize = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.CascadeSize"))
    {
        ImGui::Text("Cascade size");
        ImGui::NextColumn();

        const char* Items[] =
        {
            "1024", "2048", "4096", "8192"
        };

        const int32 Sizes[] =
        {
            1024, 2048, 4096, 8192,
        };

        const int32 CascadeSize = CVarCascadeSize->GetInt();

        int32 ItemIndex = [](int32 InCascadeSize)
        {
            if (InCascadeSize >= 8192)
            {
                return 3;
            }
            else if (InCascadeSize >= 4096)
            {
                return 2;
            }
            else if (InCascadeSize >= 2048)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }(CascadeSize);

        constexpr int32 ItemCount = ARRAY_COUNT(Items);
        if (ImGui::Combo("##CascadeSize", &ItemIndex, Items, ItemCount))
        {
            const int32 NewCascadeSize = Sizes[ItemIndex];
            CVarCascadeSize->SetAsInt(NewCascadeSize, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Reset the columns
    ImGui::Columns(1);
}

void FRendererSettingsWidget::DrawPointLightShadowSettings()
{
    // Setup the columns
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, ColumnWidth);

    // Enable point-light shadows
    if (IConsoleVariable* CVarEnablePointLightShadows = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.PointLightShadows"))
    {
        ImGui::Text("Enable point-light shadows");
        ImGui::NextColumn();

        bool bEnablePointLightShadows = CVarEnablePointLightShadows->GetBool();
        if (ImGui::Checkbox("##EnablePointLightShadows", &bEnablePointLightShadows))
        {
            CVarEnablePointLightShadows->SetAsBool(bEnablePointLightShadows, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Enable geometry-shader instancing
    if (IConsoleVariable* CVarEnableGeometryShaderInstancing = FConsoleManager::Get().FindConsoleVariable("Renderer.PointLights.EnableGeometryShaderInstancing"))
    {
        ImGui::Text("Enable geometry-shader instancing");
        ImGui::NextColumn();

        bool bEnableGeometryShaderInstancing = CVarEnableGeometryShaderInstancing->GetBool();
        if (ImGui::Checkbox("##EnableGeometryShaderInstancing", &bEnableGeometryShaderInstancing))
        {
            CVarEnableGeometryShaderInstancing->SetAsBool(bEnableGeometryShaderInstancing, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Point-light shadow-map size
    if (IConsoleVariable* CVarPointLightShadowMapSize = FConsoleManager::Get().FindConsoleVariable("Renderer.Shadows.PointLightShadowMapSize"))
    {
        ImGui::Text("Point-light shadow-map size");
        ImGui::NextColumn();

        const char* Items[] =
        {
            "128", "256", "512", "1024"
        };

        const int32 Sizes[] =
        {
            128, 256, 512, 1024,
        };

        const int32 PointLightShadowMapSize = CVarPointLightShadowMapSize->GetInt();

        int32 ItemIndex = [](int32 InPointLightShadowMapSize)
        {
            if (InPointLightShadowMapSize >= 1024)
            {
                return 3;
            }
            else if (InPointLightShadowMapSize >= 512)
            {
                return 2;
            }
            else if (InPointLightShadowMapSize >= 256)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }(PointLightShadowMapSize);

        constexpr int32 ItemCount = ARRAY_COUNT(Items);
        if (ImGui::Combo("##PointLightShadowMapSize", &ItemIndex, Items, ItemCount))
        {
            const int32 NewCascadeSize = Sizes[ItemIndex];
            CVarPointLightShadowMapSize->SetAsInt(NewCascadeSize, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Reset the columns
    ImGui::Columns(1);
}

void FRendererSettingsWidget::DrawSkyboxSettings()
{
    // Setup the columns
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, ColumnWidth);

    // Enable Skybox
    if (IConsoleVariable* CVarEnableSkybox = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.Skybox"))
    {
        ImGui::Text("Enable Skybox");
        ImGui::NextColumn();

        bool bEnableSSAO = CVarEnableSkybox->GetBool();
        if (ImGui::Checkbox("##EnableSkybox", &bEnableSSAO))
        {
            CVarEnableSkybox->SetAsBool(bEnableSSAO, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Enable clear before skybox
    if (IConsoleVariable* CVarEnableClearBeforeSkybox = FConsoleManager::Get().FindConsoleVariable("Renderer.Skybox.ClearBeforeSkybox"))
    {
        ImGui::Text("Enable clear before skybox");
        ImGui::NextColumn();

        bool bEnableClearBeforeSkybox = CVarEnableClearBeforeSkybox->GetBool();
        if (ImGui::Checkbox("##EnableClearBeforeSkybox", &bEnableClearBeforeSkybox))
        {
            CVarEnableClearBeforeSkybox->SetAsBool(bEnableClearBeforeSkybox, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Reset the columns
    ImGui::Columns(1);
}

void FRendererSettingsWidget::DrawSSAOSettings()
{
    // Setup the columns
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, ColumnWidth);

    // Enable SSAO
    if (IConsoleVariable* CVarEnableSSAO = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.SSAO"))
    {
        ImGui::Text("Enable SSAO");
        ImGui::NextColumn();

        bool bEnableSSAO = CVarEnableSSAO->GetBool();
        if (ImGui::Checkbox("##EnableSSAO", &bEnableSSAO))
        {
            CVarEnableSSAO->SetAsBool(bEnableSSAO, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Kernel-size
    if (IConsoleVariable* CVarKernelSize = FConsoleManager::Get().FindConsoleVariable("Renderer.SSAO.KernelSize"))
    {
        ImGui::Text("Kernel-size");
        ImGui::NextColumn();

        int32 KernelSize = CVarKernelSize->GetInt();
        if (ImGui::SliderInt("##KernelSize", &KernelSize, 1, 128, "%d"))
        {
            CVarKernelSize->SetAsInt(KernelSize, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Radius
    if (IConsoleVariable* CVarRadius = FConsoleManager::Get().FindConsoleVariable("Renderer.SSAO.Radius"))
    {
        ImGui::Text("Radius");
        ImGui::NextColumn();

        float Radius = CVarRadius->GetFloat();
        if (ImGui::SliderFloat("##Radius", &Radius, 0.01f, 1.0f, "%.2f"))
        {
            CVarRadius->SetAsFloat(Radius, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Bias
    if (IConsoleVariable* CVarBias = FConsoleManager::Get().FindConsoleVariable("Renderer.SSAO.Bias"))
    {
        ImGui::Text("Bias");
        ImGui::NextColumn();

        float Bias = CVarBias->GetFloat();
        if (ImGui::SliderFloat("##Bias", &Bias, 0.01f, 1.0f, "%.2f"))
        {
            CVarBias->SetAsFloat(Bias, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Reset the columns
    ImGui::Columns(1);
}

void FRendererSettingsWidget::DrawTAASettings()
{
    // Setup the columns
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, ColumnWidth);

    // Enable TemporalAA
    if (IConsoleVariable* CVarEnableTemporalAA = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.TemporalAA"))
    {
        ImGui::Text("Enable TemporalAA");
        ImGui::NextColumn();

        bool bEnableTemporalAA = CVarEnableTemporalAA->GetBool();
        if (ImGui::Checkbox("##EnableTemporalAA", &bEnableTemporalAA))
        {
            CVarEnableTemporalAA->SetAsBool(bEnableTemporalAA, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Reset the columns
    ImGui::Columns(1);
}

void FRendererSettingsWidget::DrawFXAASettings()
{
    // Setup the columns
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, ColumnWidth);

    // Enable FXAA
    if (IConsoleVariable* CVarEnableFXAA = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.FXAA"))
    {
        ImGui::Text("Enable FXAA");
        ImGui::NextColumn();

        bool bEnableFXAA = CVarEnableFXAA->GetBool();
        if (ImGui::Checkbox("##EnableFXAA", &bEnableFXAA))
        {
            CVarEnableFXAA->SetAsBool(bEnableFXAA, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Enable FXAA debug
    if (IConsoleVariable* CVarEnableFXAADebug = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.FXAADebug"))
    {
        ImGui::Text("Enable FXAA debug");
        ImGui::NextColumn();

        bool bEnableFXAADebug = CVarEnableFXAADebug->GetBool();
        if (ImGui::Checkbox("##EnableFXAADebug", &bEnableFXAADebug))
        {
            CVarEnableFXAADebug->SetAsBool(bEnableFXAADebug, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Reset the columns
    ImGui::Columns(1);
}

void FRendererSettingsWidget::DrawOtherSettings()
{
    // Setup the columns
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, ColumnWidth);

    // Enable VSync
    if (IConsoleVariable* CVarEnableVSync = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.VerticalSync"))
    {
        ImGui::Text("Enable VSync");
        ImGui::NextColumn();

        bool bEnableVSync = CVarEnableVSync->GetBool();
        if (ImGui::Checkbox("##EnableVSync", &bEnableVSync))
        {
            CVarEnableVSync->SetAsBool(bEnableVSync, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Enable frustum-culling
    if (IConsoleVariable* CVarEnableFrustumCulling = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.FrustumCulling"))
    {
        ImGui::Text("Enable frustum-culling");
        ImGui::NextColumn();

        bool bEnableFrustumCulling = CVarEnableFrustumCulling->GetBool();
        if (ImGui::Checkbox("##EnableFrustumCulling", &bEnableFrustumCulling))
        {
            CVarEnableFrustumCulling->SetAsBool(bEnableFrustumCulling, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Reset the columns
    ImGui::Columns(1);
}

void FRendererSettingsWidget::DrawDebugSettings()
{
    // Setup the columns
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, ColumnWidth);

    // Enable debug-draw AABBs
    if (IConsoleVariable* CVarEnableDebugDrawAABBs = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.DrawAABBs"))
    {
        ImGui::Text("Enable debug-draw AABBs");
        ImGui::NextColumn();

        bool bEnableDebugDrawAABBs = CVarEnableDebugDrawAABBs->GetBool();
        if (ImGui::Checkbox("##EnableDebugDrawAABBs", &bEnableDebugDrawAABBs))
        {
            CVarEnableDebugDrawAABBs->SetAsBool(bEnableDebugDrawAABBs, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Enable debug-draw point-lights
    if (IConsoleVariable* CVarEnableDebugDrawPointLights = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.DrawPointLights"))
    {
        ImGui::Text("Enable debug-draw point-lights");
        ImGui::NextColumn();

        bool bEnableDebugDrawPointLights = CVarEnableDebugDrawPointLights->GetBool();
        if (ImGui::Checkbox("##EnableDebugDrawPointLights", &bEnableDebugDrawPointLights))
        {
            CVarEnableDebugDrawPointLights->SetAsBool(bEnableDebugDrawPointLights, EConsoleVariableFlags::SetByCode);
        }

        ImGui::NextColumn();
    }

    // Reset the columns
    ImGui::Columns(1);
}
