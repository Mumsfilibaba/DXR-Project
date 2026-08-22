#pragma once
#include "Application/Draw/DrawCommandList.h"
#include "Application/Draw/UIDrawData.h"
#include "Application/IApplicationRenderer.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIShader.h"

class FRHICommandList;
class FFontAtlas;
class FWindowElement;

struct FWindowDrawState
{
    FWindowDrawState()
        : Window()
        , Commands()
        , DrawData()
        , VertexBuffer(nullptr)
        , IndexBuffer(nullptr)
        , VertexCapacity(0)
        , IndexCapacity(0)
    {
    }

    TWeakPtr<FWindowElement> Window;
    FDrawCommandList         Commands;
    FUIDrawData              DrawData;
    FRHIBufferRef            VertexBuffer;
    FRHIBufferRef            IndexBuffer;
    int32                    VertexCapacity;
    int32                    IndexCapacity;
};

class APPLICATIONRENDERER_API FApplicationRenderer final : public IApplicationRenderer
{
public:
    FApplicationRenderer();
    virtual ~FApplicationRenderer();
    
    // IApplicationRenderer Interface
    virtual FDrawCommandList* BeginWindow(const TSharedPtr<FWindowElement>& InWindow) override final;
    virtual void EndWindow(const TSharedPtr<FWindowElement>& InWindow) override final;
    virtual void OnWindowDestroyed(const TSharedPtr<FWindowElement>& InWindow) override final;

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

private:
    FWindowDrawState*       FindOrAddWindowState(const TSharedPtr<FWindowElement>& InWindow);
    bool                    PreparePipelineState(EFormat OutputFormat);
    bool                    PrepareGeometry(FRHICommandList& CommandList, FWindowDrawState& WindowState);
    FRHIShaderResourceView* PrepareAtlasTexture(FRHICommandList& CommandList, const FFontAtlas* Atlas);
    void                    RenderWindow(FRHICommandList& CommandList, const FWindowDrawState& WindowState);
    FRHIShaderResourceView* GetWhiteShaderResourceView() const;
    FRHIShaderResourceView* GetAtlasShaderResourceView(const FFontAtlas* Atlas) const;

    TArray<FWindowDrawState>     WindowStates;
    FRHIVertexShaderRef          VShader;
    FRHIPixelShaderRef           PShader;
    FRHIInputLayoutRef           InputLayout;
    FRHIDepthStencilStateRef     DepthStencilState;
    FRHIRasterizerStateRef       RasterizerState;
    FRHIBlendStateRef            BlendState;
    FRHISamplerStateRef          LinearSampler;
    FRHIGraphicsPipelineStateRef PipelineState;
    FRHITextureRef               WhiteTexture;
    FRHITextureRef               AtlasTexture;
    const FFontAtlas*            UploadedAtlas;
    uint64                       UploadedAtlasRevision;
    EFormat                      PipelineStateFormat;
};
