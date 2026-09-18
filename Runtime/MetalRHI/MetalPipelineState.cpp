#include "MetalRHI/MetalPipelineState.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalCapabilities.h"
#include "RHI/MSLShaderBindings.h"

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
        ShaderConstants[ShaderStage] = InvalidSlot;
    }
}

void FMetalPipelineBindingLayout::Collect(const TArray<FMSLShaderBinding>& ShaderBindings, EShaderVisibility::Type ShaderStage)
{
    for (const FMSLShaderBinding& Binding : ShaderBindings)
    {
        switch (Binding.BindingType)
        {
            case EMSLBindingType::ConstantBuffer:
            {
                CHECK(Binding.RegisterIndex < MAX_CONSTANT_BUFFERS);
                ConstantBuffers[ShaderStage][Binding.RegisterIndex] = Binding.SlotIndex;
                break;
            }

            case EMSLBindingType::ShaderResourceBuffer:
            {
                CHECK(Binding.RegisterIndex < MAX_SRVS);
                ShaderResourceBuffers[ShaderStage][Binding.RegisterIndex] = Binding.SlotIndex;
                break;
            }

            case EMSLBindingType::ShaderResourceTexture:
            {
                CHECK(Binding.RegisterIndex < MAX_SRVS);
                ShaderResourceTextures[ShaderStage][Binding.RegisterIndex] = Binding.SlotIndex;
                break;
            }

            case EMSLBindingType::UnorderedAccessBuffer:
            {
                CHECK(Binding.RegisterIndex < MAX_UAVS);
                UnorderedAccessBuffers[ShaderStage][Binding.RegisterIndex] = Binding.SlotIndex;
                break;
            }

            case EMSLBindingType::UnorderedAccessTexture:
            {
                CHECK(Binding.RegisterIndex < MAX_UAVS);
                UnorderedAccessTextures[ShaderStage][Binding.RegisterIndex] = Binding.SlotIndex;
                break;
            }

            case EMSLBindingType::Sampler:
            {
                CHECK(Binding.RegisterIndex < MAX_SAMPLER_STATES);
                Samplers[ShaderStage][Binding.RegisterIndex] = Binding.SlotIndex;
                break;
            }

            case EMSLBindingType::ShaderConstants:
            {
                ShaderConstants[ShaderStage] = Binding.SlotIndex;
                break;
            }

            default:
            {
                METAL_ERROR("Unhandled MSL binding type %s", ToString(Binding.BindingType));
                break;
            }
        }
    }
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

        default:
            return InvalidSlot;
    }
}

FMetalPipelineBindingLayout::FMetalPipelineBindingLayout()
{
    Reset();
}

FMetalPipelineBindingLayout::~FMetalPipelineBindingLayout() = default;

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

FMetalInputLayoutRHI::FMetalInputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements)
    : FRHIInputLayout()
    , InputElements(InInputElements)
    , VertexDescriptor(nullptr)
{
    VertexDescriptor = [[MTLVertexDescriptor vertexDescriptor] retain];
    for (int32 Index = 0; Index < InputElements.Size(); ++Index)
    {
        const FRHIInputElementDesc& Element = InputElements[Index];
        VertexDescriptor.attributes[Index].format      = MetalRHI::ConvertVertexFormat(Element.Format);
        VertexDescriptor.attributes[Index].offset      = Element.ByteOffset;
        VertexDescriptor.attributes[Index].bufferIndex = Element.InputSlot;

        VertexDescriptor.layouts[Element.InputSlot].stride       = Element.VertexStride;
        VertexDescriptor.layouts[Element.InputSlot].stepFunction = MetalRHI::ConvertVertexInputClass(Element.InputClass);
        VertexDescriptor.layouts[Element.InputSlot].stepRate     = Element.InputClass == EVertexInputClass::Vertex ? 1 : Element.InstanceStepRate;
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

    MTLRenderPipelineDescriptor* Descriptor = [MTLRenderPipelineDescriptor new];
    if (FMetalShader* VertexShader = GetMetalShader(Desc.VertexShader))
    {
        Descriptor.vertexFunction = VertexShader->GetMTLFunction();
        Bindings.Collect(VertexShader->GetBindings(), EShaderVisibility::Vertex);
    }

    if (FMetalShader* PixelShader = GetMetalShader(Desc.PixelShader))
    {
        Descriptor.fragmentFunction = PixelShader->GetMTLFunction();
        Bindings.Collect(PixelShader->GetBindings(), EShaderVisibility::Pixel);
    }

    if (!ApplyColorAttachments(Descriptor, BlendState.Get(), Desc.RasterizerOutputFormats))
    {
        [Descriptor release];
        return false;
    }

    ApplyDepthStencilFormats(Descriptor, Desc.RasterizerOutputFormats.DepthStencilFormat);
    Descriptor.rasterSampleCount = Math::Max(Desc.MultiSampleState.SampleCount, 1u);

    FMetalInputLayoutRHI* InputLayout = static_cast<FMetalInputLayoutRHI*>(Desc.InputLayout);
    Descriptor.vertexDescriptor = InputLayout ? InputLayout->GetMTLVertexDescriptor() : nil;

    NSError* Error = nil;
    PipelineState = [GetDevice()->GetMTLDevice() newRenderPipelineStateWithDescriptor:Descriptor error:&Error];
    [Descriptor release];

    if (PipelineState == nil)
    {
        const String ErrorString([Error localizedDescription]);
        METAL_ERROR("Failed to create pipeline state, error %s", *ErrorString);
        return false;
    }

    return true;
}

void FMetalGraphicsPipelineStateRHI::SetDebugName(const String&)
{
}

void FMetalGraphicsPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName.Clear();
}

