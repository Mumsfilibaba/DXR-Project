#include "D3D12PipelineState.h"

D3D12GraphicsPipelineState::D3D12GraphicsPipelineState(D3D12Device* InDevice, const TRef<D3D12RootSignature>& InRootSignature)
    : D3D12DeviceChild(InDevice)
    , PipelineState(nullptr)
    , RootSignature(InRootSignature)
{
}

Bool D3D12GraphicsPipelineState::Init(const GraphicsPipelineStateCreateInfo& CreateInfo)
{
    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT) GraphicsPipelineStream
    {
        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type0 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
            ID3D12RootSignature* RootSignature = nullptr;
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT;
            D3D12_INPUT_LAYOUT_DESC InputLayout = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type2 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type3 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS;
            D3D12_SHADER_BYTECODE VertexShader = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type4 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
            D3D12_SHADER_BYTECODE PixelShader = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type5 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
            D3D12_RT_FORMAT_ARRAY RenderTargetInfo = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type6 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
            DXGI_FORMAT DepthBufferFormat = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type7 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
            D3D12_RASTERIZER_DESC RasterizerDesc = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type8 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
            D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type9 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
            D3D12_BLEND_DESC BlendStateDesc = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type10 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC;
            DXGI_SAMPLE_DESC SampleDesc = { };
        };
    } PipelineStream;

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC& InputLayoutDesc = PipelineStream.InputLayout;

    D3D12InputLayoutState* DxInputLayoutState = static_cast<D3D12InputLayoutState*>(CreateInfo.InputLayoutState);
    if (!DxInputLayoutState)
    {
        InputLayoutDesc.pInputElementDescs = nullptr;
        InputLayoutDesc.NumElements        = 0;
    }
    else
    {
        InputLayoutDesc = DxInputLayoutState->GetDesc();
    }

    // VertexShader
    D3D12VertexShader* DxVertexShader = static_cast<D3D12VertexShader*>(CreateInfo.ShaderState.VertexShader);
    Assert(DxVertexShader != nullptr);

    D3D12_SHADER_BYTECODE& VertexShader = PipelineStream.VertexShader;
    VertexShader = DxVertexShader->GetShaderByteCode();

    // PixelShader
    D3D12PixelShader* DxPixelShader = static_cast<D3D12PixelShader*>(CreateInfo.ShaderState.PixelShader);

    D3D12_SHADER_BYTECODE& PixelShader = PipelineStream.PixelShader;
    if (!DxPixelShader)
    {
        PixelShader.pShaderBytecode = nullptr;
        PixelShader.BytecodeLength  = 0;
    }
    else
    {
        PixelShader = DxPixelShader->GetShaderByteCode();
    }

    // RenderTarget
    const UInt32 NumRenderTargets = CreateInfo.PipelineFormats.NumRenderTargets;

    D3D12_RT_FORMAT_ARRAY& RenderTargetInfo = PipelineStream.RenderTargetInfo;
    RenderTargetInfo.NumRenderTargets = NumRenderTargets;
    for (UInt32 Index = 0; Index < NumRenderTargets; Index++)
    {
        RenderTargetInfo.RTFormats[Index] = ConvertFormat(CreateInfo.PipelineFormats.RenderTargetFormats[Index]);
    }

    // DepthStencil
    PipelineStream.DepthBufferFormat = ConvertFormat(CreateInfo.PipelineFormats.DepthStencilFormat);

    // RasterizerState
    D3D12RasterizerState* DxRasterizerState = static_cast<D3D12RasterizerState*>(CreateInfo.RasterizerState);
    Assert(DxRasterizerState != nullptr);

    D3D12_RASTERIZER_DESC& RasterizerDesc = PipelineStream.RasterizerDesc;
    RasterizerDesc = DxRasterizerState->GetDesc();

    // DepthStencilState
    D3D12DepthStencilState* DxDepthStencilState = static_cast<D3D12DepthStencilState*>(CreateInfo.DepthStencilState);
    Assert(DxDepthStencilState != nullptr);

    D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc = PipelineStream.DepthStencilDesc;
    DepthStencilDesc = DxDepthStencilState->GetDesc();

    // BlendState
    D3D12BlendState* DxBlendState = static_cast<D3D12BlendState*>(CreateInfo.BlendState);
    Assert(DxBlendState != nullptr);

    D3D12_BLEND_DESC& BlendStateDesc = PipelineStream.BlendStateDesc;
    BlendStateDesc = DxBlendState->GetDesc();

    // Topology
    PipelineStream.PrimitiveTopologyType = ConvertPrimitiveTopologyType(CreateInfo.PrimitiveTopologyType);

    // MSAA
    DXGI_SAMPLE_DESC& SamplerDesc = PipelineStream.SampleDesc;
    SamplerDesc.Count   = CreateInfo.SampleCount;
    SamplerDesc.Quality = CreateInfo.SampleQuality;

    // RootSignature
    PipelineStream.RootSignature = RootSignature->GetRootSignature();
    
    // Create PipelineState
    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
    Memory::Memzero(&PipelineStreamDesc);

    PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
    PipelineStreamDesc.SizeInBytes                   = sizeof(GraphicsPipelineStream);

    TComPtr<ID3D12PipelineState> NewPipelineState;
    HRESULT hResult = Device->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&NewPipelineState));
    if (SUCCEEDED(hResult))
    {
        PipelineState = NewPipelineState;
        return true;
    }
    else
    {
        return false;
    }
}

