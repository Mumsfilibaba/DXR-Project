#pragma once
#include "Core/Modules/ModuleManager.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "RHI/RHITypes.h"
#include "RendererCore/CameraSnapshot.h"
#include "RendererCore/Interfaces/IScene.h"

class FWorld;
class FRHITexture;
struct IGPUProfiler;
typedef TSharedRef<class FRHISwapChain> FRHISwapChainRef;

struct FSceneRenderView
{
    enum class EDebugView : int32
    {
        None = 0,
        ShadowMask,
        GBufferAlbedo,
        GBufferNormal,
        GBufferMaterial,
        GBufferVelocity,
        SSAO,
        Depth,
        ShadowCascades,
        ShadowCascadeIndex,
        ShadowCascadeOverlay,
        Lit,
        RayTracingReflectionsRaw,     
        RayTracingReflectionsTemporal,
        RayTracingReflectionsSpatial, 
        RayTracingPrimaryID,          
        RayTracingReflectionsVariance,
        RayTracingReflectionsHistory, 
        Count,
    };

    IScene*         Scene              = nullptr;
    FRHITexture*    RenderTarget       = nullptr;
    EDebugView      DebugView          = EDebugView::None;
    EDebugView      SecondaryDebugView = EDebugView::None;
    FCameraSnapshot CameraSnapshot     = {};
    bool            bHasCamera         = false;
    bool            bCameraCut         = false;
};

struct FSceneRenderPacket
{
    FSceneRenderView View;
    FRHISwapChainRef SwapChain = nullptr;
    TArray<uint32>   SelectedObjectIDs;
    uint64           FrameIndex = 0;
};

struct IRendererModule : public IModule
{
public:
    static IRendererModule* Get()
    {
        return RendererModule;
    }

public:
    virtual ~IRendererModule() = default;

    /** @brief Initialize the Renderer from the EngineLoop */
    virtual bool Initialize() = 0;

    /**
     * @brief Main-thread marshalling for all scenes (FScene::Tick): snapshot the live world into a
     * by-value batch consumed later by the render thread. Does not touch the GPU.
     */
    virtual void Tick() = 0;

    /**
     * @brief Main thread: Finish the previously kicked frame.
     * Waits for the in-flight scene-render task (its scene command list is already on the RHI FIFO),
     * then dispatches the UI/present command list recorded for that frame. Keeps one GPU frame in
     * flight. A no-op if no frame is pending (first frame / after a flush).
     */
    virtual void FinishPreviousFrame() = 0;

    /**
     * @brief Main thread: Drain the previously kicked frame without presenting it.
     * Waits for the in-flight scene-render task (so the render thread no longer touches any
     * resources) and clears the pending-frame state, but does NOT dispatch the UI/present command
     * list. Used at shutdown when the window/surface is being torn down and the owed frame is unseen.
     * A no-op if no frame is pending.
     */
    virtual void DiscardPendingFrame() = 0;

    /**
     * @brief Main thread: Record ImGui draw data for the current frame into the UI command list.
     * Kept on the main thread so editor multi-viewport OS-window callbacks stay on the main thread.
     */
    virtual void RecordUI() = 0;

    /**
     * @brief Main thread: Kick the scene render for the current frame onto the render thread.
     * The render task records BeginFrame + scene apply/cull + RenderSceneView into the scene command
     * list and dispatches it to the RHI FIFO. Returns immediately so the main thread can advance.
     */
    virtual void KickSceneRender(FSceneRenderPacket&& Packet) = 0;
 
    /** @brief Request an async editor ObjectID pick at the given pixel (in render target space). */ 
    virtual void RequestEditorObjectPick(IScene* Scene, uint32 PixelX, uint32 PixelY) = 0; 
 
    /** @brief Poll for a completed editor ObjectID pick. Returns true if a result was produced. */ 
    virtual bool PollEditorObjectPickResult(IScene* Scene, uint32& OutObjectID) = 0; 
 
    /**
     * @brief Queue an extent/format/color-space change for a swap-chain. The change is coalesced 
     * with any other pending request for the same swap-chain and applied as a single RHI command 
     * at the start of the next frame.
     */
    virtual void ResizeSwapChain(FRHISwapChainRef SwapChain, uint32 InWidth, uint32 InHeight, EFormat InFormat = EFormat::Unknown, EColorSpace InColorSpace = EColorSpace::Unknown) = 0; 

    /** @brief Release the Renderer from the EngineLoop */
    virtual void Release() = 0;

    /** @brief Create a Renderer version of the World */
    virtual IScene* CreateScene(FWorld* InWorld) = 0;

    /** @brief Destroy a Renderer Scene */
    virtual void DestroyScene(IScene* Scene) = 0;

    /** @brief Returns the GPU profiler interface */
    virtual IGPUProfiler& GetGPUProfiler() = 0;

protected:
    static RENDERERCORE_API IRendererModule* RendererModule;
};
