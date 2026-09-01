#pragma once
#include "Core/Containers/Map.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Draw/UIDrawData.h"
#include "Application/IApplicationRenderer.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIShader.h"

class FRHICommandList;
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
        , VertexCapacity(0)
        , IndexCapacity(0)
    {
    }

    TWeakPtr<FWindow> Window;
    FDrawCommandList  Commands;
    FUIDrawData       DrawData;
    FRHISwapChain*    SwapChain;
    FRHIBufferRef     VertexBuffer;
    FRHIBufferRef     IndexBuffer;
    int32             VertexCapacity;
    int32             IndexCapacity;
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

    /**
    * @brief Compiles the shaders and creates the state objects and the white texel the renderer draws with.
    *
    * @return True when every resource was created.
    */
    bool InitializeRHI();

    /** @brief Releases every RHI resource the renderer owns. */
    void ReleaseRHI();

    /**
     * @brief Records everything gathered since the last call as an overlay on the swap chain back buffer.
     *
     * @param CommandList The list to record into.
     * @param SwapChain   The swap chain owning the back buffer to composite onto.
     */
    void Render(FRHICommandList& CommandList, FRHISwapChain* SwapChain);

    /**
     * @brief Binds a window to the swap chain its contents are presented through.
     *
     * @param InWindow  The window to bind.
     * @param SwapChain The swap chain, or null to unbind.
     */
    void RegisterWindowSwapChain(const TSharedPtr<FWindow>& InWindow, FRHISwapChain* SwapChain);

    /**
     * @brief Records one window into its own registered swap chain, acquiring its back buffer first. A
     * window lays itself out from its own top left corner, so its geometry only lands where the user sees
     * it when it is composited onto the surface belonging to it rather than onto another window's, and the
     * back buffer is acquired and left in the render target state whether or not there is anything to
     * draw, so the caller can transition it to Present and present unconditionally.
     *
     * @param CommandList The list to record into.
     * @param InWindow    The window to record.
     * @param LoadAction  Clear for a window the UI owns outright, Load to composite over a rendered scene.
     */
    void RenderWindowToSwapChain(FRHICommandList& CommandList, const TSharedPtr<FWindow>& InWindow, EAttachmentLoadAction LoadAction);

    /**
     * @brief Gives every window the application has opened beside the primary one a surface of its own, and
     * hands back the surfaces belonging to windows that have since closed. This runs before the windows draw,
     * because a window that gained its surface afterwards would be composited onto the primary one instead.
     *
     * @param PrimaryWindow The window the caller presents through a swap chain of its own, which is skipped.
     */
    void SyncWindowSurfaces(const TSharedPtr<FWindow>& PrimaryWindow);

    /**
     * @brief Records every window holding a surface of its own, resizing the surface to its window first.
     *
     * @param CommandList The list to record into.
     */
    void RenderWindowSurfaces(FRHICommandList& CommandList);

    /**
     * @brief Presents every window holding a surface of its own, pairing the acquire RenderWindowSurfaces did.
     *
     * @param CommandList The list to record into.
     */
    void PresentWindowSurfaces(FRHICommandList& CommandList);

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
    };

    struct FRetiredBuffer
    {
        FRHIBufferRef Buffer;
        uint64        Frame;
    };

    FWindowDrawState*       FindWindowState(const TSharedPtr<FWindow>& InWindow);
    FWindowDrawState*       FindOrAddWindowState(const TSharedPtr<FWindow>& InWindow);
    bool                    PreparePipelineState(EFormat OutputFormat);
    bool                    PrepareGeometry(FRHICommandList& CommandList, FWindowDrawState& WindowState);
    FRHIShaderResourceView* PrepareAtlasTexture(FRHICommandList& CommandList, const FFontAtlas* Atlas);
    FRHIShaderResourceView* PrepareBrushTexture(FRHICommandList& CommandList, FRHITexture* Texture);
    void                    PrepareBatchTextures(FRHICommandList& CommandList, const FUIDrawData& DrawData);
    void                    RenderWindow(FRHICommandList& CommandList, const FWindowDrawState& WindowState);
    FRHIShaderResourceView* GetDefaultShaderResourceView() const;
    FRHIShaderResourceView* GetAtlasShaderResourceView(const FFontAtlas* Atlas) const;
    FRHIShaderResourceView* GetBatchShaderResourceView(const FUITextureHandle& Texture) const;
    void                    ReleaseWindowSurfaces();
    void                    RetireWindowBuffers(FWindowDrawState& WindowState);
    void                    ReleaseRetiredBuffers(bool bReleaseEverything);

    TArray<FWindowDrawState>             WindowStates;
    TArray<FWindowSurface>               WindowSurfaces;
    TArray<FRetiredBuffer>               RetiredBuffers;
    uint64                               FrameCounter;
    FRHIVertexShaderRef                  VShader;
    FRHIPixelShaderRef                   PShader;
    FRHIInputLayoutRef                   InputLayout;
    FRHIDepthStencilStateRef             DepthStencilState;
    FRHIRasterizerStateRef               RasterizerState;
    FRHIBlendStateRef                    BlendState;
    FRHISamplerStateRef                  LinearSampler;
    FRHIGraphicsPipelineStateRef         PipelineState;
    FRHITextureRef                       DefaultTexture;
    TMap<const FFontAtlas*, FAtlasEntry> AtlasTextures;
    EFormat                              PipelineStateFormat;
};
