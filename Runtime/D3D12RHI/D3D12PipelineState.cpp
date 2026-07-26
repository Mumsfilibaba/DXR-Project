#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Tasks/Tasks.h"
#include "Core/Misc/Paths.h"
#include "D3D12RHI/D3D12PipelineState.h"
#include "D3D12RHI/D3D12Stats.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RHI.h"

static TAutoConsoleVariable<String> CVarPipelineCacheFileName(
    "D3D12RHI.PipelineCacheFileName",
    "FileName for the file storing the PipelineCache",
    "PipelineCache.d3d12psocache");

static TAutoConsoleVariable<int32> CVarPipelineCacheSaveInterval(
    "D3D12RHI.PipelineCacheSaveInterval",
    "Minimum interval in seconds between automatic pipeline cache saves",
    30);

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
        Memory::Memzero(&InputElementDesc, sizeof(D3D12_INPUT_ELEMENT_DESC));

        const String& Semantic = SemanticNames.Emplace(Element.Semantic);
        InputElementDesc.SemanticName = *Semantic;
        HashCombine(CalculatedHash, THash<String>::GetHash(Semantic));

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

void* FD3D12InputLayoutRHI::GetRHINativeState() const
{
    return nullptr;
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
    : FRHIDepthStencilState(InDesc)
    , Hash(0)
{
    Memory::Memzero(&D3D12Desc);

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

void* FD3D12DepthStencilStateRHI::GetRHINativeState() const
{
    return nullptr;
}

FD3D12RasterizerStateRHI::FD3D12RasterizerStateRHI(const FRHIRasterizerStateDesc& InDesc)
    : FRHIRasterizerState(InDesc)
    , Hash(0)
{
    Memory::Memzero(&D3D12Desc);

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

void* FD3D12RasterizerStateRHI::GetRHINativeState() const
{
    return nullptr;
}

FD3D12BlendStateRHI::FD3D12BlendStateRHI(const FRHIBlendStateDesc& InDesc)
    : FRHIBlendState(InDesc)
    , Hash(0)
{
    Memory::Memzero(&D3D12Desc);

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

void* FD3D12BlendStateRHI::GetRHINativeState() const
{
    return nullptr;
}

FD3D12PipelineState::FD3D12PipelineState(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
{
    Memory::Memzero(EffectiveDescriptorCounts, sizeof(EffectiveDescriptorCounts));
}

FD3D12PipelineState::~FD3D12PipelineState()
{
}

void FD3D12EffectiveDescriptorCounts::ComputeEffectiveDescriptorCounts(const FD3D12RootSignature* RootSignature, FD3D12Shader* const* Shaders, uint32 NumShaders)
{
    Memory::Memzero(EffectiveDescriptorCounts, sizeof(EffectiveDescriptorCounts));

    if (!RootSignature)
    {
        return;
    }

    for (uint32 i = 0; i < NumShaders; i++)
    {
        FD3D12Shader* Shader = Shaders[i];
        if (!Shader)
        {
            continue;
        }

        const EShaderVisibility::Type  Stage       = Shader->GetShaderVisibility();
        const FD3D12ShaderBindingInfo& BindingInfo = Shader->GetBindingInfo();

        for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : BindingInfo.ResourceBindings)
        {
            const uint16              Register     = Binding.OriginalBindingIndex;
            const EResourceType::Type ResourceType = static_cast<EResourceType::Type>(Binding.BindingType);
            
            if (ResourceType == EResourceType::CBV && RootSignature->IsRootCBV(Stage, Register))
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

void FD3D12PipelineState::SetDebugName(const String& InName)
{
    const WString WideName = CharToWide(InName);
    PipelineState->SetName(*WideName);
    DebugName = InName;
}

FD3D12GraphicsPipelineStateRHI::FD3D12GraphicsPipelineStateRHI(FD3D12Device* InDevice)
    : FRHIGraphicsPipelineState()
    , FD3D12PipelineState(InDevice)
    , ShaderFlags(ED3D12ShaderFlags::None)
{
}

FD3D12GraphicsPipelineStateRHI::~FD3D12GraphicsPipelineStateRHI()
{
}

void FD3D12GraphicsPipelineStateRHI::SetDebugName(const String& InName)
{
    FD3D12PipelineState::SetDebugName(InName);
}

void FD3D12GraphicsPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FD3D12GraphicsPipelineStateRHI::GetRHINativeState() const
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
    FD3D12InputLayoutRHI* D3D12InputLayout = FD3D12DeviceRHI::ResourceCast(Desc.InputLayout);
    if (D3D12InputLayout)
    {
        InputLayoutDesc = D3D12InputLayout->GetDesc();
    }

    // ShaderStages
    TArray<FD3D12Shader*> ShadersWithRootSignature;
    TArray<FD3D12Shader*> BaseShaders;

    // VertexShader
    {
        if (FD3D12VertexShaderRHI* D3D12VertexShader = FD3D12DeviceRHI::ResourceCast(Desc.VertexShader))
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
        if (FD3D12HullShaderRHI* D3D12HullShader = FD3D12DeviceRHI::ResourceCast(Desc.HullShader))
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
        if (FD3D12DomainShaderRHI* D3D12DomainShader = FD3D12DeviceRHI::ResourceCast(Desc.DomainShader))
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
        if (FD3D12GeometryShaderRHI* D3D12GeometryShader = FD3D12DeviceRHI::ResourceCast(Desc.GeometryShader))
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
        if (FD3D12PixelShaderRHI* D3D12PixelShader = FD3D12DeviceRHI::ResourceCast(Desc.PixelShader))
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
    FD3D12RasterizerStateRHI* D3D12RasterizerState = FD3D12DeviceRHI::ResourceCast(Desc.RasterizerState);
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
    FD3D12DepthStencilStateRHI* D3D12DepthStencilState = FD3D12DeviceRHI::ResourceCast(Desc.DepthStencilState);
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
    FD3D12BlendStateRHI* D3D12BlendState = FD3D12DeviceRHI::ResourceCast(Desc.BlendState);
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

    // Shader-flags
    ShaderFlags = ED3D12ShaderFlags::None;
    for (FD3D12Shader* Shader : BaseShaders)
    {
        ShaderFlags |= Shader->GetFlags();
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
                const EShaderVisibility::Type  Stage       = Shader->GetShaderVisibility();
                const FD3D12ShaderBindingInfo& BindingInfo = Shader->GetBindingInfo();

                for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : BindingInfo.ResourceBindings)
                {
                    RootSignatureLayout.AddRegister(Stage, static_cast<EResourceType::Type>(Binding.BindingType), Binding.OriginalBindingIndex);
                }
                
                NumPushConstants = Math::Max<uint8>(NumPushConstants, static_cast<uint8>(BindingInfo.NumPushConstants));
            }

            RootSignatureLayout.SetNumPushConstants(NumPushConstants);
            RootSignatureLayout.SetDirectlyIndexedResourceHeap(IsEnumFlagSet(ShaderFlags, ED3D12ShaderFlags::RequiresResourceDescriptorHeapIndexing));
            RootSignatureLayout.SetDirectlyIndexedSamplerHeap(IsEnumFlagSet(ShaderFlags, ED3D12ShaderFlags::RequiresSamplerDescriptorHeapIndexing));

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
            const EShaderVisibility::Type  Stage       = Shader->GetShaderVisibility();
            const FD3D12ShaderBindingInfo& BindingInfo = Shader->GetBindingInfo();

            const bool bShaderHasRootBindings = !BindingInfo.ResourceBindings.IsEmpty() || BindingInfo.NumPushConstants > 0;
            if (bShaderHasRootBindings && RootSignature->HasDenyFlag(Stage))
            {
                D3D12_ERROR_CRITICAL("Root Signature denies access to stage %u, but shader has %d resource bindings and %u push constants.",
                    Stage, BindingInfo.ResourceBindings.Size(), BindingInfo.NumPushConstants);
            }

            for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : BindingInfo.ResourceBindings)
            {
                const EResourceType::Type ResType = static_cast<EResourceType::Type>(Binding.BindingType);
                if (RootSignature->GetSlotForRegister(Stage, ResType, Binding.OriginalBindingIndex) < 0 &&
                    RootSignature->GetShaderStage(Stage).GetRootDescriptorParameterIndex(ResType, Binding.OriginalBindingIndex) < 0)
                {
                    D3D12_ERROR_CRITICAL("Custom root signature missing register %u (type %u) for shader stage %u", 
                        Binding.OriginalBindingIndex, Binding.BindingType, Stage);
                }
            }
        }

        ComputeEffectiveDescriptorCounts(RootSignature.Get(), BaseShaders.Data(), BaseShaders.Size());
    }

    // View Instancing: Validate that the PSO state is compatible with shader requirements
    {
        const bool bShaderRequiresViewID = IsEnumFlagSet(ShaderFlags, ED3D12ShaderFlags::RequiresViewID);
        if (bShaderRequiresViewID && !Desc.ViewInstancingState.bEnableViewInstancing)
        {
            D3D12_ERROR_CRITICAL("Shader uses SV_ViewID but the graphics PSO was created without view instancing enabled. Set FRHIGraphicsPipelineStateDesc::ViewInstancingState.bEnableViewInstancing to true.");
        }
        else if (!bShaderRequiresViewID && Desc.ViewInstancingState.bEnableViewInstancing)
        {
            D3D12_WARNING("[FD3D12GraphicsPipelineStateRHI] View instancing is enabled on the PSO but no shader stage reads SV_ViewID; the view-instancing state will have no effect.");
        }
    }

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
    Memory::Memzero(&PipelineKey, sizeof(FD3D12GraphicsPipelineKey));

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
        Memory::Memzero(&PipelineStreamDesc);

        PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
        PipelineStreamDesc.SizeInBytes                   = sizeof(FD3D12GraphicsPipelineStream);

        if (GD3D12SupportPipelineCache)
        {
            FD3D12PipelineStateManager& PipelineStateManager = GetDevice()->GetPipelineStateManager();
            if (PipelineStateManager.CreateGraphicsPipeline(PipelineHashBuffer, PipelineStreamDesc, PipelineState))
            {
                STAT_ADD(STAT_D3D12_NumGraphicsPipelineStates, 1);
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
        STAT_ADD(STAT_D3D12_NumGraphicsPipelineStates, 1);
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
        Memory::Memzero(&LegacyDesc);

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
                STAT_ADD(STAT_D3D12_NumGraphicsPipelineStates, 1);
                return true;
            }
        }

        HRESULT Result = GetDevice()->GetD3D12Device()->CreateGraphicsPipelineState(&LegacyDesc, IID_PPV_ARGS(&PipelineState));
        if (FAILED(Result))
        {
            D3D12_ERROR_CRITICAL("[D3D12GraphicsPipelineState]: FAILED to Create GraphicsPipelineState (legacy)");
            return false;
        }

        STAT_ADD(STAT_D3D12_NumGraphicsPipelineStates, 1);
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

void FD3D12ComputePipelineStateRHI::SetDebugName(const String& InName)
{
    FD3D12PipelineState::SetDebugName(InName);
}

void FD3D12ComputePipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FD3D12ComputePipelineStateRHI::GetRHINativeState() const
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
            Layout.AddRegister(EShaderVisibility::All, static_cast<EResourceType::Type>(Binding.BindingType), Binding.OriginalBindingIndex);
        }

        for (const FRHIStaticSamplerInfo& StaticSampler : Desc.StaticSamplers)
        {
            Layout.AddStaticSampler(StaticSampler);
        }

        Layout.SetNumPushConstants(static_cast<uint8>(BindingInfo.NumPushConstants));
        Layout.SetDirectlyIndexedResourceHeap(Shader->HasFlag(ED3D12ShaderFlags::RequiresResourceDescriptorHeapIndexing));
        Layout.SetDirectlyIndexedSamplerHeap(Shader->HasFlag(ED3D12ShaderFlags::RequiresSamplerDescriptorHeapIndexing));
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
            const EResourceType::Type ResType = static_cast<EResourceType::Type>(Binding.BindingType);
            if (RootSignature->GetSlotForRegister(EShaderVisibility::All, ResType, Binding.OriginalBindingIndex) < 0 &&
                RootSignature->GetShaderStage(EShaderVisibility::All).GetRootDescriptorParameterIndex(ResType, Binding.OriginalBindingIndex) < 0)
            {
                D3D12_ERROR_CRITICAL("Custom compute root signature missing register %u (type %u)",
                    Binding.OriginalBindingIndex, Binding.BindingType);
            }
        }
    }

    CHECK(RootSignature != nullptr);

    {
        FD3D12Shader* ComputeShaders[] = { Shader.Get() };
        ComputeEffectiveDescriptorCounts(RootSignature.Get(), ComputeShaders, 1);
    }

    // Build pipeline key for caching (shared by both paths)
    FD3D12ComputePipelineKey PipelineKey;
    Memory::Memzero(&PipelineKey, sizeof(FD3D12ComputePipelineKey));

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
        Memory::Memzero(&PipelineStreamDesc);

        PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
        PipelineStreamDesc.SizeInBytes                   = sizeof(FD3D12ComputePipelineStream);

        if (GD3D12SupportPipelineCache)
        {
            FD3D12PipelineStateManager& PipelineStateManager = GetDevice()->GetPipelineStateManager();
            if (PipelineStateManager.CreateComputePipeline(PipelineHashBuffer, PipelineStreamDesc, PipelineState))
            {
                STAT_ADD(STAT_D3D12_NumComputePipelineStates, 1);
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

        STAT_ADD(STAT_D3D12_NumComputePipelineStates, 1);
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
        Memory::Memzero(&LegacyDesc);

        LegacyDesc.pRootSignature = RootSignature->GetD3D12RootSignature();
        LegacyDesc.CS             = ComputeShaderCode;
        LegacyDesc.NodeMask       = GetDevice()->GetNodeMask();

        if (GD3D12SupportPipelineCache)
        {
            FD3D12PipelineStateManager& PipelineStateManager = GetDevice()->GetPipelineStateManager();
            if (PipelineStateManager.CreateComputePipeline(PipelineHashBuffer, LegacyDesc, PipelineState))
            {
                STAT_ADD(STAT_D3D12_NumComputePipelineStates, 1);
                return true;
            }
        }

        HRESULT Result = GetDevice()->GetD3D12Device()->CreateComputePipelineState(&LegacyDesc, IID_PPV_ARGS(&PipelineState));
        if (FAILED(Result))
        {
            D3D12_ERROR_CRITICAL("[D3D12ComputePipelineState]: FAILED to Create ComputePipelineState (legacy)");
            return false;
        }

        STAT_ADD(STAT_D3D12_NumComputePipelineStates, 1);
        return true;
    }
}

FD3D12MeshletPipelineStateRHI::FD3D12MeshletPipelineStateRHI(FD3D12Device* InDevice)
    : FRHIMeshletPipelineState()
    , FD3D12PipelineState(InDevice)
    , ShaderFlags(ED3D12ShaderFlags::None)
{
}

FD3D12MeshletPipelineStateRHI::~FD3D12MeshletPipelineStateRHI()
{
}

void FD3D12MeshletPipelineStateRHI::SetDebugName(const String& InName)
{
    FD3D12PipelineState::SetDebugName(InName);
}

void FD3D12MeshletPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FD3D12MeshletPipelineStateRHI::GetRHINativeState() const
{
    return reinterpret_cast<void*>(GetD3D12PipelineState());
}

bool FD3D12MeshletPipelineStateRHI::Initialize(const FRHIMeshletPipelineStateDesc& Desc)
{
#if D3D12_ENABLE_PIPELINE_STATE_STREAM
    D3D12_SHADER_BYTECODE      AmplificationShaderCode = {};
    D3D12_SHADER_BYTECODE      MeshShaderCode          = {};
    D3D12_SHADER_BYTECODE      PixelShaderCode         = {};
    D3D12_RT_FORMAT_ARRAY      RenderTargetInfo        = {};
    DXGI_FORMAT                DepthBufferFormat       = {};
    D3D12_RASTERIZER_DESC      RasterizerDesc          = {};
    D3D12_DEPTH_STENCIL_DESC   DepthStencilDesc        = {};
    D3D12_BLEND_DESC           BlendStateDesc          = {};
    DXGI_SAMPLE_DESC           SampleDesc              = {};
    D3D12_PIPELINE_STATE_FLAGS PipelineStateFlags      = D3D12_PIPELINE_STATE_FLAG_NONE;

    // ShaderStages
    TArray<FD3D12Shader*> ShadersWithRootSignature;
    TArray<FD3D12Shader*> BaseShaders;

    // AmplificationShader
    {
        if (FD3D12AmplificationShaderRHI* D3D12AmplificationShader = FD3D12DeviceRHI::ResourceCast(Desc.AmplificationShader))
        {
            if (D3D12AmplificationShader->HasRootSignature())
            {
                ShadersWithRootSignature.Emplace(D3D12AmplificationShader);
            }

            AmplificationShaderCode = D3D12AmplificationShader->GetByteCode().GetD3D12Bytecode();
            BaseShaders.Emplace(D3D12AmplificationShader);

            AmplificationShader = MakeSharedRef<FD3D12AmplificationShaderRHI>(D3D12AmplificationShader);
        }
        else
        {
            AmplificationShaderCode.pShaderBytecode = nullptr;
            AmplificationShaderCode.BytecodeLength  = 0;
        }
    }

    // MeshShader
    {
        if (FD3D12MeshShaderRHI* D3D12MeshShader = FD3D12DeviceRHI::ResourceCast(Desc.MeshShader))
        {
            if (D3D12MeshShader->HasRootSignature())
            {
                ShadersWithRootSignature.Emplace(D3D12MeshShader);
            }

            MeshShaderCode = D3D12MeshShader->GetByteCode().GetD3D12Bytecode();
            BaseShaders.Emplace(D3D12MeshShader);

            MeshShader = MakeSharedRef<FD3D12MeshShaderRHI>(D3D12MeshShader);
        }
        else
        {
            D3D12_ERROR_CRITICAL("MeshShader cannot be nullptr");
            return false;
        }
    }

    // PixelShader
    {
        if (FD3D12PixelShaderRHI* D3D12PixelShader = FD3D12DeviceRHI::ResourceCast(Desc.PixelShader))
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

        DepthBufferFormat = ConvertFormat(Desc.RasterizerOutputFormats.DepthStencilFormat);
    }

    // RasterizerState
    FD3D12RasterizerStateRHI* D3D12RasterizerState = FD3D12DeviceRHI::ResourceCast(Desc.RasterizerState);
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
    FD3D12DepthStencilStateRHI* D3D12DepthStencilState = FD3D12DeviceRHI::ResourceCast(Desc.DepthStencilState);
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
    FD3D12BlendStateRHI* D3D12BlendState = FD3D12DeviceRHI::ResourceCast(Desc.BlendState);
    if (D3D12BlendState)
    {
        BlendStateDesc = D3D12BlendState->GetD3D12Desc();
    }
    else
    {
        D3D12_ERROR_CRITICAL("BlendState cannot be nullptr");
        return false;
    }

    // MSAA
    {
        SampleDesc.Count   = Desc.MultiSampleState.SampleCount;
        SampleDesc.Quality = Desc.MultiSampleState.SampleQuality;
    }

    // Shader-flags
    ShaderFlags = ED3D12ShaderFlags::None;
    
    for (FD3D12Shader* Shader : BaseShaders)
    {
        ShaderFlags |= Shader->GetFlags();
    }

    // RootSignature
    {
        if (ShadersWithRootSignature.IsEmpty())
        {
            FD3D12RootSignatureLayout RootSignatureLayout;
            RootSignatureLayout.SetType(ERootSignatureType::Graphics);
            RootSignatureLayout.SetAllowInputAssembler(false);

            uint8 NumPushConstants = 0;
            for (FD3D12Shader* Shader : BaseShaders)
            {
                const EShaderVisibility::Type  Stage       = Shader->GetShaderVisibility();
                const FD3D12ShaderBindingInfo& BindingInfo = Shader->GetBindingInfo();

                for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : BindingInfo.ResourceBindings)
                {
                    RootSignatureLayout.AddRegister(Stage, static_cast<EResourceType::Type>(Binding.BindingType), Binding.OriginalBindingIndex);
                }

                NumPushConstants = Math::Max<uint8>(NumPushConstants, static_cast<uint8>(BindingInfo.NumPushConstants));
            }

            RootSignatureLayout.SetNumPushConstants(NumPushConstants);
            RootSignatureLayout.SetDirectlyIndexedResourceHeap(IsEnumFlagSet(ShaderFlags, ED3D12ShaderFlags::RequiresResourceDescriptorHeapIndexing));
            RootSignatureLayout.SetDirectlyIndexedSamplerHeap(IsEnumFlagSet(ShaderFlags, ED3D12ShaderFlags::RequiresSamplerDescriptorHeapIndexing));

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
                RootSignature->SetDebugName("Custom Meshlet RootSignature");
            }

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
            const EShaderVisibility::Type  Stage       = Shader->GetShaderVisibility();
            const FD3D12ShaderBindingInfo& BindingInfo = Shader->GetBindingInfo();

            const bool bShaderHasRootBindings = !BindingInfo.ResourceBindings.IsEmpty() || BindingInfo.NumPushConstants > 0;
            if (bShaderHasRootBindings && RootSignature->HasDenyFlag(Stage))
            {
                D3D12_ERROR_CRITICAL("Root Signature denies access to stage %u, but shader has %d resource bindings and %u push constants.",
                    Stage, BindingInfo.ResourceBindings.Size(), BindingInfo.NumPushConstants);
            }

            for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : BindingInfo.ResourceBindings)
            {
                const EResourceType::Type ResType = static_cast<EResourceType::Type>(Binding.BindingType);
                if (RootSignature->GetSlotForRegister(Stage, ResType, Binding.OriginalBindingIndex) < 0 &&
                    RootSignature->GetShaderStage(Stage).GetRootDescriptorParameterIndex(ResType, Binding.OriginalBindingIndex) < 0)
                {
                    D3D12_ERROR_CRITICAL("Custom root signature missing register %u (type %u) for shader stage %u",
                        Binding.OriginalBindingIndex, Binding.BindingType, Stage);
                }
            }
        }

        ComputeEffectiveDescriptorCounts(RootSignature.Get(), BaseShaders.Data(), BaseShaders.Size());
    }

    // View Instancing: Validate that the PSO state is compatible with shader requirements
    {
        const bool bShaderRequiresViewID = IsEnumFlagSet(ShaderFlags, ED3D12ShaderFlags::RequiresViewID);
        if (bShaderRequiresViewID && !Desc.ViewInstancingState.bEnableViewInstancing)
        {
            D3D12_ERROR_CRITICAL("Shader uses SV_ViewID but the meshlet PSO was created without view instancing enabled. Set FRHIMeshletPipelineStateDesc::ViewInstancingState.bEnableViewInstancing to true.");
        }
    }

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

    // Dynamic Depth Bias
