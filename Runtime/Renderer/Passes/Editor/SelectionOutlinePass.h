#pragma once
#if EDITOR_BUILD
#include "Core/Containers/ArrayView.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"

class FRenderGraphBuilder;

class FSelectionOutlinePass : public FRenderPass
{
public:
    static constexpr uint32 MaxSelectedIDs = 64;

    FSelectionOutlinePass(FSceneRenderer* InRenderer);
    virtual ~FSelectionOutlinePass();

    bool Initialize(const FFrameResources& FrameResources);
    bool CreateResources(uint32 Width, uint32 Height);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);

    FRHITexture* GetDilatedMask()       const { return DilatedMask.Get(); }
    FRHITexture* GetRingMask()          const { return RingMask.Get(); }
    FRHITexture* GetSelectionMask()     const { return SelectionMask.Get(); }
    FRHITexture* GetErosionTemp()       const { return ErosionTemp.Get(); }
    FRHITexture* GetErodedMask()        const { return ErodedMask.Get(); }
    FRHITexture* GetDilationTemp()      const { return DilationTemp.Get(); }
    FRHIBuffer*  GetSelectedIDsBuffer() const { return SelectedIDsBuffer.Get(); }

private:
    NODISCARD bool IsOutlineActive(const FFrameResources& FrameResources) const;

    void RecordSelectedIDsUpload(FRHICommandList& CommandList, TArrayView<const uint32> SelectedIDs);
    void RecordMask(FRHICommandList& CommandList, const FFrameResources& FrameResources, uint32 SelectedCount);
    void RecordErode(FRHICommandList& CommandList, const FFrameResources& FrameResources, FRHITexture* Source, int32 DirectionX, int32 DirectionY, int32 Radius, uint32 SelectedCount);
    void RecordDilate(FRHICommandList& CommandList, const FFrameResources& FrameResources, FRHITexture* Source, int32 DirectionX, int32 DirectionY, int32 Radius, uint32 SelectedCount);
    void RecordRing(FRHICommandList& CommandList, const FFrameResources& FrameResources, FRHITexture* InnerMask, uint32 SelectedCount);

    bool CreateSelectedIDsBuffer();
    bool CreatePipelineStates();

    FRHITextureRef               SelectionMask;
    FRHITextureRef               DilationTemp;
    FRHITextureRef               DilatedMask;
    FRHITextureRef               ErosionTemp;
    FRHITextureRef               ErodedMask;
    FRHITextureRef               RingMask;
    FRHIBufferRef                SelectedIDsBuffer;
    FRHIShaderResourceViewRef    SelectedIDsSRV;
    FRHIGraphicsPipelineStateRef MaskPSO;
    FRHIPixelShaderRef           MaskShader;
    FRHIGraphicsPipelineStateRef DilatePSO;
    FRHIPixelShaderRef           DilateShader;
    FRHIGraphicsPipelineStateRef ErodePSO;
    FRHIPixelShaderRef           ErodeShader;
    FRHIGraphicsPipelineStateRef ResolvePSO;
    FRHIPixelShaderRef           ResolveShader;
};

#endif
