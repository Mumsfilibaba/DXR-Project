#include "Editor.h"

#include "Rendering/DebugUI.h"
#include "Rendering/Renderer.h"

#include "Scene/Scene.h"
#include "Scene/Lights/DirectionalLight.h"
#include "Scene/Lights/PointLight.h"
#include "Scene/Components/MeshComponent.h"

#include "Core/Engine/Engine.h"
#include "Core/Engine/EngineLoop.h"
#include "Core/Engine/EngineGlobals.h"
#include "Core/Application/ApplicationModule.h"
#include "Core/Debug/Console/Console.h"
#include "Core/Math/Math.h"

#include <imgui_internal.h>

static float MainMenuBarHeight = 0.0f;

static bool ShowRenderSettings = false;

TConsoleVariable<bool> GShowSceneGraph( false );

static void DrawMenu();
static void DrawSideWindow();
static void DrawRenderSettings();
static void DrawSceneInfo();

static void DrawFloat3Control( const std::string& Label, CVector3& Value, float ResetValue = 0.0f, float ColumnWidth = 100.0f, float Speed = 0.01f )
{
    ImGui::PushID( Label.c_str() );
    ImGui::Columns( 2, nullptr, false );

    // Text
    ImGui::SetColumnWidth( 0, ColumnWidth );
    ImGui::Text( "%s", Label.c_str() );
    ImGui::NextColumn();

    // Drag Floats
    ImGui::PushMultiItemsWidths( 3, ImGui::CalcItemWidth() );
    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
    ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 0.0f );

    float    LineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2   ButtonSize = ImVec2( LineHeight + 3.0f, LineHeight );

    // X
    ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.8f, 0.1f, 0.15f, 1.0f ) );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.9f, 0.2f, 0.2f, 1.0f ) );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.8f, 0.1f, 0.15f, 1.0f ) );
    if ( ImGui::Button( "X", ButtonSize ) )
    {
        Value.x = ResetValue;
    }
    ImGui::PopStyleColor( 3 );

    ImGui::SameLine();
    ImGui::DragFloat( "##X", &Value.x, Speed );
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Y
    ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.2f, 0.7f, 0.2f, 1.0f ) );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.3f, 0.8f, 0.3f, 1.0f ) );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.2f, 0.7f, 0.2f, 1.0f ) );
    if ( ImGui::Button( "Y", ButtonSize ) )
    {
        Value.y = ResetValue;
    }
    ImGui::PopStyleColor( 3 );

    ImGui::SameLine();
    ImGui::DragFloat( "##Y", &Value.y, Speed );
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Z
    ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.1f, 0.25f, 0.8f, 1.0f ) );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.2f, 0.35f, 0.9f, 1.0f ) );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.1f, 0.25f, 0.8f, 1.0f ) );
    if ( ImGui::Button( "Z", ButtonSize ) )
    {
        Value.z = ResetValue;
    }
    ImGui::PopStyleColor( 3 );

    ImGui::SameLine();
    ImGui::DragFloat( "##Z", &Value.z, Speed );
    ImGui::PopItemWidth();

    // Reset
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::Columns( 1 );
    ImGui::PopID();
}

