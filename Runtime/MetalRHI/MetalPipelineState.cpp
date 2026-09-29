#include "MetalRHI/MetalPipelineState.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalCapabilities.h"
#include "MetalRHI/MetalStats.h"
#include "RHI/MSLShaderBindings.h"
#include "RHI/RHISamplerState.h"
#include "RHI/RHI.h"
#include "Core/Memory/Memory.h"

static NSString* PipelineDebugLabel(const String& Name, NSString* Fallback)
{
    return Name.IsEmpty() ? Fallback : Name.GetNSString();
}

static FRHIDepthStencilStateDesc GetDisabledDepthStencilDesc()
{
    FRHIDepthStencilStateDesc Desc;
    Desc.bDepthEnable      = false;
    Desc.bDepthWriteEnable = false;
    Desc.bStencilEnable    = false;
    Desc.DepthFunc         = EComparisonFunc::Always;
    return Desc;
}

static bool AssignMSLSlot(uint8& Dest, const FMSLShaderBinding& Binding, EShaderVisibility::Type Stage)
{
    const uint8 TableLimit = GetMSLMaxSlotCount(Binding.BindingType);

    if (Binding.SlotIndex >= TableLimit)
    {
        METAL_ERROR("%s register %u resolved to MSL slot %u, which exceeds the %u-slot table (stage %u)",
            ToString(Binding.BindingType), Binding.RegisterIndex, Binding.SlotIndex, TableLimit, Stage);
        return false;
    }

    if (Dest != FMetalPipelineBindingLayout::InvalidSlot && Dest != Binding.SlotIndex)
    {
        METAL_ERROR("Shader stage %u already mapped %s register %u to MSL slot %u, cannot also use slot %u",
            Stage, ToString(Binding.BindingType), Binding.RegisterIndex, Dest, Binding.SlotIndex);
        return false;
    }

    Dest = Binding.SlotIndex;
    return true;
}

template<typename PipelineDescriptorType>
static bool ApplyColorAttachments(PipelineDescriptorType* Descriptor, FMetalBlendStateRHI* BlendState, const FRHIGraphicsPipelineFormats& Formats)
{
    CHECK(Descriptor != nil);

    if (BlendState && BlendState->IsLogicOpEnabled())
    {
        METAL_ERROR("Metal does not support blend logic operations");
        return false;
    }

    Descriptor.alphaToCoverageEnabled = (BlendState && BlendState->IsAlphaToCoverageEnabled()) ? YES : NO;

    for (uint32 Index = 0; Index < Formats.NumRenderTargets; ++Index)
    {
        MTLRenderPipelineColorAttachmentDescriptor* Attachment = Descriptor.colorAttachments[Index];
        Attachment.pixelFormat = MetalRHI::ConvertFormat(Formats.RenderTargetFormats[Index]);

        if (!BlendState)
        {
            continue;
        }

        const uint32 BlendIndex = BlendState->GetDesc().bIndependentBlendEnable ? Index : 0;
        const FMetalBlendStateRHI::FBlendAttachment& Blend = BlendState->GetColorAttachment(BlendIndex);
        Attachment.blendingEnabled             = Blend.bBlendingEnabled;
        Attachment.sourceRGBBlendFactor        = Blend.SourceColorBlendFactor;
        Attachment.destinationRGBBlendFactor   = Blend.DestinationColorBlendFactor;
        Attachment.rgbBlendOperation           = Blend.ColorBlendOperation;
        Attachment.sourceAlphaBlendFactor      = Blend.SourceAlphaBlendFactor;
        Attachment.destinationAlphaBlendFactor = Blend.DestinationAlphaBlendFactor;
        Attachment.alphaBlendOperation         = Blend.AlphaBlendOperation;
        Attachment.writeMask                   = Blend.WriteMask;
    }

    const MTLPixelFormat DepthStencilFormat = MetalRHI::ConvertFormat(Formats.DepthStencilFormat);
    Descriptor.depthAttachmentPixelFormat = DepthStencilFormat;

    if (MetalRHI::IsStencilPixelFormat(DepthStencilFormat))
    {
        Descriptor.stencilAttachmentPixelFormat = DepthStencilFormat;
    }

    return true;
}

static bool CreateStaticSamplerBindings(TArray<FMetalStaticSamplerBinding>& OutSamplers, FMetalPipelineBindingLayout& Layout, const TArrayView<const FRHIStaticSamplerInfo>& Infos)
{
    OutSamplers.Clear();

    for (const FRHIStaticSamplerInfo& Info : Infos)
    {
        if (Info.ShaderRegister >= MAX_SAMPLER_STATES)
        {
            METAL_ERROR("Static sampler register s%u exceeds MAX_SAMPLER_STATES", Info.ShaderRegister);
            return false;
        }

        const uint8  RegisterIndex = static_cast<uint8>(Info.ShaderRegister);
        const uint16 Bit           = static_cast<uint16>(1u << RegisterIndex);

        uint32 StageMask = 0;

        if (Info.ShaderVisibility == EShaderStage::Vertex || Info.ShaderVisibility == EShaderStage::Pixel || Info.ShaderVisibility == EShaderStage::Compute
            || Info.ShaderVisibility == EShaderStage::Mesh || Info.ShaderVisibility == EShaderStage::Amplification)
        {
            StageMask = 1u << MetalRHI::GetShaderVisibility(Info.ShaderVisibility);
        }
        else
        {
            for (uint32 Stage = 0; Stage < EShaderVisibility::Count; ++Stage)
            {
                if (Layout.Stages[Stage].SamplerMask & Bit)
                {
                    StageMask |= 1u << Stage;
                }
            }
        }

        if (StageMask == 0)
        {
            continue;
        }

        FRHISamplerState* Created = FMetalDeviceRHI::Get()->CreateSamplerState(Info.GetSamplerStateDesc());

        if (!Created)
        {
            METAL_ERROR("Failed to create static sampler for register s%u", RegisterIndex);
            return false;
        }

        const TSharedRef<FMetalSamplerStateRHI> Sampler(static_cast<FMetalSamplerStateRHI*>(Created));
        for (uint32 Bits = StageMask; Bits != 0; Bits &= Bits - 1)
        {
            const EShaderVisibility::Type Stage = static_cast<EShaderVisibility::Type>(MetalRHI::FirstSetBit(Bits));
            Layout.Stages[Stage].StaticSamplerMask |= Bit;

            FMetalStaticSamplerBinding& Binding = OutSamplers.Emplace();
            Binding.Stage         = Stage;
            Binding.RegisterIndex = RegisterIndex;
            Binding.Sampler       = Sampler;
        }
    }

    return true;
}

