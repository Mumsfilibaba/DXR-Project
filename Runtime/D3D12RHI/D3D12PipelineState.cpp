#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Threading/AsyncTask.h"
#include "Core/Misc/Paths.h"
#include "D3D12RHI/D3D12PipelineState.h"
#include "D3D12RHI/D3D12Stats.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RHI.h"

static TAutoConsoleVariable<FString> CVarPipelineCacheFileName(
    "D3D12RHI.PipelineCacheFileName",
    "FileName for the file storing the PipelineCache",
    "PipelineCache.d3d12psocache");

static TAutoConsoleVariable<int32> CVarPipelineCacheSaveInterval(
    "D3D12RHI.PipelineCacheSaveInterval",
    "Minimum interval in seconds between automatic pipeline cache saves",
    30);

static EResourceType GetResourceTypeFromBindingType(ED3D12BindingType BindingType)
{
    switch (BindingType)
    {
    case D3D12BindingType_ConstantBuffer: return ResourceType_CBV;
    case D3D12BindingType_SRV:            return ResourceType_SRV;
    case D3D12BindingType_UAV:            return ResourceType_UAV;
    case D3D12BindingType_Sampler:        return ResourceType_Sampler;
    
    default:
        CHECK(false);
        return ResourceType_Unknown;
    }
}

static FD3D12RootSignatureLayout BuildLocalLayoutFromBindingInfo(const FD3D12ShaderBindingInfo& LocalBindingInfo)
{
    FD3D12RootSignatureLayout Layout;
    Layout.SetType(ERootSignatureType::RayTracingLocal);
    Layout.SetAllowInputAssembler(false);

    for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : LocalBindingInfo.ResourceBindings)
    {
        Layout.AddRegister(ShaderVisibility_All, GetResourceTypeFromBindingType(Binding.BindingType), Binding.OriginalBindingIndex);
    }

    Layout.SetNumPushConstants(static_cast<uint8>(LocalBindingInfo.NumPushConstants));
    return Layout;
}

FD3D12InputLayoutRHI::FD3D12InputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements)
    : FRHIInputLayout()
    , InputElements(InInputElements)
    , D3D12Desc()
    , SemanticNames()
    , ElementDesc()
    , Hash(0)
{
    const int32 NumElements = InInputElements.Size();
    ElementDesc.Reserve(NumElements);
    SemanticNames.Reserve(NumElements);

    uint64 CalculatedHash = 0;
    for (const FRHIInputElementDesc& Element : InInputElements)
    {
        D3D12_INPUT_ELEMENT_DESC InputElementDesc;
        FMemory::Memzero(&InputElementDesc, sizeof(D3D12_INPUT_ELEMENT_DESC));

        const FString& Semantic = SemanticNames.Emplace(Element.Semantic);
        InputElementDesc.SemanticName = *Semantic;
        HashCombine(CalculatedHash, GetHashForType(Semantic));

        InputElementDesc.SemanticIndex = Element.SemanticIndex;
        HashCombine(CalculatedHash, InputElementDesc.SemanticIndex);

        InputElementDesc.Format = ConvertFormat(Element.Format);
        HashCombine(CalculatedHash, InputElementDesc.Format);

        InputElementDesc.InputSlot = Element.InputSlot;
        HashCombine(CalculatedHash, InputElementDesc.InputSlot);

        InputElementDesc.AlignedByteOffset = Element.ByteOffset;
        HashCombine(CalculatedHash, InputElementDesc.AlignedByteOffset);

        InputElementDesc.InputSlotClass = ConvertVertexInputClass(Element.InputClass);
        HashCombine(CalculatedHash, InputElementDesc.InputSlotClass);

        InputElementDesc.InstanceDataStepRate = Element.InputClass == EVertexInputClass::Vertex ? 0 : Element.InstanceStepRate;
        HashCombine(CalculatedHash, InputElementDesc.InstanceDataStepRate);

        ElementDesc.Emplace(InputElementDesc);
    }

    D3D12Desc.NumElements        = ElementDesc.Size();
    D3D12Desc.pInputElementDescs = ElementDesc.Data();

    Hash = CalculatedHash;
}

FD3D12InputLayoutRHI::~FD3D12InputLayoutRHI()
{
}

const FRHIInputElementDesc* FD3D12InputLayoutRHI::GetInputElementDesc(uint32 Index) const
{
    return &InputElements[Index];
}

uint32 FD3D12InputLayoutRHI::GetNumInputElementDescs() const
{
    return InputElements.Size();
}

FD3D12DepthStencilStateRHI::FD3D12DepthStencilStateRHI(const FRHIDepthStencilStateDesc& InDesc)
    : FRHIDepthStencilState()
    , Desc(InDesc)
    , Hash(0)
{
    FMemory::Memzero(&D3D12Desc);

    D3D12Desc.DepthFunc        = ConvertComparisonFunc(InDesc.DepthFunc);
    D3D12Desc.DepthEnable      = InDesc.bDepthEnable;
    D3D12Desc.DepthWriteMask   = InDesc.bDepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    D3D12Desc.StencilEnable    = InDesc.bStencilEnable;
    D3D12Desc.StencilReadMask  = static_cast<uint8>(InDesc.StencilReadMask);
    D3D12Desc.StencilWriteMask = static_cast<uint8>(InDesc.StencilWriteMask);
    D3D12Desc.FrontFace        = ConvertStencilState(InDesc.FrontFace);
    D3D12Desc.BackFace         = ConvertStencilState(InDesc.BackFace);

    Hash = CRC32::Generate(&D3D12Desc, sizeof(D3D12_DEPTH_STENCIL_DESC));
}

FD3D12DepthStencilStateRHI::~FD3D12DepthStencilStateRHI()
{
}

FRHIDepthStencilStateDesc FD3D12DepthStencilStateRHI::GetDesc() const
{
    return Desc;
}

FD3D12RasterizerStateRHI::FD3D12RasterizerStateRHI(const FRHIRasterizerStateDesc& InDesc)
    : FRHIRasterizerState()
    , Desc(InDesc)
    , Hash(0)
{
    FMemory::Memzero(&D3D12Desc);

    D3D12Desc.AntialiasedLineEnable = InDesc.bAntialiasedLineEnable;
    D3D12Desc.CullMode              = ConvertCullMode(InDesc.CullMode);
    D3D12Desc.DepthBias             = static_cast<int32>(InDesc.DepthBias);
    D3D12Desc.DepthBiasClamp        = InDesc.DepthBiasClamp;
    D3D12Desc.DepthClipEnable       = InDesc.bDepthClipEnable;
    D3D12Desc.SlopeScaledDepthBias  = InDesc.SlopeScaledDepthBias;
    D3D12Desc.FillMode              = ConvertFillMode(InDesc.FillMode);
    D3D12Desc.ForcedSampleCount     = InDesc.ForcedSampleCount;
    D3D12Desc.FrontCounterClockwise = InDesc.bFrontCounterClockwise;
    D3D12Desc.MultisampleEnable     = InDesc.bMultisampleEnable;
    D3D12Desc.ConservativeRaster    = InDesc.bEnableConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    Hash = CRC32::Generate(&D3D12Desc, sizeof(D3D12_RASTERIZER_DESC));
}

FD3D12RasterizerStateRHI::~FD3D12RasterizerStateRHI()
{
}

FRHIRasterizerStateDesc FD3D12RasterizerStateRHI::GetDesc() const
{
    return Desc;
}

FD3D12BlendStateRHI::FD3D12BlendStateRHI(const FRHIBlendStateDesc& InDesc)
    : FRHIBlendState()
    , Desc(InDesc)
    , Hash(0)
{
    FMemory::Memzero(&D3D12Desc);

    D3D12Desc.AlphaToCoverageEnable   = InDesc.bAlphaToCoverageEnable;
    D3D12Desc.IndependentBlendEnable  = InDesc.bIndependentBlendEnable;

    const D3D12_LOGIC_OP LogicOp = ConvertLogicOp(InDesc.LogicOp);
    for (int32 Index = 0; Index < InDesc.NumRenderTargets; Index++)
    {
        D3D12Desc.RenderTarget[Index].BlendEnable           = InDesc.RenderTargets[Index].bBlendEnable;
        D3D12Desc.RenderTarget[Index].BlendOp               = ConvertBlendOp(InDesc.RenderTargets[Index].BlendOp);
        D3D12Desc.RenderTarget[Index].BlendOpAlpha          = ConvertBlendOp(InDesc.RenderTargets[Index].BlendOpAlpha);
        D3D12Desc.RenderTarget[Index].DestBlend             = ConvertBlend(InDesc.RenderTargets[Index].DstBlend);
        D3D12Desc.RenderTarget[Index].DestBlendAlpha        = ConvertBlend(InDesc.RenderTargets[Index].DstBlendAlpha);
        D3D12Desc.RenderTarget[Index].SrcBlend              = ConvertBlend(InDesc.RenderTargets[Index].SrcBlend);
        D3D12Desc.RenderTarget[Index].SrcBlendAlpha         = ConvertBlend(InDesc.RenderTargets[Index].SrcBlendAlpha);
        D3D12Desc.RenderTarget[Index].RenderTargetWriteMask = ConvertColorWriteFlags(InDesc.RenderTargets[Index].ColorWriteMask);
        D3D12Desc.RenderTarget[Index].LogicOp               = LogicOp;
        D3D12Desc.RenderTarget[Index].LogicOpEnable         = InDesc.bLogicOpEnable;
    }

    Hash = CRC32::Generate(&D3D12Desc, sizeof(D3D12_BLEND_DESC));
}