D3D12ComputePipelineState::D3D12ComputePipelineState(D3D12Device* InDevice, const TRef<D3D12ComputeShader>& InShader, const TRef<D3D12RootSignature>& InRootSignature)
    : ComputePipelineState()
    , D3D12DeviceChild(InDevice)
    , PipelineState(nullptr)
    , Shader(InShader)
    , RootSignature(InRootSignature)
{
}

Bool D3D12ComputePipelineState::Init()
{
    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT) ComputePipelineStream
    {
        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type0 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
            ID3D12RootSignature* RootSignature = nullptr;
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS;
            D3D12_SHADER_BYTECODE ComputeShader = { };
        };
    } PipelineStream;

    PipelineStream.ComputeShader = Shader->GetShaderByteCode();
    PipelineStream.RootSignature = RootSignature->GetRootSignature();

    // Create PipelineState
    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
    Memory::Memzero(&PipelineStreamDesc, sizeof(D3D12_PIPELINE_STATE_STREAM_DESC));

    PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
    PipelineStreamDesc.SizeInBytes                   = sizeof(ComputePipelineStream);

    HRESULT hResult = Device->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&PipelineState));
    if (SUCCEEDED(hResult))
    {
        LOG_INFO("[D3D12RenderLayer]: Created ComputePipelineState");
        return true;
    }
    else
    {
        LOG_ERROR("[D3D12RenderLayer]: FAILED to Create ComputePipelineState");
        return false;
    }
}

struct D3D12RootSignatureAssociation
{
    D3D12RootSignatureAssociation(const TComPtr<ID3D12RootSignature>& InRootSignature, const TArray<std::wstring>& InShaderExportNames)
        : ExportAssociation()
        , RootSignature(InRootSignature)
        , ShaderExportNames(InShaderExportNames)
        , ShaderExportNamesRef(InShaderExportNames.Size())
    {
        for (UInt32 i = 0; i < ShaderExportNames.Size(); i++)
        {
            ShaderExportNamesRef[i] = ShaderExportNames[i].c_str();
        }
    }

    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ExportAssociation;
    TComPtr<ID3D12RootSignature>           RootSignature;
    TArray<std::wstring> ShaderExportNames;
    TArray<LPCWSTR>      ShaderExportNamesRef;
};

