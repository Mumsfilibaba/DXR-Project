#include "VulkanCommandContextState.h"

FVulkanCommandContextState::FVulkanCommandContextState(FVulkanDevice* InDevice, FVulkanCommandContext& InContext)
    : FVulkanDeviceChild(InDevice)
    , GraphicsState()
    , ComputeState()
    , CommonState()
    , Context(InContext)
{
}

bool FVulkanCommandContextState::Initialize()
{
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

    FVulkanPipelineLayout* PipelineLayout = GraphicsState.PipelineState->GetPipelineLayout();
    CHECK(PipelineLayout != nullptr);

    if (GraphicsState.bBindPipelineState || GVulkanForceBinding)
    {
        VkPipeline Pipeline = GraphicsState.PipelineState->GetVkPipeline();
        Context.GetCommandBuffer()->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
        GraphicsState.bBindPipelineState = false;
    }

    BindDescriptorSets(PipelineLayout, ShaderVisibility_Vertex, ShaderVisibility_Pixel);

    if (GraphicsState.bBindPushConstants || GVulkanForceBinding)
    {
        BindPushConstants(PipelineLayout);
        GraphicsState.bBindPushConstants = false;
    }

    if (GraphicsState.bBindVertexBuffers || GVulkanForceBinding)
    {
        FVulkanVertexBufferCache& VBCache = GraphicsState.VBCache;
        Context.GetCommandBuffer()->BindVertexBuffers(0, VBCache.NumVertexBuffers, VBCache.VertexBuffers, VBCache.VertexBufferOffsets);
        GraphicsState.bBindVertexBuffers = false;
    }

    if (GraphicsState.bBindIndexBuffer || GVulkanForceBinding)
    {
        FVulkanIndexBufferCache& IBCache = GraphicsState.IBCache;
        Context.GetCommandBuffer()->BindIndexBuffer(IBCache.IndexBuffer, IBCache.Offset, IBCache.IndexType);
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

    FVulkanPipelineLayout* PipelineLayout = ComputeState.PipelineState->GetPipelineLayout();
    if (ComputeState.bBindPipelineState || GVulkanForceBinding)
    {
        VkPipeline Pipeline = ComputeState.PipelineState->GetVkPipeline();
        Context.GetCommandBuffer()->BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);
        ComputeState.bBindPipelineState = false;
    }

    BindDescriptorSets(PipelineLayout, ShaderVisibility_Compute, ShaderVisibility_Compute);

    if (ComputeState.bBindPushConstants || GVulkanForceBinding)
    {
        BindPushConstants(PipelineLayout);
        ComputeState.bBindPushConstants = false;
    }
}

void FVulkanCommandContextState::BindDescriptorSets(FVulkanPipelineLayout* PipelineLayout, EShaderVisibility StartStage, EShaderVisibility EndStage)
{
    FVulkanDescriptorState* DescriptorState = (StartStage == ShaderVisibility_Compute) ? ComputeState.CurrentDescriptorState : GraphicsState.CurrentDescriptorState;
    if (!DescriptorState)
    {
        return;
    }

    for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
    {
        // If the layout does not contain a DescriptorSet index then we continue to the next stage
        uint32 DSStageIndex;
        if (!PipelineLayout->GetDescriptorSetIndex(CurrentStage, DSStageIndex))
        {
            continue;
        }

        const FVulkanSRVCache& SRVCache = CommonState.SRVCache[CurrentStage];
        for (int32 Index = 0; Index < VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT; Index++)
        {
            uint32 BindingIndex;
            uint32 DescriptorSetIndex;
            if (!PipelineLayout->GetDescriptorBinding(CurrentStage, ResourceType_SRV, Index, DescriptorSetIndex, BindingIndex))
            {
                continue;
            }

            FVulkanShaderResourceView* ShaderResourceView = SRVCache.Views[Index];
            DescriptorState->SetSRV(ShaderResourceView, DescriptorSetIndex, BindingIndex);
        }

        const FVulkanUAVCache& UAVCache = CommonState.UAVCache[CurrentStage];
        for (int32 Index = 0; Index < VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT; Index++)
        {
            uint32 BindingIndex;
            uint32 DescriptorSetIndex;
            if (!PipelineLayout->GetDescriptorBinding(CurrentStage, ResourceType_UAV, Index, DescriptorSetIndex, BindingIndex))
            {
                continue;
            }
            
            FVulkanUnorderedAccessView* UnorderedAccessView = UAVCache.Views[Index];
            DescriptorState->SetUAV(UnorderedAccessView, DescriptorSetIndex, BindingIndex);
        }
        
        const FVulkanUniformCache& UniformBufferCache = CommonState.UniformCache[CurrentStage];
        for (int32 Index = 0; Index < VULKAN_DEFAULT_UNIFORM_BUFFER_COUNT; Index++)
        {
            uint32 BindingIndex;
            uint32 DescriptorSetIndex;
            if (!PipelineLayout->GetDescriptorBinding(CurrentStage, ResourceType_UniformBuffer, Index, DescriptorSetIndex, BindingIndex))
            {
                continue;
            }
            
            FVulkanBuffer* UniformBuffer = UniformBufferCache.UniformBuffers[Index];
            DescriptorState->SetUniform(UniformBuffer, DescriptorSetIndex, BindingIndex);
        }
        
        const FVulkanSamplerCache& SamplerCache = CommonState.SamplerCache[CurrentStage];
        for (int32 Index = 0; Index < VULKAN_DEFAULT_SAMPLER_STATE_COUNT; Index++)
        {
            uint32 BindingIndex;
            uint32 DescriptorSetIndex;
            if (!PipelineLayout->GetDescriptorBinding(CurrentStage, ResourceType_Sampler, Index, DescriptorSetIndex, BindingIndex))
            {
                continue;
            }
            
            FVulkanSamplerState* SamplerState = SamplerCache.Samplers[Index];
            DescriptorState->SetSampler(SamplerState, DescriptorSetIndex, BindingIndex);
        }
    }
    
    // Update the descriptorsets
    DescriptorState->UpdateDescriptorSets();

    // Finally bind the DescriptorSets
    if (StartStage == ShaderVisibility_Compute)
    {
        DescriptorState->BindComputeDescriptorSets(Context.GetCommandBuffer());
    }
    else
    {
        DescriptorState->BindGraphicsDescriptorSets(Context.GetCommandBuffer());
    }
}

