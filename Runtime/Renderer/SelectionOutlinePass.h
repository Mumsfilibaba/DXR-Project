#pragma once
#if EDITOR_BUILD
#include "Core/Containers/ArrayView.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Renderer/FrameResources.h"
#include "Renderer/RenderPass.h"

class FSelectionOutlinePass : public FRenderPass
{
public:
    static constexpr uint32 MaxSelectedIDs = 64;

    FSelectionOutlinePass(FSceneRenderer* InRenderer);
    virtual ~FSelectionOutlinePass();

    bool Initialize(const FFrameResources& FrameResources);
    bool CreateResources(uint32 Width, uint32 Height);

    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, TArrayView<const uint32> SelectedIDs);

    FRHITexture* GetDilatedMask()   const { return DilatedMask.Get(); }
    FRHITexture* GetRingMask()      const { return RingMask.Get(); }
    FRHITexture* GetSelectionMask() const { return SelectionMask.Get(); }

private:
    bool CreateSelectedIDsBuffer();
    bool CreatePipelineStates();

private:
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

#endif // EDITOR_BUILD
