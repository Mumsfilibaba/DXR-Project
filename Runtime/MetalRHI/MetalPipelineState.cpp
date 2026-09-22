#include "MetalRHI/MetalPipelineState.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalCapabilities.h"
#include "MetalRHI/MetalCommandContextState.h"
#include "MetalRHI/MetalStats.h"
#include "RHI/MSLShaderBindings.h"
#include "RHI/RHISamplerState.h"
#include "RHI/RHI.h"
#include <objc/message.h>

static NSString* PipelineDebugLabel(const String& Name, NSString* Fallback)
{
    return Name.IsEmpty() ? Fallback : Name.GetNSString();
}

static bool ConfigureViewInstancing(const FRHIViewInstancingState& State, NSUInteger& OutAmplificationCount)
{
    OutAmplificationCount = 1;
    if (!State.bEnableViewInstancing)
    {
        return true;
    }

    if (!RHI::bSupportsViewInstancing)
    {
        METAL_ERROR("View instancing is not supported on this Metal device");
        return false;
    }

    if (State.NumArraySlices == 0 || State.NumArraySlices > RHI::MaxViewInstanceCount)
    {
        METAL_ERROR("View instancing requested %u slices, device max is %u", State.NumArraySlices, RHI::MaxViewInstanceCount);
        return false;
    }

    OutAmplificationCount = State.NumArraySlices;
    return true;
}

static bool AssignMSLSlot(uint8& Dest, const FMSLShaderBinding& Binding, EShaderVisibility::Type ShaderStage)
{
    const uint8 TableLimit = GetMSLMaxSlotCount(Binding.BindingType);
    if (Binding.SlotIndex >= TableLimit)
    {
        METAL_ERROR("%s register %u resolved to MSL slot %u, which exceeds the %u-slot table (stage %u)",
            ToString(Binding.BindingType), Binding.RegisterIndex, Binding.SlotIndex, TableLimit, ShaderStage);
        return false;
    }

    if (Dest != FMetalPipelineBindingLayout::InvalidSlot && Dest != Binding.SlotIndex)
    {
        METAL_ERROR("Shader stage %u already mapped %s register %u to MSL slot %u, cannot also use slot %u",
            ShaderStage, ToString(Binding.BindingType), Binding.RegisterIndex, Dest, Binding.SlotIndex);
        return false;
    }

    Dest = Binding.SlotIndex;
    return true;
}

static EShaderVisibility::Type ConvertStaticSamplerVisibility(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
        case EShaderStage::Vertex:
            return EShaderVisibility::Vertex;
        case EShaderStage::Pixel:
            return EShaderVisibility::Pixel;
        case EShaderStage::Compute:
            return EShaderVisibility::Compute;
        case EShaderStage::Mesh:
            return EShaderVisibility::Mesh;
        case EShaderStage::Amplification:
            return EShaderVisibility::Amplification;
        default:
            return EShaderVisibility::Count;
    }
}

static bool CreateStaticSamplers(
    TArray<FMetalStaticSamplerBinding>& OutSamplers,
    const TArrayView<const FRHIStaticSamplerInfo>& Infos,
    const FMetalPipelineBindingLayout& Layout)
{
    OutSamplers.Clear();

    for (const FRHIStaticSamplerInfo& Info : Infos)
    {
        if (Info.ShaderRegister >= MAX_SAMPLER_STATES)
        {
            METAL_ERROR("Static sampler register s%u exceeds MAX_SAMPLER_STATES", Info.ShaderRegister);
            return false;
        }

        const uint8 RegisterIndex = static_cast<uint8>(Info.ShaderRegister);

        TArray<EShaderVisibility::Type> Stages;
        const EShaderVisibility::Type ConvertedStage = ConvertStaticSamplerVisibility(Info.ShaderVisibility);
        if (ConvertedStage < EShaderVisibility::Count)
        {
            Stages.Add(ConvertedStage);
        }
        else
        {
            for (uint8 Stage = 0; Stage < EShaderVisibility::Count; ++Stage)
            {
                const EShaderVisibility::Type Visibility = static_cast<EShaderVisibility::Type>(Stage);
                if (Layout.GetSlot(Visibility, EMSLBindingType::Sampler, RegisterIndex) != FMetalPipelineBindingLayout::InvalidSlot)
                {
                    Stages.Add(Visibility);
                }
            }
        }

        if (Stages.IsEmpty())
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

        for (EShaderVisibility::Type Stage : Stages)
        {
            FMetalStaticSamplerBinding& Binding = OutSamplers.Emplace();
            Binding.Stage         = Stage;
            Binding.RegisterIndex = RegisterIndex;
            Binding.Sampler       = Sampler;
        }
    }

    return true;
}

