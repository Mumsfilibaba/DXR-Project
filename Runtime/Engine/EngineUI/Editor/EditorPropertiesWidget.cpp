#include "Engine/EditorEngine.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Engine/World/Components/DirectionalLightComponent.h"
#include "Engine/World/Components/LightComponent.h"
#include "Engine/World/Components/LightProbeComponent.h"
#include "Engine/World/Components/PointLightComponent.h"
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
    , MaterialSelectionOwner(nullptr)
    , SelectedMaterialIndex(0)
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

ImTextureID FEditorPropertiesWidget::GetTexturePreview(EMaterialTextureSlot::Type Slot, const FRHITextureRef& Texture)
{
    FImGuiTexture& Preview = MaterialTexturePreviews[Slot];
    if (Preview.GetTexture() != Texture.Get())
    {
        Preview = FImGuiTexture(Texture);
        Preview.bEnableBlending      = false;
        Preview.bEnableLinearSampler = true;
    }

    return Preview.ShaderResourceView ? reinterpret_cast<ImTextureID>(&Preview) : nullptr;
}

void FEditorPropertiesWidget::DrawWindowContents()
{
    FActor* SelectedActor = EditorEngine->GetSelectedActor();
    if (!SelectedActor)
    {
        ImGui::TextDisabled("No selection");
        return;
    }

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

    FStaticMeshComponent* MeshComponent      = SelectedActor->GetComponentOfType<FStaticMeshComponent>();
    FLightComponent*      SelectedLight      = SelectedActor->GetComponentOfType<FLightComponent>();
    FCameraComponent*     SelectedCamera     = SelectedActor->GetComponentOfType<FCameraComponent>();
    FLightProbeComponent* SelectedLightProbe = SelectedActor->GetComponentOfType<FLightProbeComponent>();

    if (SelectedActor)
    {
        ImGui::PushID(SelectedActor);

        const String& ActorName = SelectedActor->GetName();
        DrawLabelWithSeperator(ActorName.IsEmpty() ? "Actor" : *ActorName);

        const bool bHasMeshComponent       = MeshComponent != nullptr;
        const bool bHasComponentProperties = bHasMeshComponent || SelectedLight || SelectedCamera || SelectedLightProbe;

        // An attached actor is placed relative to its parent, so the transform fields no longer edit world-space
        FActor*    ParentActor = SelectedActor->GetParentActor();
        const bool bIsAttached = ParentActor != nullptr;

        const CHAR* TransformLabel = bIsAttached ? "Transform (Relative)" : "Transform";
        if (DrawCollapsingHeader(TransformLabel, ImGuiTreeNodeFlags_DefaultOpen, bIsAttached || bHasComponentProperties))
        {
            if (EditorWidgets::BeginPropertyTable("##ActorTransformTable", LabelColumnWidth, RevertColumnWidth))
            {
                // Translation
                {
                    Vector3 Translation = SelectedActor->GetTransform().GetTranslation();
                    const Vector3 TranslationRevert = Vector3(0.0f, 0.0f, 0.0f);

                    if (EditorWidgets::DrawFloat3Control("Translation", Translation, 0.0f, &TranslationRevert, EVector3ControlType::Position))
                    {
                        SelectedActor->GetTransform().SetTranslation(Translation);
                    }
                }

                // Rotation (degrees UI)
                {
                    Vector3 Rotation = SelectedActor->GetTransform().GetRotation();
                    Rotation = Vector3::RadiansToDegrees(Rotation);

                    const Vector3 RotationRevert = Vector3(0.0f, 0.0f, 0.0f);

                    if (EditorWidgets::DrawFloat3Control("Rotation", Rotation, 0.0f, &RotationRevert, EVector3ControlType::RotationDegrees))
                    {
                        const Vector3 Radians = Vector3::DegreesToRadians(Rotation);
                        if (SelectedCamera)
                        {
                            SelectedCamera->SetRotation(Radians);
                        }
                        else
                        {
                            SelectedActor->GetTransform().SetRotation(Radians);
                        }
                    }
                }

                // Scale
                {
                    Vector3 Scale = SelectedActor->GetTransform().GetScale();
                    const Vector3 ScaleRevert = Vector3(1.0f, 1.0f, 1.0f);

                    const bool bScaleChanged = EditorWidgets::DrawFloat3Control("Scale", Scale, 1.0f, &ScaleRevert, EVector3ControlType::Scale);
                    if (bScaleChanged)
                    {
                        SelectedActor->GetTransform().SetScale(Scale);
                    }
                }

                EditorWidgets::EndPropertyTable();
            }
        }

        // Attachment
        if (bIsAttached)
        {
            if (DrawCollapsingHeader("Attachment", ImGuiTreeNodeFlags_DefaultOpen, bHasComponentProperties))
            {
                if (EditorWidgets::BeginPropertyTable("##ActorAttachmentTable", LabelColumnWidth, RevertColumnWidth))
                {
                    EditorWidgets::PropertyRowLabel("Parent");

                    const String& ParentName = ParentActor->GetName();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(ParentName.IsEmpty() ? "Actor" : *ParentName);
                    ImGui::SameLine();

                    if (ImGui::SmallButton("Detach"))
                    {
                        SelectedActor->DetachFromParent(EAttachmentRule::KeepWorld);
                    }

                    EditorWidgets::EndPropertyTable();
                }
            }
        }

        // MeshComponent
        if (bHasMeshComponent)
        {
            if (MaterialSelectionOwner != SelectedActor)
            {
                MaterialSelectionOwner = SelectedActor;
                SelectedMaterialIndex  = 0;
            }

            const int32 NumMaterials = MeshComponent->GetNumMaterials();
            SelectedMaterialIndex = Math::Clamp<int32>(SelectedMaterialIndex, 0, Math::Max<int32>(NumMaterials - 1, 0));

            TSharedPtr<FMaterial> Material = MeshComponent->GetMaterial(SelectedMaterialIndex);
            if (Material)
            {
                DrawLabelWithSeperator("StaticMesh");

                const FMaterialInfo& MaterialInfo = Material->GetMaterialInfo();

                const auto IsMaterialFlagSet = [&Material](EMaterialFlags Flag) -> bool
                {
                    return IsEnumFlagSet(Material->GetMaterialInfo().MaterialFlags, Flag);
                };

                const bool bHasHeightMap = Material->HasHeightMap();

                if (DrawCollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen, true))
                {
                    if (EditorWidgets::BeginPropertyTable("##MeshComponentMaterialTable", LabelColumnWidth, RevertColumnWidth))
                    {
                        if (NumMaterials > 1)
                        {
                            TArray<String> MaterialLabels;
                            MaterialLabels.Reserve(NumMaterials);

                            for (int32 Index = 0; Index < NumMaterials; Index++)
                            {
                                TSharedPtr<FMaterial> Entry = MeshComponent->GetMaterial(Index);
                                if (Entry && !Entry->GetName().IsEmpty())
                                {
                                    MaterialLabels.Emplace(String::Printf("%d: %s", Index, *Entry->GetName()));
                                }
                                else
                                {
                                    MaterialLabels.Emplace(String::Printf("%d: <unnamed>", Index));
                                }
                            }

                            TArray<const CHAR*> MaterialLabelText;
                            MaterialLabelText.Reserve(NumMaterials);

                            for (const String& Label : MaterialLabels)
                            {
                                MaterialLabelText.Add(*Label);
                            }

                            const int32 SelectedMaterialIndex0 = 0;
                            EditorWidgets::DrawComboProperty("Material", SelectedMaterialIndex, MaterialLabelText.Data(), MaterialLabelText.Size(), &SelectedMaterialIndex0);
                        }

                        // Albedo
                        {
                            FFloatColor Albedo = MaterialInfo.Albedo;
                            const FFloatColor Albedo0 = FFloatColor::White;

                            if (EditorWidgets::DrawColor3Property("Albedo", Albedo, Albedo0))
                            {
                                Material->SetAlbedo(Albedo);
                            }
                        }

                        // Roughness
                        {
                            float Roughness = MaterialInfo.Roughness;
                            const float Roughness0 = 0.0f;

                            if (EditorWidgets::DrawFloatProperty("Roughness", Roughness, 0.01f, 0.01f, 1.0f, "%.2f", true, &Roughness0))
                            {
                                Material->SetRoughness(Roughness);
                            }
                        }

                        // Metallic
                        {
                            float Metallic = MaterialInfo.Metallic;
                            const float Metallic0 = 0.0f;

                            if (EditorWidgets::DrawFloatProperty("Metallic", Metallic, 0.01f, 0.01f, 1.0f, "%.2f", true, &Metallic0))
                            {
                                Material->SetMetallic(Metallic);
                            }
                        }

                        // AO
                        {
                            float AO = MaterialInfo.AmbientOcclusion;
                            const float AO0 = 1.0f;

                            if (EditorWidgets::DrawFloatProperty("AO", AO, 0.01f, 0.01f, 1.0f, "%.2f", true, &AO0))
                            {
                                Material->SetAmbientOcclusion(AO);
                            }
                        }

                        // Opacity
                        {
                            float Opacity = MaterialInfo.Opacity;
                            const float Opacity0 = 1.0f;

                            if (EditorWidgets::DrawFloatProperty("Opacity", Opacity, 0.01f, 0.0f, 1.0f, "%.2f", true, &Opacity0))
                            {
                                Material->SetOpacity(Opacity);
                            }
                        }

                        // Index of refraction
                        {
                            float IndexOfRefraction = MaterialInfo.IndexOfRefraction;
                            const float IndexOfRefraction0 = 1.5f;

                            if (EditorWidgets::DrawFloatProperty("Index of Refraction", IndexOfRefraction, 0.01f, 1.0f, 3.0f, "%.2f", true, &IndexOfRefraction0))
                            {
                                Material->SetIndexOfRefraction(IndexOfRefraction);
                            }
                        }

                        // Refraction strength
                        {
                            float RefractionStrength = MaterialInfo.RefractionStrength;
                            const float RefractionStrength0 = 1.0f;

                            if (EditorWidgets::DrawFloatProperty("Refraction Strength", RefractionStrength, 0.01f, 0.0f, 1.0f, "%.2f", true, &RefractionStrength0))
                            {
                                Material->SetRefractionStrength(RefractionStrength);
                            }
                        }

                        EditorWidgets::EndPropertyTable();
                    }
                }

                if (DrawCollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen, true))
                {
                    if (EditorWidgets::BeginPropertyTable("##MeshComponentTexturesTable", LabelColumnWidth, RevertColumnWidth))
                    {
                        static const CHAR* SlotNames[] =
                        {
                            "Base Color",
                            "Normal",
                            "Height",
                            "Mask A",
                            "Mask B",
                            "Mask C",
                            "Mask D"
                        };

                        static_assert(ARRAY_COUNT(SlotNames) == EMaterialTextureSlot::Count, "Every material texture slot needs a name in the Textures panel");

                        for (uint32 Slot = 0; Slot < EMaterialTextureSlot::Count; ++Slot)
                        {
                            const FRHITextureRef& SlotTexture = Material->GetTexture(EMaterialTextureSlot::Type(Slot));
                            if (SlotTexture)
                            {
                                EditorWidgets::DrawTextureProperty(SlotNames[Slot], GetTexturePreview(EMaterialTextureSlot::Type(Slot), SlotTexture));
                            }
                        }

                        EditorWidgets::EndPropertyTable();
                    }
                }

                if (DrawCollapsingHeader("Material Flags", ImGuiTreeNodeFlags_DefaultOpen, bHasHeightMap))
                {
                    if (EditorWidgets::BeginPropertyTable("##MeshComponentMaterialFlagsTable", LabelColumnWidth, RevertColumnWidth))
                    {
                        const bool bHasNormalTexture = Material->GetTexture(EMaterialTextureSlot::Normal).IsValid();

                        // Normal mapping
                        {
                            bool bEnableNormalMapping = IsMaterialFlagSet(EMaterialFlags::EnableNormalMapping);

                            if (EditorWidgets::DrawCheckboxProperty("Normal Mapping", bEnableNormalMapping, nullptr, bHasNormalTexture))
                            {
                                Material->EnableNormalMapping(bEnableNormalMapping);
                            }
                        }

                        if (IsMaterialFlagSet(EMaterialFlags::EnableNormalMapping) && bHasNormalTexture)
                        {
                            static const CHAR* const NormalMapAxisItems[] =
                            {
                                "-Y (DirectX)",
                                "+Y (OpenGL)"
                            };

                            constexpr int32 NormalMapAxisCount = static_cast<int32>(ARRAY_COUNT(NormalMapAxisItems));

                            int32 NormalMapAxis = IsMaterialFlagSet(EMaterialFlags::NormalMapPositiveY) ? 1 : 0;

                            ImGui::Indent(12.0f);

                            if (EditorWidgets::DrawComboProperty("Normal Map Axis", NormalMapAxis, NormalMapAxisItems, NormalMapAxisCount, nullptr))
                            {
                                Material->SetNormalMapPositiveY(NormalMapAxis == 1);
                            }

                            ImGui::Unindent(12.0f);
                        }

                        // Alpha mask
                        {
                            bool bEnableAlphaMask = IsMaterialFlagSet(EMaterialFlags::EnableAlpha);

                            const bool bMaskable = Material->IsRouteFed(EMaterialScalar::Opacity) && !Material->IsTranslucent();

                            if (EditorWidgets::DrawCheckboxProperty("Alpha Mask", bEnableAlphaMask, nullptr, bMaskable))
                            {
                                Material->EnableAlphaMask(bEnableAlphaMask);
                            }
                        }

                        // Double sided
                        {
                            bool bDoubleSided = IsMaterialFlagSet(EMaterialFlags::DoubleSided);

                            if (EditorWidgets::DrawCheckboxProperty("Double Sided", bDoubleSided, nullptr))
                            {
                                Material->EnableDoubleSided(bDoubleSided);
                            }
                        }

                        // Height map
                        {
                            bool bEnableHeightMap = IsMaterialFlagSet(EMaterialFlags::EnableHeight);

                            if (EditorWidgets::DrawCheckboxProperty("Height Map", bEnableHeightMap, nullptr, Material->GetTexture(EMaterialTextureSlot::Height).IsValid()))
                            {
                                Material->EnableHeightMap(bEnableHeightMap);
                            }
                        }

                        // Parallax clipping
                        {
                            bool bEnableParallaxClipping = IsMaterialFlagSet(EMaterialFlags::EnableParallaxClipping);

                            if (EditorWidgets::DrawCheckboxProperty("Parallax Clipping", bEnableParallaxClipping, nullptr, Material->HasHeightMap()))
                            {
                                Material->EnableParallaxClipping(bEnableParallaxClipping);
                            }
                        }

                        // Force forward pass
                        {
                            bool bForceForwardPass = IsMaterialFlagSet(EMaterialFlags::ForceForwardPass);

                            if (EditorWidgets::DrawCheckboxProperty("Force Forward Pass", bForceForwardPass, nullptr))
                            {
                                Material->ForceForwardPass(bForceForwardPass);
                            }
                        }

                        // Translucent
                        {
                            bool bTranslucent = IsMaterialFlagSet(EMaterialFlags::Translucent);

                            if (EditorWidgets::DrawCheckboxProperty("Translucent", bTranslucent, nullptr))
                            {
                                Material->EnableTranslucent(bTranslucent);
                            }
                        }

                        // Refraction
                        {
                            bool bEnableRefraction = IsMaterialFlagSet(EMaterialFlags::EnableRefraction);

                            if (EditorWidgets::DrawCheckboxProperty("Refraction", bEnableRefraction, nullptr, Material->IsTranslucent()))
                            {
                                Material->EnableRefraction(bEnableRefraction);
                            }
                        }

                        EditorWidgets::EndPropertyTable();
                    }
                }

                if (bHasHeightMap)
                {
                    if (DrawCollapsingHeader("Parallax", ImGuiTreeNodeFlags_DefaultOpen, false))
                    {
                        if (EditorWidgets::BeginPropertyTable("##MeshComponentParallaxTable", LabelColumnWidth, RevertColumnWidth))
                        {
                            // Height scale
                            {
                                float ParallaxHeightScale = MaterialInfo.ParallaxHeightScale;
                                const float ParallaxHeightScale0 = 0.03f;

                                if (EditorWidgets::DrawFloatProperty("Height Scale", ParallaxHeightScale, 0.001f, 0.0f, 0.2f, "%.3f", true, &ParallaxHeightScale0))
                                {
                                    Material->SetParallaxHeightScale(ParallaxHeightScale);
                                }
                            }

                            // Min layers
                            {
                                float ParallaxMinLayers = MaterialInfo.ParallaxMinLayers;
                                const float ParallaxMinLayers0 = 32.0f;

                                if (EditorWidgets::DrawFloatProperty("Min Layers", ParallaxMinLayers, 1.0f, 1.0f, 128.0f, "%.0f", true, &ParallaxMinLayers0))
                                {
                                    Material->SetParallaxLayers(ParallaxMinLayers, Material->GetParallaxMaxLayers());
                                }
                            }

                            // Max layers
                            {
                                float ParallaxMaxLayers = MaterialInfo.ParallaxMaxLayers;
                                const float ParallaxMaxLayers0 = 64.0f;

                                if (EditorWidgets::DrawFloatProperty("Max Layers", ParallaxMaxLayers, 1.0f, 1.0f, 256.0f, "%.0f", true, &ParallaxMaxLayers0))
                                {
                                    Material->SetParallaxLayers(Material->GetParallaxMinLayers(), ParallaxMaxLayers);
                                }
                            }

                            EditorWidgets::EndPropertyTable();
                        }
                    }
                }
            }
        }

        ImGui::PopID();
    }

    // ------------------------------------------------------------
    // Lights
    // ------------------------------------------------------------

    if (SelectedLight)
    {
        ImGui::PushID(SelectedLight);

        // Point light
        if (FPointLightComponent* PointLight = Cast<FPointLightComponent>(SelectedLight))
        {
            DrawLabelWithSeperator("PointLight");

            if (DrawCollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen, true))
            {
                if (EditorWidgets::BeginPropertyTable("##PointLightSettingsTable", LabelColumnWidth, RevertColumnWidth))
                {
                    Vector3 Color = PointLight->GetColor();
                    const Vector3 Color0 = Vector3(1.0f, 1.0f, 1.0f);
                    
                    if (EditorWidgets::DrawColor3Property("Color", Color, Color0))
                    {
                        PointLight->SetColor(Color);
                    }

                    float Intensity = PointLight->GetIntensity();
                    const float Intensity0 = 1.0f;
                    
                    if (EditorWidgets::DrawFloatProperty("Intensity (Lumen)", Intensity, 0.1f, 0.0f, 200000.0f, "%.1f", true, &Intensity0))
                    {
                        PointLight->SetIntensity(Intensity);
                    }

                    EditorWidgets::EndPropertyTable();
                }
            }

            if (DrawCollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen, true))
            {
                if (EditorWidgets::BeginPropertyTable("##PointLightTransformTable", LabelColumnWidth, RevertColumnWidth))
                {
                    Vector3 Translation = PointLight->GetPosition();
                    const Vector3 Translation0 = Vector3(0.0f, 0.0f, 0.0f);
                    
                    if (EditorWidgets::DrawFloat3Control("Translation", Translation, 0.0f, &Translation0, EVector3ControlType::Position))
                    {
                        FActorTransform NewWorldTransform = SelectedActor->GetWorldTransform();
                        NewWorldTransform.SetTranslation(Translation);
                        SelectedActor->SetWorldTransform(NewWorldTransform);
                    }

                    EditorWidgets::EndPropertyTable();
                }
            }

            if (DrawCollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen, false))
            {
                if (EditorWidgets::BeginPropertyTable("##PointLightShadowsTable", LabelColumnWidth, RevertColumnWidth))
                {
                    bool bCastShadows = PointLight->CastsShadows();
                    const bool bCastShadows0 = true;

                    if (EditorWidgets::DrawCheckboxProperty("Cast shadows", bCastShadows, &bCastShadows0))
                    {
                        PointLight->SetCastShadows(bCastShadows);
                    }

                    float ShadowBias = PointLight->GetShadowBias();
                    const float ShadowBias0 = 0.005f;
                    
                    if (EditorWidgets::DrawFloatProperty("Shadow-bias", ShadowBias, 0.0001f, 0.0001f, 0.1f, "%.4f", true, &ShadowBias0, bCastShadows))
                    {
                        PointLight->SetShadowBias(ShadowBias);
                    }

                    float ShadowNearPlane = PointLight->GetShadowNearPlane();
                    const float ShadowNearPlane0 = 1.0f;
                    
                    if (EditorWidgets::DrawFloatProperty("Shadow near-plane", ShadowNearPlane, 0.01f, 0.01f, 1.0f, "%.2f", true, &ShadowNearPlane0, bCastShadows))
                    {
                        PointLight->SetShadowNearPlane(ShadowNearPlane);
                    }

                    // The far-plane doubles as the light's radius, so it stays editable for a light that
                    // casts no shadow.
                    float ShadowFarPlane = PointLight->GetShadowFarPlane();
                    const float ShadowFarPlane0 = 30.0f;

                    if (EditorWidgets::DrawFloatProperty("Shadow far-plane", ShadowFarPlane, 1.0f, 1.0f, 100.0f, "%.1f", true, &ShadowFarPlane0))
                    {
                        PointLight->SetShadowFarPlane(ShadowFarPlane);
                    }

                    EditorWidgets::EndPropertyTable();
                }
            }
        }
        else if (FDirectionalLightComponent* DirectionalLight = Cast<FDirectionalLightComponent>(SelectedLight))
        {
            DrawLabelWithSeperator("DirectionalLight");

            if (DrawCollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen, true))
            {
                if (EditorWidgets::BeginPropertyTable("##DirLightSettingsTable", LabelColumnWidth, RevertColumnWidth))
                {
                    Vector3 Color = DirectionalLight->GetColor();
                    const Vector3 Color0 = Vector3(1.0f, 1.0f, 1.0f);

                    if (EditorWidgets::DrawColor3Property("Color", Color, Color0))
                    {
                        DirectionalLight->SetColor(Color);
                    }

                    float Intensity = DirectionalLight->GetIntensity();
                    const float Intensity0 = 1.0f;

                    if (EditorWidgets::DrawFloatProperty("Intensity (Lux)", Intensity, 0.1f, 0.0f, 200000.0f, "%.1f", true, &Intensity0))
                    {
                        DirectionalLight->SetIntensity(Intensity);
                    }

                    EditorWidgets::EndPropertyTable();
                }
            }

            if (DrawCollapsingHeader("Direction", ImGuiTreeNodeFlags_DefaultOpen, true))
            {
                if (EditorWidgets::BeginPropertyTable("##DirLightDirectionTable", LabelColumnWidth, RevertColumnWidth))
                {
                    Vector3 Rotation = SelectedActor->GetWorldTransform().GetRotation();
                    
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

                        FActorTransform NewWorldTransform = SelectedActor->GetWorldTransform();
                        NewWorldTransform.SetRotation(Rotation);
                        SelectedActor->SetWorldTransform(NewWorldTransform);
                    }

                    const Vector3 Direction = DirectionalLight->GetDirectionVector();
                    EditorWidgets::DrawReadOnlyFloat3Property("Direction", Direction);

                    EditorWidgets::EndPropertyTable();
                }
            }

            if (DrawCollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen, false))
            {
                if (EditorWidgets::BeginPropertyTable("##DirLightShadowsTable", LabelColumnWidth, RevertColumnWidth))
                {
                    bool bCastShadows = DirectionalLight->CastsShadows();
                    const bool bCastShadows0 = true;

                    if (EditorWidgets::DrawCheckboxProperty("Cast shadows", bCastShadows, &bCastShadows0))
                    {
                        DirectionalLight->SetCastShadows(bCastShadows);
                    }

                    float ShadowBias = DirectionalLight->GetShadowBias();
                    const float ShadowBias0 = 0.005f;
                    
                    if (EditorWidgets::DrawFloatProperty("Shadow-bias", ShadowBias, 0.0001f, 0.0001f, 0.1f, "%.4f", true, &ShadowBias0, bCastShadows))
                    {
                        DirectionalLight->SetShadowBias(ShadowBias);
                    }

                    float Lambda = DirectionalLight->GetCascadeSplitLambda();
                    const float Lambda0 = 0.95f;
                    
                    if (EditorWidgets::DrawFloatProperty("Cascade Split Lambda", Lambda, 0.01f, 0.0f, 1.0f, "%.2f", true, &Lambda0, bCastShadows))
                    {
                        DirectionalLight->SetCascadeSplitLambda(Lambda);
                    }

                    float Offset = DirectionalLight->GetShadowPositionOffset();
                    const float Offset0 = 200.0f;
                    
                    if (EditorWidgets::DrawFloatProperty("Cascade Position Offset", Offset, 1.0f, 0.0f, 1000.0f, "%.1f", true, &Offset0, bCastShadows))
                    {
                        DirectionalLight->SetShadowPositionOffset(Offset);
                    }

                    float ShadowNearPlane = DirectionalLight->GetShadowNearPlane();
                    const float ShadowNearPlane0 = 120.0f;
                    
                    if (EditorWidgets::DrawFloatProperty("Shadow near-plane", ShadowNearPlane, 1.0f, 0.0f, 1000.0f, "%.1f", true, &ShadowNearPlane0, bCastShadows))
                    {
                        DirectionalLight->SetShadowNearPlane(ShadowNearPlane);
                    }

                    float ShadowFarPlane = DirectionalLight->GetShadowFarPlane();
                    const float ShadowFarPlane0 = 250.0f;
                    
                    if (EditorWidgets::DrawFloatProperty("Shadow far-plane", ShadowFarPlane, 1.0f, 0.0f, 1000.0f, "%.1f", true, &ShadowFarPlane0, bCastShadows))
                    {
                        DirectionalLight->SetShadowFarPlane(ShadowFarPlane);
                    }

                    float LightArea = DirectionalLight->GetLightArea();
                    const float LightArea0 = 0.05f;

                    if (EditorWidgets::DrawFloatProperty("Light area", LightArea, 0.01f, 0.0f, 1.0f, "%.2f", true, &LightArea0, bCastShadows))
                    {
                        DirectionalLight->SetLightArea(LightArea);
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
                    CString::Snprintf(ViewportText.Data(), static_cast<int32>(ViewportText.Size()), "%.1f x %.1f", SelectedCamera->GetWidth(), SelectedCamera->GetHeight());
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
                    float Near = SelectedCamera->GetNearPlane();
                    const float Near0 = 0.01f;

                    if (EditorWidgets::DrawFloatProperty("Near Plane", Near, 0.001f, 0.001f, 10.0f, "%.3f", true, &Near0))
                    {
                        SelectedCamera->SetNearPlane(Near);
                    }
                }

                {
                    float Far = SelectedCamera->GetFarPlane();
                    const float Far0 = 200.0f;

                    if (EditorWidgets::DrawFloatProperty("Far Plane", Far, 1.0f, 10.0f, 10000.0f, "%.0f", true, &Far0))
                    {
                        SelectedCamera->SetFarPlane(Far);
                    }
                }

                EditorWidgets::EndPropertyTable();
            }
        }

        if (DrawCollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen, false))
        {
            if (EditorWidgets::BeginPropertyTable("##CameraTransformTable", LabelColumnWidth, RevertColumnWidth))
            {
                Vector3 Position = SelectedCamera->GetPosition();
                const Vector3 Position0 = Vector3(0.0f, 0.0f, 0.0f);
                
                if (EditorWidgets::DrawFloat3Control("Position", Position, 0.0f, &Position0, EVector3ControlType::Position))
                {
                    SelectedCamera->SetPosition(Position.X, Position.Y, Position.Z);
                }

                Vector3 Rotation = SelectedCamera->GetRotation();
                Rotation = Vector3::RadiansToDegrees(Rotation);
                
                const Vector3 Rotation0 = Vector3(0.0f, 0.0f, 0.0f);
                if (EditorWidgets::DrawFloat3Control("Rotation", Rotation, 0.0f, &Rotation0, EVector3ControlType::RotationDegrees))
                {
                    const Vector3 Radians = Vector3::DegreesToRadians(Rotation);
                    SelectedCamera->SetRotation(Radians.X, Radians.Y, Radians.Z);
                }

                EditorWidgets::EndPropertyTable();
            }
        }

        ImGui::PopID();
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
                Vector3 Position = SelectedLightProbe->GetPosition();
                const Vector3 Position0 = Vector3(0.0f, 0.0f, 0.0f);
                
                if (EditorWidgets::DrawFloat3Control("Position", Position, 0.0f, &Position0, EVector3ControlType::Position))
                {
                    FActorTransform NewWorldTransform = SelectedActor->GetWorldTransform();
                    NewWorldTransform.SetTranslation(Position);
                    SelectedActor->SetWorldTransform(NewWorldTransform);
                }

                EditorWidgets::EndPropertyTable();
            }
        }

        if (DrawCollapsingHeader("Box Projection", ImGuiTreeNodeFlags_DefaultOpen, false))
        {
            if (EditorWidgets::BeginPropertyTable("##ProbeBoxProjectionTable", LabelColumnWidth, RevertColumnWidth))
            {
                Vector3 BoxExtent = SelectedLightProbe->GetBoxExtents();
                const Vector3 BoxExtent0 = Vector3(0.0f, 0.0f, 0.0f);
                
                if (EditorWidgets::DrawFloat3Control("Box Extent", BoxExtent, 0.0f, &BoxExtent0, EVector3ControlType::Position))
                {
                    SelectedLightProbe->SetBoxExtent(BoxExtent);
                }

                Vector3 BoxOffset = SelectedLightProbe->GetBoxOffset();
                const Vector3 BoxOffset0 = Vector3(0.0f, 0.0f, 0.0f);
                
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
    }
}