static void DrawMenu()
{
    DebugUI::DrawUI( []
    {
        if ( ImGui::BeginMainMenuBar() )
        {
            // Set Size
            MainMenuBarHeight = ImGui::GetWindowHeight();

            // Menu
            if ( ImGui::BeginMenu( "File" ) )
            {
                if ( ImGui::MenuItem( "Toggle Fullscreen" ) )
                {
                    CEngine::Get().MainWindow->ToggleFullscreen();
                }

                if ( ImGui::MenuItem( "Quit" ) )
                {
                    CEngine::Get().Exit();
                }

                ImGui::EndMenu();
            }

            if ( ImGui::BeginMenu( "View" ) )
            {
                ImGui::MenuItem( "Render Settings", NULL, &ShowRenderSettings );
                //ImGui::MenuItem("SceneGraph", NULL, &ShowSceneGraph);
                //ImGui::MenuItem("Profiler", NULL, &GlobalDrawProfiler);
                //ImGui::MenuItem("Texture Debugger", NULL, &GlobalDrawTextureDebugger);

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    } );
}

static void DrawSideWindow()
{
    DebugUI::DrawUI( []
    {
        const uint32 WindowWidth = CEngine::Get().MainWindow->GetWidth();
        const uint32 WindowHeight = CEngine::Get().MainWindow->GetHeight();
        const float Width = NMath::Max( WindowWidth * 0.3f, 400.0f );
        const float Height = WindowHeight * 0.7f;

        ImGui::PushStyleColor( ImGuiCol_ResizeGrip, 0 );
        ImGui::PushStyleColor( ImGuiCol_ResizeGripHovered, 0 );
        ImGui::PushStyleColor( ImGuiCol_ResizeGripActive, 0 );

        ImGui::SetNextWindowPos(
            ImVec2( float( WindowWidth ) * 0.5f, float( WindowHeight ) * 0.175f ),
            ImGuiCond_Appearing,
            ImVec2( 0.5f, 0.0f ) );

        ImGui::SetNextWindowSize(
            ImVec2( Width, Height ),
            ImGuiCond_Appearing );

        const ImGuiWindowFlags Flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoSavedSettings;

        bool TempDrawProfiler = GShowSceneGraph.GetBool();
        if ( ImGui::Begin(
            "SceneGraph",
            &TempDrawProfiler,
            Flags ) )
        {
            DrawSceneInfo();
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();

        ImGui::End();

        GShowSceneGraph.SetBool( TempDrawProfiler );
    } );
}

static void DrawRenderSettings()
{
    ImGui::BeginChild( "RendererInfo" );

    SWindowShape WindowShape;
    CEngine::Get().MainWindow->GetWindowShape( WindowShape );

    ImGui::Spacing();
    ImGui::Text( "Renderer Info" );
    ImGui::Separator();

    ImGui::Indent();
    ImGui::Text( "Resolution: %d x %d", WindowShape.Width, WindowShape.Height );

    //ImGui::Checkbox("Enable Z-PrePass", &GlobalPrePassEnabled);
    //ImGui::Checkbox("Enable VSync", &GlobalVSyncEnabled);
    //ImGui::Checkbox("Enable Frustum Culling", &GlobalFrustumCullEnabled);
    //ImGui::Checkbox("Draw AABBs", &GlobalDrawAABBs);

    static const char* AAItems[] =
    {
        "OFF",
        "FXAA",
    };

    static int32 CurrentItem = 0;
    //if (GlobalFXAAEnabled)
    //{
    //    CurrentItem = 1;
    //}
    //else
    //{
    //    CurrentItem = 0;
    //}

    if ( ImGui::Combo( "Anti-Aliasing", &CurrentItem, AAItems, IM_ARRAYSIZE( AAItems ) ) )
    {
        //if (CurrentItem == 0)
        //{
        //    GlobalFXAAEnabled = false;
        //}
        //else if (CurrentItem == 1)
        //{
        //    GlobalFXAAEnabled = true;
        //}
    }

    ImGui::Spacing();
    ImGui::Text( "Shadow Settings:" );
    ImGui::Separator();

    static const char* Items[] =
    {
        "8192x8192",
        "4096x4096",
        "3072x3072",
        "2048x2048",
        "1024x1024",
        "512x512",
        "256x256"
    };

    //LightSettings Settings = GlobalRenderer->GetLightSettings();
    //if (Settings.ShadowMapWidth == 8192)
    //{
    //    CurrentItem = 0;
    //}
    //else if (Settings.ShadowMapWidth == 4096)
    //{
    //    CurrentItem = 1;
    //}
    //else if (Settings.ShadowMapWidth == 3072)
    //{
    //    CurrentItem = 2;
    //}
    //else if (Settings.ShadowMapWidth == 2048)
    //{
    //    CurrentItem = 3;
    //}
    //else if (Settings.ShadowMapWidth == 1024)
    //{
    //    CurrentItem = 4;
    //}
    //else if (Settings.ShadowMapWidth == 512)
    //{
    //    CurrentItem = 5;
    //}
    //else if (Settings.ShadowMapWidth == 256)
    //{
    //    CurrentItem = 6;
    //}

    //if (ImGui::Combo("Directional Light ShadowMap", &CurrentItem, Items, IM_ARRAYSIZE(Items)))
    //{
    //    if (CurrentItem == 0)
    //    {
    //        Settings.ShadowMapWidth  = 8192;
    //        Settings.ShadowMapHeight = 8192;
    //    }
    //    else if (CurrentItem == 1)
    //    {
    //        Settings.ShadowMapWidth  = 4096;
    //        Settings.ShadowMapHeight = 4096;
    //    }
    //    else if (CurrentItem == 2)
    //    {
    //        Settings.ShadowMapWidth  = 3072;
    //        Settings.ShadowMapHeight = 3072;
    //    }
    //    else if (CurrentItem == 3)
    //    {
    //        Settings.ShadowMapWidth  = 2048;
    //        Settings.ShadowMapHeight = 2048;
    //    }
    //    else if (CurrentItem == 4)
    //    {
    //        Settings.ShadowMapWidth  = 1024;
    //        Settings.ShadowMapHeight = 1024;
    //    }
    //    else if (CurrentItem == 5)
    //    {
    //        Settings.ShadowMapWidth  = 512;
    //        Settings.ShadowMapHeight = 512;
    //    }
    //    else if (CurrentItem == 6)
    //    {
    //        Settings.ShadowMapWidth  = 256;
    //        Settings.ShadowMapHeight = 256;
    //    }

    //    GlobalRenderer->SetLightSettings(Settings);
    //}

    ImGui::Spacing();
    ImGui::Text( "SSAO:" );
    ImGui::Separator();

    ImGui::Columns( 2, nullptr, false );

    // Text
    ImGui::SetColumnWidth( 0, 100.0f );

    ImGui::Text( "Enabled: " );
    ImGui::NextColumn();

    //ImGui::Checkbox("##Enabled", &GlobalSSAOEnabled);

    ImGui::NextColumn();
    ImGui::Text( "Radius: " );
    ImGui::NextColumn();

    //float Radius = GlobalRenderer->GetSSAORadius();
    //if (ImGui::SliderFloat("##Radius", &Radius, 0.05f, 5.0f, "%.3f"))
    //{
    //    GlobalRenderer->SetSSAORadius(Radius);
    //}

    //ImGui::NextColumn();
    //ImGui::Text("Bias: ");
    //ImGui::NextColumn();

    //float Bias = GlobalRenderer->GetSSAOBias();
    //if (ImGui::SliderFloat("##Bias", &Bias, 0.0f, 0.5f, "%.3f"))
    //{
    //    GlobalRenderer->SetSSAOBias(Bias);
    //}

    //ImGui::NextColumn();
    //ImGui::Text("KernelSize: ");
    //ImGui::NextColumn();

    //int32 KernelSize = GlobalRenderer->GetSSAOKernelSize();
    //if (ImGui::SliderInt("##KernelSize", &KernelSize, 4, 64))
    //{
    //    GlobalRenderer->SetSSAOKernelSize(KernelSize);
    //}

    ImGui::Columns( 1 );
    ImGui::EndChild();
}

static void DrawSceneInfo()
{
    constexpr float Width = 450.0f;

    ImGui::Spacing();
    ImGui::Text( "Current Scene" );
    ImGui::Separator();

    SWindowShape WindowShape;
    CEngine::Get().MainWindow->GetWindowShape( WindowShape );

    // Actors
    if ( ImGui::TreeNode( "Actors" ) )
    {
        ImGui::Text( "Total Actor Count: %d", GApplicationModule->CurrentScene->GetActors().Size() );

        for ( CActor* Actor : GApplicationModule->CurrentScene->GetActors() )
        {
            ImGui::PushID( Actor );

            if ( ImGui::TreeNode( Actor->GetName().c_str() ) )
            {
                // Transform
                if ( ImGui::TreeNode( "Transform" ) )
                {
                    // Transform
                    CVector3 Translation = Actor->GetTransform().GetTranslation();
                    DrawFloat3Control( "Translation", Translation );
                    Actor->GetTransform().SetTranslation( Translation );

                    // Rotation
                    CVector3 Rotation = Actor->GetTransform().GetRotation();
                    Rotation = CVector3(
                        NMath::ToDegrees( Rotation.x ),
                        NMath::ToDegrees( Rotation.y ),
                        NMath::ToDegrees( Rotation.z ) );

                    DrawFloat3Control( "Rotation", Rotation, 0.0f, 100.0f, 1.0f );

                    Rotation = CVector3(
                        NMath::ToRadians( Rotation.x ),
                        NMath::ToRadians( Rotation.y ),
                        NMath::ToRadians( Rotation.z ) );

                    Actor->GetTransform().SetRotation( Rotation );

                    // Scale
                    CVector3 Scale0 = Actor->GetTransform().GetScale();
                    CVector3 Scale1 = Scale0;
                    DrawFloat3Control( "Scale", Scale0, 1.0f );

                    ImGui::SameLine();

                    static bool Uniform = false;
                    ImGui::Checkbox( "##Uniform", &Uniform );
                    if ( ImGui::IsItemHovered() )
                    {
                        ImGui::SetTooltip( "Enable Uniform Scaling" );
                    }

                    if ( Uniform )
                    {
                        if ( Scale1.x != Scale0.x )
                        {
                            Scale0.y = Scale0.x;
                            Scale0.z = Scale0.x;
                        }
                        else if ( Scale1.y != Scale0.y )
                        {
                            Scale0.x = Scale0.y;
                            Scale0.z = Scale0.y;
                        }
                        else if ( Scale1.z != Scale0.z )
                        {
                            Scale0.x = Scale0.z;
                            Scale0.y = Scale0.z;
                        }
                    }

                    Actor->GetTransform().SetScale( Scale0 );

                    ImGui::TreePop();
                }

                // MeshComponent
                MeshComponent* MComponent = Actor->GetComponentOfType<MeshComponent>();
                if ( MComponent )
                {
                    if ( ImGui::TreeNode( "MeshComponent" ) )
                    {
                        ImGui::Columns( 2, nullptr, false );
                        ImGui::SetColumnWidth( 0, 100.0f );

                        // Albedo
                        ImGui::Text( "Albedo" );
                        ImGui::NextColumn();

                        const CVector3& Color = MComponent->Material->GetMaterialProperties().Albedo;
                        float Arr[3] = { Color.x, Color.y, Color.z };
                        if ( ImGui::ColorEdit3( "##Albedo", Arr ) )
                        {
                            MComponent->Material->SetAlbedo( Arr[0], Arr[1], Arr[2] );
                        }

                        // Roughness
                        ImGui::NextColumn();
                        ImGui::Text( "Roughness" );
                        ImGui::NextColumn();

                        float Roughness = MComponent->Material->GetMaterialProperties().Roughness;
                        if ( ImGui::SliderFloat( "##Roughness", &Roughness, 0.01f, 1.0f, "%.2f" ) )
                        {
                            MComponent->Material->SetRoughness( Roughness );
                        }

                        // Metallic
                        ImGui::NextColumn();
                        ImGui::Text( "Metallic" );
                        ImGui::NextColumn();

                        float Metallic = MComponent->Material->GetMaterialProperties().Metallic;
                        if ( ImGui::SliderFloat( "##Metallic", &Metallic, 0.01f, 1.0f, "%.2f" ) )
                        {
                            MComponent->Material->SetMetallic( Metallic );
                        }

                        // AO
                        ImGui::NextColumn();
                        ImGui::Text( "AO" );
                        ImGui::NextColumn();

                        float AO = MComponent->Material->GetMaterialProperties().AO;
                        if ( ImGui::SliderFloat( "##AO", &AO, 0.01f, 1.0f, "%.2f" ) )
                        {
                            MComponent->Material->SetAmbientOcclusion( AO );
                        }

                        ImGui::Columns( 1 );
                        ImGui::TreePop();
                    }
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        ImGui::TreePop();
    }

    // Lights
    if ( ImGui::TreeNode( "Lights" ) )
    {
        for ( Light* CurrentLight : GApplicationModule->CurrentScene->GetLights() )
        {
            ImGui::PushID( CurrentLight );

            if ( IsSubClassOf<PointLight>( CurrentLight ) )
            {
                PointLight* CurrentPointLight = Cast<PointLight>( CurrentLight );
                if ( ImGui::TreeNode( "PointLight" ) )
                {
                    const float ColumnWidth = 150.0f;

                    // Transform
                    if ( ImGui::TreeNode( "Transform" ) )
                    {
                        CVector3 Translation = CurrentPointLight->GetPosition();
                        DrawFloat3Control( "Translation", Translation, 0.0f, ColumnWidth );
                        CurrentPointLight->SetPosition( Translation );

                        ImGui::TreePop();
                    }

                    // Color
                    if ( ImGui::TreeNode( "Light Settings" ) )
                    {
                        ImGui::Columns( 2, nullptr, false );
                        ImGui::SetColumnWidth( 0, ColumnWidth );

                        ImGui::Text( "Color" );
                        ImGui::NextColumn();

                        const CVector3& Color = CurrentPointLight->GetColor();
                        float Arr[3] = { Color.x, Color.y, Color.z };
                        if ( ImGui::ColorEdit3( "##Color", Arr ) )
                        {
                            CurrentPointLight->SetColor( Arr[0], Arr[1], Arr[2] );
                        }

                        ImGui::NextColumn();
                        ImGui::Text( "Intensity" );
                        ImGui::NextColumn();

                        float Intensity = CurrentPointLight->GetIntensity();
                        if ( ImGui::SliderFloat( "##Intensity", &Intensity, 0.01f, 1000.0f, "%.2f" ) )
                        {
                            CurrentPointLight->SetIntensity( Intensity );
                        }

                        ImGui::Columns( 1 );
                        ImGui::TreePop();
                    }

                    // Shadow Settings
                    if ( ImGui::TreeNode( "Shadows" ) )
                    {
                        ImGui::Columns( 2, nullptr, false );
                        ImGui::SetColumnWidth( 0, ColumnWidth );

                        // Bias
                        ImGui::Text( "Shadow Bias" );
                        ImGui::NextColumn();

                        float ShadowBias = CurrentPointLight->GetShadowBias();
                        if ( ImGui::SliderFloat( "##ShadowBias", &ShadowBias, 0.0001f, 0.1f, "%.4f" ) )
                        {
                            CurrentPointLight->SetShadowBias( ShadowBias );
                        }

                        if ( ImGui::IsItemHovered() )
                        {
                            ImGui::SetTooltip( "A Bias value used in lightning calculations\nwhen measuring the depth in a ShadowMap" );
                        }

                        // Max Shadow Bias
                        ImGui::NextColumn();
                        ImGui::Text( "Max Shadow Bias" );
                        ImGui::NextColumn();

                        float MaxShadowBias = CurrentPointLight->GetMaxShadowBias();
                        if ( ImGui::SliderFloat( "##MaxShadowBias", &MaxShadowBias, 0.0001f, 0.1f, "%.4f" ) )
                        {
                            CurrentPointLight->SetMaxShadowBias( MaxShadowBias );
                        }

                        // Shadow Near Plane
                        ImGui::NextColumn();
                        ImGui::Text( "Shadow Near Plane" );
                        ImGui::NextColumn();

                        float ShadowNearPlane = CurrentPointLight->GetShadowNearPlane();
                        if ( ImGui::SliderFloat( "##ShadowNearPlane", &ShadowNearPlane, 0.01f, 1.0f, "%0.2f" ) )
                        {
                            CurrentPointLight->SetShadowNearPlane( ShadowNearPlane );
                        }

                        // Shadow Far Plane
                        ImGui::NextColumn();
                        ImGui::Text( "Shadow Far Plane" );
                        ImGui::NextColumn();

                        float ShadowFarPlane = CurrentPointLight->GetShadowFarPlane();
                        if ( ImGui::SliderFloat( "##ShadowFarPlane", &ShadowFarPlane, 1.0f, 100.0f, "%.1f" ) )
                        {
                            CurrentPointLight->SetShadowFarPlane( ShadowFarPlane );
                        }

                        ImGui::Columns( 1 );
                        ImGui::TreePop();
                    }

                    ImGui::TreePop();
                }
            }
            else if ( IsSubClassOf<DirectionalLight>( CurrentLight ) )
            {
                DirectionalLight* CurrentDirectionalLight = Cast<DirectionalLight>( CurrentLight );
                if ( ImGui::TreeNode( "DirectionalLight" ) )
                {
                    const float ColumnWidth = 150.0f;

                    // Color
                    if ( ImGui::TreeNode( "Light Settings" ) )
                    {
                        ImGui::Columns( 2, nullptr, false );
                        ImGui::SetColumnWidth( 0, ColumnWidth );

                        ImGui::Text( "Color" );
                        ImGui::NextColumn();

                        const CVector3& Color = CurrentDirectionalLight->GetColor();
                        float Arr[3] = { Color.x, Color.y, Color.z };
                        if ( ImGui::ColorEdit3( "##Color", Arr ) )
                        {
                            CurrentDirectionalLight->SetColor( Arr[0], Arr[1], Arr[2] );
                        }

                        ImGui::NextColumn();
                        ImGui::Text( "Intensity" );
                        ImGui::NextColumn();

                        float Intensity = CurrentDirectionalLight->GetIntensity();
                        if ( ImGui::SliderFloat( "##Intensity", &Intensity, 0.01f, 1000.0f, "%.2f" ) )
                        {
                            CurrentDirectionalLight->SetIntensity( Intensity );
                        }

                        ImGui::Columns( 1 );
                        ImGui::TreePop();
                    }

                    // Transform
                    if ( ImGui::TreeNode( "Transform" ) )
                    {
                        CVector3 Rotation = CurrentDirectionalLight->GetRotation();
                        Rotation = CVector3(
                            NMath::ToDegrees( Rotation.x ),
                            NMath::ToDegrees( Rotation.y ),
                            NMath::ToDegrees( Rotation.z )
                        );

                        DrawFloat3Control( "Rotation", Rotation, 0.0f, ColumnWidth, 1.0f );

                        Rotation = CVector3(
                            NMath::ToRadians( Rotation.x ),
                            NMath::ToRadians( Rotation.y ),
                            NMath::ToRadians( Rotation.z )
                        );

                        CurrentDirectionalLight->SetRotation( Rotation );

                        ImGui::Columns( 2, nullptr, false );
                        ImGui::SetColumnWidth( 0, ColumnWidth );

                        ImGui::Text( "Direction" );
                        ImGui::NextColumn();

                        CVector3 Direction = CurrentDirectionalLight->GetDirection();
                        float* DirArr = reinterpret_cast<float*>(&Direction);
                        ImGui::InputFloat3( "##Direction", DirArr, "%.3f", ImGuiInputTextFlags_ReadOnly );

                        ImGui::Columns( 1 );
                        ImGui::TreePop();
                    }

                    // Shadow Settings
                    if ( ImGui::TreeNode( "Shadow Settings" ) )
                    {
                        CVector3 LookAt = CurrentDirectionalLight->GetLookAt();
                        DrawFloat3Control( "LookAt", LookAt, 0.0f, ColumnWidth );
                        CurrentDirectionalLight->SetLookAt( LookAt );

                        ImGui::Columns( 2, nullptr, false );
                        ImGui::SetColumnWidth( 0, ColumnWidth );

                        // Read only translation
                        ImGui::Text( "Translation" );
                        ImGui::NextColumn();

                        CVector3 Position = CurrentDirectionalLight->GetPosition();
                        float* PosArr = reinterpret_cast<float*>(&Position);
                        ImGui::InputFloat3( "##Translation", PosArr, "%.3f", ImGuiInputTextFlags_ReadOnly );

                        // Shadow Bias
                        ImGui::NextColumn();
                        ImGui::Text( "Shadow Bias" );
                        ImGui::NextColumn();

                        float ShadowBias = CurrentDirectionalLight->GetShadowBias();
                        if ( ImGui::SliderFloat( "##ShadowBias", &ShadowBias, 0.0001f, 0.1f, "%.4f" ) )
                        {
                            CurrentDirectionalLight->SetShadowBias( ShadowBias );
                        }

                        if ( ImGui::IsItemHovered() )
                        {
                            ImGui::SetTooltip( "A Bias value used in lightning calculations\nwhen measuring the depth in a ShadowMap" );
                        }

                        // Max Shadow Bias
                        ImGui::NextColumn();
                        ImGui::Text( "Max Shadow Bias" );
                        ImGui::NextColumn();

                        float MaxShadowBias = CurrentDirectionalLight->GetMaxShadowBias();
                        if ( ImGui::SliderFloat( "##MaxShadowBias", &MaxShadowBias, 0.0001f, 0.1f, "%.4f" ) )
                        {
                            CurrentDirectionalLight->SetMaxShadowBias( MaxShadowBias );
                        }

                        // Cascade Split Lambda
                        ImGui::NextColumn();
                        ImGui::Text( "Cascade Split Lambda" );
                        ImGui::NextColumn();

                        float CascadeSplitLambda = CurrentDirectionalLight->GetCascadeSplitLambda();
                        if ( ImGui::SliderFloat( "##CascadeSplitLambda", &CascadeSplitLambda, 0.0f, 1.0f, "%.2f" ) )
                        {
                            CurrentDirectionalLight->SetCascadeSplitLambda( CascadeSplitLambda );
                        }

                        if ( ImGui::IsItemHovered() )
                        {
                            ImGui::SetTooltip( "Value modifying the splits for the shadow map cascades" );
                        }

                        // Read only splits
                        ImGui::NextColumn();
                        ImGui::Text( "Cascade Splits" );
                        ImGui::NextColumn();

                        ImGui::PushItemWidth( 83.75f );

                        float CascadeSplit = CurrentDirectionalLight->GetCascadeSplit( 0 );
                        ImGui::InputFloat( "##CascadeSplits", &CascadeSplit, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly );

                        ImGui::SameLine();

                        CascadeSplit = CurrentDirectionalLight->GetCascadeSplit( 1 );
                        ImGui::InputFloat( "##CascadeSplits", &CascadeSplit, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly );

                        ImGui::SameLine();

                        CascadeSplit = CurrentDirectionalLight->GetCascadeSplit( 2 );
                        ImGui::InputFloat( "##CascadeSplits", &CascadeSplit, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly );

                        ImGui::SameLine();

                        CascadeSplit = CurrentDirectionalLight->GetCascadeSplit( 3 );
                        ImGui::InputFloat( "##CascadeSplits", &CascadeSplit, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly );

                        ImGui::PopItemWidth();

                        // Size
                        ImGui::NextColumn();
                        ImGui::Text( "Light Size" );
                        ImGui::NextColumn();

                        float LightSize = CurrentDirectionalLight->GetSize();
                        if ( ImGui::SliderFloat( "##LightSize", &LightSize, 0.0f, 1.0f, "%.2f" ) )
                        {
                            CurrentDirectionalLight->SetSize( LightSize );
                        }

                        if ( ImGui::IsItemHovered() )
                        {
                            ImGui::SetTooltip( "Value modifying the size when calculating soft shadows" );
                        }

                        ImGui::Columns( 1 );
                        ImGui::TreePop();
                    }

                    ImGui::TreePop();
                }
            }

            ImGui::PopID();
        }

        ImGui::TreePop();
    }
}

void Editor::Init()
{
    INIT_CONSOLE_VARIABLE( "ShowSceneGraph", &GShowSceneGraph );
}

void Editor::Tick()
{
#if 0
    DrawMenu();
#endif

    if ( GShowSceneGraph.GetBool() )
    {
        DrawSideWindow();
    }
}
