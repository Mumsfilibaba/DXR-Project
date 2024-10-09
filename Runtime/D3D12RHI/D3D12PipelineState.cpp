#include "D3D12PipelineState.h"
#include "D3D12RHIShaderCompiler.h"
#include "D3D12Device.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformFile.h"
#include "Project/ProjectManager.h"

static TAutoConsoleVariable<FString> CVarPipelineCacheFileName(
    "D3D12RHI.PipelineCacheFileName",
    "FileName for the file storing the PipelineCache",
    "PipelineCache.d3d12psocache");

FD3D12VertexLayout::FD3D12VertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList)
    : FRHIVertexLayout()
    , SemanticNames()
    , ElementDesc()
    , Desc()
    , Hash(0)
{
    const int32 NumElements = InInitializerList.Size();
    ElementDesc.Reserve(NumElements);
    SemanticNames.Reserve(NumElements);

    uint64 CalculatedHash = 0;
    for (const FVertexElement& Element : InInitializerList)
    {
        D3D12_INPUT_ELEMENT_DESC InputElementDesc;
        FMemory::Memzero(&InputElementDesc, sizeof(D3D12_INPUT_ELEMENT_DESC));

        const FString& Semantic = SemanticNames.Emplace(Element.Semantic);
        InputElementDesc.SemanticName = Semantic.GetCString();
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

    Desc.NumElements        = ElementDesc.Size();
    Desc.pInputElementDescs = ElementDesc.Data();
    Hash                    = CalculatedHash;
}

FD3D12VertexLayout::~FD3D12VertexLayout()
{
}

FD3D12DepthStencilState::FD3D12DepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer)
    : FRHIDepthStencilState()
    , Initializer(InInitializer)
    , Hash(0)
{
    FMemory::Memzero(&Desc);

    Desc.DepthFunc        = ConvertComparisonFunc(InInitializer.DepthFunc);
    Desc.DepthEnable      = InInitializer.bDepthEnable;
    Desc.DepthWriteMask   = InInitializer.bDepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    Desc.StencilEnable    = InInitializer.bStencilEnable;
    Desc.StencilReadMask  = static_cast<uint8>(InInitializer.StencilReadMask);
    Desc.StencilWriteMask = static_cast<uint8>(InInitializer.StencilWriteMask);
    Desc.FrontFace        = ConvertStencilState(InInitializer.FrontFace);
    Desc.BackFace         = ConvertStencilState(InInitializer.BackFace);

    Hash = FCRC32::Generate(&Desc, sizeof(D3D12_DEPTH_STENCIL_DESC));
}

FD3D12DepthStencilState::~FD3D12DepthStencilState()
{
}

FD3D12RasterizerState::FD3D12RasterizerState(const FRHIRasterizerStateInitializer& InInitializer)
    : FRHIRasterizerState()
    , Initializer(InInitializer)
    , Hash(0)
{
    FMemory::Memzero(&Desc);

    Desc.AntialiasedLineEnable = InInitializer.bAntialiasedLineEnable;
    Desc.CullMode              = ConvertCullMode(InInitializer.CullMode);
    Desc.DepthBias             = static_cast<int32>(InInitializer.DepthBias);
    Desc.DepthBiasClamp        = InInitializer.DepthBiasClamp;
    Desc.DepthClipEnable       = InInitializer.bDepthClipEnable;
    Desc.SlopeScaledDepthBias  = InInitializer.SlopeScaledDepthBias;
    Desc.FillMode              = ConvertFillMode(InInitializer.FillMode);
    Desc.ForcedSampleCount     = InInitializer.ForcedSampleCount;
    Desc.FrontCounterClockwise = InInitializer.bFrontCounterClockwise;
    Desc.MultisampleEnable     = InInitializer.bMultisampleEnable;
    Desc.ConservativeRaster    = InInitializer.bEnableConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    Hash = FCRC32::Generate(&Desc, sizeof(D3D12_RASTERIZER_DESC));
}

FD3D12RasterizerState::~FD3D12RasterizerState()
{
}