struct D3D12HitGroup
{
    D3D12HitGroup(std::wstring InHitGroupName, std::wstring InClosestHit, std::wstring InAnyHit, std::wstring InIntersection)
        : Desc()
        , HitGroupName(InHitGroupName)
        , ClosestHit(InClosestHit)
        , AnyHit(InAnyHit)
        , Intersection(InIntersection)
    {
        Memory::Memzero(&Desc);

        Desc.Type                   = D3D12_HIT_GROUP_TYPE_TRIANGLES;
        Desc.HitGroupExport         = HitGroupName.c_str();
        Desc.ClosestHitShaderImport = ClosestHit.c_str();

        if (AnyHit != L"")
        {
            Desc.AnyHitShaderImport = AnyHit.c_str();
        }

        if (Desc.Type != D3D12_HIT_GROUP_TYPE_TRIANGLES )
        {
            Desc.IntersectionShaderImport = Intersection.c_str();
        }
    }

    D3D12_HIT_GROUP_DESC Desc;
    std::wstring HitGroupName;
    std::wstring ClosestHit;
    std::wstring AnyHit;
    std::wstring Intersection;
};

struct D3D12Library
{
    D3D12Library(D3D12_SHADER_BYTECODE ByteCode, const TArray<std::wstring>& InExportNames)
        : ExportNames(InExportNames)
        , ExportDescs(InExportNames.Size())
        , Desc()
    {
        for (UInt32 i = 0; i < ExportDescs.Size(); i++)
        {
            D3D12_EXPORT_DESC& TempDesc = ExportDescs[i];
            TempDesc.Flags          = D3D12_EXPORT_FLAG_NONE;
            TempDesc.Name           = ExportNames[i].c_str();
            TempDesc.ExportToRename = nullptr;
        }

        Desc.DXILLibrary = ByteCode;
        Desc.pExports    = ExportDescs.Data();
        Desc.NumExports  = ExportDescs.Size();
    }

    TArray<std::wstring>      ExportNames;
    TArray<D3D12_EXPORT_DESC> ExportDescs;
    D3D12_DXIL_LIBRARY_DESC   Desc;
};

class D3D12RayTracingPipelineStateStream
{
public:
    D3D12RayTracingPipelineStateStream()  = default;
    ~D3D12RayTracingPipelineStateStream() = default;

    void AddLibrary(D3D12_SHADER_BYTECODE ByteCode, const TArray<std::wstring>& ExportNames)
    {
        Libraries.EmplaceBack(ByteCode, ExportNames);
    }

    void AddHitGroup(std::wstring HitGroupName, std::wstring ClosestHit, std::wstring AnyHit, std::wstring Intersection)
    {
        HitGroups.EmplaceBack(HitGroupName, ClosestHit, AnyHit, Intersection);
    }

    void AddRootSignatureAssociation(const TComPtr<ID3D12RootSignature>& RootSignature, const TArray<std::wstring>& ShaderExportNames)
    {
        RootSignatureAssociations.EmplaceBack(RootSignature, ShaderExportNames);
    }

