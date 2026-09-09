#pragma once
#include "Core/Modules/ModuleManager.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Templates/Utility/EnumOperators.h"
#include "RHI/RHITypes.h"
#include "RendererCore/CameraSnapshot.h"
#include "RendererCore/Interfaces/IScene.h"

#if EDITOR_BUILD
    #include "RendererCore/Debug/RenderGraphDebug.h"
#endif

class FWorld;
class FRHITexture;
struct IGPUProfiler;

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

    enum class EDebugViewChannel : int32
    {
        None  = 0,
        Red   = FLAG(0),
        Green = FLAG(1),
        Blue  = FLAG(2),
        Alpha = FLAG(3),
        All   = Red | Green | Blue | Alpha,
    };

    IScene*           Scene                      = nullptr;
    FRHITexture*      RenderTarget               = nullptr;
    EDebugView        DebugView                  = EDebugView::None;
    EDebugView        SecondaryDebugView         = EDebugView::None;
    EDebugViewChannel DebugViewChannelMask       = EDebugViewChannel::All;
    FCameraSnapshot   CameraSnapshot             = {};
    bool              bHasCamera             : 1 = false;
    bool              bCameraCut             : 1 = false;
    bool              bEditorOverlaysEnabled : 1 = false;
};

ENUM_CLASS_OPERATORS(FSceneRenderView::EDebugViewChannel);

struct FSceneRenderPacket
{
    FSceneRenderView View;
    TArray<uint32>   SelectedObjectIDs;
    uint64           FrameIndex = 0;
};

struct FEditorPickResult
{
    uint64 RequestId   = 0;
    uint32 ObjectID    = 0;
    float  DeviceDepth = 1.0f;
    bool   bHasDepth   = false;
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
     * @brief Main thread: Wait for the in-flight scene-render task, whose command list is already on the
     * RHI FIFO, so the render thread no longer touches any of the resources the frame named. Keeps exactly
     * one scene frame in flight. A no-op if no frame is pending, which is the case on the first tick and
     * after a flush.
     */
    virtual void FinishPreviousFrame() = 0;

    /**
     * @brief Main thread: Kick the scene render for the current frame onto the render thread.
     * The render task records BeginFrame + scene apply/cull + RenderSceneView into the scene command
     * list and dispatches it to the RHI FIFO. Returns immediately so the main thread can advance.
     */
    virtual void KickSceneRender(FSceneRenderPacket&& Packet) = 0;

    /** @brief Request an async editor pick at the given pixel (in render target space). */
    virtual void RequestEditorObjectPick(IScene* Scene, uint32 PixelX, uint32 PixelY, uint64 RequestId) = 0;
 
    /** @brief Poll for a completed editor pick. Returns true if a result was produced. */
    virtual bool PollEditorObjectPickResult(IScene* Scene, FEditorPickResult& OutResult) = 0;

    /** @brief Request an async editor ObjectID pick over a rectangle (in render target space), for box-select. */
    virtual void RequestEditorObjectPickRect(IScene* Scene, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY) = 0;

    /** @brief Poll for a completed editor rectangle pick. Returns true if a result was produced, filling in every unique ObjectID found. */
    virtual bool PollEditorObjectPickRectResult(IScene* Scene, TArray<uint32>& OutObjectIDs) = 0;

    /** @brief Release the Renderer from the EngineLoop */
    virtual void Release() = 0;

    /** @brief Create a Renderer version of the World */
    virtual IScene* CreateScene(FWorld* InWorld) = 0;

    /** @brief Destroy a Renderer Scene */
    virtual void DestroyScene(IScene* Scene) = 0;

    /** @brief Returns the GPU profiler interface */
    virtual IGPUProfiler& GetGPUProfiler() = 0;

#if EDITOR_BUILD
    virtual void SetRenderGraphDebugCaptureEnabled(bool bEnabled) = 0;
    virtual bool CopyLatestRenderGraphDebugSnapshot(FRenderGraphDebugSnapshot& Out) = 0;
#endif

protected:
    static RENDERERCORE_API IRendererModule* RendererModule;
};