static bool HasStaticSamplerBinding(const TArray<FMetalStaticSamplerBinding>& Samplers, EShaderVisibility::Type ShaderStage, uint32 RegisterIndex)
{
    for (const FMetalStaticSamplerBinding& Binding : Samplers)
    {
        if (Binding.Stage == ShaderStage && Binding.RegisterIndex == RegisterIndex)
        {
            return true;
        }
    }

    return false;
}

static void ApplyStaticSamplerBindings(const TArray<FMetalStaticSamplerBinding>& Samplers, FMetalSamplerStateCache& Cache)
{
    for (const FMetalStaticSamplerBinding& Binding : Samplers)
    {
        CHECK(Binding.RegisterIndex < MAX_SAMPLER_STATES);
        Cache.SamplerStates[Binding.Stage][Binding.RegisterIndex] = Binding.Sampler.Get();
        Cache.NumSamplers[Binding.Stage] = Math::Max<uint8>(Cache.NumSamplers[Binding.Stage], static_cast<uint8>(Binding.RegisterIndex + 1));
        Cache.DirtyResources(Binding.Stage);
    }
}

template<typename TPipelineDescriptor>
static bool ApplyColorAttachments(TPipelineDescriptor* Descriptor, FMetalBlendStateRHI* BlendState, const FRHIGraphicsPipelineFormats& Formats)
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

    return true;
}

template<typename TPipelineDescriptor>
static void ApplyDepthStencilFormats(TPipelineDescriptor* Descriptor, EFormat DepthStencilFormat)
{
    const MTLPixelFormat PixelFormat = MetalRHI::ConvertFormat(DepthStencilFormat);
    Descriptor.depthAttachmentPixelFormat = PixelFormat;
    if (MetalRHI::IsStencilPixelFormat(PixelFormat))
    {
        Descriptor.stencilAttachmentPixelFormat = PixelFormat;
    }
}

static bool UsesDepthOrStencil(const FRHIDepthStencilStateDesc& Desc)
{
    return Desc.bDepthEnable || Desc.bDepthWriteEnable || Desc.bStencilEnable;
}

static bool ValidateDepthStencilCompatibility(const FRHIDepthStencilStateDesc& Desc, EFormat DepthStencilFormat, const CHAR* Context)
{
    if (!UsesDepthOrStencil(Desc) || DepthStencilFormat != EFormat::Unknown)
    {
        return true;
    }

    METAL_ERROR("%s enables depth or stencil without a depth-stencil format", Context);
    return false;
}

void FMetalPipelineBindingLayout::Reset()
{
    for (uint32 ShaderStage = 0; ShaderStage < EShaderVisibility::Count; ++ShaderStage)
    {
        ConstantBuffers[ShaderStage].Fill(InvalidSlot);
        ShaderResourceBuffers[ShaderStage].Fill(InvalidSlot);
        ShaderResourceTextures[ShaderStage].Fill(InvalidSlot);
        UnorderedAccessBuffers[ShaderStage].Fill(InvalidSlot);
        UnorderedAccessTextures[ShaderStage].Fill(InvalidSlot);
        Samplers[ShaderStage].Fill(InvalidSlot);
        ShaderConstants[ShaderStage]     = InvalidSlot;
        ShaderConstantsSize[ShaderStage] = 0;
        ResourceHeapSlot[ShaderStage]    = InvalidSlot;
        SamplerHeapSlot[ShaderStage]     = InvalidSlot;
    }
}

