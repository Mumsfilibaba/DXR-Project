#include "Core/Misc/ConsoleManager.h"
#include "Engine/Engine.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Engine/World/Components/MeshComponent.h"
#include "Engine/Widgets/SceneInspectorWidget.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"

static TAutoConsoleVariable<bool> CVarDrawSceneInspector(
    "Engine.DrawSceneinspector",
    "Draws the SceneInspector",
    false,
    EConsoleVariableFlags::Default);

FSceneInspectorWidget::FSceneInspectorWidget()
    : ImGuiDelegateHandle()
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FSceneInspectorWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FSceneInspectorWidget::~FSceneInspectorWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FSceneInspectorWidget::Draw()
{
    bool bDrawInspector = CVarDrawSceneInspector.GetValue();
    if (bDrawInspector)
    {
        const uint32 WindowWidth  = GEngine->GetEngineWindow()->GetWidth();
        const uint32 WindowHeight = GEngine->GetEngineWindow()->GetHeight();

        const float Width  = FMath::Max(WindowWidth * 0.3f, 400.0f);
        const float Height = WindowHeight * 0.7f;

        ImGui::SetNextWindowPos(ImVec2(float(WindowWidth) * 0.5f, float(WindowHeight) * 0.175f), ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Appearing);

        const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::Begin("SceneInspector", &bDrawInspector, Flags))
        {
            DrawSceneInfo();
        }

        ImGui::End();

        CVarDrawSceneInspector->SetAsBool(bDrawInspector, EConsoleVariableFlags::SetByCode);
    }
}