FD3D12BlendStateRHI::~FD3D12BlendStateRHI()
{
}

FRHIBlendStateDesc FD3D12BlendStateRHI::GetDesc() const
{
    return Desc;
}

FD3D12PipelineState::FD3D12PipelineState(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
{
    FMemory::Memzero(EffectiveDescriptorCounts, sizeof(EffectiveDescriptorCounts));
}

FD3D12PipelineState::~FD3D12PipelineState()
{
}

void FD3D12PipelineState::ComputeEffectiveDescriptorCounts(FD3D12Shader* const* Shaders, uint32 NumShaders)
{
    FMemory::Memzero(EffectiveDescriptorCounts, sizeof(EffectiveDescriptorCounts));

    for (uint32 i = 0; i < NumShaders; i++)
    {
        FD3D12Shader* Shader = Shaders[i];
        if (!Shader)
        {
            continue;
        }

        const EShaderVisibility        Stage       = Shader->GetShaderVisibility();
        const FD3D12ShaderBindingInfo& BindingInfo = Shader->GetBindingInfo();

        for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : BindingInfo.ResourceBindings)
        {
            const uint16        Register     = Binding.OriginalBindingIndex;
            const EResourceType ResourceType = static_cast<EResourceType>(Binding.BindingType);
            
            if (ResourceType == ResourceType_CBV && RootSignature->IsRootCBV(Stage, Register))
            {
                continue;
            }

            const int8 Slot = RootSignature->GetSlotForRegister(Stage, ResourceType, Register);
            if (Slot >= 0)
            {
                EffectiveDescriptorCounts[Stage][ResourceType] = Math::Max<uint8>(EffectiveDescriptorCounts[Stage][ResourceType], static_cast<uint8>(Slot) + 1);
            }
        }
    }
}

void FD3D12PipelineState::SetDebugName(const FString& InName)
{
    const FStringWide WideName = CharToWide(InName);
    PipelineState->SetName(*WideName);
    DebugName = InName;
}

FD3D12GraphicsPipelineStateRHI::FD3D12GraphicsPipelineStateRHI(FD3D12Device* InDevice)
    : FRHIGraphicsPipelineState()
    , FD3D12PipelineState(InDevice)
{
}

FD3D12GraphicsPipelineStateRHI::~FD3D12GraphicsPipelineStateRHI()
{
}

void FD3D12GraphicsPipelineStateRHI::SetDebugName(const FString& InName)
{
    FD3D12PipelineState::SetDebugName(InName);
}

