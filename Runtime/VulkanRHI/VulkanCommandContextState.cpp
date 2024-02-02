#include "VulkanCommandContextState.h"

FVulkanCommandContextState::FVulkanCommandContextState(FVulkanDevice* InDevice, FVulkanCommandContext& InContext)
    : FVulkanDeviceChild(InDevice)
    , Context(InContext)
    , GraphicsState()
    , ComputeState()
    , CommonState(InDevice, InContext)
{
}

bool FVulkanCommandContextState::Initialize()
{
    if (!CommonState.DescriptorSetCache.Initialize())
    {
       VULKAN_ERROR("Failed to initialize DescriptorSetCache");
       return false;
    }

    ResetState();
    return true;
}

void FVulkanCommandContextState::BindGraphicsStates()
{
    // If there are no Pipeline, then there is no need to perform anything in here
    if (!GraphicsState.PipelineState)
    {
        return;
    }
    
    VkPipelineLayout PipelineLayout = GraphicsState.PipelineState->GetVkPipelineLayout();
    if (GraphicsState.bBindPipelineState || GVulkanForceBinding)
    {
        VkPipeline Pipeline = GraphicsState.PipelineState->GetVkPipeline();
        Context.GetCommandBuffer()->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
        GraphicsState.bBindPipelineState = false;
    }

    BindDescriptorSets(PipelineLayout, ShaderVisibility_Vertex, ShaderVisibility_Pixel, GVulkanForceBinding);

    if (GraphicsState.bBindPushConstants || GVulkanForceBinding)
    {
        BindPushConstants(PipelineLayout);
        GraphicsState.bBindPushConstants = false;
    }

    if (GraphicsState.bBindVertexBuffers || GVulkanForceBinding)
    {
        CommonState.DescriptorSetCache.SetVertexBuffers(GraphicsState.VBCache);
        GraphicsState.bBindVertexBuffers = false;
    }

    if (GraphicsState.bBindIndexBuffer || GVulkanForceBinding)
    {
        CommonState.DescriptorSetCache.SetIndexBuffer(GraphicsState.IBCache);
        GraphicsState.bBindIndexBuffer = false;
    }

    if (GraphicsState.bBindViewports || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetViewport(0, GraphicsState.NumViewports, GraphicsState.Viewports);
        GraphicsState.bBindViewports = false;
    }

    if (GraphicsState.bBindScissorRects || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetScissor(0, GraphicsState.NumScissorRects, GraphicsState.ScissorRects);
        GraphicsState.bBindScissorRects = false;
    }

    if (GraphicsState.bBindBlendFactor || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetBlendConstants(GraphicsState.BlendFactor);
        GraphicsState.bBindBlendFactor = false;
    }
}

void FVulkanCommandContextState::BindComputeState()
{
    // If there are no Pipeline, then there is no need to perform anything in here
    if (!ComputeState.PipelineState)
    {
        return;
    }
    
    VkPipelineLayout PipelineLayout = ComputeState.PipelineState->GetVkPipelineLayout();
    if (ComputeState.bBindPipelineState || GVulkanForceBinding)
    {
        VkPipeline Pipeline = ComputeState.PipelineState->GetVkPipeline();
        Context.GetCommandBuffer()->BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);
        ComputeState.bBindPipelineState = false;
    }

    BindDescriptorSets(PipelineLayout, ShaderVisibility_Compute, ShaderVisibility_Compute, GVulkanForceBinding);

    if (ComputeState.bBindPushConstants || GVulkanForceBinding)
    {
        BindPushConstants(PipelineLayout);
        ComputeState.bBindPushConstants = false;
    }
}