FMetalPipelineBindingLayout::FMetalPipelineBindingLayout()
    : bUsesBindlessHeaps(false)
{
    for (FMetalStageBindPlan& Plan : Stages)
    {
        Memory::Memzero(&Plan, sizeof(Plan));
        Memory::Memset(Plan.ConstantBufferSlots, InvalidSlot, sizeof(Plan.ConstantBufferSlots));
        Memory::Memset(Plan.ShaderResourceSlots, InvalidSlot, sizeof(Plan.ShaderResourceSlots));
        Memory::Memset(Plan.UnorderedAccessSlots, InvalidSlot, sizeof(Plan.UnorderedAccessSlots));
        Memory::Memset(Plan.SamplerSlots, InvalidSlot, sizeof(Plan.SamplerSlots));

        Plan.ShaderConstantsSlot = InvalidSlot;
        Plan.ResourceHeapSlot    = InvalidSlot;
        Plan.SamplerHeapSlot     = InvalidSlot;
    }
}

FMetalPipelineBindingLayout::~FMetalPipelineBindingLayout() = default;

bool FMetalPipelineBindingLayout::Collect(const TArray<FMSLShaderBinding>& ShaderBindings, EShaderVisibility::Type Stage, uint16 ShaderConstantsSize)
{
    FMetalStageBindPlan& Plan = Stages[Stage];

    for (const FMSLShaderBinding& Binding : ShaderBindings)
    {
        const uint32 Register = Binding.RegisterIndex;
        const uint16 Bit      = static_cast<uint16>(1u << (Register & 15));

        bool bAssigned = false;
        switch (Binding.BindingType)
        {
            case EMSLBindingType::ConstantBuffer:
            {
                CHECK(Register < MAX_CONSTANT_BUFFERS);
                bAssigned = AssignMSLSlot(Plan.ConstantBufferSlots[Register], Binding, Stage);
                Plan.ConstantBufferMask |= Bit;
                break;
            }

            case EMSLBindingType::ShaderResourceBuffer:
            case EMSLBindingType::ShaderResourceTexture:
            {
                CHECK(Register < MAX_SRVS);
                const bool bBuffer = Binding.BindingType == EMSLBindingType::ShaderResourceBuffer;

                if ((Plan.ShaderResourceMask & Bit) && bBuffer != ((Plan.ShaderResourceBufferMask & Bit) != 0))
                {
                    METAL_ERROR("Shader stage %u binds SRV register %u as both a buffer and a texture", Stage, Register);
                    return false;
                }

                bAssigned = AssignMSLSlot(Plan.ShaderResourceSlots[Register], Binding, Stage);
                Plan.ShaderResourceMask       |= Bit;
                Plan.ShaderResourceBufferMask |= bBuffer ? Bit : 0;
                Plan.ShaderResourceNullTypes[Register] = Binding.NullTextureType;
                break;
            }

            case EMSLBindingType::UnorderedAccessBuffer:
            case EMSLBindingType::UnorderedAccessTexture:
            {
                CHECK(Register < MAX_UAVS);
                const bool bBuffer = Binding.BindingType == EMSLBindingType::UnorderedAccessBuffer;

                if ((Plan.UnorderedAccessMask & Bit) && bBuffer != ((Plan.UnorderedAccessBufferMask & Bit) != 0))
                {
                    METAL_ERROR("Shader stage %u binds UAV register %u as both a buffer and a texture", Stage, Register);
                    return false;
                }

                bAssigned = AssignMSLSlot(Plan.UnorderedAccessSlots[Register], Binding, Stage);
                Plan.UnorderedAccessMask       |= Bit;
                Plan.UnorderedAccessBufferMask |= bBuffer ? Bit : 0;
                Plan.UnorderedAccessNullTypes[Register] = Binding.NullTextureType;
                break;
            }

            case EMSLBindingType::Sampler:
            {
                CHECK(Register < MAX_SAMPLER_STATES);
                bAssigned = AssignMSLSlot(Plan.SamplerSlots[Register], Binding, Stage);
                Plan.SamplerMask |= Bit;
                break;
            }

            case EMSLBindingType::ShaderConstants:
            {
                bAssigned = AssignMSLSlot(Plan.ShaderConstantsSlot, Binding, Stage);

                const uint32 PaddedSize = Math::AlignUp<uint32>(ShaderConstantsSize, 16);
                Plan.NumShaderConstants = static_cast<uint8>(Math::Min<uint32>(PaddedSize / sizeof(uint32), MAX_SHADER_CONSTANTS));
                break;
            }

            case EMSLBindingType::BindlessResourceHeap:
            {
                bAssigned = AssignMSLSlot(Plan.ResourceHeapSlot, Binding, Stage);
                bUsesBindlessHeaps = true;
                break;
            }

            case EMSLBindingType::BindlessSamplerHeap:
            {
                bAssigned = AssignMSLSlot(Plan.SamplerHeapSlot, Binding, Stage);
                bUsesBindlessHeaps = true;
                break;
            }

            default:
            {
                METAL_ERROR("Unhandled MSL binding type %s", ToString(Binding.BindingType));
                return false;
            }
        }

        if (!bAssigned)
        {
            return false;
        }
    }

    return true;
}

