#pragma once
#include "VulkanQueue.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipelineState.h"
#include "VulkanDescriptorCache.h"

class FVulkanCommandContext;

struct FVulkanCommandContextState : public FVulkanDeviceChild, public FNonCopyAndNonMovable
{
    FVulkanCommandContextState(FVulkanDevice* InDevice, FVulkanCommandContext& InContext);
    ~FVulkanCommandContextState() = default;

    bool Initialize();

    void BindGraphicsStates();
    void BindComputeState();
    void BindDescriptorSets(FVulkanPipelineLayout* PipelineLayout, EShaderVisibility StartStage, EShaderVisibility EndStage);
    void BindPushConstants(FVulkanPipelineLayout* PipelineLayout);
    
    void ResetState();
    void ResetStateResources();
    void ResetStateForNewCommandBuffer();

    void SetGraphicsPipelineState(FVulkanGraphicsPipelineState* InGraphicsPipelineState);
    void SetComputePipelineState(FVulkanComputePipelineState* InComputePipelineState);

    void SetViewports(VkViewport* Viewports, uint32 NumViewports);
    void SetScissorRects(VkRect2D* ScissorRects, uint32 NumScissorRects);

    void SetBlendFactor(const float BlendFactor[4]);

    void SetVertexBuffer(FVulkanBuffer* VertexBuffer, uint32 VertexBufferSlot);
    void SetIndexBuffer(FVulkanBuffer* IndexBuffer, VkIndexType IndexFormat);

    void SetSRV(FVulkanShaderResourceView* ShaderResourceView, EShaderVisibility ShaderStage, uint32 ResourceIndex);
    void SetUAV(FVulkanUnorderedAccessView* UnorderedAccessView, EShaderVisibility ShaderStage, uint32 ResourceIndex);
    void SetCBV(FVulkanBuffer* ConstantBuffer, EShaderVisibility ShaderStage, uint32 ResourceIndex);
    void SetSampler(FVulkanSamplerState* SamplerState, EShaderVisibility ShaderStage, uint32 SamplerIndex);

    void SetPushConstants(const uint32* ShaderConstants, uint32 NumShaderConstants);

public:
    FORCEINLINE FVulkanCommandContext& GetContext()
    {
        return Context;
    }
    
    FORCEINLINE FVulkanDescriptorSetCache& GetDescriptorSetCache()
    {
        return CommonState.DescriptorSetCache;
    }

public:
    FORCEINLINE FVulkanGraphicsPipelineState* GetGraphicsPipelineState() const
    {
        return GraphicsState.PipelineState.Get();
    }

    FORCEINLINE FVulkanComputePipelineState* GetComputePipelineState() const
    {
        return ComputeState.PipelineState.Get();
    }

    FORCEINLINE void GetViewports(VkViewport* Viewports, uint32& OutNumViewports) const
    {
        if (Viewports)
        {
            FMemory::Memcpy(Viewports, GraphicsState.Viewports, sizeof(VkViewport) * GraphicsState.NumViewports);
        }

        OutNumViewports = GraphicsState.NumViewports;
    }

    FORCEINLINE void GetViewports(VkRect2D* ScissorRects, uint32& OutNumScissorRects) const
    {
        if (ScissorRects)
        {
            FMemory::Memcpy(ScissorRects, GraphicsState.ScissorRects, sizeof(VkRect2D) * GraphicsState.NumScissorRects);
        }

        OutNumScissorRects = GraphicsState.NumScissorRects;
    }

    FORCEINLINE void GetBlendFactor(float* BlendFactor) const
    {
        if (BlendFactor)
        {
            FMemory::Memcpy(BlendFactor, GraphicsState.BlendFactor, sizeof(GraphicsState.BlendFactor));
        }
    }

private:
    void InternalSetShaderStageResourceCount(FVulkanShader* Shader, EShaderVisibility ShaderStage);

    FVulkanCommandContext& Context;

    struct FGraphicsState
    {
        FGraphicsState()
            : PipelineState(nullptr)
            , NumViewports(0)
            , NumScissorRects(0)
            , IBCache()
            , VBCache()
        {
            FMemory::Memzero(BlendFactor, sizeof(BlendFactor));
            FMemory::Memzero(Viewports, sizeof(Viewports));
            FMemory::Memzero(ScissorRects, sizeof(ScissorRects));
        }

        FVulkanGraphicsPipelineStateRef PipelineState;

        float BlendFactor[4];

        VkViewport Viewports[VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32     NumViewports;

        VkRect2D   ScissorRects[VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32     NumScissorRects;

        FVulkanIndexBufferCache  IBCache;
        FVulkanVertexBufferCache VBCache;

        bool bBindBlendFactor   : 1;
        bool bBindPipelineState : 1;
        bool bBindScissorRects  : 1;
        bool bBindViewports     : 1;
        bool bBindVertexBuffers : 1;
        bool bBindIndexBuffer   : 1;
        bool bBindPushConstants : 1;
    } GraphicsState;

    struct FComputeState
    {
        FComputeState()
            : PipelineState(nullptr)
        {
        }

        FVulkanComputePipelineStateRef PipelineState;

        bool bBindPipelineState : 1;
        bool bBindPushConstants : 1;
    } ComputeState;

    struct FCommonState
    {
        FCommonState(FVulkanDevice* InDevice, FVulkanCommandContext& InContext)
            : DescriptorSetCache(InDevice, InContext)
            , CurrentPipelineLayout(nullptr)
        {
        }

        FVulkanConstantBufferCache      ConstantBufferCache;
        FVulkanShaderResourceViewCache  ShaderResourceViewCache;
        FVulkanUnorderedAccessViewCache UnorderedAccessViewCache;
        FVulkanSamplerStateCache        SamplerStateCache;

        FVulkanDescriptorSetCache       DescriptorSetCache;
        FVulkanPushConstantsCache       PushConstantsCache;

        FVulkanPipelineLayout*          CurrentPipelineLayout;
    } CommonState;
};
