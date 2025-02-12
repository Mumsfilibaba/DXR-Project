#include "VulkanRHI/VulkanCommandContextState.h"

FVulkanCommandContextState::FVulkanCommandContextState(FVulkanDevice* InDevice, FVulkanCommandContext& InContext)
    : FVulkanDeviceChild(InDevice)
    , GraphicsState()
    , ComputeState()
    , CommonState()
    , Context(InContext)
{
}

FVulkanCommandContextState::~FVulkanCommandContextState()
{
    for (auto DescriptorStatePair : ComputeState.DescriptorStates)
    {
        delete DescriptorStatePair.Second;
    }

    for (auto DescriptorStatePair : GraphicsState.DescriptorStates)
    {
        delete DescriptorStatePair.Second;
    }

    ComputeState.DescriptorStates.Clear();
    GraphicsState.DescriptorStates.Clear();
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

    CHECK(GraphicsState.ViewInstancingInfo == GraphicsState.PipelineState->GetViewInstancingInfo());
    FVulkanPipelineLayout* PipelineLayout = GraphicsState.PipelineState->GetPipelineLayout();
    CHECK(PipelineLayout != nullptr);

    if (GraphicsState.bBindPipelineState || GVulkanForceBinding)
    {
        VkPipeline Pipeline = GraphicsState.PipelineState->GetVkPipeline();
        Context.GetCommandBuffer()->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
        GraphicsState.bBindPipelineState = false;
    }

    // Update and bind descriptor-sets
    CHECK(PipelineLayout == GraphicsState.CurrentDescriptorState->GetLayout());
    GraphicsState.CurrentDescriptorState->UpdateDescriptorSets();
    GraphicsState.CurrentDescriptorState->BindGraphicsDescriptorSets(Context.GetCommandBuffer());
    
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
    
    // Update and bind descriptor-sets
    CHECK(PipelineLayout == ComputeState.CurrentDescriptorState->GetLayout());
    ComputeState.CurrentDescriptorState->UpdateDescriptorSets();
    ComputeState.CurrentDescriptorState->BindComputeDescriptorSets(Context.GetCommandBuffer());
    
    if (ComputeState.bBindPushConstants || GVulkanForceBinding)
    {
        BindPushConstants(PipelineLayout);
        ComputeState.bBindPushConstants = false;
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
    
    GraphicsState.PipelineState = nullptr;
    GraphicsState.CurrentDescriptorState = nullptr;
    GraphicsState.CurrentLayout = nullptr;
    GraphicsState.bBindIndexBuffer = true;
    GraphicsState.bBindBlendFactor = true;
    GraphicsState.bBindPipelineState = true;
    GraphicsState.bBindScissorRects = true;
    GraphicsState.bBindViewports = true;
    GraphicsState.bBindVertexBuffers = true;
    GraphicsState.bBindPushConstants = true;

    ComputeState.PipelineState = nullptr;
    ComputeState.CurrentDescriptorState = nullptr;
    ComputeState.CurrentLayout = nullptr;
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

void FVulkanCommandContextState::SetViewInstanceInfo(const FViewInstancingInfo& InViewInstancingInfo)
{
    GraphicsState.ViewInstancingInfo = InViewInstancingInfo;
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
            GraphicsState.CurrentLayout = InGraphicsPipelineState->GetPipelineLayout();
            
            if (FVulkanDescriptorState** State = GraphicsState.DescriptorStates.Find(InGraphicsPipelineState))
            {
                GraphicsState.CurrentDescriptorState = *State;
                if (GVulkanForceBinding)
                {
                    GraphicsState.CurrentDescriptorState->Reset();
                }
            }
            else
            {
                GraphicsState.CurrentDescriptorState = new FVulkanDescriptorState(GetDevice(), GraphicsState.CurrentLayout, GetDevice()->GetDefaultResources());
                GraphicsState.DescriptorStates.Add(InGraphicsPipelineState, GraphicsState.CurrentDescriptorState);
            }
        }
        else
        {
            GraphicsState.CurrentLayout          = nullptr;
            GraphicsState.CurrentDescriptorState = nullptr;
        }

        // NOTE: When we change PipelineLayout/PipelineState we need to ensure that PushConstants are also bound
        GraphicsState.bBindPushConstants = true;
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
            ComputeState.CurrentLayout = InComputePipelineState->GetPipelineLayout();

            if (FVulkanDescriptorState** State = ComputeState.DescriptorStates.Find(InComputePipelineState))
            {
                ComputeState.CurrentDescriptorState = *State;
                if (GVulkanForceBinding)
                {
                    ComputeState.CurrentDescriptorState->Reset();
                }
            }
            else
            {
                ComputeState.CurrentDescriptorState = new FVulkanDescriptorState(GetDevice(), ComputeState.CurrentLayout, GetDevice()->GetDefaultResources());
                ComputeState.DescriptorStates.Add(InComputePipelineState, ComputeState.CurrentDescriptorState);
            }
        }
        else
        {
            ComputeState.CurrentLayout          = nullptr;
            ComputeState.CurrentDescriptorState = nullptr;
        }

        // NOTE: When we change PipelineLayout/PipelineState we need to ensure that PushConstants are also bound
        GraphicsState.bBindPushConstants = true;
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

void FVulkanCommandContextState::SetSRV(FVulkanShaderResourceView* ShaderResourceView, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    CHECK(ResourceIndex < VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);
    
    FVulkanPipelineLayout*  Layout = nullptr;
    FVulkanDescriptorState* DescriptorState = nullptr;
    if (ShaderStage == ShaderVisibility_Compute)
    {
        Layout          = ComputeState.CurrentLayout;
        DescriptorState = ComputeState.CurrentDescriptorState;
    }
    else
    {
        Layout          = GraphicsState.CurrentLayout;
        DescriptorState = GraphicsState.CurrentDescriptorState;
    }
    
    if (!Layout || !DescriptorState)
    {
        VULKAN_WARNING("Binding a ShaderResource without having a PipelineState set, this does not have any effect");
        DEBUG_BREAK();
        return;
    }
    
    uint32 BindingIndex;
    uint32 DescriptorSetIndex;
    if (!Layout->GetDescriptorBinding(ShaderStage, ResourceType_SRV, ResourceIndex, DescriptorSetIndex, BindingIndex))
    {
        return;
    }
    
    DescriptorState->SetSRV(ShaderResourceView, DescriptorSetIndex, BindingIndex);
}

void FVulkanCommandContextState::SetUAV(FVulkanUnorderedAccessView* UnorderedAccessView, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    CHECK(ResourceIndex < VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);

    FVulkanPipelineLayout*  Layout = nullptr;
    FVulkanDescriptorState* DescriptorState = nullptr;
    if (ShaderStage == ShaderVisibility_Compute)
    {
        Layout          = ComputeState.CurrentLayout;
        DescriptorState = ComputeState.CurrentDescriptorState;
    }
    else
    {
        Layout          = GraphicsState.CurrentLayout;
        DescriptorState = GraphicsState.CurrentDescriptorState;
    }
    
    if (!Layout || !DescriptorState)
    {
        VULKAN_WARNING("Binding a ShaderResource without having a PipelineState set, this does not have any effect");
        DEBUG_BREAK();
        return;
    }
    
    uint32 BindingIndex;
    uint32 DescriptorSetIndex;
    if (!Layout->GetDescriptorBinding(ShaderStage, ResourceType_UAV, ResourceIndex, DescriptorSetIndex, BindingIndex))
    {
        return;
    }
    
    DescriptorState->SetUAV(UnorderedAccessView, DescriptorSetIndex, BindingIndex);
}

void FVulkanCommandContextState::SetUniformBuffer(FVulkanBuffer* UniformBuffer, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    CHECK(ResourceIndex < VULKAN_DEFAULT_UNIFORM_BUFFER_COUNT);
    
    FVulkanPipelineLayout*  Layout = nullptr;
    FVulkanDescriptorState* DescriptorState = nullptr;
    if (ShaderStage == ShaderVisibility_Compute)
    {
        Layout          = ComputeState.CurrentLayout;
        DescriptorState = ComputeState.CurrentDescriptorState;
    }
    else
    {
        Layout          = GraphicsState.CurrentLayout;
        DescriptorState = GraphicsState.CurrentDescriptorState;
    }
    
    if (!Layout || !DescriptorState)
    {
        VULKAN_WARNING("Binding a ShaderResource without having a PipelineState set, this does not have any effect");
        DEBUG_BREAK();
        return;
    }
    
    uint32 BindingIndex;
    uint32 DescriptorSetIndex;
    if (!Layout->GetDescriptorBinding(ShaderStage, ResourceType_UniformBuffer, ResourceIndex, DescriptorSetIndex, BindingIndex))
    {
        return;
    }
    
    DescriptorState->SetUniform(UniformBuffer, DescriptorSetIndex, BindingIndex);
}

void FVulkanCommandContextState::SetSampler(FVulkanSamplerState* SamplerState, EShaderVisibility ShaderStage, uint32 SamplerIndex)
{
    CHECK(SamplerIndex < VULKAN_DEFAULT_SAMPLER_STATE_COUNT);

    FVulkanPipelineLayout*  Layout = nullptr;
    FVulkanDescriptorState* DescriptorState = nullptr;
    if (ShaderStage == ShaderVisibility_Compute)
    {
        Layout          = ComputeState.CurrentLayout;
        DescriptorState = ComputeState.CurrentDescriptorState;
    }
    else
    {
        Layout          = GraphicsState.CurrentLayout;
        DescriptorState = GraphicsState.CurrentDescriptorState;
    }
    
    if (!Layout || !DescriptorState)
    {
        VULKAN_WARNING("Binding a ShaderResource without having a PipelineState set, this does not have any effect");
        DEBUG_BREAK();
        return;
    }
    
    uint32 BindingIndex;
    uint32 DescriptorSetIndex;
    if (!Layout->GetDescriptorBinding(ShaderStage, ResourceType_Sampler, SamplerIndex, DescriptorSetIndex, BindingIndex))
    {
        return;
    }
    
    DescriptorState->SetSampler(SamplerState, DescriptorSetIndex, BindingIndex);
}