FD3D12BlendState::FD3D12BlendState(const FRHIBlendStateInitializer& InInitializer)
    : FRHIBlendState()
    , Initializer(InInitializer)
    , Hash(0)
{
    FMemory::Memzero(&Desc);

    const D3D12_LOGIC_OP LogicOp = ConvertLogicOp(InInitializer.LogicOp);
    Desc.AlphaToCoverageEnable   = InInitializer.bAlphaToCoverageEnable;
    Desc.IndependentBlendEnable  = InInitializer.bIndependentBlendEnable;

    for (int32 Index = 0; Index < InInitializer.NumRenderTargets; Index++)
    {
        Desc.RenderTarget[Index].BlendEnable           = InInitializer.RenderTargets[Index].bBlendEnable;
        Desc.RenderTarget[Index].BlendOp               = ConvertBlendOp(InInitializer.RenderTargets[Index].BlendOp);
        Desc.RenderTarget[Index].BlendOpAlpha          = ConvertBlendOp(InInitializer.RenderTargets[Index].BlendOpAlpha);
        Desc.RenderTarget[Index].DestBlend             = ConvertBlend(InInitializer.RenderTargets[Index].DstBlend);
        Desc.RenderTarget[Index].DestBlendAlpha        = ConvertBlend(InInitializer.RenderTargets[Index].DstBlendAlpha);
        Desc.RenderTarget[Index].SrcBlend              = ConvertBlend(InInitializer.RenderTargets[Index].SrcBlend);
        Desc.RenderTarget[Index].SrcBlendAlpha         = ConvertBlend(InInitializer.RenderTargets[Index].SrcBlendAlpha);
        Desc.RenderTarget[Index].RenderTargetWriteMask = ConvertColorWriteFlags(InInitializer.RenderTargets[Index].ColorWriteMask);
        Desc.RenderTarget[Index].LogicOp               = LogicOp;
        Desc.RenderTarget[Index].LogicOpEnable         = InInitializer.bLogicOpEnable;
    }

    Hash = FCRC32::Generate(&Desc, sizeof(D3D12_BLEND_DESC));
}

FD3D12BlendState::~FD3D12BlendState()
{
}

