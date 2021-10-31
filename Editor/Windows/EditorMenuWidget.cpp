#include "EditorMenuWidget.h"

#include "Engine/Engine.h"

#include <imgui.h>
#include <imgui_internal.h>

static float GMainMenuBarHeight = 0.0f;

static bool GShowRenderSettings = false;

///////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
static void DrawRenderSettings()
{
    ImGui::BeginChild( "RendererInfo" );

    SWindowShape WindowShape;
    GEngine->MainWindow->GetWindowShape( WindowShape );

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
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

void CEditorMenuWidget::InitContext( UIContextHandle ContextHandle )
{
    INIT_CONTEXT( ContextHandle );
}

void CEditorMenuWidget::Tick()
{
    if ( ImGui::BeginMainMenuBar() )
    {
        // Set Size
        GMainMenuBarHeight = ImGui::GetWindowHeight();

        // Menu
        if ( ImGui::BeginMenu( "File" ) )
        {
            if ( ImGui::MenuItem( "Toggle Fullscreen" ) )
            {
                GEngine->MainWindow->ToggleFullscreen();
            }

            if ( ImGui::MenuItem( "Quit" ) )
            {
                GEngine->Exit();
            }

            ImGui::EndMenu();
        }

        if ( ImGui::BeginMenu( "View" ) )
        {
            ImGui::MenuItem( "Render Settings", NULL, &GShowRenderSettings );
            //ImGui::MenuItem("SceneGraph", NULL, &ShowSceneGraph);
            //ImGui::MenuItem("Profiler", NULL, &GlobalDrawProfiler);
            //ImGui::MenuItem("Texture Debugger", NULL, &GlobalDrawTextureDebugger);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

bool CEditorMenuWidget::IsTickable()
{
    return true;
}