void FD3D12GraphicsPipelineStateRHI::GetDebugName(FString& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FD3D12GraphicsPipelineStateRHI::GetRHINativeHandle() const
{
    return reinterpret_cast<void*>(GetD3D12PipelineState());
}

bool FD3D12GraphicsPipelineStateRHI::Initialize(const FRHIGraphicsPipelineStateDesc& Desc)
{
    D3D12_INPUT_LAYOUT_DESC            InputLayoutDesc          = {};
    D3D12_SHADER_BYTECODE              VertexShaderCode         = {};
    D3D12_SHADER_BYTECODE              HullShaderCode           = {};
    D3D12_SHADER_BYTECODE              DomainShaderCode         = {};
    D3D12_SHADER_BYTECODE              GeometryShaderCode       = {};
    D3D12_SHADER_BYTECODE              PixelShaderCode          = {};
    D3D12_RT_FORMAT_ARRAY              RenderTargetInfo         = {};
    DXGI_FORMAT                        DepthBufferFormat        = {};
    D3D12_RASTERIZER_DESC              RasterizerDesc           = {};
    D3D12_DEPTH_STENCIL_DESC           DepthStencilDesc         = {};
    D3D12_BLEND_DESC                   BlendStateDesc           = {};
    D3D12_PRIMITIVE_TOPOLOGY_TYPE      PrimitiveTopologyType    = {};
    D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IndexBufferStripCutValue = {};
    DXGI_SAMPLE_DESC                   SampleDesc               = {};
    D3D12_STREAM_OUTPUT_DESC           StreamOutputDesc         = {};
    D3D12_PIPELINE_STATE_FLAGS         PipelineStateFlags       = D3D12_PIPELINE_STATE_FLAG_NONE;

    // InputLayout
    FD3D12InputLayoutRHI* D3D12InputLayout = FD3D12RHI::ResourceCast(Desc.InputLayout);
    if (D3D12InputLayout)
    {
        InputLayoutDesc = D3D12InputLayout->GetDesc();
    }

    // ShaderStages
    TArray<FD3D12Shader*> ShadersWithRootSignature;
    TArray<FD3D12Shader*> BaseShaders;

    // VertexShader
    {
        if (FD3D12VertexShaderRHI* D3D12VertexShader = FD3D12RHI::ResourceCast(Desc.VertexShader))
        {
            if (D3D12VertexShader->HasRootSignature())
            {
                ShadersWithRootSignature.Emplace(D3D12VertexShader);
            }

            VertexShaderCode = D3D12VertexShader->GetByteCode().GetD3D12Bytecode();
            BaseShaders.Emplace(D3D12VertexShader);

            VertexShader = MakeSharedRef<FD3D12VertexShaderRHI>(D3D12VertexShader);
        }
        else
        {
            D3D12_ERROR_CRITICAL("VertexShader cannot be nullptr");
            return false;
        }
    }

    // HullShader
    {
        if (FD3D12HullShaderRHI* D3D12HullShader = FD3D12RHI::ResourceCast(Desc.HullShader))
        {
            if (D3D12HullShader->HasRootSignature())
            {
                ShadersWithRootSignature.Emplace(D3D12HullShader);
            }

            HullShaderCode = D3D12HullShader->GetByteCode().GetD3D12Bytecode();
            BaseShaders.Emplace(D3D12HullShader);

            HullShader = MakeSharedRef<FD3D12HullShaderRHI>(D3D12HullShader);
        }
        else
        {
            HullShaderCode.pShaderBytecode = nullptr;
            HullShaderCode.BytecodeLength  = 0;
        }
    }

    // DomainShader
    {
        if (FD3D12DomainShaderRHI* D3D12DomainShader = FD3D12RHI::ResourceCast(Desc.DomainShader))
        {
            if (D3D12DomainShader->HasRootSignature())
            {
                ShadersWithRootSignature.Emplace(D3D12DomainShader);
            }

            DomainShaderCode = D3D12DomainShader->GetByteCode().GetD3D12Bytecode();
            BaseShaders.Emplace(D3D12DomainShader);

            DomainShader = MakeSharedRef<FD3D12DomainShaderRHI>(D3D12DomainShader);
        }
        else
        {
            DomainShaderCode.pShaderBytecode = nullptr;
            DomainShaderCode.BytecodeLength  = 0;
        }
    }

    // GeometryShader
    {
        if (FD3D12GeometryShaderRHI* D3D12GeometryShader = FD3D12RHI::ResourceCast(Desc.GeometryShader))
        {
            if (D3D12GeometryShader->HasRootSignature())
            {
                ShadersWithRootSignature.Emplace(D3D12GeometryShader);
            }

            GeometryShaderCode = D3D12GeometryShader->GetByteCode().GetD3D12Bytecode();
            BaseShaders.Emplace(D3D12GeometryShader);

            GeometryShader = MakeSharedRef<FD3D12GeometryShaderRHI>(D3D12GeometryShader);
        }
        else
        {
            GeometryShaderCode.pShaderBytecode = nullptr;
            GeometryShaderCode.BytecodeLength  = 0;
        }
    }

    // PixelShader
    {
        if (FD3D12PixelShaderRHI* D3D12PixelShader = FD3D12RHI::ResourceCast(Desc.PixelShader))
        {
            if (D3D12PixelShader->HasRootSignature())
            {
                ShadersWithRootSignature.Emplace(D3D12PixelShader);
            }

            PixelShaderCode = D3D12PixelShader->GetByteCode().GetD3D12Bytecode();
            BaseShaders.Emplace(D3D12PixelShader);

            PixelShader = MakeSharedRef<FD3D12PixelShaderRHI>(D3D12PixelShader);
        }
        else
        {
            PixelShaderCode.pShaderBytecode = nullptr;
            PixelShaderCode.BytecodeLength  = 0;
        }
    }

    // RenderTarget
    {
        RenderTargetInfo.NumRenderTargets = Desc.RasterizerOutputFormats.NumRenderTargets;

        for (uint32 Index = 0; Index < RenderTargetInfo.NumRenderTargets; Index++)
        {
            RenderTargetInfo.RTFormats[Index] = ConvertFormat(Desc.RasterizerOutputFormats.RenderTargetFormats[Index]);
        }

        // DepthStencil
        DepthBufferFormat = ConvertFormat(Desc.RasterizerOutputFormats.DepthStencilFormat);
    }

    // RasterizerState
    FD3D12RasterizerStateRHI* D3D12RasterizerState = FD3D12RHI::ResourceCast(Desc.RasterizerState);
    if (D3D12RasterizerState)
    {
        RasterizerDesc = D3D12RasterizerState->GetD3D12Desc();
    }
    else
    {
        D3D12_ERROR_CRITICAL("RasterizerState cannot be nullptr");
        return false;
    }

    // DepthStencilState
    FD3D12DepthStencilStateRHI* D3D12DepthStencilState = FD3D12RHI::ResourceCast(Desc.DepthStencilState);
    if (D3D12DepthStencilState)
    {
        DepthStencilDesc = D3D12DepthStencilState->GetD3D12Desc();
    }
    else
    {
        D3D12_ERROR_CRITICAL("DepthStencilState cannot be nullptr");
        return false;
    }

    // BlendState
    FD3D12BlendStateRHI* D3D12BlendState = FD3D12RHI::ResourceCast(Desc.BlendState);
    if (D3D12BlendState)
    {
        BlendStateDesc = D3D12BlendState->GetD3D12Desc();
    }
    else
    {
        D3D12_ERROR_CRITICAL("BlendState cannot be nullptr");
        return false;
    }

    // Topology
    {
        PrimitiveTopologyType = ConvertPrimitiveTopologyType(Desc.PrimitiveTopology);
        PrimitiveTopology     = ConvertPrimitiveTopology(Desc.PrimitiveTopology);
    }

    // IndexBufferStripCutValue
    {
        IndexBufferStripCutValue = Desc.bPrimitiveRestartEnable ? 
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF : 
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    }

    // MSAA
    {
        SampleDesc.Count   = Desc.MultiSampleState.SampleCount;
        SampleDesc.Quality = Desc.MultiSampleState.SampleQuality;
    }

    // RootSignature
    {
        if (ShadersWithRootSignature.IsEmpty())
        {
            FD3D12RootSignatureLayout RootSignatureLayout;
            RootSignatureLayout.SetType(ERootSignatureType::Graphics);
            RootSignatureLayout.SetAllowInputAssembler(D3D12InputLayout ? true : false);

            uint8 NumPushConstants = 0;
            for (FD3D12Shader* Shader : BaseShaders)
            {
                const EShaderVisibility        Stage       = Shader->GetShaderVisibility();
                const FD3D12ShaderBindingInfo& BindingInfo = Shader->GetBindingInfo();

                for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : BindingInfo.ResourceBindings)
                {
                    RootSignatureLayout.AddRegister(Stage, static_cast<EResourceType>(Binding.BindingType), Binding.OriginalBindingIndex);
                }
                
                NumPushConstants = Math::Max<uint8>(NumPushConstants, static_cast<uint8>(BindingInfo.NumPushConstants));
            }

            RootSignatureLayout.SetNumPushConstants(NumPushConstants);

            if (Desc.StreamOutputDeclaration)
            {
                RootSignatureLayout.SetAllowStreamOutput(true);
            }

            for (const FRHIStaticSamplerInfo& StaticSampler : Desc.StaticSamplers)
            {
                RootSignatureLayout.AddStaticSampler(StaticSampler);
            }

            RootSignatureLayout.ComputeRootCBVs();

            FD3D12RootSignatureManager& RootSignatureManager = GetDevice()->GetRootSignatureManager();
            RootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(RootSignatureLayout));
        }
        else
        {
            const FD3D12ShaderBytecode& ByteCode = ShadersWithRootSignature.FirstElement()->GetByteCode();

            RootSignature = new FD3D12RootSignature(GetDevice());
            if (!RootSignature->Initialize(ByteCode.GetCode(), ByteCode.GetCodeSize()))
            {
                return false;
            }
            else
            {
                RootSignature->SetDebugName("Custom Graphics RootSignature");
            }

            // Validate that all shaders with embedded root signatures are compatible
            for (int32 i = 1; i < ShadersWithRootSignature.Size(); i++)
            {
                FD3D12RootSignature ValidationRS(GetDevice());
                const FD3D12ShaderBytecode& OtherByteCode = ShadersWithRootSignature[i]->GetByteCode();
                if (ValidationRS.Initialize(OtherByteCode.GetCode(), OtherByteCode.GetCodeSize()))
                {
                    if (ValidationRS.GetHash() != RootSignature->GetHash())
                    {
                        D3D12_ERROR_CRITICAL("Shader at index %d has an incompatible embedded root signature (hash mismatch: 0x%llx vs 0x%llx)",
                            i, ValidationRS.GetHash(), RootSignature->GetHash());
                    }
                }
            }
        }

        CHECK(RootSignature != nullptr);

        for (FD3D12Shader* Shader : BaseShaders)
        {
            const EShaderVisibility        Stage       = Shader->GetShaderVisibility();
            const FD3D12ShaderBindingInfo& BindingInfo = Shader->GetBindingInfo();

            const bool bShaderHasRootBindings = !BindingInfo.ResourceBindings.IsEmpty() || BindingInfo.NumPushConstants > 0;
            if (bShaderHasRootBindings && RootSignature->HasDenyFlag(Stage))
            {
                D3D12_ERROR_CRITICAL("Root Signature denies access to stage %u, but shader has %d resource bindings and %u push constants.",
                    Stage, BindingInfo.ResourceBindings.Size(), BindingInfo.NumPushConstants);
            }

            for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : BindingInfo.ResourceBindings)
            {
                const EResourceType ResType = static_cast<EResourceType>(Binding.BindingType);
                if (RootSignature->GetSlotForRegister(Stage, ResType, Binding.OriginalBindingIndex) < 0 &&
                    RootSignature->GetShaderStage(Stage).GetRootDescriptorParameterIndex(ResType, Binding.OriginalBindingIndex) < 0)
                {
                    D3D12_ERROR_CRITICAL("Custom root signature missing register %u (type %u) for shader stage %u", 
                        Binding.OriginalBindingIndex, Binding.BindingType, Stage);
                }
            }
        }

        ComputeEffectiveDescriptorCounts(BaseShaders.Data(), BaseShaders.Size());
    }

    // View Instancing
    FD3D12HashableViewInstanceDesc ViewInstanceDesc;
    if (Desc.ViewInstancingState.bEnableViewInstancing)
    {
        ViewInstanceDesc.ViewInstanceCount = Math::Min<uint32>(Desc.ViewInstancingState.NumArraySlices, D3D12_MAX_VIEW_INSTANCE_COUNT);
        for (uint32 Index = 0; Index < ViewInstanceDesc.ViewInstanceCount; Index++)
        {
            ViewInstanceDesc.ViewInstanceLocations[Index].RenderTargetArrayIndex = Desc.ViewInstancingState.StartRenderTargetArrayIndex;
            ViewInstanceDesc.ViewInstanceLocations[Index].ViewportArrayIndex     = 0;
        }
    }

    // Stream Output
    TArray<UINT>                       StreamOutputStrides;
    TArray<D3D12_SO_DECLARATION_ENTRY> StreamOutputEntries;

    if (Desc.StreamOutputDeclaration)
    {
        const FRHIStreamOutputDeclaration& StreamOutputDecl = *Desc.StreamOutputDeclaration;
        for (const FRHIStreamOutputEntry& Entry : StreamOutputDecl.Entries)
        {
            D3D12_SO_DECLARATION_ENTRY D3DEntry = {};
            D3DEntry.SemanticName   = Entry.SemanticName;
            D3DEntry.SemanticIndex  = Entry.SemanticIndex;
            D3DEntry.StartComponent = Entry.StartComponent;
            D3DEntry.ComponentCount = Entry.ComponentCount;
            D3DEntry.OutputSlot     = Entry.OutputSlot;
            D3DEntry.Stream         = 0;

            StreamOutputEntries.Emplace(D3DEntry);
        }
        for (uint32 Stride : StreamOutputDecl.BufferStrides)
        {
            StreamOutputStrides.Emplace(Stride);
        }

        StreamOutputDesc.pSODeclaration    = StreamOutputEntries.Data();
        StreamOutputDesc.NumEntries        = StreamOutputEntries.Size();
        StreamOutputDesc.pBufferStrides    = StreamOutputStrides.Data();
        StreamOutputDesc.NumStrides        = StreamOutputStrides.Size();
        StreamOutputDesc.RasterizedStream  = StreamOutputDecl.RasterizedStream;
    }

    // Dynamic Depth Bias
#if D3D12_ENABLE_DYNAMIC_DEPTH_BIAS
    if (GD3D12SupportDynamicDepthBias && D3D12RasterizerState->GetDesc().bEnableDepthBias)
    {
        PipelineStateFlags = D3D12_PIPELINE_STATE_FLAG_DYNAMIC_DEPTH_BIAS;
    }
