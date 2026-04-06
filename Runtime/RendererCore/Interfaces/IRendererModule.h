#pragma once
#include "Core/Modules/ModuleManager.h"
#include "Core/Containers/SharedRef.h"
#include "RendererCore/Interfaces/IScene.h"

class FWorld;
class FRHITexture;
struct IGPUProfiler;
typedef TSharedRef<class FRHISwapChain> FRHISwapChainRef;

struct FSceneRenderView
{
    IScene*      Scene        = nullptr;
    FRHITexture* RenderTarget = nullptr;
};

struct IRendererModule : public FModuleInterface
{
    static IRendererModule* Get()
    {
        IRendererModule* RendererModule = FModuleManager::Get().GetModule<IRendererModule>("Renderer");
        return RendererModule;
    }

    virtual ~IRendererModule() = default;

    /** @brief Initialize the Renderer from the EngineLoop */
    virtual bool Initialize() = 0;

    /** @brief Begin the frame */
    virtual void BeginFrame() = 0;

    /** @brief Run a frame on the Renderer side */
    virtual void Tick() = 0;

    /** @brief End the frame */
    virtual void EndFrame() = 0;

    /** @brief Render the scene */
    virtual void RenderSceneView(const FSceneRenderView& SceneRenderView) = 0;

    /** @brief Render UI */ 
    virtual void RenderUI() = 0; 
 
    /** @brief Request an async editor ObjectID pick at the given pixel (in render target space). */ 
    virtual void RequestEditorObjectPick(IScene* Scene, uint32 PixelX, uint32 PixelY) = 0; 
 
    /** @brief Poll for a completed editor ObjectID pick. Returns true if a result was produced. */ 
    virtual bool PollEditorObjectPickResult(IScene* Scene, uint32& OutObjectID) = 0; 
 
    /** @brief Resize a SwapChain on the RHIThread */ 
    virtual void ResizeSwapChain(FRHISwapChainRef SwapChain, uint32 InWidth, uint32 InHeight) = 0; 

    /** @brief Prepare a swapchain for being used in rendering */
    virtual void PrepareSwapChain(FRHISwapChainRef SwapChain) = 0;

    /** @brief Submit a swapchain for presentation */
    virtual void PresentSwapChain(FRHISwapChainRef SwapChain) = 0;

    /** @brief Release the Renderer from the EngineLoop */
    virtual void Release() = 0;

    /** @brief Create a Renderer version of the World */
    virtual IScene* CreateScene(FWorld* InWorld) = 0;

    /** @brief Destroy a Renderer Scene */
    virtual void DestroyScene(IScene* Scene) = 0;

    /** @brief Returns the GPU profiler interface */
    virtual IGPUProfiler& GetGPUProfiler() = 0;
};