void FMetalPipelineBindingLayout::PruneToReflection(EShaderVisibility::Type Stage, NSArray<id<MTLBinding>>* ReflectedBindings)
{
    if (!ReflectedBindings)
    {
        return;
    }

    uint64 UnusedBuffers     = 0;
    uint64 UnusedTextures[2] = { 0, 0 };
    uint64 UnusedSamplers    = 0;

    for (id<MTLBinding> Binding in ReflectedBindings)
    {
        const NSUInteger Index = Binding.index;

        if (Binding.used || Index >= 128)
        {
            continue;
        }

        switch (Binding.type)
        {
            case MTLBindingTypeBuffer:
                UnusedBuffers  |= Index < 64 ? (uint64(1) << Index) : 0;
                break;

            case MTLBindingTypeTexture:
                UnusedTextures[Index / 64] |= (uint64(1) << (Index % 64));
                break;

            case MTLBindingTypeSampler:
                UnusedSamplers |= Index < 64 ? (uint64(1) << Index) : 0;
                break;

            default:
                break;
        }
    }

    auto IsBufferUsed = [&](uint8 Slot)
    {
        return Slot >= 64 || (UnusedBuffers & (uint64(1) << Slot)) == 0;
    };

    auto IsTextureUsed = [&](uint8 Slot)
    {
        return Slot >= 128 || (UnusedTextures[Slot / 64] & (uint64(1) << (Slot % 64))) == 0;
    };

    auto IsSamplerUsed = [&](uint8 Slot)
    {
        return Slot >= 64 || (UnusedSamplers & (uint64(1) << Slot)) == 0;
    };

    FMetalStageBindPlan& Plan = Stages[Stage];
    auto PruneTable = [](uint16& Mask, uint8* Slots, auto&& IsUsed)
    {
        for (uint32 Bits = Mask; Bits != 0; Bits &= Bits - 1)
        {
            const uint32 Register = MetalRHI::FirstSetBit(Bits);

            if (!IsUsed(Register))
            {
                Mask &= static_cast<uint16>(~(1u << Register));
                Slots[Register] = InvalidSlot;
            }
        }
    };

    PruneTable(Plan.ConstantBufferMask, Plan.ConstantBufferSlots, [&](uint32 Register)
    {
        return IsBufferUsed(Plan.ConstantBufferSlots[Register]);
    });

    PruneTable(Plan.SamplerMask, Plan.SamplerSlots, [&](uint32 Register)
    {
        return IsSamplerUsed(Plan.SamplerSlots[Register]);
    });

    PruneTable(Plan.ShaderResourceMask, Plan.ShaderResourceSlots, [&](uint32 Register)
    {
        const uint8 Slot = Plan.ShaderResourceSlots[Register];
        return (Plan.ShaderResourceBufferMask & (1u << Register)) ? IsBufferUsed(Slot) : IsTextureUsed(Slot);
    });

    PruneTable(Plan.UnorderedAccessMask, Plan.UnorderedAccessSlots, [&](uint32 Register)
    {
        const uint8 Slot = Plan.UnorderedAccessSlots[Register];
        return (Plan.UnorderedAccessBufferMask & (1u << Register)) ? IsBufferUsed(Slot) : IsTextureUsed(Slot);
    });

    Plan.ShaderResourceBufferMask  &= Plan.ShaderResourceMask;
    Plan.UnorderedAccessBufferMask &= Plan.UnorderedAccessMask;

    if (Plan.ShaderConstantsSlot != InvalidSlot && !IsBufferUsed(Plan.ShaderConstantsSlot))
    {
        Plan.ShaderConstantsSlot = InvalidSlot;
        Plan.NumShaderConstants  = 0;
    }
}

bool FMetalPipelineBindingLayout::ConflictsWithVertexInputs(const FMetalInputLayoutRHI* InputLayout) const
{
    if (!InputLayout)
    {
        return false;
    }

    const FMetalStageBindPlan& Plan = Stages[EShaderVisibility::Vertex];

    uint64 ReservedSlots = 0;
    const uint32 NumElements = InputLayout->GetNumInputElementDescs();
    for (uint32 ElementIndex = 0; ElementIndex < NumElements; ++ElementIndex)
    {
        if (const FRHIInputElementDesc* Element = InputLayout->GetInputElementDesc(ElementIndex))
        {
            ReservedSlots |= uint64(1) << GetMSLVertexStreamBufferIndex(Element->InputSlot);
        }
    }

    auto CheckSlot = [ReservedSlots](uint8 Slot, const CHAR* What, uint32 RegisterIndex) -> bool
    {
        if (Slot != InvalidSlot && Slot < 64 && (ReservedSlots & (uint64(1) << Slot)))
        {
            METAL_ERROR("Vertex shader %s %u uses MSL slot %u, which is reserved for a vertex input stream", What, RegisterIndex, Slot);
            return true;
        }

        return false;
    };

    for (uint32 Bits = Plan.ConstantBufferMask; Bits != 0; Bits &= Bits - 1)
    {
        const uint32 Register = MetalRHI::FirstSetBit(Bits);

        if (CheckSlot(Plan.ConstantBufferSlots[Register], "constant buffer register", Register))
        {
            return true;
        }
    }

    for (uint32 Bits = Plan.ShaderResourceBufferMask; Bits != 0; Bits &= Bits - 1)
    {
        const uint32 Register = MetalRHI::FirstSetBit(Bits);

        if (CheckSlot(Plan.ShaderResourceSlots[Register], "SRV buffer register", Register))
        {
            return true;
        }
    }

    for (uint32 Bits = Plan.UnorderedAccessBufferMask; Bits != 0; Bits &= Bits - 1)
    {
        const uint32 Register = MetalRHI::FirstSetBit(Bits);

        if (CheckSlot(Plan.UnorderedAccessSlots[Register], "UAV buffer register", Register))
        {
            return true;
        }
    }

    return CheckSlot(Plan.ShaderConstantsSlot, "shader constants", 0)
        || CheckSlot(Plan.ResourceHeapSlot, "bindless resource heap", 0)
        || CheckSlot(Plan.SamplerHeapSlot, "bindless sampler heap", 0);
}

uint8 FMetalPipelineBindingLayout::GetSlot(EShaderVisibility::Type Stage, EMSLBindingType BindingType, uint32 RegisterIndex) const
{
    const FMetalStageBindPlan& Plan = Stages[Stage];
    const uint32 Bit = RegisterIndex < 16 ? (1u << RegisterIndex) : 0;

    switch (BindingType)
    {
        case EMSLBindingType::ConstantBuffer:
            return (Plan.ConstantBufferMask & Bit) ? Plan.ConstantBufferSlots[RegisterIndex] : InvalidSlot;

        case EMSLBindingType::ShaderResourceBuffer:
            return (Plan.ShaderResourceMask & Plan.ShaderResourceBufferMask & Bit)
                ? Plan.ShaderResourceSlots[RegisterIndex]
                : InvalidSlot;

        case EMSLBindingType::ShaderResourceTexture:
            return (Plan.ShaderResourceMask & ~Plan.ShaderResourceBufferMask & Bit)
                ? Plan.ShaderResourceSlots[RegisterIndex]
                : InvalidSlot;

        case EMSLBindingType::UnorderedAccessBuffer:
            return (Plan.UnorderedAccessMask & Plan.UnorderedAccessBufferMask & Bit)
                ? Plan.UnorderedAccessSlots[RegisterIndex]
                : InvalidSlot;

        case EMSLBindingType::UnorderedAccessTexture:
            return (Plan.UnorderedAccessMask & ~Plan.UnorderedAccessBufferMask & Bit)
                ? Plan.UnorderedAccessSlots[RegisterIndex]
                : InvalidSlot;

        case EMSLBindingType::Sampler:
            return (Plan.SamplerMask & Bit) ? Plan.SamplerSlots[RegisterIndex] : InvalidSlot;

        case EMSLBindingType::ShaderConstants:
            return Plan.ShaderConstantsSlot;

        case EMSLBindingType::BindlessResourceHeap:
            return Plan.ResourceHeapSlot;

        case EMSLBindingType::BindlessSamplerHeap:
            return Plan.SamplerHeapSlot;

        default:
            return InvalidSlot;
    }
}

