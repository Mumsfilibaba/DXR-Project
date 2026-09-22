#pragma once
#include "Core/Containers/Map.h"
#include "Core/Platform/PlatformEvent.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Draw/UIDrawData.h"
#include "Application/Draw/UIPaintStats.h"
#include "Application/IApplicationRenderer.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIShader.h"
#include "RHI/RHICommandList.h"
#include "RendererCore/Interfaces/IGPUProfiler.h"

class FFontAtlas;
class FWindow;

struct FWindowDrawState
{
    FWindowDrawState()
        : Window()
        , Commands()
        , DrawData()
        , SwapChain(nullptr)
        , VertexBuffer(nullptr)
        , IndexBuffer(nullptr)
        , ShapeVertexBuffer(nullptr)
        , ShapeIndexBuffer(nullptr)
        , TextGlyphBuffer(nullptr)
        , VertexCapacity(0)
        , IndexCapacity(0)
        , ShapeVertexCapacity(0)
        , ShapeIndexCapacity(0)
        , TextGlyphCapacity(0)
        , UploadedGeometryHash(0)
        , IndexFormat(EIndexFormat::uint32)
        , ShapeIndexFormat(EIndexFormat::uint32)
        , Stats()
        , AccumulatedStats()
        , WalkStartTime(0)
        , TimedFrameCount(0)
    {
    }

    TWeakPtr<FWindow> Window;
    FDrawCommandList  Commands;
    FUIDrawData       DrawData;
    FRHISwapChain*    SwapChain;
    FRHIBufferRef     VertexBuffer;
    FRHIBufferRef     IndexBuffer;
    FRHIBufferRef     ShapeVertexBuffer;
    FRHIBufferRef     ShapeIndexBuffer;
    FRHIBufferRef     TextGlyphBuffer;
    int32             VertexCapacity;
    int32             IndexCapacity;
    int32             ShapeVertexCapacity;
    int32             ShapeIndexCapacity;
    int32             TextGlyphCapacity;
    uint64            UploadedGeometryHash;
    EIndexFormat      IndexFormat;
    EIndexFormat      ShapeIndexFormat;
    FUIPaintStats     Stats;
    FUIPaintStats     AccumulatedStats;
    uint64            WalkStartTime;
    int32             TimedFrameCount;
};

class APPLICATIONRENDERER_API FApplicationRenderer final : public IApplicationRenderer
{
public:
    FApplicationRenderer();
    virtual ~FApplicationRenderer();
    
    // IApplicationRenderer Interface
    virtual FDrawCommandList* BeginWindow(const TSharedPtr<FWindow>& InWindow) override final;
    virtual void EndWindow(const TSharedPtr<FWindow>& InWindow) override final;
    virtual void OnWindowDestroyed(const TSharedPtr<FWindow>& InWindow) override final;
    virtual FRHITextureRef RenderElementToTexture(const TSharedPtr<FVisualElement>& Element, const IntVector2& Size, float DPIScale) override final;
    virtual void RetireTexture(const FRHITextureRef& Texture) override final;
    virtual void SetPrimaryWindow(const TSharedPtr<FWindow>& InWindow) override final;
    virtual void EnsureWindowSurface(const TSharedPtr<FWindow>& InWindow) override final;
    virtual FRHISwapChainRef GetWindowSwapChain(const TSharedPtr<FWindow>& InWindow) const override final;

    /**
    * @brief Compiles the shaders and creates the state objects and the white texel the renderer draws with.
    *
    * @return True when every resource was created.
    */
    bool InitializeRHI();

    /** @brief Releases every RHI resource the renderer owns. */
    void ReleaseRHI();

    /**
     * @brief Names the profiler the frame boundary is reported to, which the renderer does not own. The
     * closing half of the measure is recorded here because the UI list is the last one a frame submits,
     * while the opening half is recorded by whoever submits the first.
     *
     * @param InGPUProfiler The profiler, or null to stop reporting.
     */
    void SetGPUProfiler(IGPUProfiler* InGPUProfiler)
    {
        GPUProfiler = InGPUProfiler;
    }

    /**
     * @brief Opens the frame, reconciling the set of surfaces against the set of windows first, since a
     * window that gained its surface after the windows drew would have nothing to present through.
     */
    void BeginFrame();

    /** @brief Lays out and records every window into the surface belonging to it. */
    void RecordWindows();

    /** @return The list the frame records into, which is only valid between BeginFrame and EndFrameAndPresent. */
    NODISCARD FORCEINLINE FRHICommandList& GetCommandList()
    {
        return CommandList;
    }

    /**
     * @brief Presents every surface and submits the frame, then holds the calling thread until the frame
     * before it has left the GPU, which is what keeps exactly one frame in flight.
     */
    void EndFrameAndPresent();

    /**
     * @brief Repaints one window at its current size on a list of its own and flushes it, leaving the frame
     * the loop is recording untouched, so a live resize shows each step of the drag rather than one frame in
     * however many the platform takes over. Does nothing for a window still waiting on its first present,
     * since stepping its deferred show along would put it on screen before the frame loop means to.
     *
     * @param InWindow The window to repaint, which is expected to already carry the size being painted.
     */
    void RedrawWindow(const TSharedPtr<FWindow>& InWindow);

private:
    static constexpr uint64 NumRetiredFrames = 3;

    enum class EDeferredShowState : uint8
    {
        // The window is already visible, either because it was created that way or because it has been shown
        None = 0,

        // Created hidden, and no present naming its surface has been recorded yet
        AwaitingPresent,

        // A present has been recorded, and the list carrying it is dispatched at the end of this frame
        PresentRecorded,
    };
    
    struct FAtlasEntry
    {
        FRHITextureRef Texture;
        uint64         Revision;
    };

