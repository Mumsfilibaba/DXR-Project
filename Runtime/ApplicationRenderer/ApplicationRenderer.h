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

private:
    struct FAtlasEntry
    {
        FRHITextureRef Texture;
        uint64         Revision;
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

    TArray<FWindowDrawState>             WindowStates;
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