FMetalInputLayoutRHI::FMetalInputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements)
    : FRHIInputLayout()
    , InputElements(InInputElements)
    , VertexDescriptor(nullptr)
    , NumVertexStreams(0)
{
    VertexDescriptor = [[MTLVertexDescriptor vertexDescriptor] retain];
    for (int32 Index = 0; Index < InputElements.Size(); ++Index)
    {
        const FRHIInputElementDesc& Element = InputElements[Index];

        if (Element.InputSlot >= MSL_MAX_VERTEX_STREAMS)
        {
            METAL_ERROR("Input element %d uses vertex stream slot %u, past the %u streams Metal reserves",
                Index, Element.InputSlot, MSL_MAX_VERTEX_STREAMS);
            continue;
        }

        const uint8 StreamBufferIndex = GetMSLVertexStreamBufferIndex(Element.InputSlot);
        VertexDescriptor.attributes[Index].format      = MetalRHI::ConvertVertexFormat(Element.Format);
        VertexDescriptor.attributes[Index].offset      = Element.ByteOffset;
        VertexDescriptor.attributes[Index].bufferIndex = StreamBufferIndex;

        VertexDescriptor.layouts[StreamBufferIndex].stride       = Element.VertexStride;
        VertexDescriptor.layouts[StreamBufferIndex].stepFunction = MetalRHI::ConvertVertexInputClass(Element.InputClass);
        VertexDescriptor.layouts[StreamBufferIndex].stepRate     = Element.InputClass == EVertexInputClass::Vertex
            ? 1
            : Element.InstanceStepRate;

        NumVertexStreams = Math::Max(NumVertexStreams, Element.InputSlot + 1);
    }
}

FMetalInputLayoutRHI::~FMetalInputLayoutRHI()
{
    if (VertexDescriptor)
    {
        FMetalDeviceRHI::DeferDeletion(VertexDescriptor);
        [VertexDescriptor release];
        VertexDescriptor = nil;
    }
}

const FRHIInputElementDesc* FMetalInputLayoutRHI::GetInputElementDesc(uint32 Index) const
{
    return &InputElements[Index];
}

uint32 FMetalInputLayoutRHI::GetNumInputElementDescs() const
{
    return InputElements.Size();
}

void* FMetalInputLayoutRHI::GetRHINativeState() const
{
    return nullptr;
}

FMetalDepthStencilStateRHI::FMetalDepthStencilStateRHI(const FRHIDepthStencilStateDesc& InDesc)
    : FRHIDepthStencilState(InDesc)
{
}

FMetalDepthStencilStateRHI::~FMetalDepthStencilStateRHI() = default;

void* FMetalDepthStencilStateRHI::GetRHINativeState() const
{
    return nullptr;
}

FMetalRasterizerStateRHI::FMetalRasterizerStateRHI(const FRHIRasterizerStateDesc& InDesc)
    : FRHIRasterizerState(InDesc)
{
}

FMetalRasterizerStateRHI::~FMetalRasterizerStateRHI() = default;

void* FMetalRasterizerStateRHI::GetRHINativeState() const
{
    return nullptr;
}

FMetalBlendStateRHI::FMetalBlendStateRHI(const FRHIBlendStateDesc& InDesc)
    : FRHIBlendState(InDesc)
    , bAlphaToCoverageEnable(InDesc.bAlphaToCoverageEnable)
    , bLogicOpEnable(InDesc.bLogicOpEnable)
{
    Memory::Memzero(ColorAttachments, sizeof(ColorAttachments));

    const int32 NumAttachments = InDesc.bIndependentBlendEnable
        ? InDesc.NumRenderTargets
        : Math::Max<int32>(InDesc.NumRenderTargets, 1);
    for (int32 Index = 0; Index < NumAttachments; Index++)
    {
        const FRenderTargetBlendInfo& RenderTarget = InDesc.bIndependentBlendEnable
            ? InDesc.RenderTargets[Index]
            : InDesc.RenderTargets[0];
        ColorAttachments[Index].bBlendingEnabled            = RenderTarget.bBlendEnable ? YES : NO;
        ColorAttachments[Index].SourceColorBlendFactor      = MetalRHI::ConvertBlend(RenderTarget.SrcBlend);
        ColorAttachments[Index].DestinationColorBlendFactor = MetalRHI::ConvertBlend(RenderTarget.DstBlend);
        ColorAttachments[Index].ColorBlendOperation         = MetalRHI::ConvertBlendOp(RenderTarget.BlendOp);
        ColorAttachments[Index].SourceAlphaBlendFactor      = MetalRHI::ConvertBlend(RenderTarget.SrcBlendAlpha);
        ColorAttachments[Index].DestinationAlphaBlendFactor = MetalRHI::ConvertBlend(RenderTarget.DstBlendAlpha);
        ColorAttachments[Index].AlphaBlendOperation         = MetalRHI::ConvertBlendOp(RenderTarget.BlendOpAlpha);
        ColorAttachments[Index].WriteMask                   = MetalRHI::ConvertColorWriteFlags(RenderTarget.ColorWriteMask);
    }
}

FMetalBlendStateRHI::~FMetalBlendStateRHI() = default;

void* FMetalBlendStateRHI::GetRHINativeState() const
{
    return nullptr;
}

FMetalRenderPipeline::FMetalRenderPipeline(FMetalDevice* InDevice, EMetalRenderPipelineType InType)
    : FMetalDeviceChild(InDevice)
    , Type(InType)
    , PipelineState(nil)
    , RenderState()
    , Bindings()
    , StaticSamplers()
    , PrimitiveType(MTLPrimitiveTypeTriangle)
    , ViewInstancing()
    , NumVertexStreams(0)
{
}