bool FMetalPipelineBindingLayout::Collect(const TArray<FMSLShaderBinding>& ShaderBindings, EShaderVisibility::Type ShaderStage, uint16 InShaderConstantsSize)
{
    for (const FMSLShaderBinding& Binding : ShaderBindings)
    {
        switch (Binding.BindingType)
        {
            case EMSLBindingType::ConstantBuffer:
            {
                CHECK(Binding.RegisterIndex < MAX_CONSTANT_BUFFERS);
                if (!AssignMSLSlot(ConstantBuffers[ShaderStage][Binding.RegisterIndex], Binding, ShaderStage))
                {
                    return false;
                }

                break;
            }

            case EMSLBindingType::ShaderResourceBuffer:
            {
                CHECK(Binding.RegisterIndex < MAX_SRVS);
                if (!AssignMSLSlot(ShaderResourceBuffers[ShaderStage][Binding.RegisterIndex], Binding, ShaderStage))
                {
                    return false;
                }

                break;
            }

            case EMSLBindingType::ShaderResourceTexture:
            {
                CHECK(Binding.RegisterIndex < MAX_SRVS);
                if (!AssignMSLSlot(ShaderResourceTextures[ShaderStage][Binding.RegisterIndex], Binding, ShaderStage))
                {
                    return false;
                }

                break;
            }

            case EMSLBindingType::UnorderedAccessBuffer:
            {
                CHECK(Binding.RegisterIndex < MAX_UAVS);
                if (!AssignMSLSlot(UnorderedAccessBuffers[ShaderStage][Binding.RegisterIndex], Binding, ShaderStage))
                {
                    return false;
                }

                break;
            }

            case EMSLBindingType::UnorderedAccessTexture:
            {
                CHECK(Binding.RegisterIndex < MAX_UAVS);
                if (!AssignMSLSlot(UnorderedAccessTextures[ShaderStage][Binding.RegisterIndex], Binding, ShaderStage))
                {
                    return false;
                }

                break;
            }

            case EMSLBindingType::Sampler:
            {
                CHECK(Binding.RegisterIndex < MAX_SAMPLER_STATES);
                if (!AssignMSLSlot(Samplers[ShaderStage][Binding.RegisterIndex], Binding, ShaderStage))
                {
                    return false;
                }

                break;
            }

            case EMSLBindingType::ShaderConstants:
            {
                if (!AssignMSLSlot(ShaderConstants[ShaderStage], Binding, ShaderStage))
                {
                    return false;
                }

                ShaderConstantsSize[ShaderStage] = InShaderConstantsSize;
                break;
            }

            case EMSLBindingType::BindlessResourceHeap:
            {
                if (!AssignMSLSlot(ResourceHeapSlot[ShaderStage], Binding, ShaderStage))
                {
                    return false;
                }

                break;
            }

            case EMSLBindingType::BindlessSamplerHeap:
            {
                if (!AssignMSLSlot(SamplerHeapSlot[ShaderStage], Binding, ShaderStage))
                {
                    return false;
                }

                break;
            }

            default:
            {
                METAL_ERROR("Unhandled MSL binding type %s", ToString(Binding.BindingType));
                return false;
            }
        }
    }

    return true;
}

bool FMetalPipelineBindingLayout::ConflictsWithVertexInputs(const FMetalInputLayoutRHI* InputLayout) const
{
    if (!InputLayout)
    {
        return false;
    }

    const uint32 NumElements = InputLayout->GetNumInputElementDescs();
    for (uint32 ElementIndex = 0; ElementIndex < NumElements; ++ElementIndex)
    {
        const FRHIInputElementDesc* Element = InputLayout->GetInputElementDesc(ElementIndex);
        if (!Element)
        {
            continue;
        }

        const uint32 InputSlot         = Element->InputSlot;
        const uint8  StreamBufferIndex = GetMSLVertexStreamBufferIndex(InputSlot);
        auto OccupiesSlot = [StreamBufferIndex](uint8 Slot) -> bool
        {
            return Slot != InvalidSlot && Slot == StreamBufferIndex;
        };

        const EShaderVisibility::Type VertexStage = EShaderVisibility::Vertex;
        for (uint32 RegisterIndex = 0; RegisterIndex < MAX_CONSTANT_BUFFERS; ++RegisterIndex)
        {
            if (OccupiesSlot(ConstantBuffers[VertexStage][RegisterIndex]))
            {
                METAL_ERROR("Vertex shader constant buffer register %u uses MSL slot %u, which is reserved for input stream slot %u",
                    RegisterIndex, ConstantBuffers[VertexStage][RegisterIndex], InputSlot);
                return true;
            }
        }

        for (uint32 RegisterIndex = 0; RegisterIndex < MAX_SRVS; ++RegisterIndex)
        {
            if (OccupiesSlot(ShaderResourceBuffers[VertexStage][RegisterIndex]))
            {
                METAL_ERROR("Vertex shader SRV buffer register %u uses MSL slot %u, which is reserved for input stream slot %u",
                    RegisterIndex, ShaderResourceBuffers[VertexStage][RegisterIndex], InputSlot);
                return true;
            }
        }

        for (uint32 RegisterIndex = 0; RegisterIndex < MAX_UAVS; ++RegisterIndex)
        {
            if (OccupiesSlot(UnorderedAccessBuffers[VertexStage][RegisterIndex]))
            {
                METAL_ERROR("Vertex shader UAV buffer register %u uses MSL slot %u, which is reserved for input stream slot %u",
                    RegisterIndex, UnorderedAccessBuffers[VertexStage][RegisterIndex], InputSlot);
                return true;
            }
        }

        if (OccupiesSlot(ShaderConstants[VertexStage]))
        {
            METAL_ERROR("Vertex shader constants use MSL slot %u, which is reserved for input stream slot %u",
                ShaderConstants[VertexStage], InputSlot);
            return true;
        }

        if (OccupiesSlot(ResourceHeapSlot[VertexStage]))
        {
            METAL_ERROR("Vertex shader bindless resource heap uses MSL slot %u, which is reserved for input stream slot %u",
                ResourceHeapSlot[VertexStage], InputSlot);
            return true;
        }

        if (OccupiesSlot(SamplerHeapSlot[VertexStage]))
        {
            METAL_ERROR("Vertex shader bindless sampler heap uses MSL slot %u, which is reserved for input stream slot %u",
                SamplerHeapSlot[VertexStage], InputSlot);
            return true;
        }
    }

    return false;
}