void* FMetalGraphicsPipelineStateRHI::GetRHINativeState() const
{
    return (__bridge void*)PipelineState;
}

FMetalComputePipelineStateRHI::FMetalComputePipelineStateRHI(FMetalDevice* InDevice)
    : FRHIComputePipelineState()
    , FMetalDeviceChild(InDevice)
    , PipelineState(nil)
    , MaxTotalThreadsPerThreadgroup(0)
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
    PipelineState = [GetDevice()->GetMTLDevice() newComputePipelineStateWithFunction:ComputeShader->GetMTLFunction() error:&Error];
    if (PipelineState == nil)
    {
        const String ErrorString([Error localizedDescription]);
        METAL_ERROR("Failed to create compute pipeline state, error %s", *ErrorString);
        return false;
    }

    MaxTotalThreadsPerThreadgroup = static_cast<uint32>(PipelineState.maxTotalThreadsPerThreadgroup);
    Bindings.Collect(ComputeShader->GetBindings(), EShaderVisibility::Compute);
    return true;
}

void FMetalComputePipelineStateRHI::SetDebugName(const String&)
{
}

void FMetalComputePipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName.Clear();
}

void* FMetalComputePipelineStateRHI::GetRHINativeState() const
{
    return (__bridge void*)PipelineState;
}

FMetalMeshletPipelineStateRHI::FMetalMeshletPipelineStateRHI(FMetalDevice* InDevice, const FRHIMeshletPipelineStateDesc& InDesc)
    : FRHIMeshletPipelineState()
    , FMetalDeviceChild(InDevice)
    , Desc(InDesc)
    , BlendState(nullptr)
    , DepthStencilState(nullptr)
    , RasterizerState(nullptr)
    , PipelineState(nil)
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

    MTLMeshRenderPipelineDescriptor* Descriptor = [MTLMeshRenderPipelineDescriptor new];
    Descriptor.meshFunction = MeshShader->GetMTLFunction();
    Bindings.Collect(MeshShader->GetBindings(), EShaderVisibility::Mesh);

    if (FMetalShader* AmplificationShader = GetMetalShader(Desc.AmplificationShader))
    {
        Descriptor.objectFunction = AmplificationShader->GetMTLFunction();
        Bindings.Collect(AmplificationShader->GetBindings(), EShaderVisibility::Amplification);
    }

    if (FMetalShader* PixelShader = GetMetalShader(Desc.PixelShader))
    {
        Descriptor.fragmentFunction = PixelShader->GetMTLFunction();
        Bindings.Collect(PixelShader->GetBindings(), EShaderVisibility::Pixel);
    }

    if (!ApplyColorAttachments(Descriptor, BlendState.Get(), Desc.RasterizerOutputFormats))
    {
        [Descriptor release];
        return false;
    }

    ApplyDepthStencilFormats(Descriptor, Desc.RasterizerOutputFormats.DepthStencilFormat);
    Descriptor.rasterSampleCount = Math::Max(Desc.MultiSampleState.SampleCount, 1u);

    NSError* Error = nil;
    PipelineState = [GetDevice()->GetMTLDevice() newRenderPipelineStateWithMeshDescriptor:Descriptor error:&Error];
    [Descriptor release];

    if (PipelineState == nil)
    {
        const String ErrorString([Error localizedDescription]);
        METAL_ERROR("Failed to create meshlet pipeline state, error %s", *ErrorString);
        return false;
    }

    return true;
}

void FMetalMeshletPipelineStateRHI::SetDebugName(const String&)
{
}

void FMetalMeshletPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName.Clear();
}

void* FMetalMeshletPipelineStateRHI::GetRHINativeState() const
{
    return (__bridge void*)PipelineState;
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

void FMetalRayTracingPipelineStateRHI::SetDebugName(const String&)
{
}

void FMetalRayTracingPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName.Clear();
}

void* FMetalRayTracingPipelineStateRHI::GetRHINativeState() const
{
    return (__bridge void*)PipelineState;
}
