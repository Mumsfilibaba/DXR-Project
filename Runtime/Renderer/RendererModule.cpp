#include "RendererModule.h"
#include "RendererScene.h"
#include "SceneRenderer.h"
#include "Core/Misc/CoreDelegates.h"
#include "Application/Application.h"
#include <imgui.h>

IMPLEMENT_ENGINE_MODULE(FRendererModule, Renderer);

FRendererModule::FRendererModule()
    : IRendererModule()
    , Renderer(nullptr)
    , Scenes()
{
}

FRendererModule::~FRendererModule()
{
    for (FRendererScene* Scene : Scenes)
    {
        delete Scene;
    }
}

bool FRendererModule::Load()
{
    PostApplicationCreateHandle = CoreDelegates::PostApplicationCreateDelegate.AddLambda([this]()
    {
        if (FImGui::IsInitialized())
        {
            ImGuiContext* NewImGuiContext     = FImGui::GetContext();
            ImGuiContext* CurrentImGuiContext = ImGui::GetCurrentContext();
            if (NewImGuiContext != CurrentImGuiContext)
            {
                ImGui::SetCurrentContext(NewImGuiContext);
            }
        }
        else
        {
            CHECK(false);
        }

        FDelegateHandle Handle = PostApplicationCreateHandle;
        PostApplicationCreateHandle = FDelegateHandle();
        CoreDelegates::PostApplicationCreateDelegate.Unbind(Handle);
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
    for (FRendererScene* Scene : Scenes)
    {
        Renderer->Tick(Scene);
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

IRendererScene* FRendererModule::CreateRendererScene(FScene* Scene)
{
    FRendererScene* NewScene = new FRendererScene(Scene);
    Scenes.Add(NewScene);
    return NewScene;
}

void FRendererModule::DestroyRendererScene(IRendererScene* Scene)
{
    if (FRendererScene* SceneToRemove = static_cast<FRendererScene*>(Scene))
    {
        Scenes.Remove(SceneToRemove);
        delete SceneToRemove;
    }
}