uint8 FMetalPipelineBindingLayout::GetSlot(EShaderVisibility::Type ShaderVisibility, EMSLBindingType BindingType, uint32 RegisterIndex) const
{
    switch (BindingType)
    {
        case EMSLBindingType::ConstantBuffer:
            return (RegisterIndex < MAX_CONSTANT_BUFFERS) ? ConstantBuffers[ShaderVisibility][RegisterIndex] : InvalidSlot;

        case EMSLBindingType::ShaderResourceBuffer:
            return (RegisterIndex < MAX_SRVS) ? ShaderResourceBuffers[ShaderVisibility][RegisterIndex] : InvalidSlot;

        case EMSLBindingType::ShaderResourceTexture:
            return (RegisterIndex < MAX_SRVS) ? ShaderResourceTextures[ShaderVisibility][RegisterIndex] : InvalidSlot;

        case EMSLBindingType::UnorderedAccessBuffer:
            return (RegisterIndex < MAX_UAVS) ? UnorderedAccessBuffers[ShaderVisibility][RegisterIndex] : InvalidSlot;

        case EMSLBindingType::UnorderedAccessTexture:
            return (RegisterIndex < MAX_UAVS) ? UnorderedAccessTextures[ShaderVisibility][RegisterIndex] : InvalidSlot;

        case EMSLBindingType::Sampler:
            return (RegisterIndex < MAX_SAMPLER_STATES) ? Samplers[ShaderVisibility][RegisterIndex] : InvalidSlot;

        case EMSLBindingType::ShaderConstants:
            return ShaderConstants[ShaderVisibility];

        case EMSLBindingType::BindlessResourceHeap:
            return ResourceHeapSlot[ShaderVisibility];

        case EMSLBindingType::BindlessSamplerHeap:
            return SamplerHeapSlot[ShaderVisibility];

        default:
            return InvalidSlot;
    }
}

FMetalPipelineBindingLayout::FMetalPipelineBindingLayout()
{
    Reset();
}

FMetalPipelineBindingLayout::~FMetalPipelineBindingLayout() = default;

FMetalInputLayoutRHI::FMetalInputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements)
    : FRHIInputLayout()
    , InputElements(InInputElements)
    , VertexDescriptor(nullptr)
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
        VertexDescriptor.layouts[StreamBufferIndex].stepRate     = Element.InputClass == EVertexInputClass::Vertex ? 1 : Element.InstanceStepRate;
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

FMetalDepthStencilStateRHI::FMetalDepthStencilStateRHI(FMetalDevice* InDevice, const FRHIDepthStencilStateDesc& InDesc)
    : FRHIDepthStencilState(InDesc)
    , FMetalDeviceChild(InDevice)
    , DepthStencilState(nullptr)
{
}

FMetalDepthStencilStateRHI::~FMetalDepthStencilStateRHI()
{
    if (DepthStencilState)
    {
        FMetalDeviceRHI::DeferDeletion(DepthStencilState);
        [DepthStencilState release];
        DepthStencilState = nil;
    }
}

void* FMetalDepthStencilStateRHI::GetRHINativeState() const
{
    return nullptr;
}

