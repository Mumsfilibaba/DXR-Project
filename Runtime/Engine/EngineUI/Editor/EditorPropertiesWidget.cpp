#include "Engine/EditorEngine.h"
#include "Engine/World/Components/StaticMeshComponent.h"
#include "Engine/EngineUI/Editor/EditorPropertiesWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Core/Containers/StaticArray.h"
#include "ImGuiPlugin/ImGuiCore.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include "ImGuiPlugin/ImGuiExtensions.h"

FEditorPropertiesWidget::FEditorPropertiesWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , ImGuiDelegateHandle()
    , bVisible(true)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorPropertiesWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorPropertiesWidget::~FEditorPropertiesWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDrawDelegate(ImGuiDelegateHandle);
    }
}

void FEditorPropertiesWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    if (!EditorEngine)
    {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, EditorStyleVars::PropertiesItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, EditorStyleVars::PropertiesWindowPadding);

    if (ImGui::Begin("Properties", &bVisible))
    {
        DrawWindowContents();
    }
    
    ImGui::End();

    ImGui::PopStyleVar(2);
}

void FEditorPropertiesWidget::DrawWindowContents()
{
    FActor*      SelectedActor      = EditorEngine->GetSelectedActor();
    FLight*      SelectedLight      = EditorEngine->GetSelectedLight();
    FCamera*     SelectedCamera     = EditorEngine->GetSelectedCamera();
    FLightProbe* SelectedLightProbe = EditorEngine->GetSelectedLightProbe();

    if (!SelectedActor && !SelectedLight && !SelectedCamera && !SelectedLightProbe)
    {
        ImGui::TextDisabled("No selection");
        return;
    }

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

                const float Alpha01  = Math::Clamp(Style.Alpha, 0.0f, 1.0f);
                const int32 Alpha255 = static_cast<int32>(Alpha01 * 255.0f);
                const ImU32 Color    = IM_COL32(26, 26, 26, Alpha255);

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

    static constexpr float LabelColumnWidth  = 128.0f;
    static constexpr float RevertColumnWidth = 28.0f;

    // ------------------------------------------------------------
    // Actor
    // ------------------------------------------------------------

    if (SelectedActor)
    {
        ImGui::PushID(SelectedActor);

        const FString& ActorName = SelectedActor->GetName();
        DrawLabelWithSeperator(ActorName.IsEmpty() ? "Actor" : *ActorName);

        FStaticMeshComponent* MeshComponent = SelectedActor->GetComponentOfType<FStaticMeshComponent>();
        
        const bool bHasMeshComponent = MeshComponent != nullptr;
        if (DrawCollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen, bHasMeshComponent))
        {
            if (EditorWidgets::BeginPropertyTable("##ActorTransformTable", LabelColumnWidth, RevertColumnWidth))
            {
                // Translation
                {
                    FVector3 Translation = SelectedActor->GetTransform().GetTranslation();
                    const FVector3 TranslationRevert = FVector3(0.0f, 0.0f, 0.0f);

                    if (EditorWidgets::DrawFloat3Control("Translation", Translation, 0.0f, &TranslationRevert, EVector3ControlType::Position))
                    {
                        SelectedActor->GetTransform().SetTranslation(Translation);
                    }
                }

                // Rotation (degrees UI)
                {
                    FVector3 Rotation = SelectedActor->GetTransform().GetRotation();
                    Rotation = FVector3::RadiansToDegrees(Rotation);

                    const FVector3 RotationRevert = FVector3(0.0f, 0.0f, 0.0f);

                    if (EditorWidgets::DrawFloat3Control("Rotation", Rotation, 0.0f, &RotationRevert, EVector3ControlType::RotationDegrees))
                    {
                        const FVector3 Radians = FVector3::DegreesToRadians(Rotation);
                        SelectedActor->GetTransform().SetRotation(Radians);
                    }
                }

                // Scale
                {
                    FVector3 Scale = SelectedActor->GetTransform().GetScale();
                    const FVector3 ScaleRevert = FVector3(1.0f, 1.0f, 1.0f);

                    const bool bScaleChanged = EditorWidgets::DrawFloat3Control("Scale", Scale, 1.0f, &ScaleRevert, EVector3ControlType::Scale);
                    if (bScaleChanged)
                    {
                        SelectedActor->GetTransform().SetScale(Scale);
                    }
                }

                EditorWidgets::EndPropertyTable();
            }
        }

        // MeshComponent
        if (bHasMeshComponent)
        {
            if (DrawCollapsingHeader("MeshComponent", ImGuiTreeNodeFlags_DefaultOpen, false))
            {
                if (EditorWidgets::BeginPropertyTable("##MeshComponentMaterialTable", LabelColumnWidth, RevertColumnWidth))
                {
                    FMaterialInfo MaterialInfo = MeshComponent->GetMaterial()->GetMaterialInfo();
                    const FMaterialInfo MaterialOriginal = MaterialInfo;

                    // Albedo
                    {
                        FFloatColor Albedo = MaterialInfo.Albedo;
                        const FFloatColor Albedo0 = FFloatColor::White;

                        if (EditorWidgets::DrawColor3Property("Albedo", Albedo, Albedo0))
                        {
                            MeshComponent->GetMaterial()->SetAlbedo(Albedo);
                        }
                    }

                    // Roughness
                    {
                        float Roughness = MaterialInfo.Roughness;
                        const float Roughness0 = 0.0f;

                        if (EditorWidgets::DrawFloatProperty("Roughness", Roughness, 0.01f, 0.01f, 1.0f, "%.2f", true, &Roughness0))
                        {
                            MeshComponent->GetMaterial()->SetRoughness(Roughness);
                        }
                    }

                    // Metallic
                    {
                        float Metallic = MaterialInfo.Metallic;
                        const float Metallic0 = 0.0f;

                        if (EditorWidgets::DrawFloatProperty("Metallic", Metallic, 0.01f, 0.01f, 1.0f, "%.2f", true, &Metallic0))
                        {
                            MeshComponent->GetMaterial()->SetMetallic(Metallic);
                        }
                    }

                    // AO
                    {
                        float AO = MaterialInfo.AmbientOcclusion;
                        const float AO0 = 1.0f;
                        
                        if (EditorWidgets::DrawFloatProperty("AO", AO, 0.01f, 0.01f, 1.0f, "%.2f", true, &AO0))
                        {
                            MeshComponent->GetMaterial()->SetAmbientOcclusion(AO);
                        }
                    }

                    // Parallax height scale
                    if (MeshComponent->GetMaterial()->HasHeightMap())
                    {
                        float ParallaxHeightScale = MaterialInfo.ParallaxHeightScale;
                        const float ParallaxHeightScale0 = 0.03f;

                        if (EditorWidgets::DrawFloatProperty("Parallax Height Scale", ParallaxHeightScale, 0.001f, 0.0f, 0.2f, "%.3f", true, &ParallaxHeightScale0))
                        {
                            MeshComponent->GetMaterial()->SetParallaxHeightScale(ParallaxHeightScale);
                        }

                        float ParallaxMinLayers = MaterialInfo.ParallaxMinLayers;
                        const float ParallaxMinLayers0 = 32.0f;
                        if (EditorWidgets::DrawFloatProperty("Parallax Min Layers", ParallaxMinLayers, 1.0f, 1.0f, 128.0f, "%.0f", true, &ParallaxMinLayers0))
                        {
                            MeshComponent->GetMaterial()->SetParallaxLayers(ParallaxMinLayers, MeshComponent->GetMaterial()->GetParallaxMaxLayers());
                        }

                        float ParallaxMaxLayers = MaterialInfo.ParallaxMaxLayers;
                        const float ParallaxMaxLayers0 = 64.0f;
                        if (EditorWidgets::DrawFloatProperty("Parallax Max Layers", ParallaxMaxLayers, 1.0f, 1.0f, 256.0f, "%.0f", true, &ParallaxMaxLayers0))
                        {
                            MeshComponent->GetMaterial()->SetParallaxLayers(MeshComponent->GetMaterial()->GetParallaxMinLayers(), ParallaxMaxLayers);
                        }
                    }

                    EditorWidgets::EndPropertyTable();
                }
            }
        }

        ImGui::PopID();
        return;
    }

    // ------------------------------------------------------------
    // Lights
    // ------------------------------------------------------------

    if (SelectedLight)
    {
        ImGui::PushID(SelectedLight);

        // Point light
        if (FPointLight* Point = Cast<FPointLight>(SelectedLight))
        {
            DrawLabelWithSeperator("PointLight");

            if (DrawCollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen, true))
            {
                if (EditorWidgets::BeginPropertyTable("##PointLightSettingsTable", LabelColumnWidth, RevertColumnWidth))
                {
                    FVector3 Color = Point->GetColor();
                    const FVector3 Color0 = FVector3(1.0f, 1.0f, 1.0f);
                    
                    if (EditorWidgets::DrawColor3Property("Color", Color, Color0))
                    {
                        Point->SetColor(Color);
                    }

                    float Intensity = Point->GetIntensity();
                    const float Intensity0 = 1.0f;
                    
                    if (EditorWidgets::DrawFloatProperty("Intensity (Lumen)", Intensity, 0.1f, 0.0f, 200000.0f, "%.1f", true, &Intensity0))
                    {
                        Point->SetIntensity(Intensity);
                    }

                    EditorWidgets::EndPropertyTable();
                }
            }

            if (DrawCollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen, true))
            {
                if (EditorWidgets::BeginPropertyTable("##PointLightTransformTable", LabelColumnWidth, RevertColumnWidth))
                {
                    FVector3 Translation = Point->GetPosition();
                    const FVector3 Translation0 = FVector3(0.0f, 0.0f, 0.0f);
                    
                    if (EditorWidgets::DrawFloat3Control("Translation", Translation, 0.0f, &Translation0, EVector3ControlType::Position))
                    {
                        Point->SetPosition(Translation);
                    }

                    EditorWidgets::EndPropertyTable();
                }
            }

            if (DrawCollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen, false))
            {
                if (EditorWidgets::BeginPropertyTable("##PointLightShadowsTable", LabelColumnWidth, RevertColumnWidth))
                {
                    float ShadowBias = Point->GetShadowBias();
                    const float ShadowBias0 = 0.005f;
                    
                    if (EditorWidgets::DrawFloatProperty("Shadow-bias", ShadowBias, 0.0001f, 0.0001f, 0.1f, "%.4f", true, &ShadowBias0))
                    {
                        Point->SetShadowBias(ShadowBias);
                    }

                    float ShadowNearPlane = Point->GetShadowNearPlane();
                    const float ShadowNearPlane0 = 1.0f;
                    
                    if (EditorWidgets::DrawFloatProperty("Shadow near-plane", ShadowNearPlane, 0.01f, 0.01f, 1.0f, "%.2f", true, &ShadowNearPlane0))
                    {
                        Point->SetShadowNearPlane(ShadowNearPlane);
                    }

                    float ShadowFarPlane = Point->GetShadowFarPlane();
                    const float ShadowFarPlane0 = 30.0f;

                    if (EditorWidgets::DrawFloatProperty("Shadow far-plane", ShadowFarPlane, 1.0f, 1.0f, 100.0f, "%.1f", true, &ShadowFarPlane0))
                    {
                        Point->SetShadowFarPlane(ShadowFarPlane);
                    }

                    EditorWidgets::EndPropertyTable();
                }
            }
        }
        else if (FDirectionalLight* Dir = Cast<FDirectionalLight>(SelectedLight))
        {
            DrawLabelWithSeperator("DirectionalLight");

            if (DrawCollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen, true))
            {
                if (EditorWidgets::BeginPropertyTable("##DirLightSettingsTable", LabelColumnWidth, RevertColumnWidth))
                {
                    FVector3 Color = Dir->GetColor();
                    const FVector3 Color0 = FVector3(1.0f, 1.0f, 1.0f);

                    if (EditorWidgets::DrawColor3Property("Color", Color, Color0))
                    {
                        Dir->SetColor(Color);
                    }

                    float Intensity = Dir->GetIntensity();
                    const float Intensity0 = 1.0f;

                    if (EditorWidgets::DrawFloatProperty("Intensity (Lux)", Intensity, 0.1f, 0.0f, 200000.0f, "%.1f", true, &Intensity0))
                    {
                        Dir->SetIntensity(Intensity);
                    }

                    EditorWidgets::EndPropertyTable();
                }
            }

            if (DrawCollapsingHeader("Direction", ImGuiTreeNodeFlags_DefaultOpen, true))
            {
                if (EditorWidgets::BeginPropertyTable("##DirLightDirectionTable", LabelColumnWidth, RevertColumnWidth))
                {
                    FVector3 Rotation = Dir->GetRotation();
                    
                    float RotationTheta = Math::RadiansToDegrees(Rotation.X);
                    float RotationPhi   = Math::RadiansToDegrees(Rotation.Y);

                    const float RotationTheta0 = 0.0f;
                    const float RotationPhi0   = 0.0f;

                    bool bSetRotation = false;
                    bSetRotation |= EditorWidgets::DrawFloatProperty("Rotation theta (degrees)", RotationTheta, 0.25f, -90.0f, 90.0f, "%.2f", true, &RotationTheta0);
                    bSetRotation |= EditorWidgets::DrawFloatProperty("Rotation phi (degrees)", RotationPhi, 0.25f, 0.0f, 360.0f, "%.2f", true, &RotationPhi0);

                    if (bSetRotation)
                    {
                        Rotation.X = Math::DegreesToRadians(RotationTheta);
                        Rotation.Y = Math::DegreesToRadians(RotationPhi);
                        Dir->SetRotation(Rotation);
                    }

                    const FVector3 Direction = Dir->GetDirectionVector();
                    EditorWidgets::DrawReadOnlyFloat3Property("Direction", Direction);

                    EditorWidgets::EndPropertyTable();
                }
            }

            if (DrawCollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen, false))
            {
                if (EditorWidgets::BeginPropertyTable("##DirLightShadowsTable", LabelColumnWidth, RevertColumnWidth))
                {
                    float ShadowBias = Dir->GetShadowBias();
                    const float ShadowBias0 = 0.0005f;
                    
                    if (EditorWidgets::DrawFloatProperty("Shadow-bias", ShadowBias, 0.0001f, 0.0001f, 0.1f, "%.4f", true, &ShadowBias0))
                    {
                        Dir->SetShadowBias(ShadowBias);
                    }

                    float Lambda = Dir->GetCascadeSplitLambda();
                    const float Lambda0 = 0.60f;
                    
                    if (EditorWidgets::DrawFloatProperty("Cascade Split Lambda", Lambda, 0.01f, 0.0f, 1.0f, "%.2f", true, &Lambda0))
                    {
                        Dir->SetCascadeSplitLambda(Lambda);
                    }

                    float LightArea = Dir->GetLightArea();
                    const float LightArea0 = 0.5f;

                    if (EditorWidgets::DrawFloatProperty("Angular diameter (deg)", LightArea, 0.05f, 0.0f, 10.0f, "%.2f", true, &LightArea0))
                    {
                        Dir->SetLightArea(LightArea);
                    }

                    EditorWidgets::EndPropertyTable();
                }
            }
        }
        else
        {
            ImGui::TextDisabled("Unknown light type");
        }

        ImGui::PopID();
        return;
    }

    // ------------------------------------------------------------
    // Camera
    // ------------------------------------------------------------

    if (SelectedCamera)
    {
        ImGui::PushID(SelectedCamera);

        DrawLabelWithSeperator("Camera");

        if (DrawCollapsingHeader("Projection", ImGuiTreeNodeFlags_DefaultOpen, true))
        {
            if (EditorWidgets::BeginPropertyTable("##CameraProjectionTable", LabelColumnWidth, RevertColumnWidth))
            {
                {
                    TStaticArray<CHAR, 64> ViewportText{};
                    FCString::Snprintf(ViewportText.Data(), static_cast<int32>(ViewportText.Size()), "%.1f x %.1f", SelectedCamera->GetWidth(), SelectedCamera->GetHeight());
                    EditorWidgets::DrawTextProperty("Viewport size", ViewportText.Data());
                }

                {
                    float FieldOfView = SelectedCamera->GetFieldOfView();
                    const float FieldOfView0 = 60.0f;
                    
                    if (EditorWidgets::DrawFloatProperty("Field Of View", FieldOfView, 0.1f, 40.0f, 120.0f, "%.1f", true, &FieldOfView0))
                    {
                        SelectedCamera->SetFieldOfView(FieldOfView);
                    }
                }

                {
                    float NearPlane = SelectedCamera->GetNearPlane();
                    const float NearPlane0 = 0.05f;

                    if (EditorWidgets::DrawFloatProperty("Near Plane", NearPlane, 0.001f, 0.001f, 1000.0f, "%.3f", true, &NearPlane0))
                    {
                        SelectedCamera->SetNearPlane(NearPlane);
                    }
                }

                {
                    float FarPlane = SelectedCamera->GetFarPlane();
                    const float FarPlane0 = 200.0f;

                    if (EditorWidgets::DrawFloatProperty("Far Plane", FarPlane, 1.0f, 1.0f, 100000.0f, "%.1f", true, &FarPlane0))
                    {
                        SelectedCamera->SetFarPlane(FarPlane);
                    }
                }

                EditorWidgets::EndPropertyTable();
            }
        }

        if (DrawCollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen, false))
        {
            if (EditorWidgets::BeginPropertyTable("##CameraTransformTable", LabelColumnWidth, RevertColumnWidth))
            {
                FVector3 Position = SelectedCamera->GetPosition();
                const FVector3 Position0 = FVector3(0.0f, 0.0f, 0.0f);
                
                if (EditorWidgets::DrawFloat3Control("Position", Position, 0.0f, &Position0, EVector3ControlType::Position))
                {
                    SelectedCamera->SetPosition(Position.X, Position.Y, Position.Z);
                }

                FVector3 Rotation = SelectedCamera->GetRotation();
                Rotation = FVector3::RadiansToDegrees(Rotation);
                
                const FVector3 Rotation0 = FVector3(0.0f, 0.0f, 0.0f);
                if (EditorWidgets::DrawFloat3Control("Rotation", Rotation, 0.0f, &Rotation0, EVector3ControlType::RotationDegrees))
                {
                    const FVector3 Radians = FVector3::DegreesToRadians(Rotation);
                    SelectedCamera->SetRotation(Radians.X, Radians.Y, Radians.Z);
                }

                EditorWidgets::EndPropertyTable();
            }
        }

        ImGui::PopID();
        return;
    }

    // ------------------------------------------------------------
    // Light-Probe
    // ------------------------------------------------------------

    if (SelectedLightProbe)
    {
        ImGui::PushID(SelectedLightProbe);

        DrawLabelWithSeperator("Light Probe");

        if (DrawCollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen, true))
        {
            if (EditorWidgets::BeginPropertyTable("##ProbeTransformTable", LabelColumnWidth, RevertColumnWidth))
            {
                FVector3 Position = SelectedLightProbe->GetPosition();
                const FVector3 Position0 = FVector3(0.0f, 0.0f, 0.0f);
                
                if (EditorWidgets::DrawFloat3Control("Position", Position, 0.0f, &Position0, EVector3ControlType::Position))
                {
                    SelectedLightProbe->SetPosition(Position);
                }

                EditorWidgets::EndPropertyTable();
            }
        }

        if (DrawCollapsingHeader("Box Projection", ImGuiTreeNodeFlags_DefaultOpen, false))
        {
            if (EditorWidgets::BeginPropertyTable("##ProbeBoxProjectionTable", LabelColumnWidth, RevertColumnWidth))
            {
                FVector3 BoxExtent = SelectedLightProbe->GetBoxExtents();
                const FVector3 BoxExtent0 = FVector3(0.0f, 0.0f, 0.0f);
                
                if (EditorWidgets::DrawFloat3Control("Box Extent", BoxExtent, 0.0f, &BoxExtent0, EVector3ControlType::Position))
                {
                    SelectedLightProbe->SetBoxExtent(BoxExtent);
                }

                FVector3 BoxOffset = SelectedLightProbe->GetBoxOffset();
                const FVector3 BoxOffset0 = FVector3(0.0f, 0.0f, 0.0f);
                
                if (EditorWidgets::DrawFloat3Control("Box Origin", BoxOffset, 0.0f, &BoxOffset0, EVector3ControlType::Position))
                {
                    SelectedLightProbe->SetBoxOffset(BoxOffset);
                }

                bool bBoxProjection = SelectedLightProbe->GetBoxProjection();
                const bool bBoxProjection0 = false;
                
                if (EditorWidgets::DrawCheckboxProperty("Enable Box-Projection", bBoxProjection, &bBoxProjection0))
                {
                    SelectedLightProbe->SetBoxProjection(bBoxProjection);
                }

                EditorWidgets::EndPropertyTable();
            }
        }

        ImGui::PopID();
        return;
    }
}