    struct FWindowSurface
    {
        TSharedPtr<FWindow> Window;
        FRHISwapChainRef    SwapChain;
        IntVector2          Size;
        EDeferredShowState  DeferredShowState = EDeferredShowState::None;
        bool                bIsPrimary        = false;
        bool                bIsRendered       = false;
    };

    struct FRetiredBuffer
    {
        FRHIBufferRef Buffer;
        uint64        Frame;
    };

    struct FRetiredTexture
    {
        FRHITextureRef Texture;
        uint64         Frame;
    };

    FWindowDrawState*       FindWindowState(const TSharedPtr<FWindow>& InWindow);
    FWindowDrawState*       FindOrAddWindowState(const TSharedPtr<FWindow>& InWindow);
    const FWindowSurface*   FindWindowSurface(const TSharedPtr<FWindow>& InWindow) const;
    bool                    AddWindowSurface(const TSharedPtr<FWindow>& InWindow);
    void                    SyncWindowSurfaces();
    void                    RegisterWindowSwapChain(const TSharedPtr<FWindow>& InWindow, FRHISwapChain* SwapChain);
    void                    RenderWindowToSwapChain(FRHICommandList& InCommandList, const TSharedPtr<FWindow>& InWindow, EAttachmentLoadAction LoadAction);
    void                    RenderWindowSurfaces(FRHICommandList& InCommandList);
    void                    PresentWindowSurfaces(FRHICommandList& InCommandList);
    FWindowSurface*         FindRedrawableSurface(const TSharedPtr<FWindow>& InWindow);
    void                    RedrawWindowSurface(FRHICommandList& InCommandList, FWindowSurface& Surface);
    bool                    PreparePipelineState(EFormat OutputFormat);
    bool                    PrepareGeometry(FRHICommandList& InCommandList, FWindowDrawState& WindowState);
    
    bool UploadStream(
        FRHICommandList& InCommandList, 
        FRHIBufferRef&   VertexBuffer, 
        FRHIBufferRef&   IndexBuffer,
        int32&           VertexCapacity,
        int32&           IndexCapacity,
        EIndexFormat&    IndexFormat,
        const void*      Vertices,
        int32            VertexStride,
        int32            VertexCount,
        const uint32*    Indices,
        int32            IndexCount,
        const CHAR*      VertexDebugName,
        const CHAR*      IndexDebugName);
    
    bool UploadVertexStream(
        FRHICommandList& InCommandList,
        FRHIBufferRef&   VertexBuffer,
        int32&           VertexCapacity,
        const void*      Vertices,
        int32            VertexStride,
        int32            VertexCount,
        const CHAR*      VertexDebugName);

    FRHIShaderResourceView* PrepareAtlasTexture(FRHICommandList& InCommandList, const FFontAtlas* Atlas);
    FRHIShaderResourceView* PrepareBrushTexture(FRHICommandList& InCommandList, FRHITexture* Texture);
    void                    PrepareBatchTextures(FRHICommandList& InCommandList, const FUIDrawData& DrawData);
    void                    RenderWindow(FRHICommandList& InCommandList, const FWindowDrawState& WindowState);
    void                    RenderDrawData(FRHICommandList& InCommandList, const FWindowDrawState& WindowState, const IntVector2& GeometrySize, float SupersampleScale);
    FRHIShaderResourceView* GetDefaultShaderResourceView() const;
    FRHIShaderResourceView* GetAtlasShaderResourceView(const FFontAtlas* Atlas) const;
    FRHIShaderResourceView* GetBatchShaderResourceView(const FUITextureHandle& Texture) const;
    void                    ReleaseWindowSurfaces();
    void                    RetireWindowBuffers(FWindowDrawState& WindowState);
    void                    ReleaseRetiredResources(bool bReleaseEverything);
    void                    ReconcileSurfaceSize(FRHICommandList& InCommandList, FWindowSurface& Surface);
    void                    ReportPaintStats(FWindowDrawState& WindowState);

    TArray<FWindowDrawState>             WindowStates;
    TArray<FWindowSurface>               WindowSurfaces;
    TWeakPtr<FWindow>                    PrimaryWindow;
    TArray<FRetiredBuffer>               RetiredBuffers;
    TArray<FRetiredTexture>              RetiredTextures;
    FRHICommandList                      CommandList;
    FRHICommandList                      ResizeCommandList;
    IGPUProfiler*                        GPUProfiler;
    IPlatformEvent*                      LastFrameFinishedEvent;
    uint64                               FrameCounter;
    bool                                 bHasOpenFrame;
    FRHIVertexShaderRef                  VShader;
    FRHIPixelShaderRef                   PShader;
    FRHIInputLayoutRef                   InputLayout;
    FRHIVertexShaderRef                  ShapeVShader;
    FRHIPixelShaderRef                   ShapePShader;
    FRHIInputLayoutRef                   ShapeInputLayout;
    FRHIVertexShaderRef                  TextVShader;
    FRHIInputLayoutRef                   TextInputLayout;
    FRHIDepthStencilStateRef             DepthStencilState;
    FRHIRasterizerStateRef               RasterizerState;
    FRHIBlendStateRef                    BlendState;
    FRHISamplerStateRef                  LinearSampler;
    FRHIGraphicsPipelineStateRef         PipelineState;
    FRHIGraphicsPipelineStateRef         ShapePipelineState;
    FRHIGraphicsPipelineStateRef         TextPipelineState;
    FRHITextureRef                       DefaultTexture;
    TMap<const FFontAtlas*, FAtlasEntry> AtlasTextures;
    EFormat                              PipelineStateFormat;
    FWindowDrawState*                    ActiveWindowState;
    TArray<uint16>                       Index16Scratch;
};