    void Generate()
    {
        UInt32 NumSubObjects = Libraries.Size() + HitGroups.Size() + (RootSignatureAssociations.Size() * 2) + 4;
        SubObjects.Resize(NumSubObjects);

        UInt32 SubObjectIndex = 0;
        for (D3D12Library& Lib : Libraries)
        {
            D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
            SubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            SubObject.pDesc = &Lib.Desc;
        }

        for (D3D12HitGroup& HitGroup : HitGroups)
        {
            D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
            SubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            SubObject.pDesc = &HitGroup.Desc;
        }

        for (D3D12RootSignatureAssociation& Association : RootSignatureAssociations)
        {
            D3D12_STATE_SUBOBJECT& LocalRootSubObject = SubObjects[SubObjectIndex++];
            LocalRootSubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
            LocalRootSubObject.pDesc = Association.RootSignature.GetAddressOf();

            Association.ExportAssociation.pExports   = Association.ShaderExportNamesRef.Data();
            Association.ExportAssociation.NumExports = Association.ShaderExportNamesRef.Size();
            Association.ExportAssociation.pSubobjectToAssociate = &SubObjects[SubObjectIndex - 1];

            D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
            SubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            SubObject.pDesc = &Association.ExportAssociation;
        }

        D3D12_STATE_SUBOBJECT& GlobalRootSubObject = SubObjects[SubObjectIndex++];
        GlobalRootSubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        GlobalRootSubObject.pDesc = GlobalRootSignature.GetAddressOf();

        D3D12_STATE_SUBOBJECT& PipelineConfigSubObject = SubObjects[SubObjectIndex++];
        PipelineConfigSubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        PipelineConfigSubObject.pDesc = &PipelineConfig;

        D3D12_STATE_SUBOBJECT& ShaderConfigObject = SubObjects[SubObjectIndex++];
        ShaderConfigObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        ShaderConfigObject.pDesc = &ShaderConfig;

        PayLoadExportNamesRef.Resize(PayLoadExportNames.Size());
        for (UInt32 i = 0; i < PayLoadExportNames.Size(); i++)
        {
            PayLoadExportNamesRef[i] = PayLoadExportNames[i].c_str();
        }

        ShaderConfigAssociation.pExports   = PayLoadExportNamesRef.Data();
        ShaderConfigAssociation.NumExports = PayLoadExportNamesRef.Size();
        ShaderConfigAssociation.pSubobjectToAssociate = &SubObjects[SubObjectIndex-1];

        D3D12_STATE_SUBOBJECT& ShaderConfigAssociationSubObject = SubObjects[SubObjectIndex++];
        ShaderConfigAssociationSubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        ShaderConfigAssociationSubObject.pDesc = &ShaderConfigAssociation;
    }

    TArray<D3D12Library>  Libraries;
    TArray<D3D12HitGroup> HitGroups;
    TArray<D3D12RootSignatureAssociation> RootSignatureAssociations;

    D3D12_RAYTRACING_PIPELINE_CONFIG       PipelineConfig;
    D3D12_RAYTRACING_SHADER_CONFIG         ShaderConfig;
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ShaderConfigAssociation;

    TArray<std::wstring> PayLoadExportNames;
    TArray<LPCWSTR>      PayLoadExportNamesRef;

    TComPtr<ID3D12RootSignature>  GlobalRootSignature;
    TArray<D3D12_STATE_SUBOBJECT> SubObjects;
};

D3D12RayTracingPipelineState::D3D12RayTracingPipelineState(D3D12Device* InDevice)
    : D3D12DeviceChild(InDevice)
    , StateObject(nullptr)
{
}