#endif

    // Build pipeline key for caching (shared by both paths)
    FD3D12GraphicsPipelineKey PipelineKey;
    FMemory::Memzero(&PipelineKey, sizeof(FD3D12GraphicsPipelineKey));

    PipelineKey.RootSignatureHash        = RootSignature->GetHash();
    PipelineKey.PrimitiveTopologyType    = PrimitiveTopologyType;
    PipelineKey.IndexBufferStripCutValue = IndexBufferStripCutValue;
    PipelineKey.DepthBufferFormat        = DepthBufferFormat;
    PipelineKey.RenderTargetInfo         = RenderTargetInfo;
    PipelineKey.ViewInstancingHash       = ViewInstanceDesc.GenerateHash();
    PipelineKey.InputLayoutHash          = D3D12InputLayout ? D3D12InputLayout->GetHash() : 0;
    PipelineKey.RasterizerHash           = D3D12RasterizerState->GetHash();
    PipelineKey.DepthStencilHash         = D3D12DepthStencilState->GetHash();
    PipelineKey.BlendStateHash           = D3D12BlendState->GetHash();
    PipelineKey.SampleDesc               = SampleDesc;

    PipelineKey.VSHash = VertexShader->GetHash();
    PipelineKey.HSHash = HullShader     ? HullShader->GetHash()     : FD3D12ShaderHash();
    PipelineKey.DSHash = DomainShader   ? DomainShader->GetHash()   : FD3D12ShaderHash();
    PipelineKey.GSHash = GeometryShader ? GeometryShader->GetHash() : FD3D12ShaderHash();
    PipelineKey.PSHash = PixelShader    ? PixelShader->GetHash()    : FD3D12ShaderHash();

    const uint64 PipelineHash = CRC32::Generate(&PipelineKey, sizeof(FD3D12GraphicsPipelineKey));
    constexpr uint64 BufferLength = 128;
    WIDECHAR PipelineHashBuffer[BufferLength] = { 0 };
    FPlatformString::Snprintf(PipelineHashBuffer, BufferLength, L"GraphicsPSO[%llu]", PipelineHash);

    // Create pipeline-state via stream path (ID3D12Device2) or legacy path (ID3D12Device)
#if D3D12_ENABLE_PIPELINE_STATE_STREAM
    if (GD3D12SupportPipelineStream)
    {
        FD3D12GraphicsPipelineStream PipelineStream;
        PipelineStream.RootSignature            = RootSignature->GetD3D12RootSignature();
        PipelineStream.InputLayout              = InputLayoutDesc;
        PipelineStream.PrimitiveTopologyType    = PrimitiveTopologyType;
        PipelineStream.VertexShaderCode         = VertexShaderCode;
        PipelineStream.HullShaderCode           = HullShaderCode;
        PipelineStream.DomainShaderCode         = DomainShaderCode;
        PipelineStream.GeometryShaderCode       = GeometryShaderCode;
        PipelineStream.PixelShaderCode          = PixelShaderCode;
        PipelineStream.RenderTargetInfo         = RenderTargetInfo;
        PipelineStream.DepthBufferFormat        = DepthBufferFormat;
        PipelineStream.RasterizerDesc           = RasterizerDesc;
        PipelineStream.DepthStencilDesc         = DepthStencilDesc;
        PipelineStream.BlendStateDesc           = BlendStateDesc;
        PipelineStream.SampleDesc               = SampleDesc;
        PipelineStream.IndexBufferStripCutValue = IndexBufferStripCutValue;
        PipelineStream.StreamOutputDesc         = StreamOutputDesc;
        PipelineStream.PipelineStateFlags       = PipelineStateFlags;
        PipelineStream.ViewInstancingDesc.Flags = ViewInstanceDesc.Flags;

        if (Desc.ViewInstancingState.bEnableViewInstancing)
        {
            PipelineStream.ViewInstancingDesc.pViewInstanceLocations = ViewInstanceDesc.ViewInstanceLocations;
            PipelineStream.ViewInstancingDesc.ViewInstanceCount      = ViewInstanceDesc.ViewInstanceCount;
        }

        D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
        FMemory::Memzero(&PipelineStreamDesc);

        PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
        PipelineStreamDesc.SizeInBytes                   = sizeof(FD3D12GraphicsPipelineStream);

        if (GD3D12SupportPipelineCache)
        {
            FD3D12PipelineStateManager& PipelineStateManager = GetDevice()->GetPipelineStateManager();
            if (PipelineStateManager.CreateGraphicsPipeline(PipelineHashBuffer, PipelineStreamDesc, PipelineState))
            {
                return true;
            }
        }

    #if D3D12_USE_ID3D12DEVICE_2
        TComPtr<ID3D12PipelineState> NewPipelineState;
        HRESULT Result = GetDevice()->GetD3D12Device2()->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&NewPipelineState));
        if (FAILED(Result))
        {
            D3D12_ERROR_CRITICAL("[D3D12GraphicsPipelineState]: FAILED to Create GraphicsPipelineState");
            return false;
        }

        PipelineState = NewPipelineState;
        return true;
    #else
        D3D12_ERROR_CRITICAL("[D3D12GraphicsPipelineState]: ID3D12Device2 is required for pipeline stream creation");
        return false;
    #endif
    }
    else
#endif
    {
        if (Desc.ViewInstancingState.bEnableViewInstancing)
        {
            D3D12_WARNING("[D3D12GraphicsPipelineState]: View instancing requested but pipeline stream is not available, feature will be disabled");
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC LegacyDesc;
        FMemory::Memzero(&LegacyDesc);

        LegacyDesc.pRootSignature        = RootSignature->GetD3D12RootSignature();
        LegacyDesc.VS                    = VertexShaderCode;
        LegacyDesc.HS                    = HullShaderCode;
        LegacyDesc.DS                    = DomainShaderCode;
        LegacyDesc.GS                    = GeometryShaderCode;
        LegacyDesc.PS                    = PixelShaderCode;
        LegacyDesc.BlendState            = BlendStateDesc;
        LegacyDesc.SampleMask            = UINT_MAX;
        LegacyDesc.RasterizerState       = RasterizerDesc;
        LegacyDesc.DepthStencilState     = DepthStencilDesc;
        LegacyDesc.InputLayout           = InputLayoutDesc;
        LegacyDesc.IBStripCutValue       = IndexBufferStripCutValue;
        LegacyDesc.PrimitiveTopologyType = PrimitiveTopologyType;
        LegacyDesc.DSVFormat             = DepthBufferFormat;
        LegacyDesc.SampleDesc            = SampleDesc;
        LegacyDesc.NodeMask              = GetDevice()->GetNodeMask();

        LegacyDesc.NumRenderTargets = RenderTargetInfo.NumRenderTargets;
        for (uint32 Index = 0; Index < LegacyDesc.NumRenderTargets; Index++)
        {
            LegacyDesc.RTVFormats[Index] = RenderTargetInfo.RTFormats[Index];
        }

        LegacyDesc.StreamOutput = StreamOutputDesc;
        LegacyDesc.Flags        = PipelineStateFlags;

        if (GD3D12SupportPipelineCache)
        {
            FD3D12PipelineStateManager& PipelineStateManager = GetDevice()->GetPipelineStateManager();
            if (PipelineStateManager.CreateGraphicsPipeline(PipelineHashBuffer, LegacyDesc, PipelineState))
            {
                return true;
            }
        }

        HRESULT Result = GetDevice()->GetD3D12Device()->CreateGraphicsPipelineState(&LegacyDesc, IID_PPV_ARGS(&PipelineState));
        if (FAILED(Result))
        {
            D3D12_ERROR_CRITICAL("[D3D12GraphicsPipelineState]: FAILED to Create GraphicsPipelineState (legacy)");
            return false;
        }

        return true;
    }
}

FD3D12ComputePipelineStateRHI::FD3D12ComputePipelineStateRHI(FD3D12Device* InDevice, const TSharedRef<FD3D12ComputeShaderRHI>& InShader)
    : FRHIComputePipelineState()
    , FD3D12PipelineState(InDevice)
    , Shader(InShader)
{
}

FD3D12ComputePipelineStateRHI::~FD3D12ComputePipelineStateRHI()
{
}

void FD3D12ComputePipelineStateRHI::SetDebugName(const FString& InName)
{
    FD3D12PipelineState::SetDebugName(InName);
}