#if D3D12_ENABLE_DYNAMIC_DEPTH_BIAS
    if (GD3D12SupportDynamicDepthBias && D3D12RasterizerState->GetDesc().bEnableDepthBias)
    {
        PipelineStateFlags = D3D12_PIPELINE_STATE_FLAG_DYNAMIC_DEPTH_BIAS;
    }
#endif

    // Build pipeline key for caching
    FD3D12MeshletPipelineKey PipelineKey;
    Memory::Memzero(&PipelineKey, sizeof(FD3D12MeshletPipelineKey));

    PipelineKey.RootSignatureHash  = RootSignature->GetHash();
    PipelineKey.DepthBufferFormat  = DepthBufferFormat;
    PipelineKey.RenderTargetInfo   = RenderTargetInfo;
    PipelineKey.ViewInstancingHash = ViewInstanceDesc.GenerateHash();
    PipelineKey.RasterizerHash     = D3D12RasterizerState->GetHash();
    PipelineKey.DepthStencilHash   = D3D12DepthStencilState->GetHash();
    PipelineKey.BlendStateHash     = D3D12BlendState->GetHash();
    PipelineKey.SampleDesc         = SampleDesc;

    PipelineKey.ASHash = AmplificationShader ? AmplificationShader->GetHash() : FD3D12ShaderHash();
    PipelineKey.MSHash = MeshShader->GetHash();
    PipelineKey.PSHash = PixelShader ? PixelShader->GetHash() : FD3D12ShaderHash();

    const uint64 PipelineHash = CRC32::Generate(&PipelineKey, sizeof(FD3D12MeshletPipelineKey));
    constexpr uint64 BufferLength = 128;
    WIDECHAR PipelineHashBuffer[BufferLength] = { 0 };
    FPlatformString::Snprintf(PipelineHashBuffer, BufferLength, L"MeshletPSO[%llu]", PipelineHash);

    if (!GD3D12SupportPipelineStream)
    {
        D3D12_ERROR_CRITICAL("[D3D12MeshletPipelineState]: Pipeline stream support (ID3D12Device2) is required for mesh-shader pipelines");
        return false;
    }

    FD3D12MeshletPipelineStream PipelineStream;
    PipelineStream.RootSignature            = RootSignature->GetD3D12RootSignature();
    PipelineStream.AmplificationShaderCode  = AmplificationShaderCode;
    PipelineStream.MeshShaderCode           = MeshShaderCode;
    PipelineStream.PixelShaderCode          = PixelShaderCode;
    PipelineStream.RenderTargetInfo         = RenderTargetInfo;
    PipelineStream.DepthBufferFormat        = DepthBufferFormat;
    PipelineStream.RasterizerDesc           = RasterizerDesc;
    PipelineStream.DepthStencilDesc         = DepthStencilDesc;
    PipelineStream.BlendStateDesc           = BlendStateDesc;
    PipelineStream.SampleDesc               = SampleDesc;
    PipelineStream.PipelineStateFlags       = PipelineStateFlags;
    PipelineStream.ViewInstancingDesc.Flags = ViewInstanceDesc.Flags;

    if (Desc.ViewInstancingState.bEnableViewInstancing)
    {
        PipelineStream.ViewInstancingDesc.pViewInstanceLocations = ViewInstanceDesc.ViewInstanceLocations;
        PipelineStream.ViewInstancingDesc.ViewInstanceCount      = ViewInstanceDesc.ViewInstanceCount;
    }

    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
    Memory::Memzero(&PipelineStreamDesc);

    PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
    PipelineStreamDesc.SizeInBytes                   = sizeof(FD3D12MeshletPipelineStream);

    if (GD3D12SupportPipelineCache)
    {
        FD3D12PipelineStateManager& PipelineStateManager = GetDevice()->GetPipelineStateManager();
        if (PipelineStateManager.CreateMeshletPipeline(PipelineHashBuffer, PipelineStreamDesc, PipelineState))
        {
            STAT_ADD(STAT_D3D12_NumMeshletPipelineStates, 1);
            return true;
        }
    }