Bool D3D12RayTracingPipelineState::Init(const RayTracingPipelineStateCreateInfo& CreateInfo, const D3D12DefaultRootSignatures& DefaultRootSignatures)
{
    D3D12RayTracingPipelineStateStream PipelineStream;
    
    // TODO: Do not call converttowide for all
    D3D12RayGenShader* RayGen = static_cast<D3D12RayGenShader*>(CreateInfo.RayGen);
    PipelineStream.AddLibrary(RayGen->GetShaderByteCode(), { ConvertToWide(RayGen->Identifier) });
    PipelineStream.AddRootSignatureAssociation(DefaultRootSignatures.LocalRayGen->RootSignature, { ConvertToWide(RayGen->Identifier) });
    PipelineStream.PayLoadExportNames.EmplaceBack(ConvertToWide(RayGen->Identifier));

    for (const RayTracingHitGroup& HitGroup : CreateInfo.HitGroups)
    {
        D3D12RayAnyhitShader*     DxAnyHit     = static_cast<D3D12RayAnyhitShader*>(HitGroup.AnyHit);
        D3D12RayClosestHitShader* DxClosestHit = static_cast<D3D12RayClosestHitShader*>(HitGroup.ClosestHit);
        
        Assert(DxClosestHit != nullptr);

        PipelineStream.AddHitGroup(ConvertToWide(HitGroup.Name), ConvertToWide(DxClosestHit->Identifier), DxAnyHit ? ConvertToWide(DxAnyHit->Identifier) : L"", L"");
    }

    for (RayAnyHitShader* AnyHit : CreateInfo.AnyHitShaders)
    {
        D3D12RayAnyhitShader* DxAnyHit = static_cast<D3D12RayAnyhitShader*>(AnyHit);
        PipelineStream.AddLibrary(DxAnyHit->GetShaderByteCode(), { ConvertToWide(DxAnyHit->Identifier) });
        PipelineStream.AddRootSignatureAssociation(DefaultRootSignatures.LocalRayHit->RootSignature, { ConvertToWide(DxAnyHit->Identifier) });
        PipelineStream.PayLoadExportNames.EmplaceBack(ConvertToWide(DxAnyHit->Identifier));
    }

    for (RayClosestHitShader* ClosestHit : CreateInfo.ClosestHitShaders)
    {
        D3D12RayClosestHitShader* DxClosestHit = static_cast<D3D12RayClosestHitShader*>(ClosestHit);
        PipelineStream.AddLibrary(DxClosestHit->GetShaderByteCode(), { ConvertToWide(DxClosestHit->Identifier) });
        PipelineStream.AddRootSignatureAssociation(DefaultRootSignatures.LocalRayHit->RootSignature, { ConvertToWide(DxClosestHit->Identifier) });
        PipelineStream.PayLoadExportNames.EmplaceBack(ConvertToWide(DxClosestHit->Identifier));
    }

    for (RayMissShader* Miss : CreateInfo.MissShaders)
    {
        D3D12RayMissShader* DxMiss = static_cast<D3D12RayMissShader*>(Miss);
        PipelineStream.AddLibrary(DxMiss->GetShaderByteCode(), { ConvertToWide(DxMiss->Identifier) });
        PipelineStream.AddRootSignatureAssociation(DefaultRootSignatures.LocalRayMiss->RootSignature, { ConvertToWide(DxMiss->Identifier) });
        PipelineStream.PayLoadExportNames.EmplaceBack(ConvertToWide(DxMiss->Identifier));
    }

    PipelineStream.ShaderConfig.MaxAttributeSizeInBytes  = CreateInfo.MaxAttributeSizeInBytes;
    PipelineStream.ShaderConfig.MaxPayloadSizeInBytes    = CreateInfo.MaxPayloadSizeInBytes;
    PipelineStream.PipelineConfig.MaxTraceRecursionDepth = CreateInfo.MaxRecursionDepth;
    PipelineStream.GlobalRootSignature                   = DefaultRootSignatures.GlobalRayGen->RootSignature;

    PipelineStream.Generate();

    D3D12_STATE_OBJECT_DESC RayTracingPipeline;
    Memory::Memzero(&RayTracingPipeline);

    RayTracingPipeline.Type          = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    RayTracingPipeline.pSubobjects   = PipelineStream.SubObjects.Data();
    RayTracingPipeline.NumSubobjects = PipelineStream.SubObjects.Size();

    TComPtr<ID3D12StateObject> TempStateObject;
    HRESULT Result = Device->GetDXRDevice()->CreateStateObject(&RayTracingPipeline, IID_PPV_ARGS(&TempStateObject));
    if (FAILED(Result))
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        StateObject = TempStateObject;
    }
    
    TComPtr<ID3D12StateObjectProperties> TempStateObjectProperties;
    Result = StateObject->QueryInterface(IID_PPV_ARGS(&TempStateObjectProperties));
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12RayTracingPipelineState] Failed to retrive ID3D12StateObjectProperties");
        return false;
    }
    else
    {
        StateObjectProperties = TempStateObjectProperties;
    }

    return true;
}