FMetalRenderPipeline::~FMetalRenderPipeline()
{
    if (PipelineState)
    {
        FMetalDeviceRHI::DeferDeletion(PipelineState);
        [PipelineState release];
        PipelineState = nil;
    }
}

bool FMetalRenderPipeline::Initialize(const FRHIDepthStencilStateDesc& DepthStencilDesc, const FRHIRasterizerStateDesc& RasterizerDesc, const FRHIGraphicsPipelineFormats& Formats,
    const FRHIViewInstancingState& InViewInstancing)
{
    if (DepthStencilDesc.bDepthBoundsTestEnable)
    {
        METAL_ERROR("Metal does not support depth bounds tests");
        return false;
    }

    const bool bHasDepthStencil    = Formats.DepthStencilFormat != EFormat::Unknown;
    const bool bUsesDepthOrStencil = DepthStencilDesc.bDepthEnable || DepthStencilDesc.bDepthWriteEnable || DepthStencilDesc.bStencilEnable;

    if (bUsesDepthOrStencil && !bHasDepthStencil)
    {
        METAL_ERROR("A render pipeline enables depth or stencil without a depth-stencil format");
        return false;
    }

    if (RasterizerDesc.bEnableConservativeRaster)
    {
        METAL_ERROR("Metal does not support conservative rasterization");
        return false;
    }

    if (InViewInstancing.bEnableViewInstancing)
    {
        if (!RHI::bSupportsViewInstancing)
        {
            METAL_ERROR("View instancing is not supported on this Metal device");
            return false;
        }

        if (InViewInstancing.NumArraySlices == 0 || InViewInstancing.NumArraySlices > RHI::MaxViewInstanceCount)
        {
            METAL_ERROR("View instancing requested %u slices, device max is %u", InViewInstancing.NumArraySlices, RHI::MaxViewInstanceCount);
            return false;
        }
    }

    RenderState.DepthStencilState  = GetDevice()->GetDepthStencilState(bHasDepthStencil ? DepthStencilDesc : GetDisabledDepthStencilDesc());
    RenderState.FrontFacingWinding = RasterizerDesc.bFrontCounterClockwise ? MTLWindingCounterClockwise : MTLWindingClockwise;
    RenderState.CullMode           = MetalRHI::ConvertCullMode(RasterizerDesc.CullMode);
    RenderState.FillMode           = MetalRHI::ConvertFillMode(RasterizerDesc.FillMode);
    RenderState.DepthClipMode      = RasterizerDesc.bDepthClipEnable ? MTLDepthClipModeClip : MTLDepthClipModeClamp;
    ViewInstancing                 = InViewInstancing;

    if (!RenderState.DepthStencilState)
    {
        METAL_ERROR("Failed to create the depth-stencil state for a render pipeline");
        return false;
    }

    return true;
}

bool FMetalRenderPipeline::CreateStaticSamplers(const TArrayView<const FRHIStaticSamplerInfo>& Infos)
{
    return CreateStaticSamplerBindings(StaticSamplers, Bindings, Infos);
}

void FMetalRenderPipeline::SetPipelineState(id<MTLRenderPipelineState> InPipelineState, MTLRenderPipelineReflection* Reflection)
{
    PipelineState = InPipelineState;

    if (!Reflection)
    {
        return;
    }

    if (Type == EMetalRenderPipelineType::Graphics)
    {
        Bindings.PruneToReflection(EShaderVisibility::Vertex, Reflection.vertexBindings);
    }
    else
    {
        Bindings.PruneToReflection(EShaderVisibility::Mesh, Reflection.meshBindings);
        Bindings.PruneToReflection(EShaderVisibility::Amplification, Reflection.objectBindings);
    }

    Bindings.PruneToReflection(EShaderVisibility::Pixel, Reflection.fragmentBindings);
}

void FMetalRenderPipeline::Apply(id<MTLRenderCommandEncoder> Encoder, const FMetalRenderPipeline* Previous) const
{
    [Encoder setRenderPipelineState:PipelineState];

    const FMetalRenderStateBlock* PreviousState = Previous ? &Previous->RenderState : nullptr;

    if (!PreviousState || PreviousState->DepthStencilState != RenderState.DepthStencilState)
    {
        [Encoder setDepthStencilState:RenderState.DepthStencilState];
    }

    if (!PreviousState || PreviousState->FrontFacingWinding != RenderState.FrontFacingWinding)
    {
        [Encoder setFrontFacingWinding:RenderState.FrontFacingWinding];
    }

    if (!PreviousState || PreviousState->CullMode != RenderState.CullMode)
    {
        [Encoder setCullMode:RenderState.CullMode];
    }

    if (!PreviousState || PreviousState->FillMode != RenderState.FillMode)
    {
        [Encoder setTriangleFillMode:RenderState.FillMode];
    }

    if (!PreviousState || PreviousState->DepthClipMode != RenderState.DepthClipMode)
    {
        [Encoder setDepthClipMode:RenderState.DepthClipMode];
    }

    if (Previous ? Previous->ViewInstancing != ViewInstancing : ViewInstancing.bEnableViewInstancing)
    {
        if (ViewInstancing.bEnableViewInstancing)
        {
            MTLVertexAmplificationViewMapping Mappings[8];
            const uint32 Count = Math::Min<uint32>(ViewInstancing.NumArraySlices, ARRAY_COUNT(Mappings));
            for (uint32 Index = 0; Index < Count; ++Index)
            {
                Mappings[Index].renderTargetArrayIndexOffset = ViewInstancing.StartRenderTargetArrayIndex + Index;
                Mappings[Index].viewportArrayIndexOffset     = 0;
            }

            [Encoder setVertexAmplificationCount:Count viewMappings:Mappings];
        }
        else
        {
            [Encoder setVertexAmplificationCount:1 viewMappings:nil];
        }
    }
}

FMetalGraphicsPipelineStateRHI::FMetalGraphicsPipelineStateRHI(FMetalDevice* InDevice, const FRHIGraphicsPipelineStateDesc& InDesc)
    : FRHIGraphicsPipelineState()
    , FMetalDeviceChild(InDevice)
    , Desc(InDesc)
    , RenderPipeline(InDevice, EMetalRenderPipelineType::Graphics)
{
}

FMetalGraphicsPipelineStateRHI::~FMetalGraphicsPipelineStateRHI() = default;