bool FMetalDepthStencilStateRHI::Initialize()
{
    SCOPED_AUTORELEASE_POOL();

    MTLDepthStencilDescriptor* Descriptor = [[MTLDepthStencilDescriptor new] autorelease];
    Descriptor.depthWriteEnabled    = Desc.bDepthWriteEnable ? YES : NO;
    Descriptor.depthCompareFunction = Desc.bDepthEnable ? MetalRHI::ConvertCompareFunction(Desc.DepthFunc) : MTLCompareFunctionAlways;

    if (Desc.bStencilEnable)
    {
        Descriptor.backFaceStencil                            = [[MTLStencilDescriptor new] autorelease];
        Descriptor.backFaceStencil.stencilCompareFunction     = MetalRHI::ConvertCompareFunction(Desc.BackFace.StencilFunc);
        Descriptor.backFaceStencil.stencilFailureOperation    = MetalRHI::ConvertStencilOp(Desc.BackFace.StencilFailOp);
        Descriptor.backFaceStencil.depthFailureOperation      = MetalRHI::ConvertStencilOp(Desc.BackFace.StencilDepthFailOp);
        Descriptor.backFaceStencil.depthStencilPassOperation  = MetalRHI::ConvertStencilOp(Desc.BackFace.StencilDepthPassOp);
        Descriptor.backFaceStencil.readMask                   = Desc.StencilReadMask;
        Descriptor.backFaceStencil.writeMask                  = Desc.StencilWriteMask;

        Descriptor.frontFaceStencil                           = [[MTLStencilDescriptor new] autorelease];
        Descriptor.frontFaceStencil.stencilCompareFunction    = MetalRHI::ConvertCompareFunction(Desc.FrontFace.StencilFunc);
        Descriptor.frontFaceStencil.stencilFailureOperation   = MetalRHI::ConvertStencilOp(Desc.FrontFace.StencilFailOp);
        Descriptor.frontFaceStencil.depthFailureOperation     = MetalRHI::ConvertStencilOp(Desc.FrontFace.StencilDepthFailOp);
        Descriptor.frontFaceStencil.depthStencilPassOperation = MetalRHI::ConvertStencilOp(Desc.FrontFace.StencilDepthPassOp);
        Descriptor.frontFaceStencil.readMask                  = Desc.StencilReadMask;
        Descriptor.frontFaceStencil.writeMask                 = Desc.StencilWriteMask;
    }
    else
    {
        Descriptor.backFaceStencil  = nil;
        Descriptor.frontFaceStencil = nil;
    }

    id<MTLDevice> DeviceHandle = GetDevice()->GetMTLDevice();
    CHECK(DeviceHandle != nil);

    DepthStencilState = [DeviceHandle newDepthStencilStateWithDescriptor:Descriptor];
    if (!DepthStencilState)
    {
        METAL_ERROR("Failed to create DepthStencilState");
        return false;
    }

    return true;
}

FMetalRasterizerStateRHI::FMetalRasterizerStateRHI(const FRHIRasterizerStateDesc& InDesc)
    : FRHIRasterizerState(InDesc)
    , FillMode(MetalRHI::ConvertFillMode(InDesc.FillMode))
    , FrontFaceWinding(InDesc.bFrontCounterClockwise ? MTLWindingCounterClockwise : MTLWindingClockwise)
    , CullMode(MetalRHI::ConvertCullMode(InDesc.CullMode))
{
}