void FD3D12ComputePipelineStateRHI::GetDebugName(FString& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FD3D12ComputePipelineStateRHI::GetRHINativeHandle() const
{
    return reinterpret_cast<void*>(GetD3D12PipelineState());
}

bool FD3D12ComputePipelineStateRHI::Initialize(const FRHIComputePipelineStateDesc& Desc)
{
    D3D12_SHADER_BYTECODE ComputeShaderCode = Shader->GetByteCode().GetD3D12Bytecode();

    if (!Shader->HasRootSignature())
    {
        FD3D12RootSignatureLayout Layout;
        Layout.SetType(ERootSignatureType::Compute);
        Layout.SetAllowInputAssembler(false);

        const FD3D12ShaderBindingInfo& BindingInfo = Shader->GetBindingInfo();
        for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : BindingInfo.ResourceBindings)
        {
            Layout.AddRegister(ShaderVisibility_All, static_cast<EResourceType>(Binding.BindingType), Binding.OriginalBindingIndex);
        }

        for (const FRHIStaticSamplerInfo& StaticSampler : Desc.StaticSamplers)
        {
            Layout.AddStaticSampler(StaticSampler);
        }

        Layout.SetNumPushConstants(static_cast<uint8>(BindingInfo.NumPushConstants));
        Layout.ComputeRootCBVs();

        FD3D12RootSignatureManager& RootSignatureManager = GetDevice()->GetRootSignatureManager();
        RootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(Layout));
    }
    else
    {
        const FD3D12ShaderBytecode& ByteCode = Shader->GetByteCode();

        RootSignature = new FD3D12RootSignature(GetDevice());
        if (!RootSignature->Initialize(ByteCode.GetCode(), ByteCode.GetCodeSize()))
        {
            return false;
        }
        else
        {
            RootSignature->SetDebugName("Custom Compute RootSignature");
        }

        const FD3D12ShaderBindingInfo& BindingInfo = Shader->GetBindingInfo();
        for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : BindingInfo.ResourceBindings)
        {
            const EResourceType ResType = static_cast<EResourceType>(Binding.BindingType);
            if (RootSignature->GetSlotForRegister(ShaderVisibility_All, ResType, Binding.OriginalBindingIndex) < 0 &&
                RootSignature->GetShaderStage(ShaderVisibility_All).GetRootDescriptorParameterIndex(ResType, Binding.OriginalBindingIndex) < 0)
            {
                D3D12_ERROR_CRITICAL("Custom compute root signature missing register %u (type %u)",
                    Binding.OriginalBindingIndex, Binding.BindingType);
            }
        }
    }

    CHECK(RootSignature != nullptr);

    {
        FD3D12Shader* ComputeShaders[] = { Shader.Get() };
        ComputeEffectiveDescriptorCounts(ComputeShaders, 1);
    }

    // Build pipeline key for caching (shared by both paths)
    FD3D12ComputePipelineKey PipelineKey;
    FMemory::Memzero(&PipelineKey, sizeof(FD3D12ComputePipelineKey));

    PipelineKey.CSHash            = Shader->GetHash();
    PipelineKey.RootSignatureHash = RootSignature->GetHash();

    const uint64 PipelineHash = CRC32::Generate(&PipelineKey, sizeof(FD3D12ComputePipelineKey));
    constexpr uint64 BufferLength = 64;
    WIDECHAR PipelineHashBuffer[BufferLength] = { 0 };
    FPlatformString::Snprintf(PipelineHashBuffer, BufferLength, L"ComputePSO[%llu]", PipelineHash);

    // Create PipelineState via stream path (ID3D12Device2) or legacy path (ID3D12Device)
#if D3D12_ENABLE_PIPELINE_STATE_STREAM
    if (GD3D12SupportPipelineStream)
    {
        FD3D12ComputePipelineStream PipelineStream;
        PipelineStream.RootSignature = RootSignature->GetD3D12RootSignature();
        PipelineStream.ComputeShader = ComputeShaderCode;

        D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
        FMemory::Memzero(&PipelineStreamDesc);

        PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
        PipelineStreamDesc.SizeInBytes                   = sizeof(FD3D12ComputePipelineStream);

        if (GD3D12SupportPipelineCache)
        {
            FD3D12PipelineStateManager& PipelineStateManager = GetDevice()->GetPipelineStateManager();
            if (PipelineStateManager.CreateComputePipeline(PipelineHashBuffer, PipelineStreamDesc, PipelineState))
            {
                return true;
            }
        }

    #if D3D12_USE_ID3D12DEVICE_2
        HRESULT Result = GetDevice()->GetD3D12Device2()->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&PipelineState));
        if (FAILED(Result))
        {
            D3D12_ERROR_CRITICAL("[D3D12ComputePipelineState]: FAILED to Create ComputePipelineState");
            return false;
        }

        return true;
    #else
        D3D12_ERROR_CRITICAL("[D3D12ComputePipelineState]: ID3D12Device2 is required for pipeline stream creation");
        return false;
    #endif
    }
    else
#endif
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC LegacyDesc;
        FMemory::Memzero(&LegacyDesc);

        LegacyDesc.pRootSignature = RootSignature->GetD3D12RootSignature();
        LegacyDesc.CS             = ComputeShaderCode;
        LegacyDesc.NodeMask       = GetDevice()->GetNodeMask();

        if (GD3D12SupportPipelineCache)
        {
            FD3D12PipelineStateManager& PipelineStateManager = GetDevice()->GetPipelineStateManager();
            if (PipelineStateManager.CreateComputePipeline(PipelineHashBuffer, LegacyDesc, PipelineState))
            {
                return true;
            }
        }

        HRESULT Result = GetDevice()->GetD3D12Device()->CreateComputePipelineState(&LegacyDesc, IID_PPV_ARGS(&PipelineState));
        if (FAILED(Result))
        {
            D3D12_ERROR_CRITICAL("[D3D12ComputePipelineState]: FAILED to Create ComputePipelineState (legacy)");
            return false;
        }

        return true;
    }
}

struct FD3D12RootSignatureAssociation
{
    FD3D12RootSignatureAssociation(ID3D12RootSignature* InRootSignature, const TArray<FStringWide>& InShaderExportNames)
        : ExportAssociation()
        , RootSignature(InRootSignature)
        , ShaderExportNames(InShaderExportNames)
        , ShaderExportNamesRef(InShaderExportNames.Size())
    {
        for (int32 i = 0; i < ShaderExportNames.Size(); i++)
        {
            ShaderExportNamesRef[i] = *ShaderExportNames[i];
        }
    }

    ID3D12RootSignature*                   RootSignature;
    TArray<FStringWide>                    ShaderExportNames;
    TArray<LPCWSTR>                        ShaderExportNamesRef;
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ExportAssociation;
};

struct FD3D12HitGroup
{
    FD3D12HitGroup(const FStringWide& InHitGroupName, const FStringWide& InClosestHit, const FStringWide& InAnyHit, const FStringWide& InIntersection)
        : Desc()
        , HitGroupName(InHitGroupName)
        , ClosestHit(InClosestHit)
        , AnyHit(InAnyHit)
        , Intersection(InIntersection)
    {
        FMemory::Memzero(&Desc);

        Desc.Type                   = D3D12_HIT_GROUP_TYPE_TRIANGLES;
        Desc.HitGroupExport         = *HitGroupName;
        Desc.ClosestHitShaderImport = *ClosestHit;

        if (AnyHit != L"")
        {
            Desc.AnyHitShaderImport = *AnyHit;
        }

        if (Desc.Type != D3D12_HIT_GROUP_TYPE_TRIANGLES)
        {
            Desc.IntersectionShaderImport = *Intersection;
        }
    }

    D3D12_HIT_GROUP_DESC Desc;
    FStringWide          HitGroupName;
    FStringWide          ClosestHit;
    FStringWide          AnyHit;
    FStringWide          Intersection;
};

struct FD3D12Library
{
    FD3D12Library(D3D12_SHADER_BYTECODE ByteCode, const TArray<FStringWide>& InExportNames)
        : ExportNames(InExportNames)
        , ExportDescs(InExportNames.Size())
        , Desc()
    {
        for (int32 i = 0; i < ExportDescs.Size(); i++)
        {
            D3D12_EXPORT_DESC& TempDesc = ExportDescs[i];
            TempDesc.Flags          = D3D12_EXPORT_FLAG_NONE;
            TempDesc.Name           = *ExportNames[i];
            TempDesc.ExportToRename = nullptr;
        }

        Desc.DXILLibrary = ByteCode;
        Desc.pExports    = ExportDescs.Data();
        Desc.NumExports  = ExportDescs.Size();
    }

    TArray<FStringWide>       ExportNames;
    TArray<D3D12_EXPORT_DESC> ExportDescs;
    D3D12_DXIL_LIBRARY_DESC   Desc;
};

struct FD3D12RayTracingPipelineStateStream
{
    void AddLibrary(D3D12_SHADER_BYTECODE ByteCode, const TArray<FStringWide>& ExportNames)
    {
        Libraries.Emplace(ByteCode, ExportNames);
    }

    void AddHitGroup(const FStringWide& HitGroupName, const FStringWide& ClosestHit, const FStringWide& AnyHit, const FStringWide& Intersection)
    {
        HitGroups.Emplace(HitGroupName, ClosestHit, AnyHit, Intersection);
    }

    void AddRootSignatureAssociation(ID3D12RootSignature* RootSignature, const TArray<FStringWide>& ShaderExportNames)
    {
        RootSignatureAssociations.Emplace(RootSignature, ShaderExportNames);
    }

    void Generate()
    {
        uint32 NumSubObjects = Libraries.Size() + HitGroups.Size() + (RootSignatureAssociations.Size() * 2) + 4;
        SubObjects.Resize(NumSubObjects);

        uint32 SubObjectIndex = 0;
        for (FD3D12Library& Lib : Libraries)
        {
            D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
            SubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            SubObject.pDesc = &Lib.Desc;
        }

        for (FD3D12HitGroup& HitGroup : HitGroups)
        {
            D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
            SubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            SubObject.pDesc = &HitGroup.Desc;
        }

        for (FD3D12RootSignatureAssociation& Association : RootSignatureAssociations)
        {
            D3D12_STATE_SUBOBJECT& LocalRootSubObject = SubObjects[SubObjectIndex++];
            LocalRootSubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
            LocalRootSubObject.pDesc = &Association.RootSignature;

            Association.ExportAssociation.pExports              = Association.ShaderExportNamesRef.Data();
            Association.ExportAssociation.NumExports            = Association.ShaderExportNamesRef.Size();
            Association.ExportAssociation.pSubobjectToAssociate = &SubObjects[SubObjectIndex - 1];

            D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
            SubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            SubObject.pDesc = &Association.ExportAssociation;
        }

        D3D12_STATE_SUBOBJECT& GlobalRootSubObject = SubObjects[SubObjectIndex++];
        GlobalRootSubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        GlobalRootSubObject.pDesc = &GlobalRootSignature;

        D3D12_STATE_SUBOBJECT& PipelineConfigSubObject = SubObjects[SubObjectIndex++];
        PipelineConfigSubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        PipelineConfigSubObject.pDesc = &PipelineConfig;

        D3D12_STATE_SUBOBJECT& ShaderConfigObject = SubObjects[SubObjectIndex++];
        ShaderConfigObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        ShaderConfigObject.pDesc = &ShaderConfig;

        PayLoadExportNamesRef.Resize(PayLoadExportNames.Size());
        for (int32 i = 0; i < PayLoadExportNames.Size(); i++)
        {
            PayLoadExportNamesRef[i] = *PayLoadExportNames[i];
        }

        ShaderConfigAssociation.pExports              = PayLoadExportNamesRef.Data();
        ShaderConfigAssociation.NumExports            = PayLoadExportNamesRef.Size();
        ShaderConfigAssociation.pSubobjectToAssociate = &SubObjects[SubObjectIndex - 1]; 

        D3D12_STATE_SUBOBJECT& ShaderConfigAssociationSubObject = SubObjects[SubObjectIndex++];
        ShaderConfigAssociationSubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        ShaderConfigAssociationSubObject.pDesc = &ShaderConfigAssociation;
    }