void FVulkanCommandContextState::BindDescriptorSets(VkPipelineLayout PipelineLayout, EShaderVisibility StartStage, EShaderVisibility EndStage, bool bForceBinding)
{
    for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
    {
        VkDescriptorSetLayout DescriptorSetLayout;
        if (CurrentStage == ShaderVisibility_Compute)
        {
            DescriptorSetLayout = ComputeState.PipelineState->GetVkDescriptorSetLayout();
        }
        else
        {
            DescriptorSetLayout = GraphicsState.PipelineState->GetVkDescriptorSetLayout(CurrentStage);
        }
        
        CHECK(DescriptorSetLayout != VK_NULL_HANDLE);
        
        // TODO: Validate that we actually have all the descriptors in the DescriptorPool that the DescriptorSetLayout wants
        if (!CommonState.DescriptorSetCache.AllocateDescriptorSets(CurrentStage, DescriptorSetLayout))
        {
            VULKAN_ERROR("Failed to Allocate and Update DescriptorSets");
            return;
        }

        // Set resources that are going to be written to the DescriptorSet
        CommonState.DescriptorSetCache.SetSRVs(CommonState.ShaderResourceViewCache, CurrentStage, VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);
        CommonState.DescriptorSetCache.SetUAVs(CommonState.UnorderedAccessViewCache, CurrentStage, VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);
        CommonState.DescriptorSetCache.SetConstantBuffers(CommonState.ConstantBufferCache, CurrentStage, VULKAN_DEFAULT_CONSTANT_BUFFER_COUNT);
        CommonState.DescriptorSetCache.SetSamplers(CommonState.SamplerStateCache, CurrentStage, VULKAN_DEFAULT_SAMPLER_STATE_COUNT);
        
        // Bind DescriptorSet for this ShaderStage
        CommonState.DescriptorSetCache.SetDescriptorSet(PipelineLayout, CurrentStage);
    }
}

void FVulkanCommandContextState::BindPushConstants(VkPipelineLayout PipelineLayout)
{
    if (CommonState.PushConstantsCache.NumConstants > 0)
    {
        Context.GetCommandBuffer()->PushConstants(PipelineLayout, VK_SHADER_STAGE_ALL, 0, CommonState.PushConstantsCache.NumConstants * sizeof(uint32), CommonState.PushConstantsCache.Constants);
    }
}

void FVulkanCommandContextState::ResetState()
{
    CommonState.PushConstantsCache.Clear();

    CommonState.ConstantBufferCache.Clear();
    CommonState.SamplerStateCache.Clear();
    CommonState.ShaderResourceViewCache.Clear();
    CommonState.UnorderedAccessViewCache.Clear();

    GraphicsState.VBCache.Clear();
    GraphicsState.IBCache.Clear();

    FMemory::Memzero(GraphicsState.BlendFactor, sizeof(GraphicsState.BlendFactor));
    FMemory::Memzero(GraphicsState.Viewports, sizeof(GraphicsState.Viewports));
    GraphicsState.NumViewports = 0;

    FMemory::Memzero(GraphicsState.ScissorRects, sizeof(GraphicsState.ScissorRects));
    GraphicsState.NumScissorRects = 0;
    
    GraphicsState.PipelineState      = nullptr;
    GraphicsState.bBindIndexBuffer   = true;
    GraphicsState.bBindBlendFactor   = true;
    GraphicsState.bBindPipelineState = true;
    GraphicsState.bBindScissorRects  = true;
    GraphicsState.bBindViewports     = true;
    GraphicsState.bBindVertexBuffers = true;
    GraphicsState.bBindPushConstants = true;

    ComputeState.PipelineState      = nullptr;
    ComputeState.bBindPipelineState = true;
    ComputeState.bBindPushConstants = true;
}

void FVulkanCommandContextState::ResetStateResources()
{
    CommonState.ConstantBufferCache.DirtyStateAll();
    CommonState.ShaderResourceViewCache.DirtyStateAll();
    CommonState.UnorderedAccessViewCache.DirtyStateAll();
}