FD3D12PipelineStateCommon::FD3D12PipelineStateCommon(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
{
}

void FD3D12PipelineStateCommon::SetDebugName(const FString& InName)
{
    const FStringWide WideName = CharToWide(InName);
    PipelineState->SetName(WideName.GetCString());
    DebugName = InName;
}

FD3D12GraphicsPipelineState::FD3D12GraphicsPipelineState(FD3D12Device* InDevice)
    : FRHIGraphicsPipelineState()
    , FD3D12PipelineStateCommon(InDevice)
{
}

FD3D12GraphicsPipelineState::~FD3D12GraphicsPipelineState()
{
}

bool FD3D12GraphicsPipelineState::Initialize(const FRHIGraphicsPipelineStateInitializer& Initializer)
{
    FD3D12GraphicsPipelineStream PipelineStream;

    // InputLayout
    FD3D12VertexLayout* D3D12InputLayoutState = static_cast<FD3D12VertexLayout*>(Initializer.VertexInputLayout);
    if (D3D12InputLayoutState)
    {
        PipelineStream.InputLayout = D3D12InputLayoutState->GetDesc();
    }
    else
    {
        PipelineStream.InputLayout.pInputElementDescs = nullptr;
        PipelineStream.InputLayout.NumElements = 0;
    }

    // ShaderStages
    TArray<FD3D12Shader*> ShadersWithRootSignature;
    TArray<FD3D12Shader*> BaseShaders;

    // VertexShader
    {
        if (FD3D12VertexShader* D3D12VertexShader = static_cast<FD3D12VertexShader*>(Initializer.ShaderState.VertexShader))
        {
            if (D3D12VertexShader->HasRootSignature())
            {
                ShadersWithRootSignature.Emplace(D3D12VertexShader);
            }

            D3D12_SHADER_BYTECODE& VertexShaderCode = PipelineStream.VertexShaderCode;
            VertexShaderCode = D3D12VertexShader->GetByteCode();
            BaseShaders.Emplace(D3D12VertexShader);
            VertexShader = MakeSharedRef<FD3D12VertexShader>(D3D12VertexShader);
        }
        else
        {
            D3D12_ERROR("VertexShader cannot be nullptr");
            return false;
        }
    }

    // HullShader
    {
        D3D12_SHADER_BYTECODE& HullShaderCode = PipelineStream.HullShaderCode;
        if (FD3D12HullShader* D3D12HullShader = static_cast<FD3D12HullShader*>(Initializer.ShaderState.HullShader))
        {
            if (D3D12HullShader->HasRootSignature())
            {
                ShadersWithRootSignature.Emplace(D3D12HullShader);
            }

            HullShaderCode = D3D12HullShader->GetByteCode();
            BaseShaders.Emplace(D3D12HullShader);
            HullShader = MakeSharedRef<FD3D12HullShader>(D3D12HullShader);
        }
        else
        {
            HullShaderCode.pShaderBytecode = nullptr;
            HullShaderCode.BytecodeLength  = 0;
        }
    }

    // DomainShader
    {
        D3D12_SHADER_BYTECODE& DomainShaderCode = PipelineStream.DomainShaderCode;
        if (FD3D12DomainShader* D3D12DomainShader = static_cast<FD3D12DomainShader*>(Initializer.ShaderState.DomainShader))
        {
            if (D3D12DomainShader->HasRootSignature())
            {
                ShadersWithRootSignature.Emplace(D3D12DomainShader);
            }

            DomainShaderCode = D3D12DomainShader->GetByteCode();
            BaseShaders.Emplace(D3D12DomainShader);
            DomainShader = MakeSharedRef<FD3D12DomainShader>(D3D12DomainShader);
        }
        else
        {
            DomainShaderCode.pShaderBytecode = nullptr;
            DomainShaderCode.BytecodeLength  = 0;
        }
    }

    // GeometryShader
    {
        D3D12_SHADER_BYTECODE& GeometryShaderCode = PipelineStream.GeometryShaderCode;
        if (FD3D12GeometryShader* D3D12GeometryShader = static_cast<FD3D12GeometryShader*>(Initializer.ShaderState.GeometryShader))
        {
            if (D3D12GeometryShader->HasRootSignature())
            {
                ShadersWithRootSignature.Emplace(D3D12GeometryShader);
            }

            GeometryShaderCode = D3D12GeometryShader->GetByteCode();
            BaseShaders.Emplace(D3D12GeometryShader);
            GeometryShader = MakeSharedRef<FD3D12GeometryShader>(D3D12GeometryShader);
        }
        else
        {
            GeometryShaderCode.pShaderBytecode = nullptr;
            GeometryShaderCode.BytecodeLength  = 0;
        }
    }

    // PixelShader
    {
        D3D12_SHADER_BYTECODE& PixelShaderCode = PipelineStream.PixelShaderCode;
        if (FD3D12PixelShader* D3D12PixelShader = static_cast<FD3D12PixelShader*>(Initializer.ShaderState.PixelShader))
        {
            if (D3D12PixelShader->HasRootSignature())
            {
                ShadersWithRootSignature.Emplace(D3D12PixelShader);
            }

            PixelShaderCode = D3D12PixelShader->GetByteCode();
            BaseShaders.Emplace(D3D12PixelShader);
            PixelShader = MakeSharedRef<FD3D12PixelShader>(D3D12PixelShader);
        }
        else
        {
            PixelShaderCode.pShaderBytecode = nullptr;
            PixelShaderCode.BytecodeLength  = 0;
        }
    }

    // RenderTarget
    {
        D3D12_RT_FORMAT_ARRAY& RenderTargetInfo = PipelineStream.RenderTargetInfo;
        RenderTargetInfo.NumRenderTargets = Initializer.PipelineFormats.NumRenderTargets;

        for (uint32 Index = 0; Index < RenderTargetInfo.NumRenderTargets; Index++)
        {
            RenderTargetInfo.RTFormats[Index] = ConvertFormat(Initializer.PipelineFormats.RenderTargetFormats[Index]);
        }

        // DepthStencil
        PipelineStream.DepthBufferFormat = ConvertFormat(Initializer.PipelineFormats.DepthStencilFormat);
    }

    // RasterizerState
    FD3D12RasterizerState* D3D12RasterizerState = static_cast<FD3D12RasterizerState*>(Initializer.RasterizerState);
    if (D3D12RasterizerState)
    {
        D3D12_RASTERIZER_DESC& RasterizerDesc = PipelineStream.RasterizerDesc;
        RasterizerDesc = D3D12RasterizerState->GetD3D12Desc();
    }
    else
    {
        D3D12_ERROR("RasterizerState cannot be nullptr");
        return false;
    }

    // DepthStencilState
    FD3D12DepthStencilState* D3D12DepthStencilState = static_cast<FD3D12DepthStencilState*>(Initializer.DepthStencilState);
    if (D3D12DepthStencilState)
    {
        D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc = PipelineStream.DepthStencilDesc;
        DepthStencilDesc = D3D12DepthStencilState->GetD3D12Desc();
    }
    else
    {
        D3D12_ERROR("DepthStencilState cannot be nullptr");
        return false;
    }

    // BlendState
    FD3D12BlendState* D3D12BlendState = static_cast<FD3D12BlendState*>(Initializer.BlendState);
    if (D3D12BlendState)
    {
        D3D12_BLEND_DESC& BlendStateDesc = PipelineStream.BlendStateDesc;
        BlendStateDesc = D3D12BlendState->GetD3D12Desc();
    }
    else
    {
        D3D12_ERROR("BlendState cannot be nullptr");
        return false;
    }

    // Topology
    {
        PipelineStream.PrimitiveTopologyType = ConvertPrimitiveTopologyType(Initializer.PrimitiveTopology);
        PrimitiveTopology = ConvertPrimitiveTopology(Initializer.PrimitiveTopology);
    }

    // IndexBufferStripCutValue
    {
        PipelineStream.IndexBufferStripCutValue = Initializer.bPrimitiveRestartEnable ? D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF : D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    }

    // MSAA
    {
        DXGI_SAMPLE_DESC& SamplerDesc = PipelineStream.SampleDesc;
        SamplerDesc.Count   = Initializer.SampleCount;
        SamplerDesc.Quality = Initializer.SampleQuality;
    }

    // RootSignature
    {
        if (ShadersWithRootSignature.IsEmpty())
        {
            FD3D12RootSignatureLayout RootSignatureLayout;
            RootSignatureLayout.Type                 = ERootSignatureType::Graphics;
            RootSignatureLayout.bAllowInputAssembler = D3D12InputLayoutState ? true : false;

            // NOTE: For now all constants are put in visibility_all
            uint8 Num32BitConstants = 0;
            for (FD3D12Shader* Shader : BaseShaders)
            {
                const uint32 Index = Shader->GetShaderVisibility();
                RootSignatureLayout.ResourceCounts[Index] = Shader->GetResourceCount();
                Num32BitConstants = FMath::Max<uint8>(RootSignatureLayout.ResourceCounts[Index].Num32BitConstants, Num32BitConstants);
                RootSignatureLayout.ResourceCounts[Index].Num32BitConstants = 0;
            }

            RootSignatureLayout.ResourceCounts[ShaderVisibility_All].Num32BitConstants = Num32BitConstants;

            FD3D12RootSignatureManager& RootSignatureManager = GetDevice()->GetRootSignatureManager();
            RootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(RootSignatureLayout));
        }
        else
        {
            // TODO: Maybe use all shaders and create one that fits all
            D3D12_SHADER_BYTECODE ByteCode = ShadersWithRootSignature.FirstElement()->GetByteCode();

            RootSignature = new FD3D12RootSignature(GetDevice());
            if (!RootSignature->Initialize(ByteCode.pShaderBytecode, ByteCode.BytecodeLength))
            {
                return false;
            }
            else
            {
                RootSignature->SetDebugName("Custom Graphics RootSignature");
            }
        }

        CHECK(RootSignature != nullptr);
        PipelineStream.RootSignature = RootSignature->GetD3D12RootSignature();
    }

    // View Instancing
    FD3D12HashableViewInstanceDesc ViewInstanceDesc;
    PipelineStream.ViewInstancingDesc.Flags = ViewInstanceDesc.Flags;

    if (Initializer.ViewInstancingInfo.NumArraySlices)
    {
        ViewInstanceDesc.ViewInstanceCount = FMath::Min<uint32>(Initializer.ViewInstancingInfo.NumArraySlices, D3D12_MAX_VIEW_INSTANCE_COUNT);
        for (uint32 Index = 0; Index < ViewInstanceDesc.ViewInstanceCount; Index++)
        {
            // NOTE: This does not work on NVIDIA for some reason, only way to work around this is by using the SV_RenderTargetArrayIndex
            ViewInstanceDesc.ViewInstanceLocations[Index].RenderTargetArrayIndex = Initializer.ViewInstancingInfo.StartRenderTargetArrayIndex;
            ViewInstanceDesc.ViewInstanceLocations[Index].ViewportArrayIndex = 0;
        }

        PipelineStream.ViewInstancingDesc.pViewInstanceLocations = ViewInstanceDesc.ViewInstanceLocations;
        PipelineStream.ViewInstancingDesc.ViewInstanceCount = ViewInstanceDesc.ViewInstanceCount;
    }
    else
    {
        PipelineStream.ViewInstancingDesc.pViewInstanceLocations = nullptr;
        PipelineStream.ViewInstancingDesc.ViewInstanceCount = 0;
    }

    // Create pipeline-state
    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
    FMemory::Memzero(&PipelineStreamDesc);

    PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
    PipelineStreamDesc.SizeInBytes                   = sizeof(FD3D12GraphicsPipelineStream);

    if (GD3D12SupportPipelineCache)
    {
        FD3D12GraphicsPipelineKey PipelineKey;
        FMemory::Memzero(&PipelineKey, sizeof(FD3D12GraphicsPipelineKey));

        PipelineKey.RootSignatureHash        = RootSignature->GetHash();
        PipelineKey.PrimitiveTopologyType    = PipelineStream.PrimitiveTopologyType;
        PipelineKey.IndexBufferStripCutValue = PipelineStream.IndexBufferStripCutValue;
        PipelineKey.DepthBufferFormat        = PipelineStream.DepthBufferFormat;
        PipelineKey.RenderTargetInfo         = PipelineStream.RenderTargetInfo;
        PipelineKey.ViewInstancingHash       = ViewInstanceDesc.GenerateHash();
        PipelineKey.InputLayoutHash          = D3D12InputLayoutState ? D3D12InputLayoutState->GetHash() : 0;
        PipelineKey.RasterizerHash           = D3D12RasterizerState->GetHash();
        PipelineKey.DepthStencilHash         = D3D12DepthStencilState->GetHash();
        PipelineKey.BlendStateHash           = D3D12BlendState->GetHash();
        PipelineKey.SampleDesc               = PipelineStream.SampleDesc;

        // Get Hashes for Shaders
        PipelineKey.VSHash = VertexShader->GetHash();
        PipelineKey.HSHash = HullShader     ? HullShader->GetHash()     : FD3D12ShaderHash();
        PipelineKey.DSHash = DomainShader   ? DomainShader->GetHash()   : FD3D12ShaderHash();
        PipelineKey.GSHash = GeometryShader ? GeometryShader->GetHash() : FD3D12ShaderHash();
        PipelineKey.PSHash = PixelShader    ? PixelShader->GetHash()    : FD3D12ShaderHash();

        // Generate a PipelineLibrary name
        const uint64 Hash = FCRC32::Generate(&PipelineKey, sizeof(FD3D12GraphicsPipelineKey));
        constexpr uint64 BufferLength = 128;
        WIDECHAR Buffer[BufferLength] = { 0 };
        FPlatformString::Snprintf(Buffer, BufferLength, L"GraphicsPSO[%llu]", Hash);

        // Cache the PipelineState
        FD3D12PipelineStateManager& PipelineStateManager = GetDevice()->GetPipelineStateManager();
        if (PipelineStateManager.CreateGraphicsPipeline(Buffer, PipelineStreamDesc, PipelineState))
        {
            return true;
        }
    }

    TComPtr<ID3D12PipelineState> NewPipelineState;
    HRESULT Result = GetDevice()->GetD3D12Device2()->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&NewPipelineState));
    if (FAILED(Result))
    {
        D3D12_ERROR("[D3D12GraphicsPipelineState]: FAILED to Create GraphicsPipelineState");
        return false;
    }

    PipelineState = NewPipelineState;
    return true;
}

