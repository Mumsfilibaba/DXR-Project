#include "RendererModule.h"
#include "Scene.h"
#include "SceneRenderer.h"
#include "Core/Misc/CoreDelegates.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"

IMPLEMENT_ENGINE_MODULE(FRendererModule, Renderer);

FRendererModule::FRendererModule()
    : IRendererModule()
    , Renderer(nullptr)
    , Scenes()
{
}

FRendererModule::~FRendererModule()
{
    for (FScene* Scene : Scenes)
    {
        delete Scene;
    }
}

bool FRendererModule::Load()
{
    PreEngineInitHandle = CoreDelegates::PreEngineInitDelegate.AddLambda([this]()
    {
        if (IImguiPlugin::IsEnabled())
        {
            ImGuiContext* Context = IImguiPlugin::Get().GetImGuiContext();
            ImGui::SetCurrentContext(Context);
        }
        else
        {
            CHECK(false);
        }
    });

    return true;
}

bool FRendererModule::Initialize()
{
    if (!Renderer)
    {
        Renderer = new FSceneRenderer();
        return Renderer->Initialize();
    }
    else
    {
        LOG_WARNING("Renderer is already initialized");
        return false;
    }
}

void FRendererModule::Tick()
{
    for (FScene* Scene : Scenes)
    {
        // Performs frustum culling and updates visible primitives
        Scene->Tick();

        // Render the scene
        Renderer->Tick(Scene);

        // TODO: Break for now since we always output to the BackBuffer
        break;
    }
}

void FRendererModule::Release()
{
    if (Renderer)
    {
        delete Renderer;
        Renderer = nullptr;
    }
}

IScene* FRendererModule::CreateScene(FWorld* World)
{
    FScene* NewScene = new FScene(World);
    Scenes.Add(NewScene);
    return NewScene;
}

void FRendererModule::DestroyScene(IScene* Scene)
{
    if (FScene* SceneToRemove = static_cast<FScene*>(Scene))
    {
        Scenes.Remove(SceneToRemove);
        delete SceneToRemove;
    }
}