#if D3D12_USE_ID3D12DEVICE_2
    TComPtr<ID3D12PipelineState> NewPipelineState;
    HRESULT Result = GetDevice()->GetD3D12Device2()->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&NewPipelineState));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[D3D12MeshletPipelineState]: FAILED to Create MeshletPipelineState");
        return false;
    }

    PipelineState = NewPipelineState;
    STAT_ADD(STAT_D3D12_NumMeshletPipelineStates, 1);
    return true;
#else
    D3D12_ERROR_CRITICAL("[D3D12MeshletPipelineState]: ID3D12Device2 is required for pipeline stream creation");
    return false;
#endif
#else
    UNREFERENCED_VARIABLE(Desc);
    D3D12_ERROR_CRITICAL("[D3D12MeshletPipelineState]: Pipeline stream support is required for mesh-shader pipelines");
    return false;
#endif
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

bool FD3D12PipelineStateManager::CreateMeshletPipeline(const WIDECHAR* PipelineHash, const D3D12_PIPELINE_STATE_STREAM_DESC& PipelineStream, TComPtr<ID3D12PipelineState>& OutPipelineState)
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
            D3D12_ERROR_CRITICAL("Failed to create MeshletPipelineState");
            return false;
        }

        hResult = PipelineLibrary->StorePipeline(PipelineHash, OutPipelineState.Get());
        if (FAILED(hResult))
        {
            D3D12_WARNING("Failed to store MeshletPipelineState");
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

    const String PipelineCacheFilename = CVarPipelineCacheFileName.GetValue();
    const String PipelineCacheFilepath = Paths::GetAssetDir() + '/' + PipelineCacheFilename;

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
        Memory::Memcpy(Header.Magic, "D3D12PSO", sizeof(Header.Magic));

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

    const uint64 CurrentTime    = FPlatformTime::QueryPerformanceCounter();
    const uint64 Frequency      = FPlatformTime::QueryPerformanceFrequency();
    const double ElapsedSeconds = static_cast<double>(CurrentTime - LastSaveTimestamp) / static_cast<double>(Frequency);

    const int32 SaveInterval = CVarPipelineCacheSaveInterval.GetValue();
    if (ElapsedSeconds < static_cast<double>(SaveInterval))
    {
        return;
    }

    const String PipelineCacheFilename = CVarPipelineCacheFileName.GetValue();
    const String PipelineCacheFilepath = Paths::GetAssetDir() + '/' + PipelineCacheFilename;

    TSharedPtr<uint8[]> SerializedData;
    SIZE_T SerializedSize = 0;

    {
        TScopedLock Lock(PipelineLibraryCS);

        SerializedSize = PipelineLibrary->GetSerializedSize();
        SerializedData = MakeSharedPtr<uint8[]>(static_cast<uint32>(SerializedSize));

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
    Memory::Memcpy(Header.Magic, "D3D12PSO", sizeof(Header.Magic));

    Header.DataCRC  = CRC32::Generate(SerializedData.Get(), SerializedSize);
    Header.DataSize = SerializedSize;

    Tasks::Async([FilePath = PipelineCacheFilepath, Header, Data = Move(SerializedData), DataSize = SerializedSize]()
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
    const String PipelineCacheFilename = CVarPipelineCacheFileName.GetValue();
    const String PipelineCacheFilepath = Paths::GetAssetDir() + '/' + PipelineCacheFilename;

    if (!FPlatformFile::IsFile(*PipelineCacheFilepath))
    {
        D3D12_INFO("No PipelineCache-file found at '%s' (expected on first run)", *PipelineCacheFilepath);
        return false;
    }

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

    if (Memory::Memcmp(Header.Magic, "D3D12PSO", sizeof(Header.Magic)) != 0)
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

    PipelineData     = Memory::Malloc(Header.DataSize);
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
        Memory::Free(PipelineData);
        PipelineData = nullptr;
    }

    PipelineDataSize = 0;
}