bool FMetalGraphicsPipelineStateRHI::Initialize()
{
    SCOPED_AUTORELEASE_POOL();

    if (Desc.HullShader || Desc.DomainShader || Desc.GeometryShader)
    {
        METAL_ERROR("Metal does not support hull, domain or geometry shaders");
        return false;
    }

    if (Desc.StreamOutputDeclaration)
    {
        METAL_ERROR("Metal does not support stream output");
        return false;
    }

    const MTLPrimitiveType PrimitiveType = MetalRHI::ConvertPrimitiveTopology(Desc.PrimitiveTopology);

    if (PrimitiveType == MTLPrimitiveType(-1))
    {
        METAL_ERROR("Unsupported primitive topology for a Metal graphics PSO");
        return false;
    }

    RenderPipeline.SetPrimitiveType(PrimitiveType);

    const FRHIDepthStencilStateDesc DepthStencilDesc = Desc.DepthStencilState
        ? Desc.DepthStencilState->GetDesc()
        : FRHIDepthStencilStateDesc();

    const FRHIRasterizerStateDesc RasterizerDesc = Desc.RasterizerState
        ? Desc.RasterizerState->GetDesc()
        : FRHIRasterizerStateDesc();

    if (!RenderPipeline.Initialize(DepthStencilDesc, RasterizerDesc, Desc.RasterizerOutputFormats, Desc.ViewInstancingState))
    {
        return false;
    }

    FMetalPipelineBindingLayout& Bindings = RenderPipeline.GetBindings();

    MTLRenderPipelineDescriptor* Descriptor = [[MTLRenderPipelineDescriptor new] autorelease];

    if (FMetalShader* VertexShader = GetMetalShader(Desc.VertexShader))
    {
        Descriptor.vertexFunction = VertexShader->GetMTLFunction();

        if (!Bindings.Collect(VertexShader->GetBindings(), EShaderVisibility::Vertex, VertexShader->GetShaderConstantsSize()))
        {
            return false;
        }
    }

    FMetalInputLayoutRHI* InputLayout = static_cast<FMetalInputLayoutRHI*>(Desc.InputLayout);

    if (Bindings.ConflictsWithVertexInputs(InputLayout))
    {
        return false;
    }

    RenderPipeline.SetNumVertexStreams(InputLayout ? InputLayout->GetNumVertexStreams() : 0);

    if (FMetalShader* PixelShader = GetMetalShader(Desc.PixelShader))
    {
        Descriptor.fragmentFunction = PixelShader->GetMTLFunction();

        if (!Bindings.Collect(PixelShader->GetBindings(), EShaderVisibility::Pixel, PixelShader->GetShaderConstantsSize()))
        {
            return false;
        }
    }

    if (!ApplyColorAttachments(Descriptor, static_cast<FMetalBlendStateRHI*>(Desc.BlendState), Desc.RasterizerOutputFormats))
    {
        return false;
    }

    Descriptor.rasterSampleCount      = Math::Max(Desc.MultiSampleState.SampleCount, 1u);
    Descriptor.inputPrimitiveTopology = MetalRHI::ConvertPrimitiveTopologyClass(Desc.PrimitiveTopology);
    Descriptor.vertexDescriptor       = InputLayout ? InputLayout->GetMTLVertexDescriptor() : nil;
    Descriptor.label                  = PipelineDebugLabel(DebugName, @"GraphicsPSO");

    if (Desc.ViewInstancingState.bEnableViewInstancing)
    {
        Descriptor.maxVertexAmplificationCount = Desc.ViewInstancingState.NumArraySlices;
    }

    NSError*                     Error      = nil;
    MTLRenderPipelineReflection* Reflection = nil;
    id<MTLRenderPipelineState>   PipelineState = [GetDevice()->GetMTLDevice() newRenderPipelineStateWithDescriptor:Descriptor
                                                                                                         options:MTLPipelineOptionBindingInfo
                                                                                                      reflection:&Reflection
                                                                                                           error:&Error];

    if (PipelineState == nil)
    {
        const String ErrorString([Error localizedDescription]);
        METAL_ERROR("Failed to create pipeline state, error %s", *ErrorString);
        return false;
    }

    RenderPipeline.SetPipelineState(PipelineState, Reflection);

    if (!RenderPipeline.CreateStaticSamplers(Desc.StaticSamplers))
    {
        return false;
    }

    STAT_ADD(STAT_Metal_PSOCreateCount, 1);
    STAT_ADD(STAT_Metal_NumGraphicsPipelineStates, 1);
    return true;
}

void FMetalGraphicsPipelineStateRHI::SetDebugName(const String& InName)
{
    DebugName = InName;
}

void FMetalGraphicsPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FMetalGraphicsPipelineStateRHI::GetRHINativeState() const
{
    return (__bridge void*)RenderPipeline.GetMTLPipelineState();
}

FMetalComputePipelineStateRHI::FMetalComputePipelineStateRHI(FMetalDevice* InDevice, const FRHIComputePipelineStateDesc& InDesc)
    : FRHIComputePipelineState()
    , FMetalDeviceChild(InDevice)
    , Desc(InDesc)
    , PipelineState(nil)
    , Bindings()
    , StaticSamplers()
    , ThreadsPerThreadgroup(MTLSizeMake(0, 0, 0))
{
}

FMetalComputePipelineStateRHI::~FMetalComputePipelineStateRHI()
{
    if (PipelineState)
    {
        FMetalDeviceRHI::DeferDeletion(PipelineState);
        [PipelineState release];
        PipelineState = nil;
    }
}

bool FMetalComputePipelineStateRHI::Initialize()
{
    SCOPED_AUTORELEASE_POOL();

    FMetalShader* ComputeShader = GetMetalShader(Desc.Shader);

    if (!ComputeShader || !ComputeShader->GetMTLFunction())
    {
        METAL_ERROR("Compute Shader cannot be nullptr");
        return false;
    }

    ThreadsPerThreadgroup = MTLSizeMake(ComputeShader->GetThreadGroupSizeX(), ComputeShader->GetThreadGroupSizeY(), ComputeShader->GetThreadGroupSizeZ());

    if (ThreadsPerThreadgroup.width == 0 || ThreadsPerThreadgroup.height == 0 || ThreadsPerThreadgroup.depth == 0)
    {
        METAL_ERROR("A compute pipeline requires a non-zero threadgroup size from the compute shader");
        return false;
    }

    MTLComputePipelineDescriptor* Descriptor = [[MTLComputePipelineDescriptor new] autorelease];
    Descriptor.computeFunction = ComputeShader->GetMTLFunction();
    Descriptor.label           = PipelineDebugLabel(DebugName, @"ComputePSO");

    NSError*                      Error      = nil;
    MTLComputePipelineReflection* Reflection = nil;
    PipelineState = [GetDevice()->GetMTLDevice() newComputePipelineStateWithDescriptor:Descriptor
                                                                               options:MTLPipelineOptionBindingInfo
                                                                            reflection:&Reflection
                                                                                 error:&Error];

    if (PipelineState == nil)
    {
        const String ErrorString([Error localizedDescription]);
        METAL_ERROR("Failed to create compute pipeline state, error %s", *ErrorString);
        return false;
    }

    if (!Bindings.Collect(ComputeShader->GetBindings(), EShaderVisibility::Compute, ComputeShader->GetShaderConstantsSize()))
    {
        return false;
    }

    Bindings.PruneToReflection(EShaderVisibility::Compute, Reflection ? Reflection.bindings : nil);

    if (!CreateStaticSamplerBindings(StaticSamplers, Bindings, Desc.StaticSamplers))
    {
        return false;
    }

    STAT_ADD(STAT_Metal_PSOCreateCount, 1);
    STAT_ADD(STAT_Metal_NumComputePipelineStates, 1);
    return true;
}