void FSceneInspectorWidget::DrawSceneInfo()
{
    // Same column size for all different types
    const float ColumnWidth = 200.0f;

    // Cache the current world
    FWorld* CurrentWorld = GEngine->GetWorld();

    // Camera
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_None))
    {
        if (FCamera* Camera = CurrentWorld->GetCamera())
        {
            ImGui::PushID(Camera);

            ImGui::SeparatorText("Projection");

            // Setup the columns
            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, ColumnWidth);

            // Add some extra spacing for the text elements...
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 10.0f));

            // Display the size of the viewport
            ImGui::Text("Viewport size");
            ImGui::NextColumn();
            ImGui::Text("%.1f x %.1f", Camera->GetWidth(), Camera->GetHeight());

            // ... pop the extra spacing
            ImGui::PopStyleVar();

            ImGui::NextColumn();

            // Field Of View
            ImGui::Text("Field Of View");
            ImGui::NextColumn();

            float FieldOfView = Camera->GetFieldOfView();
            if (ImGui::SliderFloat("##FieldOfView", &FieldOfView, 40.0f, 120.0f, "%.1f Degrees"))
            {
                Camera->SetFieldOfView(FieldOfView);
            }

            // Reset the columns
            ImGui::Columns(1);

            // Transform
            ImGui::SeparatorText("Transform");

            FVector3 Position = Camera->GetPosition();
            ImGuiExtensions::DrawFloat3Control("Position", Position);
            Camera->SetPosition(Position.X, Position.Y, Position.Z);

            // Rotation
            FVector3 Rotation = Camera->GetRotation();
            Rotation = FMath::ToDegrees(Rotation);

            ImGuiExtensions::DrawFloat3Control("Rotation", Rotation, 0.0f, 100.0f, 1.0f);

            Rotation = FMath::ToRadians(Rotation);

            Camera->SetRotation(Rotation.X, Rotation.Y, Rotation.Z);

            ImGui::PopID();
        }

        ImGui::NewLine();
    }

    // Lights
    if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_None))
    {
        for (FLight* CurrentLight : CurrentWorld->GetLights())
        {
            ImGui::PushID(CurrentLight);

            if (FPointLight* CurrentPointLight = Cast<FPointLight>(CurrentLight))
            {
                if (ImGui::TreeNode("PointLight"))
                {
                    // Color
                    ImGui::SeparatorText("Settings");

                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, ColumnWidth);

                    ImGui::Text("Color");
                    ImGui::NextColumn();

                    FVector3 Color = CurrentPointLight->GetColor();
                    if (ImGuiExtensions::DrawColorEdit3("##Color", Color))
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

                    // Transform
                    ImGui::SeparatorText("Transform");

                    FVector3 Translation = CurrentPointLight->GetPosition();
                    ImGuiExtensions::DrawFloat3Control("Translation", Translation, 0.0f, ColumnWidth);
                    CurrentPointLight->SetPosition(Translation);

                    // Shadow Settings
                    ImGui::SeparatorText("Shadows");

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
            }
            else if (FDirectionalLight* CurrentDirectionalLight = Cast<FDirectionalLight>(CurrentLight))
            {
                if (ImGui::TreeNode("DirectionalLight"))
                {
                    // Color and Intensity
                    ImGui::SeparatorText("Settings");

                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, ColumnWidth);

                    ImGui::Text("Color");
                    ImGui::NextColumn();

                    FVector3 Color = CurrentDirectionalLight->GetColor();
                    if (ImGuiExtensions::DrawColorEdit3("##Color", Color))
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

                    // Transform
                    ImGui::SeparatorText("Direction");

                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, ColumnWidth);

                    bool bSetRotation = false;
                    FVector3 Rotation = CurrentDirectionalLight->GetRotation();

                    ImGui::Text("Rotation Theta (Degrees)");
                    ImGui::NextColumn();

                    float RotationTheta = FMath::ToDegrees(Rotation.X);
                    if (ImGui::SliderFloat("##RotationTheta", &RotationTheta, -90.0f, 90.0f, "%.2f"))
                    {
                        bSetRotation = true;
                    }

                    ImGui::NextColumn();
                    ImGui::Text("Rotation Phi (Degrees)");
                    ImGui::NextColumn();

                    float RotationPhi = FMath::ToDegrees(Rotation.Y);
                    if (ImGui::SliderFloat("##RotationPhi", &RotationPhi, 0.0f, 360.0f, "%.2f"))
                    {
                        bSetRotation = true;
                    }

                    if (bSetRotation)
                    {
                        Rotation.X = FMath::ToRadians(RotationTheta);
                        Rotation.Y = FMath::ToRadians(RotationPhi);
                        CurrentDirectionalLight->SetRotation(Rotation);
                    }

                    ImGui::NextColumn();
                    ImGui::Text("Direction");
                    ImGui::NextColumn();

                    FVector3 Direction = CurrentDirectionalLight->GetDirectionVector();
                    ImGui::InputFloat3("##Direction", Direction.XYZ, "%.3f", ImGuiInputTextFlags_ReadOnly);

                    ImGui::Columns(1);

                    // Shadow Settings
                    ImGui::SeparatorText("Shadows");

                    FVector3 LookAt = CurrentDirectionalLight->GetLookAt();
                    ImGuiExtensions::DrawFloat3Control("LookAt", LookAt, 0.0f, ColumnWidth);

                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, ColumnWidth);

                    // Read only translation
                    ImGui::Text("Translation");
                    ImGui::NextColumn();

                    FVector3 Position = CurrentDirectionalLight->GetPosition();
                    ImGui::InputFloat3("##Translation", Position.XYZ, "%.3f", ImGuiInputTextFlags_ReadOnly);

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

                    // Size
                    ImGui::NextColumn();
                    ImGui::Text("Light Size");
                    ImGui::NextColumn();

                    float LightSize = CurrentDirectionalLight->GetSize();
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
            }

            ImGui::PopID();
        }

        ImGui::NewLine();
    }

    // Actors
    if (ImGui::CollapsingHeader("Actors", ImGuiTreeNodeFlags_None))
    {
        // Add some spacing above the text...
        ImGui::Dummy(ImVec2(ColumnWidth, 5.0f));

        // Actor count info
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, ColumnWidth);

        // ... add some spacing before the text ...
        ImGui::Dummy(ImVec2(5.0f, 5.0f));
        ImGui::SameLine();

        ImGui::Text("Number of Actors");
        ImGui::NextColumn();
        ImGui::Text("%d", CurrentWorld->GetActors().Size());

        ImGui::Columns(1);

        // ... add some space after the text
        ImGui::Dummy(ImVec2(ColumnWidth, 5.0f));

        // Display all Actor Info
        for (FActor* Actor : CurrentWorld->GetActors())
        {
            ImGui::PushID(Actor);

            const FString ActorName = Actor->GetName();
            if (ImGui::TreeNode(*ActorName))
            {
                // Transform
                ImGui::SeparatorText("Transform");

                FVector3 Translation = Actor->GetTransform().GetTranslation();
                ImGuiExtensions::DrawFloat3Control("Translation", Translation);
                Actor->GetTransform().SetTranslation(Translation);

                // Rotation
                FVector3 Rotation = Actor->GetTransform().GetRotation();
                Rotation = FMath::ToDegrees(Rotation);

                ImGuiExtensions::DrawFloat3Control("Rotation", Rotation, 0.0f, 100.0f, 1.0f);

                Rotation = FMath::ToRadians(Rotation);

                Actor->GetTransform().SetRotation(Rotation);

                // Scale
                FVector3 Scale0 = Actor->GetTransform().GetScale();
                FVector3 Scale1 = Scale0;
                ImGuiExtensions::DrawFloat3Control("Scale", Scale0, 1.0f);

                ImGui::SameLine();

                static bool bUniform = false;
                ImGui::Checkbox("##Uniform", &bUniform);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Enable Uniform Scaling");
                }

                if (bUniform)
                {
                    if (Scale1.X != Scale0.X)
                    {
                        Scale0.Y = Scale0.X;
                        Scale0.Z = Scale0.X;
                    }
                    else if (Scale1.Y != Scale0.Y)
                    {
                        Scale0.X = Scale0.Y;
                        Scale0.Z = Scale0.Y;
                    }
                    else if (Scale1.Z != Scale0.Z)
                    {
                        Scale0.X = Scale0.Z;
                        Scale0.Y = Scale0.Z;
                    }
                }

                Actor->GetTransform().SetScale(Scale0);

                // MeshComponent
                if (FMeshComponent* MeshComponent = Actor->GetComponentOfType<FMeshComponent>())
                {
                    if (ImGui::CollapsingHeader("MeshComponent", ImGuiTreeNodeFlags_None))
                    {
                        ImGui::Columns(2, nullptr, false);
                        ImGui::SetColumnWidth(0, ColumnWidth);

                        // Albedo
                        ImGui::Text("Albedo");
                        ImGui::NextColumn();

                        FMaterialInfo MaterialInfo = MeshComponent->GetMaterial()->GetMaterialInfo();
                        if (ImGuiExtensions::DrawColorEdit3("##Albedo", MaterialInfo.Albedo))
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
                    }
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        ImGui::NewLine();
    }
}