void FVulkanCommandContextState::ResetStateForNewCommandBuffer()
{
    CommonState.ConstantBufferCache.DirtyStateAll();
    CommonState.ShaderResourceViewCache.DirtyStateAll();
    CommonState.UnorderedAccessViewCache.DirtyStateAll();
    CommonState.SamplerStateCache.DirtyStateAll();
    
    GraphicsState.bBindIndexBuffer   = true;
    GraphicsState.bBindBlendFactor   = true;
    GraphicsState.bBindPipelineState = true;
    GraphicsState.bBindScissorRects  = true;
    GraphicsState.bBindViewports     = true;
    GraphicsState.bBindVertexBuffers = true;
    GraphicsState.bBindPushConstants = true;

    ComputeState.bBindPipelineState = true;
    ComputeState.bBindPushConstants = true;
}

void FVulkanCommandContextState::SetGraphicsPipelineState(FVulkanGraphicsPipelineState* InGraphicsPipelineState)
{
    FVulkanGraphicsPipelineState* CurrentGraphicsPipelineState = GraphicsState.PipelineState.Get();
    if (CurrentGraphicsPipelineState != InGraphicsPipelineState || GVulkanForceBinding)
    {
        if (InGraphicsPipelineState)
        {
            if (FVulkanVertexShader* VertexShader = InGraphicsPipelineState->GetVertexShader())
            {
                InternalSetShaderStageResourceCount(VertexShader, ShaderVisibility_Vertex);
            }
            if (FVulkanDomainShader* DomainShader = InGraphicsPipelineState->GetDomainShader())
            {
                InternalSetShaderStageResourceCount(DomainShader, ShaderVisibility_Domain);
            }
            if (FVulkanHullShader* HullShader = InGraphicsPipelineState->GetHullShader())
            {
                InternalSetShaderStageResourceCount(HullShader, ShaderVisibility_Hull);
            }
            if (FVulkanGeometryShader* GeometryShader = InGraphicsPipelineState->GetGeometryShader())
            {
                InternalSetShaderStageResourceCount(GeometryShader, ShaderVisibility_Geometry);
            }
            if (FVulkanPixelShader* PixelShader = InGraphicsPipelineState->GetPixelShader())
            {
                InternalSetShaderStageResourceCount(PixelShader, ShaderVisibility_Pixel);
            }
            
            GraphicsState.PipelineState = MakeSharedRef<FVulkanGraphicsPipelineState>(InGraphicsPipelineState);
        }

        GraphicsState.bBindPipelineState = true;
    }
}

void FVulkanCommandContextState::SetComputePipelineState(FVulkanComputePipelineState* InComputePipelineState)
{
    FVulkanComputePipelineState* CurrentComputePipelineState = ComputeState.PipelineState.Get();
    if (CurrentComputePipelineState != InComputePipelineState || GVulkanForceBinding)
    {
        if (InComputePipelineState)
        {
            if (FVulkanComputeShader* PixelShader = InComputePipelineState->GetComputeShader())
            {
                InternalSetShaderStageResourceCount(PixelShader, ShaderVisibility_Compute);
            }
            
            ComputeState.PipelineState = MakeSharedRef<FVulkanComputePipelineState>(InComputePipelineState);
        }

        ComputeState.bBindPipelineState = true;
    }
}

void FVulkanCommandContextState::SetViewports(VkViewport* Viewports, uint32 NumViewports)
{
    CHECK(NumViewports < VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT);

    const uint32 ViewportArraySize = sizeof(VkViewport) * NumViewports;
    if (GraphicsState.NumViewports != NumViewports || FMemory::Memcmp(GraphicsState.Viewports, Viewports, ViewportArraySize) != 0 || GVulkanForceBinding)
    {
        FMemory::Memcpy(GraphicsState.Viewports, Viewports, ViewportArraySize);
        GraphicsState.NumViewports   = NumViewports;
        GraphicsState.bBindViewports = true;
    }
}