void FMetalComputePipelineStateRHI::SetDebugName(const String& InName)
{
    DebugName = InName;
}

void FMetalComputePipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FMetalComputePipelineStateRHI::GetRHINativeState() const
{
    return (__bridge void*)PipelineState;
}

FMetalMeshletPipelineStateRHI::FMetalMeshletPipelineStateRHI(FMetalDevice* InDevice, const FRHIMeshletPipelineStateDesc& InDesc)
    : FRHIMeshletPipelineState()
    , FMetalDeviceChild(InDevice)
    , Desc(InDesc)
    , RenderPipeline(InDevice, EMetalRenderPipelineType::Meshlet)
    , MeshThreadgroupSize(MTLSizeMake(0, 0, 0))
    , ObjectThreadgroupSize(MTLSizeMake(1, 1, 1))
{
}

FMetalMeshletPipelineStateRHI::~FMetalMeshletPipelineStateRHI() = default;

bool FMetalMeshletPipelineStateRHI::Initialize()
{
    SCOPED_AUTORELEASE_POOL();

    if (!GMetalSupportsMeshShaders)
    {
        METAL_ERROR("This Metal device does not support mesh shaders");
        return false;
    }

    FMetalShader* MeshShader = GetMetalShader(Desc.MeshShader);

    if (!MeshShader || !MeshShader->GetMTLFunction())
    {
        METAL_ERROR("Mesh Shader cannot be nullptr");
        return false;
    }

    MeshThreadgroupSize = MTLSizeMake(MeshShader->GetThreadGroupSizeX(), MeshShader->GetThreadGroupSizeY(), MeshShader->GetThreadGroupSizeZ());

    if (MeshThreadgroupSize.width == 0 || MeshThreadgroupSize.height == 0 || MeshThreadgroupSize.depth == 0)
    {
        METAL_ERROR("A meshlet pipeline requires a non-zero threadgroup size from the mesh shader");
        return false;
    }

    const FRHIDepthStencilStateDesc DepthStencilDesc = Desc.DepthStencilState
        ? Desc.DepthStencilState->GetDesc()
        : FRHIDepthStencilStateDesc();

    const FRHIRasterizerStateDesc RasterizerDesc = Desc.RasterizerState
        ? Desc.RasterizerState->GetDesc()
        : FRHIRasterizerStateDesc();

    if (!RenderPipeline.Initialize(DepthStencilDesc, RasterizerDesc, Desc.RasterizerOutputFormats, Desc.ViewInstancingState))
    {
        return false;
    }

    FMetalPipelineBindingLayout& Bindings = RenderPipeline.GetBindings();

    MTLMeshRenderPipelineDescriptor* Descriptor = [[MTLMeshRenderPipelineDescriptor new] autorelease];
    Descriptor.meshFunction = MeshShader->GetMTLFunction();

    if (!Bindings.Collect(MeshShader->GetBindings(), EShaderVisibility::Mesh, MeshShader->GetShaderConstantsSize()))
    {
        return false;
    }

    if (FMetalShader* AmplificationShader = GetMetalShader(Desc.AmplificationShader))
    {
        Descriptor.objectFunction = AmplificationShader->GetMTLFunction();
        ObjectThreadgroupSize     = MTLSizeMake(AmplificationShader->GetThreadGroupSizeX(), AmplificationShader->GetThreadGroupSizeY(), AmplificationShader->GetThreadGroupSizeZ());

        if (!Bindings.Collect(AmplificationShader->GetBindings(), EShaderVisibility::Amplification, AmplificationShader->GetShaderConstantsSize()))
        {
            return false;
        }
    }

    if (FMetalShader* PixelShader = GetMetalShader(Desc.PixelShader))
    {
        Descriptor.fragmentFunction = PixelShader->GetMTLFunction();

        if (!Bindings.Collect(PixelShader->GetBindings(), EShaderVisibility::Pixel, PixelShader->GetShaderConstantsSize()))
        {
            return false;
        }
    }

    if (!ApplyColorAttachments(Descriptor, static_cast<FMetalBlendStateRHI*>(Desc.BlendState), Desc.RasterizerOutputFormats))
    {
        return false;
    }

    Descriptor.rasterSampleCount = Math::Max(Desc.MultiSampleState.SampleCount, 1u);
    Descriptor.label             = PipelineDebugLabel(DebugName, @"MeshletPSO");

    if (Desc.ViewInstancingState.bEnableViewInstancing)
    {
        Descriptor.maxVertexAmplificationCount = Desc.ViewInstancingState.NumArraySlices;
    }

    NSError*                     Error      = nil;
    MTLRenderPipelineReflection* Reflection = nil;
    id<MTLRenderPipelineState>   PipelineState = [GetDevice()->GetMTLDevice() newRenderPipelineStateWithMeshDescriptor:Descriptor
                                                                                                             options:MTLPipelineOptionBindingInfo
                                                                                                          reflection:&Reflection
                                                                                                               error:&Error];

    if (PipelineState == nil)
    {
        const String ErrorString([Error localizedDescription]);
        METAL_ERROR("Failed to create meshlet pipeline state, error %s", *ErrorString);
        return false;
    }

    RenderPipeline.SetPipelineState(PipelineState, Reflection);

    if (!RenderPipeline.CreateStaticSamplers(Desc.StaticSamplers))
    {
        return false;
    }

    STAT_ADD(STAT_Metal_PSOCreateCount, 1);
    STAT_ADD(STAT_Metal_NumMeshletPipelineStates, 1);
    return true;
}