void FVulkanCommandContextState::BindPushConstants(FVulkanPipelineLayout* PipelineLayout)
{
    FPushConstantsInfo ConstantsInfo = PipelineLayout->GetConstantsInfo();
    if (ConstantsInfo.NumConstants > 0)
    {
        CHECK(ConstantsInfo.NumConstants <= VULKAN_MAX_NUM_PUSH_CONSTANTS);
        Context.GetCommandBuffer()->PushConstants(PipelineLayout->GetVkPipelineLayout(), ConstantsInfo.StageFlags, 0, ConstantsInfo.NumConstants * sizeof(uint32), CommonState.PushConstantsCache.Constants);
    }
}

void FVulkanCommandContextState::ResetState()
{
    CommonState.PushConstantsCache.Clear();

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

void FVulkanCommandContextState::ResetStateForNewCommandBuffer()
{
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
        GraphicsState.PipelineState = MakeSharedRef<FVulkanGraphicsPipelineState>(InGraphicsPipelineState);
        GraphicsState.bBindPipelineState = true;

        if (InGraphicsPipelineState)
        {
            if (FVulkanDescriptorState** State = GraphicsState.DescriptorStates.Find(InGraphicsPipelineState))
            {
                GraphicsState.CurrentDescriptorState = *State;
            }
            else
            {
                GraphicsState.CurrentDescriptorState = new FVulkanDescriptorState(GetDevice(), InGraphicsPipelineState->GetPipelineLayout(), GetDevice()->GetDefaultResources());
                GraphicsState.DescriptorStates.Add(InGraphicsPipelineState, GraphicsState.CurrentDescriptorState);
            }
        }
        else
        {
            GraphicsState.CurrentDescriptorState = nullptr;
        }
    }
}

void FVulkanCommandContextState::SetComputePipelineState(FVulkanComputePipelineState* InComputePipelineState)
{
    FVulkanComputePipelineState* CurrentComputePipelineState = ComputeState.PipelineState.Get();
    if (CurrentComputePipelineState != InComputePipelineState || GVulkanForceBinding)
    {
        ComputeState.PipelineState = MakeSharedRef<FVulkanComputePipelineState>(InComputePipelineState);
        ComputeState.bBindPipelineState = true;

        if (InComputePipelineState)
        {
            if (FVulkanDescriptorState** State = ComputeState.DescriptorStates.Find(InComputePipelineState))
            {
                ComputeState.CurrentDescriptorState = *State;
            }
            else
            {
                ComputeState.CurrentDescriptorState = new FVulkanDescriptorState(GetDevice(), InComputePipelineState->GetPipelineLayout(), GetDevice()->GetDefaultResources());
                ComputeState.DescriptorStates.Add(InComputePipelineState, ComputeState.CurrentDescriptorState);
            }
        }
        else
        {
            ComputeState.CurrentDescriptorState = nullptr;
        }
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