void FVulkanCommandContextState::SetScissorRects(VkRect2D* ScissorRects, uint32 NumScissorRects)
{
    CHECK(NumScissorRects < VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT);

    const uint32 ScissorRectArraySize = sizeof(VkRect2D) * NumScissorRects;
    if (GraphicsState.NumScissorRects != NumScissorRects || FMemory::Memcmp(GraphicsState.ScissorRects, ScissorRects, ScissorRectArraySize) != 0 || GVulkanForceBinding)
    {
        FMemory::Memcpy(GraphicsState.ScissorRects, ScissorRects, ScissorRectArraySize);
        GraphicsState.NumScissorRects   = NumScissorRects;
        GraphicsState.bBindScissorRects = true;
    }
}

void FVulkanCommandContextState::SetBlendFactor(const float BlendFactor[4])
{
    if (FMemory::Memcmp(GraphicsState.BlendFactor, BlendFactor, sizeof(GraphicsState.BlendFactor)) != 0 || GVulkanForceBinding)
    {
        FMemory::Memcpy(GraphicsState.BlendFactor, BlendFactor, sizeof(GraphicsState.BlendFactor));
        GraphicsState.bBindBlendFactor = true;
    }
}

void FVulkanCommandContextState::SetVertexBuffer(FVulkanBuffer* VertexBuffer, uint32 VertexBufferSlot)
{
    CHECK(VertexBufferSlot < VULKAN_MAX_VERTEX_BUFFER_SLOTS);
    
    VkBuffer     Buffer;
    VkDeviceSize Offset;
    if (VertexBuffer)
    {
        Buffer = VertexBuffer->GetVkBuffer();
        Offset = 0;
    }
    else
    {
        Buffer = VK_NULL_HANDLE;
        Offset = 0;
    }

    VkBuffer     CurrentBuffer = GraphicsState.VBCache.VertexBuffers[VertexBufferSlot];
    VkDeviceSize CurrentOffset = GraphicsState.VBCache.VertexBufferOffsets[VertexBufferSlot];
    if (Buffer != CurrentBuffer || Offset != CurrentOffset || GVulkanForceBinding)
    {
        GraphicsState.VBCache.VertexBuffers[VertexBufferSlot]       = Buffer;
        GraphicsState.VBCache.VertexBufferOffsets[VertexBufferSlot] = Offset;
        GraphicsState.VBCache.NumVertexBuffers = FMath::Max(GraphicsState.VBCache.NumVertexBuffers, VertexBufferSlot + 1);
        GraphicsState.bBindVertexBuffers       = true;
    }
}

void FVulkanCommandContextState::SetIndexBuffer(FVulkanBuffer* IndexBuffer, VkIndexType IndexType)
{
    VkBuffer     Buffer;
    VkDeviceSize Offset;
    if (IndexBuffer)
    {
        Buffer = IndexBuffer->GetVkBuffer();
        Offset = 0;
    }
    else
    {
        Buffer = VK_NULL_HANDLE;
        Offset = 0;
    }

    VkBuffer     CurrentBuffer    = GraphicsState.IBCache.IndexBuffer;
    VkDeviceSize CurrentOffset    = GraphicsState.IBCache.Offset;
    VkIndexType  CurrentIndexType = GraphicsState.IBCache.IndexType;
    if (Buffer != CurrentBuffer || Offset != CurrentOffset || IndexType != CurrentIndexType || GVulkanForceBinding)
    {
        GraphicsState.IBCache.IndexBuffer = Buffer;
        GraphicsState.IBCache.Offset      = Offset;
        GraphicsState.IBCache.IndexType   = IndexType;
        GraphicsState.bBindIndexBuffer    = true;
    }
}