void FMetalMeshletPipelineStateRHI::SetDebugName(const String& InName)
{
    DebugName = InName;
}

void FMetalMeshletPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FMetalMeshletPipelineStateRHI::GetRHINativeState() const
{
    return (__bridge void*)RenderPipeline.GetMTLPipelineState();
}

FMetalRayTracingPipelineStateRHI::FMetalRayTracingPipelineStateRHI(FMetalDevice* InDevice, const FRHIRayTracingPipelineStateDesc& InDesc)
    : FRHIRayTracingPipelineState()
    , FMetalDeviceChild(InDevice)
    , Desc(InDesc)
    , PipelineState(nil)
{
}

FMetalRayTracingPipelineStateRHI::~FMetalRayTracingPipelineStateRHI()
{
    if (PipelineState)
    {
        FMetalDeviceRHI::DeferDeletion(PipelineState);
        [PipelineState release];
        PipelineState = nil;
    }
}

TArray<String>& FMetalRayTracingPipelineStateRHI::GetExportNameArray(ERayTracingShaderRecordKind Kind)
{
    switch (Kind)
    {
        case ERayTracingShaderRecordKind::RayGeneration: return RayGenerationExports;
        case ERayTracingShaderRecordKind::Miss:          return MissExports;
        case ERayTracingShaderRecordKind::Callable:      return CallableExports;
        case ERayTracingShaderRecordKind::HitGroup:      return HitGroupExports;
        default:                                         return RayGenerationExports;
    }
}

const TArray<String>& FMetalRayTracingPipelineStateRHI::GetExportNameArray(ERayTracingShaderRecordKind Kind) const
{
    return const_cast<FMetalRayTracingPipelineStateRHI*>(this)->GetExportNameArray(Kind);
}

bool FMetalRayTracingPipelineStateRHI::Initialize()
{
    SCOPED_AUTORELEASE_POOL();

    if (Desc.BasePipeline)
    {
        METAL_ERROR("Metal does not support ray-tracing pipeline additions");
        return false;
    }

    if (Desc.RayGenShaders.IsEmpty())
    {
        METAL_ERROR("A ray-tracing pipeline requires a ray-generation shader");
        return false;
    }

    FMetalShader* RayGenShader = GetMetalShader(Desc.RayGenShaders[0]);

    if (!RayGenShader || !RayGenShader->GetMTLFunction())
    {
        METAL_ERROR("Ray-generation shader cannot be nullptr");
        return false;
    }

    NSMutableArray<id<MTLFunction>>* LinkedShaderFunctions = [NSMutableArray array];

    auto AddLinkedFunction = [&](FRHIShader* Shader)
    {
        FMetalShader* MetalShader = GetMetalShader(Shader);

        if (MetalShader && MetalShader->GetMTLFunction())
        {
            [LinkedShaderFunctions addObject:MetalShader->GetMTLFunction()];
        }
    };

    for (FRHIRayMissShader* Miss : Desc.MissShaders)
    {
        AddLinkedFunction(Miss);
        FMetalRayTracingShader* MetalMiss = GetMetalRayTracingShader(Miss);
        MissExports.Emplace(MetalMiss ? MetalMiss->GetIdentifier() : String());
    }

    for (FRHIRayCallableShader* Callable : Desc.CallableShaders)
    {
        AddLinkedFunction(Callable);
        FMetalRayTracingShader* MetalCallable = GetMetalRayTracingShader(Callable);
        CallableExports.Emplace(MetalCallable ? MetalCallable->GetIdentifier() : String());
    }

    for (const FRHIRayTracingHitGroupInfo& HitGroup : Desc.HitGroups)
    {
        for (FRHIRayTracingShader* HitShader : HitGroup.Shaders)
        {
            AddLinkedFunction(HitShader);
        }

        HitGroupExports.Emplace(HitGroup.Name);
    }

    for (int32 Index = 1; Index < Desc.RayGenShaders.Size(); ++Index)
    {
        AddLinkedFunction(Desc.RayGenShaders[Index]);
    }

    for (FRHIRayGenShader* RayGen : Desc.RayGenShaders)
    {
        FMetalRayTracingShader* MetalRayGen = GetMetalRayTracingShader(RayGen);
        RayGenerationExports.Emplace(MetalRayGen ? MetalRayGen->GetIdentifier() : String());
    }

    MTLComputePipelineDescriptor* Descriptor = [[MTLComputePipelineDescriptor new] autorelease];
    Descriptor.computeFunction   = RayGenShader->GetMTLFunction();
    Descriptor.maxCallStackDepth = Math::Max(Desc.MaxRecursionDepth, 1u);
    Descriptor.label             = PipelineDebugLabel(DebugName, @"RayTracingPSO");

    if (LinkedShaderFunctions.count > 0)
    {
        MTLLinkedFunctions* LinkedFunctions = [[MTLLinkedFunctions new] autorelease];
        LinkedFunctions.functions = LinkedShaderFunctions;
        Descriptor.linkedFunctions = LinkedFunctions;
    }

    NSError* Error = nil;
    PipelineState = [GetDevice()->GetMTLDevice() newComputePipelineStateWithDescriptor:Descriptor
                                                                               options:MTLPipelineOptionNone
                                                                            reflection:nil
                                                                                 error:&Error];

    if (PipelineState == nil)
    {
        const String ErrorString([Error localizedDescription]);
        METAL_ERROR("Failed to create ray-tracing pipeline state, error %s", *ErrorString);
        return false;
    }

    return true;
}

void FMetalRayTracingPipelineStateRHI::GetExportName(ERayTracingShaderRecordKind Kind, uint32 RecordIndex, String& OutExportName) const
{
    const TArray<String>& Names = GetExportNameArray(Kind);
    OutExportName = (RecordIndex < static_cast<uint32>(Names.Size())) ? Names[RecordIndex] : String();
}

uint32 FMetalRayTracingPipelineStateRHI::GetNumExportNames(ERayTracingShaderRecordKind Kind) const
{
    return static_cast<uint32>(GetExportNameArray(Kind).Size());
}

void FMetalRayTracingPipelineStateRHI::SetDebugName(const String& InName)
{
    DebugName = InName;
}

void FMetalRayTracingPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FMetalRayTracingPipelineStateRHI::GetRHINativeState() const
{
    return (__bridge void*)PipelineState;
}