FD3D12ComputePipelineState::FD3D12ComputePipelineState(FD3D12Device* InDevice, const TSharedRef<FD3D12ComputeShader>& InShader)
    : FRHIComputePipelineState()
    , FD3D12PipelineStateCommon(InDevice)
    , Shader(InShader)
{
}

FD3D12ComputePipelineState::~FD3D12ComputePipelineState()
{
}

bool FD3D12ComputePipelineState::Initialize()
{
    FD3D12ComputePipelineStream PipelineStream;
    PipelineStream.ComputeShader = Shader->GetByteCode();

    if (!Shader->HasRootSignature())
    {
        FD3D12RootSignatureLayout ResourceCounts;
        ResourceCounts.Type                                 = ERootSignatureType::Compute;
        ResourceCounts.bAllowInputAssembler                 = false;
        ResourceCounts.ResourceCounts[ShaderVisibility_All] = Shader->GetResourceCount();

        FD3D12RootSignatureManager& RootSignatureManager = GetDevice()->GetRootSignatureManager();
        RootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(ResourceCounts));
    }
    else
    {
        D3D12_SHADER_BYTECODE ByteCode = Shader->GetByteCode();

        RootSignature = new FD3D12RootSignature(GetDevice());
        if (!RootSignature->Initialize(ByteCode.pShaderBytecode, ByteCode.BytecodeLength))
        {
            return false;
        }
        else
        {
            RootSignature->SetDebugName("Custom Compute RootSignature");
        }
    }

    CHECK(RootSignature != nullptr);
    PipelineStream.RootSignature = RootSignature->GetD3D12RootSignature();

    // Create PipelineState
    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
    FMemory::Memzero(&PipelineStreamDesc);

    PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
    PipelineStreamDesc.SizeInBytes                   = sizeof(FD3D12ComputePipelineStream);

    if (GD3D12SupportPipelineCache)
    {
        FD3D12ComputePipelineKey PipelineKey;
        FMemory::Memzero(&PipelineKey, sizeof(FD3D12ComputePipelineKey));

        PipelineKey.CSHash            = Shader->GetHash();
        PipelineKey.RootSignatureHash = RootSignature->GetHash();

        // Generate a PipelineLibrary name
        const uint64 Hash = FCRC32::Generate(&PipelineKey, sizeof(FD3D12ComputePipelineKey));
        constexpr uint64 BufferLength = 64;
        WIDECHAR Buffer[BufferLength] = { 0 };
        FPlatformString::Snprintf(Buffer, BufferLength, L"ComputePSO[%llu]", Hash);

        // Cache the PipelineState
        FD3D12PipelineStateManager& PipelineStateManager = GetDevice()->GetPipelineStateManager();
        if (PipelineStateManager.CreateComputePipeline(Buffer, PipelineStreamDesc, PipelineState))
        {
            return true;
        }
    }

    HRESULT Result = GetDevice()->GetD3D12Device2()->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&PipelineState));
    if (FAILED(Result))
    {
        D3D12_ERROR("[D3D12ComputePipelineState]: FAILED to Create ComputePipelineState");
        return false;
    }

    return true;
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
            ShaderExportNamesRef[i] = ShaderExportNames[i].GetCString();
        }
    }

    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ExportAssociation;

    ID3D12RootSignature* RootSignature;

    TArray<FStringWide> ShaderExportNames;
    TArray<LPCWSTR> ShaderExportNamesRef;
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
        Desc.HitGroupExport         = HitGroupName.GetCString();
        Desc.ClosestHitShaderImport = ClosestHit.GetCString();

        if (AnyHit != L"")
        {
            Desc.AnyHitShaderImport = AnyHit.GetCString();
        }

        if (Desc.Type != D3D12_HIT_GROUP_TYPE_TRIANGLES)
        {
            Desc.IntersectionShaderImport = Intersection.GetCString();
        }
    }

    D3D12_HIT_GROUP_DESC Desc;

    FStringWide HitGroupName;
    FStringWide ClosestHit;
    FStringWide AnyHit;
    FStringWide Intersection;
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
            TempDesc.Name           = ExportNames[i].GetCString();
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
            PayLoadExportNamesRef[i] = PayLoadExportNames[i].GetCString();
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