FMetalRasterizerStateRHI::~FMetalRasterizerStateRHI()
{
}

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

    const int32 NumAttachments = InDesc.bIndependentBlendEnable ? InDesc.NumRenderTargets : Math::Max<int32>(InDesc.NumRenderTargets, 1);
    for (int32 Index = 0; Index < NumAttachments; Index++)
    {
        const FRenderTargetBlendInfo& RenderTarget = InDesc.bIndependentBlendEnable ? InDesc.RenderTargets[Index] : InDesc.RenderTargets[0];
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

FMetalBlendStateRHI::~FMetalBlendStateRHI()
{
}

void* FMetalBlendStateRHI::GetRHINativeState() const
{
    return nullptr;
}

FMetalGraphicsPipelineStateRHI::FMetalGraphicsPipelineStateRHI(FMetalDevice* InDevice, const FRHIGraphicsPipelineStateDesc& InDesc)
    : FRHIGraphicsPipelineState()
    , FMetalDeviceChild(InDevice)
    , Desc(InDesc)
    , BlendState(nullptr)
    , DepthStencilState(nullptr)
    , RasterizerState(nullptr)
    , PipelineState(nil)
    , PrimitiveType(MTLPrimitiveTypeTriangle)
{
}

FMetalGraphicsPipelineStateRHI::~FMetalGraphicsPipelineStateRHI()
{
    if (PipelineState)
    {
        FMetalDeviceRHI::DeferDeletion(PipelineState);
        [PipelineState release];
        PipelineState = nil;
    }
}

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

    PrimitiveType = MetalRHI::ConvertPrimitiveTopology(Desc.PrimitiveTopology);
    if (PrimitiveType == MTLPrimitiveType(-1))
    {
        METAL_ERROR("Unsupported primitive topology for a Metal graphics PSO");
        return false;
    }

    DepthStencilState = MakeSharedRef<FMetalDepthStencilStateRHI>(Desc.DepthStencilState);
    if (!DepthStencilState)
    {
        METAL_ERROR("Failed to create DepthStencilState for graphics PSO");
        return false;
    }

    if (DepthStencilState->GetDesc().bDepthBoundsTestEnable)
    {
        METAL_ERROR("Metal does not support depth bounds tests");
        return false;
    }

    if (!ValidateDepthStencilCompatibility(DepthStencilState->GetDesc(), Desc.RasterizerOutputFormats.DepthStencilFormat, "Graphics PSO"))
    {
        return false;
    }

    RasterizerState = MakeSharedRef<FMetalRasterizerStateRHI>(Desc.RasterizerState);
    if (!RasterizerState)
    {
        METAL_ERROR("Failed to create RasterizerState for graphics PSO");
        return false;
    }

    if (RasterizerState->GetDesc().bEnableConservativeRaster)
    {
        METAL_ERROR("Metal does not support conservative rasterization");
        return false;
    }

    BlendState = MakeSharedRef<FMetalBlendStateRHI>(Desc.BlendState);

    NSUInteger AmplificationCount = 1;
    if (!ConfigureViewInstancing(Desc.ViewInstancingState, AmplificationCount))
    {
        return false;
    }

    MTLRenderPipelineDescriptor* Descriptor = [MTLRenderPipelineDescriptor new];
    if (FMetalShader* VertexShader = GetMetalShader(Desc.VertexShader))
    {
        Descriptor.vertexFunction = VertexShader->GetMTLFunction();
        if (!Bindings.Collect(VertexShader->GetBindings(), EShaderVisibility::Vertex, VertexShader->GetShaderConstantsSize()))
        {
            [Descriptor release];
            return false;
        }
    }

    FMetalInputLayoutRHI* InputLayout = static_cast<FMetalInputLayoutRHI*>(Desc.InputLayout);
    if (Bindings.ConflictsWithVertexInputs(InputLayout))
    {
        [Descriptor release];
        return false;
    }

    if (FMetalShader* PixelShader = GetMetalShader(Desc.PixelShader))
    {
        Descriptor.fragmentFunction = PixelShader->GetMTLFunction();
        if (!Bindings.Collect(PixelShader->GetBindings(), EShaderVisibility::Pixel, PixelShader->GetShaderConstantsSize()))
        {
            [Descriptor release];
            return false;
        }
    }

    if (!CreateStaticSamplers(StaticSamplers, Desc.StaticSamplers, Bindings))
    {
        [Descriptor release];
        return false;
    }

    if (!ApplyColorAttachments(Descriptor, BlendState.Get(), Desc.RasterizerOutputFormats))
    {
        [Descriptor release];
        return false;
    }

    ApplyDepthStencilFormats(Descriptor, Desc.RasterizerOutputFormats.DepthStencilFormat);
    Descriptor.rasterSampleCount = Math::Max(Desc.MultiSampleState.SampleCount, 1u);

    Descriptor.inputPrimitiveTopology = MetalRHI::ConvertPrimitiveTopologyClass(Desc.PrimitiveTopology);

    Descriptor.vertexDescriptor = InputLayout ? InputLayout->GetMTLVertexDescriptor() : nil;
    if (AmplificationCount > 1)
    {
        Descriptor.maxVertexAmplificationCount = AmplificationCount;
    }

    Descriptor.label = PipelineDebugLabel(DebugName, @"GraphicsPSO");

    NSError* Error = nil;
    PipelineState = [GetDevice()->GetMTLDevice() newRenderPipelineStateWithDescriptor:Descriptor error:&Error];
    [Descriptor release];

    if (PipelineState == nil)
    {
        const String ErrorString([Error localizedDescription]);
        METAL_ERROR("Failed to create pipeline state, error %s", *ErrorString);
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
    return (__bridge void*)PipelineState;
}

void FMetalGraphicsPipelineStateRHI::ApplyStaticSamplers(FMetalSamplerStateCache& Cache) const
{
    ApplyStaticSamplerBindings(StaticSamplers, Cache);
}

bool FMetalGraphicsPipelineStateRHI::HasStaticSampler(EShaderVisibility::Type ShaderStage, uint32 RegisterIndex) const
{
    return HasStaticSamplerBinding(StaticSamplers, ShaderStage, RegisterIndex);
}

FMetalComputePipelineStateRHI::FMetalComputePipelineStateRHI(FMetalDevice* InDevice)
    : FRHIComputePipelineState()
    , FMetalDeviceChild(InDevice)
    , PipelineState(nil)
    , ThreadGroupSizeX(0)
    , ThreadGroupSizeY(0)
    , ThreadGroupSizeZ(0)
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

bool FMetalComputePipelineStateRHI::Initialize(const FRHIComputePipelineStateDesc& InDesc)
{
    SCOPED_AUTORELEASE_POOL();

    FMetalShader* ComputeShader = GetMetalShader(InDesc.Shader);
    if (!ComputeShader || !ComputeShader->GetMTLFunction())
    {
        METAL_ERROR("Compute Shader cannot be nullptr");
        return false;
    }

    NSError* Error = nil;
    MTLComputePipelineDescriptor* Descriptor = [[MTLComputePipelineDescriptor new] autorelease];
    Descriptor.computeFunction = ComputeShader->GetMTLFunction();
    Descriptor.label           = PipelineDebugLabel(DebugName, @"ComputePSO");
    PipelineState = [GetDevice()->GetMTLDevice() newComputePipelineStateWithDescriptor:Descriptor
                                                                               options:MTLPipelineOptionNone
                                                                            reflection:nil
                                                                                 error:&Error];
    if (PipelineState == nil)
    {
        const String ErrorString([Error localizedDescription]);
        METAL_ERROR("Failed to create compute pipeline state, error %s", *ErrorString);
        return false;
    }

    ThreadGroupSizeX = ComputeShader->GetThreadGroupSizeX();
    ThreadGroupSizeY = ComputeShader->GetThreadGroupSizeY();
    ThreadGroupSizeZ = ComputeShader->GetThreadGroupSizeZ();

    if (!Bindings.Collect(ComputeShader->GetBindings(), EShaderVisibility::Compute, ComputeShader->GetShaderConstantsSize()))
    {
        return false;
    }

    if (!CreateStaticSamplers(StaticSamplers, InDesc.StaticSamplers, Bindings))
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

void FMetalComputePipelineStateRHI::ApplyStaticSamplers(FMetalSamplerStateCache& Cache) const
{
    ApplyStaticSamplerBindings(StaticSamplers, Cache);
}

bool FMetalComputePipelineStateRHI::HasStaticSampler(EShaderVisibility::Type ShaderStage, uint32 RegisterIndex) const
{
    return HasStaticSamplerBinding(StaticSamplers, ShaderStage, RegisterIndex);
}

FMetalMeshletPipelineStateRHI::FMetalMeshletPipelineStateRHI(FMetalDevice* InDevice, const FRHIMeshletPipelineStateDesc& InDesc)
    : FRHIMeshletPipelineState()
    , FMetalDeviceChild(InDevice)
    , Desc(InDesc)
    , BlendState(nullptr)
    , DepthStencilState(nullptr)
    , RasterizerState(nullptr)
    , PipelineState(nil)
    , MeshThreadGroupSizeX(0)
    , MeshThreadGroupSizeY(0)
    , MeshThreadGroupSizeZ(0)
    , ObjectThreadGroupSizeX(0)
    , ObjectThreadGroupSizeY(0)
    , ObjectThreadGroupSizeZ(0)
{
}

FMetalMeshletPipelineStateRHI::~FMetalMeshletPipelineStateRHI()
{
    if (PipelineState)
    {
        FMetalDeviceRHI::DeferDeletion(PipelineState);
        [PipelineState release];
        PipelineState = nil;
    }
}

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

    MeshThreadGroupSizeX = MeshShader->GetThreadGroupSizeX();
    MeshThreadGroupSizeY = MeshShader->GetThreadGroupSizeY();
    MeshThreadGroupSizeZ = MeshShader->GetThreadGroupSizeZ();

    DepthStencilState = MakeSharedRef<FMetalDepthStencilStateRHI>(Desc.DepthStencilState);
    if (!DepthStencilState)
    {
        METAL_ERROR("Failed to create DepthStencilState for meshlet PSO");
        return false;
    }

    if (DepthStencilState->GetDesc().bDepthBoundsTestEnable)
    {
        METAL_ERROR("Metal does not support depth bounds tests");
        return false;
    }

    if (!ValidateDepthStencilCompatibility(DepthStencilState->GetDesc(), Desc.RasterizerOutputFormats.DepthStencilFormat, "Meshlet PSO"))
    {
        return false;
    }

    RasterizerState = MakeSharedRef<FMetalRasterizerStateRHI>(Desc.RasterizerState);
    if (!RasterizerState)
    {
        METAL_ERROR("Failed to create RasterizerState for meshlet PSO");
        return false;
    }

    if (RasterizerState->GetDesc().bEnableConservativeRaster)
    {
        METAL_ERROR("Metal does not support conservative rasterization");
        return false;
    }

    BlendState = MakeSharedRef<FMetalBlendStateRHI>(Desc.BlendState);

    NSUInteger AmplificationCount = 1;
    if (!ConfigureViewInstancing(Desc.ViewInstancingState, AmplificationCount))
    {
        return false;
    }

    MTLMeshRenderPipelineDescriptor* Descriptor = [MTLMeshRenderPipelineDescriptor new];
    Descriptor.meshFunction = MeshShader->GetMTLFunction();

    if (!Bindings.Collect(MeshShader->GetBindings(), EShaderVisibility::Mesh, MeshShader->GetShaderConstantsSize()))
    {
        [Descriptor release];
        return false;
    }

    if (FMetalShader* AmplificationShader = GetMetalShader(Desc.AmplificationShader))
    {
        Descriptor.objectFunction = AmplificationShader->GetMTLFunction();
        ObjectThreadGroupSizeX    = AmplificationShader->GetThreadGroupSizeX();
        ObjectThreadGroupSizeY    = AmplificationShader->GetThreadGroupSizeY();
        ObjectThreadGroupSizeZ    = AmplificationShader->GetThreadGroupSizeZ();
        if (!Bindings.Collect(AmplificationShader->GetBindings(), EShaderVisibility::Amplification, AmplificationShader->GetShaderConstantsSize()))
        {
            [Descriptor release];
            return false;
        }
    }

    if (FMetalShader* PixelShader = GetMetalShader(Desc.PixelShader))
    {
        Descriptor.fragmentFunction = PixelShader->GetMTLFunction();
        if (!Bindings.Collect(PixelShader->GetBindings(), EShaderVisibility::Pixel, PixelShader->GetShaderConstantsSize()))
        {
            [Descriptor release];
            return false;
        }
    }

    if (!CreateStaticSamplers(StaticSamplers, Desc.StaticSamplers, Bindings))
    {
        [Descriptor release];
        return false;
    }

    if (!ApplyColorAttachments(Descriptor, BlendState.Get(), Desc.RasterizerOutputFormats))
    {
        [Descriptor release];
        return false;
    }

    ApplyDepthStencilFormats(Descriptor, Desc.RasterizerOutputFormats.DepthStencilFormat);
    Descriptor.rasterSampleCount = Math::Max(Desc.MultiSampleState.SampleCount, 1u);

    if (AmplificationCount > 1)
    {
        Descriptor.maxVertexAmplificationCount = AmplificationCount;
    }

    Descriptor.label = PipelineDebugLabel(DebugName, @"MeshletPSO");

    NSError*      Error     = nil;
    id<MTLDevice> MTLDevice = GetDevice()->GetMTLDevice();
    const SEL     Selector  = NSSelectorFromString(@"newRenderPipelineStateWithMeshDescriptor:error:");
    if ([MTLDevice respondsToSelector:Selector])
    {
        typedef id<MTLRenderPipelineState> (*CreateFn)(id, SEL, MTLMeshRenderPipelineDescriptor*, NSError**);
        PipelineState = ((CreateFn)objc_msgSend)(MTLDevice, Selector, Descriptor, &Error);
    }
    else
    {
        Error = [NSError errorWithDomain:@"MetalRHI"
                                    code:0
                                userInfo:@{ NSLocalizedDescriptionKey: @"MTLDevice does not implement newRenderPipelineStateWithMeshDescriptor:error:" }];
    }

    [Descriptor release];

    if (PipelineState == nil)
    {
        const String ErrorString([Error localizedDescription]);
        METAL_ERROR("Failed to create meshlet pipeline state, error %s", *ErrorString);
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
    return (__bridge void*)PipelineState;
}

void FMetalMeshletPipelineStateRHI::ApplyStaticSamplers(FMetalSamplerStateCache& Cache) const
{
    ApplyStaticSamplerBindings(StaticSamplers, Cache);
}

bool FMetalMeshletPipelineStateRHI::HasStaticSampler(EShaderVisibility::Type ShaderStage, uint32 RegisterIndex) const
{
    return HasStaticSamplerBinding(StaticSamplers, ShaderStage, RegisterIndex);
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
    switch (Kind)
    {
        case ERayTracingShaderRecordKind::RayGeneration: return RayGenerationExports;
        case ERayTracingShaderRecordKind::Miss:          return MissExports;
        case ERayTracingShaderRecordKind::Callable:      return CallableExports;
        case ERayTracingShaderRecordKind::HitGroup:      return HitGroupExports;
        default:                                         return RayGenerationExports;
    }
}

bool FMetalRayTracingPipelineStateRHI::Initialize()
{
    SCOPED_AUTORELEASE_POOL();

    if (!GMetalSupportsRayTracing)
    {
        return false;
    }

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