    TArray<FD3D12Library>                  Libraries;
    TArray<FD3D12HitGroup>                 HitGroups;
    TArray<FD3D12RootSignatureAssociation> RootSignatureAssociations;
    D3D12_RAYTRACING_PIPELINE_CONFIG       PipelineConfig;
    D3D12_RAYTRACING_SHADER_CONFIG         ShaderConfig;
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ShaderConfigAssociation;
    TArray<FStringWide>                    PayLoadExportNames;
    TArray<LPCWSTR>                        PayLoadExportNamesRef;
    ID3D12RootSignature*                   GlobalRootSignature;
    TArray<D3D12_STATE_SUBOBJECT>          SubObjects;
};

FD3D12RayTracingPipelineStateRHI::FD3D12RayTracingPipelineStateRHI(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , StateObject(nullptr)
{
}

FD3D12RayTracingPipelineStateRHI::~FD3D12RayTracingPipelineStateRHI()
{
}

void FD3D12RayTracingPipelineStateRHI::SetDebugName(const FString& InName)
{
    FStringWide WideName = CharToWide(InName);
    StateObject->SetName(*WideName);
    DebugName = InName;
}

void FD3D12RayTracingPipelineStateRHI::GetDebugName(FString& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FD3D12RayTracingPipelineStateRHI::GetRHINativeHandle() const
{
    return reinterpret_cast<void*>(GetD3D12StateObject());
}

bool FD3D12RayTracingPipelineStateRHI::Initialize(const FRHIRayTracingPipelineStateDesc& Desc)
{
    FD3D12RayTracingPipelineStateStream PipelineStream;
    TArray<FD3D12Shader*> Shaders;

    FD3D12RootSignatureManager& RootSignatureManager = GetDevice()->GetRootSignatureManager();

    // Collect and add all RayGen-Shaders
    for (FRHIRayGenShader* RayGen : Desc.RayGenShaders)
    {
        FD3D12RayGenShaderRHI* D3D12RayGen = FD3D12RHI::ResourceCast(RayGen);
        Shaders.Emplace(D3D12RayGen);

        FD3D12RootSignatureLayout RayGenLocalLayout = BuildLocalLayoutFromBindingInfo(D3D12RayGen->GetLocalBindingInfo());
        HitLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(RayGenLocalLayout));
        if (!HitLocalRootSignature)
        {
            return false;
        }

        FStringWide RayGenIdentifier = CharToWide(D3D12RayGen->GetIdentifier());
        PipelineStream.AddLibrary(D3D12RayGen->GetByteCode().GetD3D12Bytecode(), { RayGenIdentifier });
        PipelineStream.AddRootSignatureAssociation(RayGenLocalRootSignature->GetD3D12RootSignature(), { RayGenIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(RayGenIdentifier);
    }

    // Collect and add all HitGroups
    FStringWide HitGroupName;
    FStringWide ClosestHitName;
    FStringWide AnyHitName;
    FStringWide IntersectionName;

    TArray<FRHIRayAnyHitShader*>     AnyHitShaders;
    TArray<FRHIRayClosestHitShader*> ClosestHitShaders;

    for (const FRHIRayTracingHitGroupInfo& HitGroup : Desc.HitGroups)
    {
        HitGroupName.Clear();
        ClosestHitName.Clear();
        AnyHitName.Clear();
        IntersectionName.Clear();

        for (FRHIRayTracingShader* HitGroupShader : HitGroup.Shaders)
        {
            FD3D12RayTracingShader* D3D12HitGroupShader = GetD3D12RayTracingShader(HitGroupShader);
            if (HitGroupShader->GetShaderStage() == EShaderStage::RayClosestHit)
            {
                // TODO: Not the greatest way to handle this
                CHECK(ClosestHitName.IsEmpty());
                ClosestHitName = CharToWide(D3D12HitGroupShader->GetIdentifier());
                ClosestHitShaders.Emplace(static_cast<FRHIRayClosestHitShader*>(HitGroupShader));
            }
            else if (HitGroupShader->GetShaderStage() == EShaderStage::RayAnyHit)
            {
                // TODO: Not the greatest way to handle this
                CHECK(AnyHitName.IsEmpty());

                AnyHitName = CharToWide(D3D12HitGroupShader->GetIdentifier());
                AnyHitShaders.Emplace(static_cast<FRHIRayAnyHitShader*>(HitGroupShader));
            }
            else if (HitGroupShader->GetShaderStage() == EShaderStage::RayIntersection)
            {
                // TODO: Not the greatest way to handle this
                CHECK(IntersectionName.IsEmpty());
                IntersectionName = CharToWide(D3D12HitGroupShader->GetIdentifier());
            }
        }

        HitGroupName = CharToWide(HitGroup.Name);
        PipelineStream.AddHitGroup(HitGroupName, ClosestHitName, AnyHitName, IntersectionName);
    }

    // Collect and add all AnyHit shaders
    for (FRHIRayAnyHitShader* AnyHit : AnyHitShaders)
    {
        FD3D12RayAnyHitShaderRHI* D3D12AnyHit = FD3D12RHI::ResourceCast(AnyHit);
        Shaders.Emplace(D3D12AnyHit);

        FD3D12RootSignatureLayout AnyHitLocalLayout = BuildLocalLayoutFromBindingInfo(D3D12AnyHit->GetLocalBindingInfo());

        HitLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(AnyHitLocalLayout));
        if (!HitLocalRootSignature)
        {
            return false;
        }

        FStringWide AnyHitIdentifier = CharToWide(D3D12AnyHit->GetIdentifier());
        PipelineStream.AddLibrary(D3D12AnyHit->GetByteCode().GetD3D12Bytecode(), { AnyHitIdentifier });
        PipelineStream.AddRootSignatureAssociation(HitLocalRootSignature->GetD3D12RootSignature(), { AnyHitIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(AnyHitIdentifier);
    }

    // Collect and add all ClosestHit shaders
    for (FRHIRayClosestHitShader* ClosestHit : ClosestHitShaders)
    {
        FD3D12RayClosestHitShaderRHI* D3D12ClosestHit = FD3D12RHI::ResourceCast(ClosestHit);
        Shaders.Emplace(D3D12ClosestHit);

        FD3D12RootSignatureLayout ClosestHitLocalLayout = BuildLocalLayoutFromBindingInfo(D3D12ClosestHit->GetLocalBindingInfo());

        HitLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(ClosestHitLocalLayout));
        if (!HitLocalRootSignature)
        {
            return false;
        }

        FStringWide ClosestHitIdentifier = CharToWide(D3D12ClosestHit->GetIdentifier());
        PipelineStream.AddLibrary(D3D12ClosestHit->GetByteCode().GetD3D12Bytecode(), { ClosestHitIdentifier });
        PipelineStream.AddRootSignatureAssociation(HitLocalRootSignature->GetD3D12RootSignature(), { ClosestHitIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(ClosestHitIdentifier);
    }

    // Collect and add all Miss shaders
    for (FRHIRayMissShader* Miss : Desc.MissShaders)
    {
        FD3D12RayMissShaderRHI* D3D12MissShader = FD3D12RHI::ResourceCast(Miss);
        Shaders.Emplace(D3D12MissShader);

        FD3D12RootSignatureLayout MissLocalLayout = BuildLocalLayoutFromBindingInfo(D3D12MissShader->GetLocalBindingInfo());

        MissLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(MissLocalLayout));
        if (!MissLocalRootSignature)
        {
            return false;
        }

        FStringWide MissIdentifier = CharToWide(D3D12MissShader->GetIdentifier());
        PipelineStream.AddLibrary(D3D12MissShader->GetByteCode().GetD3D12Bytecode(), { MissIdentifier });
        PipelineStream.AddRootSignatureAssociation(MissLocalRootSignature->GetD3D12RootSignature(), { MissIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(MissIdentifier);
    }

    PipelineStream.ShaderConfig.MaxAttributeSizeInBytes  = Desc.MaxAttributeSizeInBytes;
    PipelineStream.ShaderConfig.MaxPayloadSizeInBytes    = Desc.MaxPayloadSizeInBytes;
    PipelineStream.PipelineConfig.MaxTraceRecursionDepth = Desc.MaxRecursionDepth;

    FD3D12RootSignatureLayout GlobalLayout;
    GlobalLayout.SetType(ERootSignatureType::RayTracingGlobal);
    GlobalLayout.SetAllowInputAssembler(false);

    uint8 MaxPushConstants = 0;
    for (FD3D12Shader* Shader : Shaders)
    {
        CHECK(Shader != nullptr);
        const FD3D12ShaderBindingInfo& BindingInfo = Shader->GetBindingInfo();
        for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : BindingInfo.ResourceBindings)
        {
            GlobalLayout.AddRegister(ShaderVisibility_All, static_cast<EResourceType>(Binding.BindingType), Binding.OriginalBindingIndex);
        }

        MaxPushConstants = Math::Max<uint8>(MaxPushConstants, static_cast<uint8>(BindingInfo.NumPushConstants));
    }

    GlobalLayout.SetNumPushConstants(MaxPushConstants);
    GlobalLayout.ComputeRootCBVs();

    GlobalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(GlobalLayout));
    if (!GlobalRootSignature)
    {
        return false;
    }

    PipelineStream.GlobalRootSignature = GlobalRootSignature->GetD3D12RootSignature();
    PipelineStream.Generate();

    D3D12_STATE_OBJECT_DESC RayTracingPipeline;
    FMemory::Memzero(&RayTracingPipeline);

    RayTracingPipeline.Type          = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    RayTracingPipeline.pSubobjects   = PipelineStream.SubObjects.Data();
    RayTracingPipeline.NumSubobjects = PipelineStream.SubObjects.Size();

#if D3D12_USE_ID3D12DEVICE_5
    TComPtr<ID3D12StateObject> TempStateObject;
    HRESULT Result = GetDevice()->GetD3D12Device5()->CreateStateObject(&RayTracingPipeline, IID_PPV_ARGS(&TempStateObject));
    if (FAILED(Result))
    {
        DEBUG_BREAK();
        return false;
    }

    TComPtr<ID3D12StateObjectProperties> TempStateObjectProperties;
    Result = TempStateObject->QueryInterface(IID_PPV_ARGS(&TempStateObjectProperties));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[D3D12RayTracingPipelineState] Failed to retrieve ID3D12StateObjectProperties");
        return false;
    }

    StateObject           = TempStateObject;
    StateObjectProperties = TempStateObjectProperties;
    return true;
#else
    D3D12_ERROR_CRITICAL("[D3D12RayTracingPipelineState]: ID3D12Device5 is required for ray tracing pipeline creation");
    return false;
#endif
}

void* FD3D12RayTracingPipelineStateRHI::GetShaderIdentifier(const FString& ExportName)
{
    if (FD3D12RayTracingShaderIdentifier* MapItem = ShaderIdentifiers.Find(ExportName))
    {
        return MapItem->ShaderIdentifier;
    }
    else
    {
        FStringWide WideExportName = CharToWide(ExportName);

        void* Result = StateObjectProperties->GetShaderIdentifier(*WideExportName);
        if (!Result)
        {
            return nullptr;
        }

        FD3D12RayTracingShaderIdentifier Identifier;
        FMemory::Memcpy(Identifier.ShaderIdentifier, Result, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

        FD3D12RayTracingShaderIdentifier& NewIdentifier = ShaderIdentifiers.Add(ExportName, Identifier);
        return NewIdentifier.ShaderIdentifier;
    }
}

FD3D12PipelineStateManager::FD3D12PipelineStateManager(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , PipelineData(nullptr)
    , PipelineDataSize(0)
    , PipelineLibrary(nullptr)
    , bPipelineLibraryDirty(false)
    , LastSaveTimestamp(FPlatformTime::QueryPerformanceCounter())
{
}

FD3D12PipelineStateManager::~FD3D12PipelineStateManager()
{
    FreePipelineData();
}

bool FD3D12PipelineStateManager::Initialize()
{
    if (LoadCacheFromFile())
    {
        return true;
    }

    // In case we allocated data, let's free it
    FreePipelineData();

#if D3D12_USE_ID3D12DEVICE_1
    ID3D12Device1* Device1 = GetDevice()->GetD3D12Device1();
    if (!Device1)
    {
        D3D12_WARNING("ID3D12Device1 is not supported, PipelineCache not supported");
        return false;
    }

    HRESULT hResult = Device1->CreatePipelineLibrary(nullptr, 0, IID_PPV_ARGS(&PipelineLibrary));
    if (FAILED(hResult))
    {
        D3D12_ERROR("Failed to create PipelineLibrary %d", hResult);
        return false;
    }
    else
    {
        D3D12_INFO("Created PipelineLibrary");
    }

    return true;
#else
    D3D12_WARNING("ID3D12Device1 is not supported, PipelineCache not supported");
    return false;
#endif
}

bool FD3D12PipelineStateManager::CreateGraphicsPipeline(const WIDECHAR* PipelineHash, const D3D12_PIPELINE_STATE_STREAM_DESC& PipelineStream, TComPtr<ID3D12PipelineState>& OutPipelineState)
{
    if (!PipelineLibrary)
    {
        return false;
    }

    TScopedLock Lock(PipelineLibraryCS);

    HRESULT hResult = PipelineLibrary->LoadPipeline(PipelineHash, &PipelineStream, IID_PPV_ARGS(&OutPipelineState));
    if (hResult == E_INVALIDARG)
    {
    #if D3D12_USE_ID3D12DEVICE_2
        hResult = GetDevice()->GetD3D12Device2()->CreatePipelineState(&PipelineStream, IID_PPV_ARGS(&OutPipelineState));
        if (FAILED(hResult))
        {
            D3D12_ERROR_CRITICAL("Failed to create GraphicsPipelineState");
            return false;
        }

        hResult = PipelineLibrary->StorePipeline(PipelineHash, OutPipelineState.Get());
        if (FAILED(hResult))
        {
            D3D12_WARNING("Failed to store GraphicsPipelineState");
        }

        bPipelineLibraryDirty = true;
    #else
        D3D12_ERROR_CRITICAL("ID3D12Device2 is required for pipeline stream creation");
        return false;
    #endif
    }

    STAT_ADD(STAT_D3D12_PSOCreateCount, 1);
    return true;
}

bool FD3D12PipelineStateManager::CreateComputePipeline(const WIDECHAR* PipelineHash, const D3D12_PIPELINE_STATE_STREAM_DESC& PipelineStream, TComPtr<ID3D12PipelineState>& OutPipelineState)
{
    if (!PipelineLibrary)
    {
        return false;
    }

    TScopedLock Lock(PipelineLibraryCS);

    HRESULT hResult = PipelineLibrary->LoadPipeline(PipelineHash, &PipelineStream, IID_PPV_ARGS(&OutPipelineState));
    if (hResult == E_INVALIDARG)
    {
    #if D3D12_USE_ID3D12DEVICE_2
        hResult = GetDevice()->GetD3D12Device2()->CreatePipelineState(&PipelineStream, IID_PPV_ARGS(&OutPipelineState));
        if (FAILED(hResult))
        {
            D3D12_ERROR_CRITICAL("Failed to create ComputePipelineState");
            return false;
        }

        hResult = PipelineLibrary->StorePipeline(PipelineHash, OutPipelineState.Get());
        if (FAILED(hResult))
        {
            D3D12_WARNING("Failed to store ComputePipelineState");
        }

        bPipelineLibraryDirty = true;
    #else
        D3D12_ERROR_CRITICAL("ID3D12Device2 is required for pipeline stream creation");
        return false;
    #endif
    }

    STAT_ADD(STAT_D3D12_PSOCreateCount, 1);
    return true;
}

bool FD3D12PipelineStateManager::CreateGraphicsPipeline(const WIDECHAR* PipelineHash, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& Desc, TComPtr<ID3D12PipelineState>& OutPipelineState)
{
    if (!PipelineLibrary)
    {
        return false;
    }

    TScopedLock Lock(PipelineLibraryCS);

    HRESULT hResult = PipelineLibrary->LoadGraphicsPipeline(PipelineHash, &Desc, IID_PPV_ARGS(&OutPipelineState));
    if (hResult == E_INVALIDARG)
    {
        hResult = GetDevice()->GetD3D12Device()->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(&OutPipelineState));
        if (FAILED(hResult))
        {
            D3D12_ERROR_CRITICAL("Failed to create GraphicsPipelineState (legacy)");
            return false;
        }

        hResult = PipelineLibrary->StorePipeline(PipelineHash, OutPipelineState.Get());
        if (FAILED(hResult))
        {
            D3D12_WARNING("Failed to store GraphicsPipelineState (legacy)");
        }

        bPipelineLibraryDirty = true;
    }

    STAT_ADD(STAT_D3D12_PSOCreateCount, 1);
    return true;
}

bool FD3D12PipelineStateManager::CreateComputePipeline(const WIDECHAR* PipelineHash, const D3D12_COMPUTE_PIPELINE_STATE_DESC& Desc, TComPtr<ID3D12PipelineState>& OutPipelineState)
{
    if (!PipelineLibrary)
    {
        return false;
    }

    TScopedLock Lock(PipelineLibraryCS);

    HRESULT hResult = PipelineLibrary->LoadComputePipeline(PipelineHash, &Desc, IID_PPV_ARGS(&OutPipelineState));
    if (hResult == E_INVALIDARG)
    {
        hResult = GetDevice()->GetD3D12Device()->CreateComputePipelineState(&Desc, IID_PPV_ARGS(&OutPipelineState));
        if (FAILED(hResult))
        {
            D3D12_ERROR_CRITICAL("Failed to create ComputePipelineState (legacy)");
            return false;
        }

        hResult = PipelineLibrary->StorePipeline(PipelineHash, OutPipelineState.Get());
        if (FAILED(hResult))
        {
            D3D12_WARNING("Failed to store ComputePipelineState (legacy)");
        }

        bPipelineLibraryDirty = true;
    }

    STAT_ADD(STAT_D3D12_PSOCreateCount, 1);
    return true;
}

bool FD3D12PipelineStateManager::SaveCacheData()
{
    if (!PipelineLibrary)
    {
        D3D12_WARNING("No valid PipelineCache created");
        return false;
    }
    
    // No changes has been made to the pipeline
    if (!bPipelineLibraryDirty)
    {
        return true;
    }

    const FString PipelineCacheFilename = CVarPipelineCacheFileName.GetValue();
    const FString PipelineCacheFilepath = FPaths::GetAssetDir() + '/' + PipelineCacheFilename;

    TFileRef<IPlatformFile> CacheFile = FPlatformFile::OpenForWrite(PipelineCacheFilepath);
    if (!CacheFile)
    {
        D3D12_WARNING("Failed to open PipelineCache-file");
        return false;
    }

    {
        TScopedLock Lock(PipelineLibraryCS);

        const SIZE_T PipelineCacheSize = PipelineLibrary->GetSerializedSize();
        STAT_SET(STAT_D3D12_PSOCacheSize, static_cast<int64>(PipelineCacheSize));
        TUniquePtr<uint8[]> PipelineCacheData = MakeUniquePtr<uint8[]>(PipelineCacheSize);

        HRESULT hResult = PipelineLibrary->Serialize(PipelineCacheData.Get(), PipelineCacheSize);
        if (FAILED(hResult))
        {
            D3D12_ERROR("Failed to serielize PipelineCache");
            return false;
        }

        FD3D12PipelineDiskHeader Header;
        FMemory::Memcpy(Header.Magic, "D3D12PSO", sizeof(Header.Magic));

        Header.DataCRC  = CRC32::Generate(PipelineCacheData.Get(), PipelineCacheSize);
        Header.DataSize = PipelineCacheSize;

        int32 BytesWritten = CacheFile->Write(reinterpret_cast<const uint8*>(&Header), sizeof(FD3D12PipelineDiskHeader));
        if (BytesWritten != sizeof(FD3D12PipelineDiskHeader))
        {
            D3D12_ERROR("Failed to write PipelineCacheHader to disk");
            return false;
        } 

        BytesWritten = CacheFile->Write(PipelineCacheData.Get(), static_cast<uint32>(PipelineCacheSize));
        if (BytesWritten != static_cast<int32>(PipelineCacheSize))
        {
            D3D12_ERROR("Failed to write PipelineCache to disk");
            return false;
        }
        else
        {
            D3D12_INFO("Saved PipelineCache to file '%s'", *PipelineCacheFilepath);
        }
    }

    bPipelineLibraryDirty = false;
    return true;
}

void FD3D12PipelineStateManager::SaveCacheDataAsync()
{
    if (!PipelineLibrary || !bPipelineLibraryDirty)
    {
        return;
    }

    const uint64 CurrentTime = FPlatformTime::QueryPerformanceCounter();
    const uint64 Frequency   = FPlatformTime::QueryPerformanceFrequency();
    const double ElapsedSeconds = static_cast<double>(CurrentTime - LastSaveTimestamp) / static_cast<double>(Frequency);

    const int32 SaveInterval = CVarPipelineCacheSaveInterval.GetValue();
    if (ElapsedSeconds < static_cast<double>(SaveInterval))
    {
        return;
    }

    const FString PipelineCacheFilename = CVarPipelineCacheFileName.GetValue();
    const FString PipelineCacheFilepath = FPaths::GetAssetDir() + '/' + PipelineCacheFilename;

    TUniquePtr<uint8[]> SerializedData;
    SIZE_T SerializedSize = 0;

    {
        TScopedLock Lock(PipelineLibraryCS);

        SerializedSize = PipelineLibrary->GetSerializedSize();
        SerializedData = MakeUniquePtr<uint8[]>(SerializedSize);

        HRESULT hResult = PipelineLibrary->Serialize(SerializedData.Get(), SerializedSize);
        if (FAILED(hResult))
        {
            D3D12_ERROR("[FD3D12PipelineStateManager] Failed to serialize PipelineCache for async save");
            return;
        }

        bPipelineLibraryDirty = false;
    }

    LastSaveTimestamp = CurrentTime;

    FD3D12PipelineDiskHeader Header;
    FMemory::Memcpy(Header.Magic, "D3D12PSO", sizeof(Header.Magic));
    Header.DataCRC  = CRC32::Generate(SerializedData.Get(), SerializedSize);
    Header.DataSize = SerializedSize;

    Async([FilePath = PipelineCacheFilepath, Header, Data = Move(SerializedData), DataSize = SerializedSize]()
    {
        TFileRef<IPlatformFile> CacheFile = FPlatformFile::OpenForWrite(FilePath);
        if (!CacheFile)
        {
            D3D12_WARNING("[FD3D12PipelineStateManager] Failed to open PipelineCache file for async save");
            return;
        }

        CacheFile->Write(reinterpret_cast<const uint8*>(&Header), sizeof(FD3D12PipelineDiskHeader));
        CacheFile->Write(Data.Get(), static_cast<uint32>(DataSize));

        D3D12_INFO("[FD3D12PipelineStateManager] Async saved PipelineCache to '%s'", *FilePath);
    });
}

bool FD3D12PipelineStateManager::LoadCacheFromFile()
{
    const FString PipelineCacheFilename = CVarPipelineCacheFileName.GetValue();
    const FString PipelineCacheFilepath = FPaths::GetAssetDir() + '/' + PipelineCacheFilename;
    
    TFileRef<IPlatformFile> CacheFile = FPlatformFile::OpenForRead(PipelineCacheFilepath);
    if (!CacheFile)
    {
        D3D12_WARNING("Failed to open PipelineCache-file");
        return false;
    }

    FD3D12PipelineDiskHeader Header;

    int64 BytesRead = CacheFile->Read(reinterpret_cast<uint8*>(&Header), sizeof(FD3D12PipelineDiskHeader));
    if (BytesRead != sizeof(FD3D12PipelineDiskHeader))
    {
        D3D12_WARNING("Something went wrong when reading PipelineCacheHeader");
        return false;
    }

    if (FMemory::Memcmp(Header.Magic, "D3D12PSO", sizeof(Header.Magic)) != 0)
    {
        D3D12_WARNING("Invalid PipelineCacheHeader");
        return false;
    }

    // NOTE: if the cache size is more than 1GB something is probably off
    constexpr uint64 MaxCacheSize = 1024 * 1024 * 1024;
    if (Header.DataSize >= MaxCacheSize)
    {
        D3D12_WARNING("Invalid PipelineCacheHeader");
        return false;
    }

    PipelineData     = FMemory::Malloc(Header.DataSize);
    PipelineDataSize = Header.DataSize;

    BytesRead = CacheFile->Read(reinterpret_cast<uint8*>(PipelineData), static_cast<uint32>(PipelineDataSize));
    if (PipelineDataSize != static_cast<uint64>(BytesRead))
    {
        D3D12_WARNING("Something went wrong when reading PipelineCache");
        return false;
    }
    
    const uint32 DataCRC = CRC32::Generate(PipelineData, PipelineDataSize);
    if (DataCRC != Header.DataCRC)
    {
        D3D12_WARNING("PipelineCacheData is invalid");
        return false;
    }
    
#if D3D12_USE_ID3D12DEVICE_1
    ID3D12Device1* Device1 = GetDevice()->GetD3D12Device1();
    if (!Device1)
    {
        D3D12_WARNING("ID3D12Device1 is not supported, PipelineCache not supported");
        return false;
    }

    // First verify the data
    HRESULT hResult = Device1->CreatePipelineLibrary(PipelineData, PipelineDataSize, __uuidof(ID3D12PipelineLibrary1), nullptr);

    // When verifying the data the function should return S_FALSE if the data is valid, including a driver version check
    if (hResult != S_FALSE)
    {
        D3D12_WARNING("Verification of PipelineState data is invalid");
        return false;
    }

    hResult = Device1->CreatePipelineLibrary(PipelineData, PipelineDataSize, IID_PPV_ARGS(&PipelineLibrary));
    if (FAILED(hResult))
    {
        D3D12_WARNING("Failed to create PipelineLibrary");
        return false;
    }

    return true;
#else
    D3D12_WARNING("ID3D12Device1 is not supported, PipelineCache not supported");
    return false;
#endif
}

void FD3D12PipelineStateManager::FreePipelineData()
{
    if (PipelineData)
    {
        FMemory::Free(PipelineData);
        PipelineData = nullptr;
    }

    PipelineDataSize = 0;
}
