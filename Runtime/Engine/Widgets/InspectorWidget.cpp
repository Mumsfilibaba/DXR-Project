#include "InspectorWidget.h"
#include "Engine/Engine.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Engine/World/Components/MeshComponent.h"
#include "Application/WidgetUtilities.h"
#include "Core/Misc/ConsoleManager.h"

#include <imgui.h>

static TAutoConsoleVariable<bool> CVarDrawSceneInspector(
    "Engine.DrawSceneinspector",
    "Draws the SceneInspector",
    false,
    EConsoleVariableFlags::Default);

void FInspectorWidget::DrawSceneInfo()
{
    constexpr float Width = 450.0f;

    FWindowShape WindowShape;
    GEngine->MainWindow->GetWindowShape(WindowShape);

    // Lights
    if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (FLight* CurrentLight : GEngine->World->GetLights())
        {
            ImGui::PushID(CurrentLight);

            if (IsSubClassOf<FPointLight>(CurrentLight))
            {
                FPointLight* CurrentPointLight = Cast<FPointLight>(CurrentLight);
                if (ImGui::TreeNode("PointLight"))
                {
                    const float ColumnWidth = 150.0f;

                    // Transform
                    if (ImGui::TreeNode("Transform"))
                    {
                        FVector3 Translation = CurrentPointLight->GetPosition();
                        DrawFloat3Control("Translation", Translation, 0.0f, ColumnWidth);
                        CurrentPointLight->SetPosition(Translation);

                        ImGui::TreePop();
                    }

                    // Color
                    if (ImGui::TreeNode("Light Settings"))
                    {
                        ImGui::Columns(2, nullptr, false);
                        ImGui::SetColumnWidth(0, ColumnWidth);

                        ImGui::Text("Color");
                        ImGui::NextColumn();

                        FVector3 Color = CurrentPointLight->GetColor();
                        if (DrawColorEdit3("##Color", Color))
                        {
                            CurrentPointLight->SetColor(Color);
                        }

                        ImGui::NextColumn();
                        ImGui::Text("Intensity");
                        ImGui::NextColumn();

                        float Intensity = CurrentPointLight->GetIntensity();
                        if (ImGui::SliderFloat("##Intensity", &Intensity, 0.01f, 1000.0f, "%.2f"))
                        {
                            CurrentPointLight->SetIntensity(Intensity);
                        }

                        ImGui::Columns(1);
                        ImGui::TreePop();
                    }

                    // Shadow Settings
                    if (ImGui::TreeNode("Shadows"))
                    {
                        ImGui::Columns(2, nullptr, false);
                        ImGui::SetColumnWidth(0, ColumnWidth);

                        // Bias
                        ImGui::Text("Shadow Bias");
                        ImGui::NextColumn();

                        float ShadowBias = CurrentPointLight->GetShadowBias();
                        if (ImGui::SliderFloat("##ShadowBias", &ShadowBias, 0.0001f, 0.1f, "%.4f"))
                        {
                            CurrentPointLight->SetShadowBias(ShadowBias);
                        }

                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("A Bias value used in lightning calculations\nwhen measuring the depth in a ShadowMap");
                        }

                        // Max Shadow Bias
                        ImGui::NextColumn();
                        ImGui::Text("Max Shadow Bias");
                        ImGui::NextColumn();

                        float MaxShadowBias = CurrentPointLight->GetMaxShadowBias();
                        if (ImGui::SliderFloat("##MaxShadowBias", &MaxShadowBias, 0.0001f, 0.1f, "%.4f"))
                        {
                            CurrentPointLight->SetMaxShadowBias(MaxShadowBias);
                        }

                        // Shadow Near Plane
                        ImGui::NextColumn();
                        ImGui::Text("Shadow Near Plane");
                        ImGui::NextColumn();

                        float ShadowNearPlane = CurrentPointLight->GetShadowNearPlane();
                        if (ImGui::SliderFloat("##ShadowNearPlane", &ShadowNearPlane, 0.01f, 1.0f, "%0.2f"))
                        {
                            CurrentPointLight->SetShadowNearPlane(ShadowNearPlane);
                        }

                        // Shadow Far Plane
                        ImGui::NextColumn();
                        ImGui::Text("Shadow Far Plane");
                        ImGui::NextColumn();

                        float ShadowFarPlane = CurrentPointLight->GetShadowFarPlane();
                        if (ImGui::SliderFloat("##ShadowFarPlane", &ShadowFarPlane, 1.0f, 100.0f, "%.1f"))
                        {
                            CurrentPointLight->SetShadowFarPlane(ShadowFarPlane);
                        }

                        ImGui::Columns(1);
                        ImGui::TreePop();
                    }

                    ImGui::TreePop();
                }
            }
            else if (IsSubClassOf<FDirectionalLight>(CurrentLight))
            {
                FDirectionalLight* CurrentDirectionalLight = Cast<FDirectionalLight>(CurrentLight);
                if (ImGui::TreeNode("DirectionalLight"))
                {
                    const float ColumnWidth = 200.0f;

                    // Color and Intensity
                    ImGui::SeparatorText("General");

                    if (ImGui::TreeNodeEx("Color", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::Columns(2, nullptr, false);
                        ImGui::SetColumnWidth(0, ColumnWidth);

                        ImGui::Text("Color");
                        ImGui::NextColumn();

                        FVector3 Color = CurrentDirectionalLight->GetColor();
                        if (DrawColorEdit3("##Color", Color))
                        {
                            CurrentDirectionalLight->SetColor(Color);
                        }

                        ImGui::NextColumn();
                        ImGui::Text("Intensity");
                        ImGui::NextColumn();

                        float Intensity = CurrentDirectionalLight->GetIntensity();
                        if (ImGui::SliderFloat("##Intensity", &Intensity, 0.01f, 1000.0f, "%.2f"))
                        {
                            CurrentDirectionalLight->SetIntensity(Intensity);
                        }

                        ImGui::Columns(1);
                        ImGui::TreePop();
                    }

                    // Transform
                    if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::Columns(2, nullptr, false);
                        ImGui::SetColumnWidth(0, ColumnWidth);

                        bool bSetRotation = false;
                        FVector3 Rotation = CurrentDirectionalLight->GetRotation();

                        ImGui::Text("Rotation Theta (Degrees)");
                        ImGui::NextColumn();

                        float RotationTheta = FMath::ToDegrees(Rotation.x);
                        if (ImGui::SliderFloat("##RotationTheta", &RotationTheta, -90.0f, 90.0f, "%.2f"))
                        {
                            bSetRotation = true;
                        }

                        ImGui::NextColumn();
                        ImGui::Text("Rotation Phi (Degrees)");
                        ImGui::NextColumn();

                        float RotationPhi = FMath::ToDegrees(Rotation.y);
                        if (ImGui::SliderFloat("##RotationPhi", &RotationPhi, 0.0f, 360.0f, "%.2f"))
                        {
                            bSetRotation = true;
                        }

                        if (bSetRotation)
                        {
                            Rotation.x = FMath::ToRadians(RotationTheta);
                            Rotation.y = FMath::ToRadians(RotationPhi);
                            CurrentDirectionalLight->SetRotation(Rotation);
                        }

                        ImGui::NextColumn();
                        ImGui::Text("Direction");
                        ImGui::NextColumn();

                        FVector3 Direction = CurrentDirectionalLight->GetDirection();
                        ImGui::InputFloat3("##Direction", Direction.Data(), "%.3f", ImGuiInputTextFlags_ReadOnly);

                        ImGui::Columns(1);
                        ImGui::TreePop();
                    }

                    // Shadow Settings
                    if (ImGui::TreeNode("Shadow Settings"))
                    {
                        FVector3 LookAt = CurrentDirectionalLight->GetLookAt();
                        DrawFloat3Control("LookAt", LookAt, 0.0f, ColumnWidth);

                        ImGui::Columns(2, nullptr, false);
                        ImGui::SetColumnWidth(0, ColumnWidth);

                        // Read only translation
                        ImGui::Text("Translation");
                        ImGui::NextColumn();

                        FVector3 Position = CurrentDirectionalLight->GetPosition();
                        ImGui::InputFloat3("##Translation", Position.Data(), "%.3f", ImGuiInputTextFlags_ReadOnly);

                        // Shadow Bias
                        ImGui::NextColumn();
                        ImGui::Text("Shadow Bias");
                        ImGui::NextColumn();

                        float ShadowBias = CurrentDirectionalLight->GetShadowBias();
                        if (ImGui::SliderFloat("##ShadowBias", &ShadowBias, 0.0001f, 0.1f, "%.4f"))
                        {
                            CurrentDirectionalLight->SetShadowBias(ShadowBias);
                        }

                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("A Bias value used in lightning calculations\nwhen measuring the depth in a ShadowMap");
                        }

                        // Max Shadow Bias
                        ImGui::NextColumn();
                        ImGui::Text("Max Shadow Bias");
                        ImGui::NextColumn();

                        float MaxShadowBias = CurrentDirectionalLight->GetMaxShadowBias();
                        if (ImGui::SliderFloat("##MaxShadowBias", &MaxShadowBias, 0.0001f, 0.1f, "%.4f"))
                        {
                            CurrentDirectionalLight->SetMaxShadowBias(MaxShadowBias);
                        }

                        // Cascade Split Lambda
                        ImGui::NextColumn();
                        ImGui::Text("Cascade Split Lambda");
                        ImGui::NextColumn();

                        float CascadeSplitLambda = CurrentDirectionalLight->GetCascadeSplitLambda();
                        if (ImGui::SliderFloat("##CascadeSplitLambda", &CascadeSplitLambda, 0.0f, 1.0f, "%.2f"))
                        {
                            CurrentDirectionalLight->SetCascadeSplitLambda(CascadeSplitLambda);
                        }

                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Value modifying the splits for the shadow map cascades");
                        }

                        // Read only splits
                        ImGui::NextColumn();
                        ImGui::Text("Cascade Splits");
                        ImGui::NextColumn();

                        ImGui::PushItemWidth(83.75f);

                        float CascadeSplit = 0.0f;//CurrentDirectionalLight->GetCascadeSplit(0);
                        ImGui::InputFloat("##CascadeSplits", &CascadeSplit, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);

                        ImGui::SameLine();

                        CascadeSplit = 0.0f;//CurrentDirectionalLight->GetCascadeSplit(1);
                        ImGui::InputFloat("##CascadeSplits", &CascadeSplit, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);

                        ImGui::SameLine();

                        CascadeSplit = 0.0f;//CurrentDirectionalLight->GetCascadeSplit(2);
                        ImGui::InputFloat("##CascadeSplits", &CascadeSplit, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);

                        ImGui::SameLine();

                        CascadeSplit = 0.0f;//CurrentDirectionalLight->GetCascadeSplit(3);
                        ImGui::InputFloat("##CascadeSplits", &CascadeSplit, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);

                        ImGui::PopItemWidth();

                        // Size
                        ImGui::NextColumn();
                        ImGui::Text("Light Size");
                        ImGui::NextColumn();

                        float LightSize = 1.0f; //CurrentDirectionalLight->Size();
                        if (ImGui::SliderFloat("##LightSize", &LightSize, 0.0f, 1.0f, "%.2f"))
                        {
                            CurrentDirectionalLight->SetSize(LightSize);
                        }

                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Value modifying the size when calculating soft shadows");
                        }

                        ImGui::Columns(1);
                        ImGui::TreePop();
                    }

                    ImGui::TreePop();
                }
            }

            ImGui::PopID();
        }
    }

    // Actors
    if (ImGui::CollapsingHeader("Actors", ImGuiTreeNodeFlags_None))
    {
        ImGui::Text("Actor Count: %d", GEngine->World->GetActors().Size());

        for (FActor* Actor : GEngine->World->GetActors())
        {
            ImGui::PushID(Actor);

            if (ImGui::TreeNode(Actor->GetName().GetCString()))
            {
                // Transform
                if (ImGui::TreeNode("Transform"))
                {
                    // Transform
                    FVector3 Translation = Actor->GetTransform().GetTranslation();
                    DrawFloat3Control("Translation", Translation);
                    Actor->GetTransform().SetTranslation(Translation);

                    // Rotation
                    FVector3 Rotation = Actor->GetTransform().GetRotation();
                    Rotation = FMath::ToDegrees(Rotation);

                    DrawFloat3Control("Rotation", Rotation, 0.0f, 100.0f, 1.0f);

                    Rotation = FMath::ToRadians(Rotation);

                    Actor->GetTransform().SetRotation(Rotation);

                    // Scale
                    FVector3 Scale0 = Actor->GetTransform().GetScale();
                    FVector3 Scale1 = Scale0;
                    DrawFloat3Control("Scale", Scale0, 1.0f);

                    ImGui::SameLine();

                    static bool bUniform = false;
                    ImGui::Checkbox("##Uniform", &bUniform);
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Enable Uniform Scaling");
                    }

                    if (bUniform)
                    {
                        if (Scale1.x != Scale0.x)
                        {
                            Scale0.y = Scale0.x;
                            Scale0.z = Scale0.x;
                        }
                        else if (Scale1.y != Scale0.y)
                        {
                            Scale0.x = Scale0.y;
                            Scale0.z = Scale0.y;
                        }
                        else if (Scale1.z != Scale0.z)
                        {
                            Scale0.x = Scale0.z;
                            Scale0.y = Scale0.z;
                        }
                    }

                    Actor->GetTransform().SetScale(Scale0);

                    ImGui::TreePop();
                }

                // MeshComponent
                FMeshComponent* MeshComponent = Actor->GetComponentOfType<FMeshComponent>();
                if (MeshComponent)
                {
                    if (ImGui::TreeNode("MeshComponent"))
                    {
                        ImGui::Columns(2, nullptr, false);
                        ImGui::SetColumnWidth(0, 100.0f);

                        // Albedo
                        ImGui::Text("Albedo");
                        ImGui::NextColumn();

                        FMaterialInfo MaterialInfo = MeshComponent->GetMaterial()->GetMaterialInfo();
                        if (DrawColorEdit3("##Albedo", MaterialInfo.Albedo))
                        {
                            MeshComponent->GetMaterial()->SetAlbedo(MaterialInfo.Albedo);
                        }

                        // Roughness
                        ImGui::NextColumn();
                        ImGui::Text("Roughness");
                        ImGui::NextColumn();

                        if (ImGui::SliderFloat("##Roughness", &MaterialInfo.Roughness, 0.01f, 1.0f, "%.2f"))
                        {
                            MeshComponent->GetMaterial()->SetRoughness(MaterialInfo.Roughness);
                        }

                        // Metallic
                        ImGui::NextColumn();
                        ImGui::Text("Metallic");
                        ImGui::NextColumn();

                        if (ImGui::SliderFloat("##Metallic", &MaterialInfo.Metallic, 0.01f, 1.0f, "%.2f"))
                        {
                            MeshComponent->GetMaterial()->SetMetallic(MaterialInfo.Metallic);
                        }

                        // AO
                        ImGui::NextColumn();
                        ImGui::Text("AO");
                        ImGui::NextColumn();

                        if (ImGui::SliderFloat("##AO", &MaterialInfo.AmbientOcclusion, 0.01f, 1.0f, "%.2f"))
                        {
                            MeshComponent->GetMaterial()->SetAmbientOcclusion(MaterialInfo.AmbientOcclusion);
                        }

                        ImGui::Columns(1);
                        ImGui::TreePop();
                    }
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }
}

void FInspectorWidget::Paint()
{
    const uint32 WindowWidth  = GEngine->MainWindow->GetWidth();
    const uint32 WindowHeight = GEngine->MainWindow->GetHeight();
    const float Width  = FMath::Max(WindowWidth * 0.3f, 400.0f);
    const float Height = WindowHeight * 0.7f;

    ImGui::SetNextWindowPos(ImVec2(float(WindowWidth) * 0.5f, float(WindowHeight) * 0.175f), ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Appearing);

    const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoSavedSettings;

    bool DrawInspector = CVarDrawSceneInspector.GetValue();
    if (ImGui::Begin("SceneInspector", &DrawInspector, Flags))
    {
        DrawSceneInfo();
    }

    ImGui::End();

    CVarDrawSceneInspector->SetAsBool(DrawInspector, EConsoleVariableFlags::SetByCode);
}