void FVulkanCommandContextState::SetSRV(FVulkanShaderResourceView* ShaderResourceView, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    auto& SRVCache = CommonState.ShaderResourceViewCache.ResourceViews[ShaderStage];
    if (SRVCache[ResourceIndex] != ShaderResourceView || GVulkanForceBinding)
    {
        SRVCache[ResourceIndex] = ShaderResourceView;
        CommonState.ShaderResourceViewCache.NumViews[ShaderStage] = FMath::Max<uint8>(CommonState.ShaderResourceViewCache.NumViews[ShaderStage], static_cast<uint8>(ResourceIndex) + 1);
        CommonState.ShaderResourceViewCache.bDirty[ShaderStage]   = true;
    }
}

void FVulkanCommandContextState::SetUAV(FVulkanUnorderedAccessView* UnorderedAccessView, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    auto& UAVCache = CommonState.UnorderedAccessViewCache.ResourceViews[ShaderStage];
    if (UAVCache[ResourceIndex] != UnorderedAccessView || GVulkanForceBinding)
    {
        UAVCache[ResourceIndex] = UnorderedAccessView;
        CommonState.UnorderedAccessViewCache.NumViews[ShaderStage] = FMath::Max<uint8>(CommonState.UnorderedAccessViewCache.NumViews[ShaderStage], static_cast<uint8>(ResourceIndex) + 1);
        CommonState.UnorderedAccessViewCache.bDirty[ShaderStage]   = true;
    }
}

void FVulkanCommandContextState::SetCBV(FVulkanBuffer* ConstantBuffer, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    auto& CBVCache = CommonState.ConstantBufferCache.ConstantBuffers[ShaderStage];
    if (CBVCache[ResourceIndex] != ConstantBuffer || GVulkanForceBinding)
    {
        CBVCache[ResourceIndex] = ConstantBuffer;
        CommonState.ConstantBufferCache.NumBuffers[ShaderStage] = FMath::Max<uint8>(CommonState.ConstantBufferCache.NumBuffers[ShaderStage], static_cast<uint8>(ResourceIndex) + 1);
        CommonState.ConstantBufferCache.bDirty[ShaderStage]     = true;
    }
}

void FVulkanCommandContextState::SetSampler(FVulkanSamplerState* SamplerState, EShaderVisibility ShaderStage, uint32 SamplerIndex)
{
    auto& SamplerCache = CommonState.SamplerStateCache.SamplerStates[ShaderStage];
    if (SamplerCache[SamplerIndex] != SamplerState || GVulkanForceBinding)
    {
        SamplerCache[SamplerIndex] = SamplerState;
        CommonState.SamplerStateCache.NumSamplers[ShaderStage] = FMath::Max<uint8>(CommonState.SamplerStateCache.NumSamplers[ShaderStage], static_cast<uint8>(SamplerIndex) + 1);
        CommonState.SamplerStateCache.bDirty[ShaderStage]      = true;
    }
}

void FVulkanCommandContextState::SetPushConstants(const uint32* ShaderConstants, uint32 NumShaderConstants)
{
    FVulkanPushConstantsCache& ConstantCache = CommonState.PushConstantsCache;
    if (NumShaderConstants != ConstantCache.NumConstants || FMemory::Memcmp(ShaderConstants, ConstantCache.Constants, sizeof(uint32) * NumShaderConstants) != 0 || GVulkanForceBinding)
    {
        FMemory::Memcpy(ConstantCache.Constants, ShaderConstants, sizeof(uint32) * NumShaderConstants);
        ConstantCache.NumConstants       = NumShaderConstants;
        GraphicsState.bBindPushConstants = true;
        ComputeState.bBindPushConstants  = true;
    }
}

void FVulkanCommandContextState::InternalSetShaderStageResourceCount(FVulkanShader* Shader, EShaderVisibility ShaderStage)
{
    CHECK(Shader != nullptr);
    UNREFERENCED_VARIABLE(ShaderStage);

    //const FShaderResourceCount& ResourceCount = Shader->GetResourceCount();
    //CommonState.ShaderResourceCounts[ShaderStage] = ResourceCount.Ranges;
}