FD3D12RayTracingPipelineState::FD3D12RayTracingPipelineState(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , StateObject(nullptr)
{
}

FD3D12RayTracingPipelineState::~FD3D12RayTracingPipelineState()
{
}

bool FD3D12RayTracingPipelineState::Initialize(const FRHIRayTracingPipelineStateInitializer& Initializer)
{
    FD3D12RayTracingPipelineStateStream PipelineStream;
    TArray<FD3D12Shader*> Shaders;

    FD3D12RootSignatureManager& RootSignatureManager = GetDevice()->GetRootSignatureManager();

    // Collect and add all RayGen-Shaders
    for (FRHIRayGenShader* RayGen : Initializer.RayGenShaders)
    {
        FD3D12RayGenShader* D3D12RayGen = static_cast<FD3D12RayGenShader*>(RayGen);
        Shaders.Emplace(D3D12RayGen);

        FD3D12RootSignatureLayout RayGenLocalResourceCounts;
        RayGenLocalResourceCounts.Type                                 = ERootSignatureType::RayTracingLocal;
        RayGenLocalResourceCounts.bAllowInputAssembler                  = false;
        RayGenLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = D3D12RayGen->GetRTLocalResourceCount();

        HitLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(RayGenLocalResourceCounts));
        if (!HitLocalRootSignature)
        {
            return false;
        }

        FStringWide RayGenIdentifier = CharToWide(D3D12RayGen->GetIdentifier());
        PipelineStream.AddLibrary(D3D12RayGen->GetByteCode(), { RayGenIdentifier });
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

    for (const FRHIRayTracingHitGroupInfo& HitGroup : Initializer.HitGroups)
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
        FD3D12RayAnyHitShader* D3D12AnyHit = static_cast<FD3D12RayAnyHitShader*>(AnyHit);
        Shaders.Emplace(D3D12AnyHit);

        FD3D12RootSignatureLayout AnyHitLocalResourceCounts;
        AnyHitLocalResourceCounts.Type                                 = ERootSignatureType::RayTracingLocal;
        AnyHitLocalResourceCounts.bAllowInputAssembler                  = false;
        AnyHitLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = D3D12AnyHit->GetRTLocalResourceCount();

        HitLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(AnyHitLocalResourceCounts));
        if (!HitLocalRootSignature)
        {
            return false;
        }

        FStringWide AnyHitIdentifier = CharToWide(D3D12AnyHit->GetIdentifier());
        PipelineStream.AddLibrary(D3D12AnyHit->GetByteCode(), { AnyHitIdentifier });
        PipelineStream.AddRootSignatureAssociation(HitLocalRootSignature->GetD3D12RootSignature(), { AnyHitIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(AnyHitIdentifier);
    }

    // Collect and add all ClosestHit shaders
    for (FRHIRayClosestHitShader* ClosestHit : ClosestHitShaders)
    {
        FD3D12RayClosestHitShader* D3D12ClosestHit = static_cast<FD3D12RayClosestHitShader*>(ClosestHit);
        Shaders.Emplace(D3D12ClosestHit);

        FD3D12RootSignatureLayout ClosestHitLocalResourceCounts;
        ClosestHitLocalResourceCounts.Type                                 = ERootSignatureType::RayTracingLocal;
        ClosestHitLocalResourceCounts.bAllowInputAssembler                  = false;
        ClosestHitLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = D3D12ClosestHit->GetRTLocalResourceCount();

        HitLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(ClosestHitLocalResourceCounts));
        if (!HitLocalRootSignature)
        {
            return false;
        }

        FStringWide ClosestHitIdentifier = CharToWide(D3D12ClosestHit->GetIdentifier());
        PipelineStream.AddLibrary(D3D12ClosestHit->GetByteCode(), { ClosestHitIdentifier });
        PipelineStream.AddRootSignatureAssociation(HitLocalRootSignature->GetD3D12RootSignature(), { ClosestHitIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(ClosestHitIdentifier);
    }

    // Collect and add all Miss shaders
    for (FRHIRayMissShader* Miss : Initializer.MissShaders)
    {
        FD3D12RayMissShader* D3D12MissShader = static_cast<FD3D12RayMissShader*>(Miss);
        Shaders.Emplace(D3D12MissShader);

        FD3D12RootSignatureLayout MissLocalResourceCounts;
        MissLocalResourceCounts.Type                                 = ERootSignatureType::RayTracingLocal;
        MissLocalResourceCounts.bAllowInputAssembler                  = false;
        MissLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = D3D12MissShader->GetRTLocalResourceCount();

        MissLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(MissLocalResourceCounts));
        if (!MissLocalRootSignature)
        {
            return false;
        }

        FStringWide MissIdentifier = CharToWide(D3D12MissShader->GetIdentifier());
        PipelineStream.AddLibrary(D3D12MissShader->GetByteCode(), { MissIdentifier });
        PipelineStream.AddRootSignatureAssociation(MissLocalRootSignature->GetD3D12RootSignature(), { MissIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(MissIdentifier);
    }

    PipelineStream.ShaderConfig.MaxAttributeSizeInBytes  = Initializer.MaxAttributeSizeInBytes;
    PipelineStream.ShaderConfig.MaxPayloadSizeInBytes    = Initializer.MaxPayloadSizeInBytes;
    PipelineStream.PipelineConfig.MaxTraceRecursionDepth = Initializer.MaxRecursionDepth;

    FShaderResourceCount CombinedResourceCount;
    for (FD3D12Shader* Shader : Shaders)
    {
        CHECK(Shader != nullptr);
        CombinedResourceCount.Combine(Shader->GetResourceCount());
    }

    FD3D12RootSignatureLayout GlobalResourceCounts;
    GlobalResourceCounts.Type                                 = ERootSignatureType::RayTracingGlobal;
    GlobalResourceCounts.bAllowInputAssembler                  = false;
    GlobalResourceCounts.ResourceCounts[ShaderVisibility_All] = CombinedResourceCount;

    GlobalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(GlobalResourceCounts));
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
        D3D12_ERROR("[D3D12RayTracingPipelineState] Failed to retrieve ID3D12StateObjectProperties");
        return false;
    }

    StateObject           = TempStateObject;
    StateObjectProperties = TempStateObjectProperties;
    return true;
}

void* FD3D12RayTracingPipelineState::GetShaderIdentifer(const FString& ExportName)
{
    if (FD3D12RayTracingShaderIdentifer* MapItem = ShaderIdentifers.Find(ExportName))
    {
        return MapItem->ShaderIdentifier;
    }
    else
    {
        FStringWide WideExportName = CharToWide(ExportName);

        void* Result = StateObjectProperties->GetShaderIdentifier(WideExportName.GetCString());
        if (!Result)
        {
            return nullptr;
        }

        FD3D12RayTracingShaderIdentifer Identifier;
        FMemory::Memcpy(Identifier.ShaderIdentifier, Result, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

        FD3D12RayTracingShaderIdentifer& NewIdentifier = ShaderIdentifers.Add(ExportName, Identifier);
        return NewIdentifier.ShaderIdentifier;
    }
}

FD3D12PipelineStateManager::FD3D12PipelineStateManager(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , PipelineData(nullptr)
    , PipelineDataSize(0)
    , PipelineLibrary(nullptr)
    , bPipelineLibraryDirty(false)
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
        hResult = GetDevice()->GetD3D12Device2()->CreatePipelineState(&PipelineStream, IID_PPV_ARGS(&OutPipelineState));
        if (FAILED(hResult))
        {
            D3D12_ERROR("Failed to create GraphicsPipelineState");
            return false;
        }

        hResult = PipelineLibrary->StorePipeline(PipelineHash, OutPipelineState.Get());
        if (FAILED(hResult))
        {
            D3D12_ERROR("Failed to store GraphicsPipelineState");
            return false;
        }

        bPipelineLibraryDirty = true;
    }

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
        hResult = GetDevice()->GetD3D12Device2()->CreatePipelineState(&PipelineStream, IID_PPV_ARGS(&OutPipelineState));
        if (FAILED(hResult))
        {
            D3D12_ERROR("Failed to create ComputePipelineState");
            return false;
        }

        hResult = PipelineLibrary->StorePipeline(PipelineHash, OutPipelineState.Get());
        if (FAILED(hResult))
        {
            D3D12_ERROR("Failed to store ComputePipelineState");
            return false;
        }

        bPipelineLibraryDirty = true;
    }

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
    const FString PipelineCacheFilepath = FString(FProjectManager::Get().GetAssetPath()) + '/' + PipelineCacheFilename;

    FFileHandleRef CacheFile = FPlatformFile::OpenForWrite(PipelineCacheFilepath);
    if (!CacheFile)
    {
        D3D12_WARNING("Failed to open PipelineCache-file");
        return false;
    }

    {
        TScopedLock Lock(PipelineLibraryCS);

        SIZE_T PipelineCacheSize = PipelineLibrary->GetSerializedSize();
        TUniquePtr<uint8[]> PipelineCacheData = MakeUnique<uint8[]>(PipelineCacheSize);
        HRESULT hResult = PipelineLibrary->Serialize(PipelineCacheData.Get(), PipelineCacheSize);
        if (FAILED(hResult))
        {
            D3D12_ERROR("Failed to serielize PipelineCache");
            return false;
        }

        FD3D12PipelineDiskHeader Header;
        FMemory::Memcpy(Header.Magic, "D3D12PSO", sizeof(Header.Magic));

        Header.DataCRC  = FCRC32::Generate(PipelineCacheData.Get(), PipelineCacheSize);
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
            D3D12_INFO("Saved PipelineCache to file '%s'", PipelineCacheFilepath.GetCString());
        }
    }

    bPipelineLibraryDirty = false;
    return true;
}

bool FD3D12PipelineStateManager::LoadCacheFromFile()
{
    const FString PipelineCacheFilename = CVarPipelineCacheFileName.GetValue();
    const FString PipelineCacheFilepath = FString(FProjectManager::Get().GetAssetPath()) + '/' + PipelineCacheFilename;
    
    FFileHandleRef CacheFile = FPlatformFile::OpenForRead(PipelineCacheFilepath);
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
    
    const uint32 DataCRC = FCRC32::Generate(PipelineData, PipelineDataSize);
    if (DataCRC != Header.DataCRC)
    {
        D3D12_WARNING("PipelineCacheData is invalid");
        return false;
    }
    
    ID3D12Device1* Device1 = GetDevice()->GetD3D12Device1();
    if (!Device1)
    {
        D3D12_WARNING("ID3D12Device1 is not supported, PipelineCache not supported");
        return false;
    }

    HRESULT hResult = Device1->CreatePipelineLibrary(PipelineData, PipelineDataSize, IID_PPV_ARGS(&PipelineLibrary));
    if (FAILED(hResult))
    {
        D3D12_ERROR("Failed to create PipelineLibrary");
        return false;
    }

    return true;
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
