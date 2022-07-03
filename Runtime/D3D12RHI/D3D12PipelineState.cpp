#include "D3D12RHIShaderCompiler.h"
#include "D3D12PipelineState.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12GraphicsPipelineState 

FD3D12GraphicsPipelineState::FD3D12GraphicsPipelineState(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , PipelineState(nullptr)
    , RootSignature(nullptr)
{ }

bool FD3D12GraphicsPipelineState::Init(const FRHIGraphicsPipelineStateInitializer& Initializer)
{
    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT) FD3D12GraphicsPipelineStream
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

    D3D12_INPUT_LAYOUT_DESC& InputLayoutDesc = PipelineStream.InputLayout;

    FD3D12VertexInputLayout* D3D12InputLayoutState = static_cast<FD3D12VertexInputLayout*>(Initializer.VertexInputLayout);
    if (!D3D12InputLayoutState)
    {
        InputLayoutDesc.pInputElementDescs = nullptr;
        InputLayoutDesc.NumElements        = 0;
    }
    else
    {
        InputLayoutDesc = D3D12InputLayoutState->GetDesc();
    }

    TArray<FD3D12Shader*> ShadersWithRootSignature;
    TArray<FD3D12Shader*> BaseShaders;

    // VertexShader
    FD3D12VertexShader* D3D12VertexShader = static_cast<FD3D12VertexShader*>(Initializer.ShaderState.VertexShader);
    Check(D3D12VertexShader != nullptr);

    if (D3D12VertexShader->HasRootSignature())
    {
        ShadersWithRootSignature.Emplace(D3D12VertexShader);
    }

    D3D12_SHADER_BYTECODE& VertexShader = PipelineStream.VertexShader;
    VertexShader = D3D12VertexShader->GetByteCode();
    BaseShaders.Emplace(D3D12VertexShader);

    // PixelShader
    FD3D12PixelShader* D3D12PixelShader = static_cast<FD3D12PixelShader*>(Initializer.ShaderState.PixelShader);

    D3D12_SHADER_BYTECODE& PixelShader = PipelineStream.PixelShader;
    if (D3D12PixelShader)
    {
        PixelShader = D3D12PixelShader->GetByteCode();
        BaseShaders.Emplace(D3D12PixelShader);

        if (D3D12PixelShader->HasRootSignature())
        {
            ShadersWithRootSignature.Emplace(D3D12PixelShader);
        }
    }
    else
    {
        PixelShader.pShaderBytecode = nullptr;
        PixelShader.BytecodeLength  = 0;
    }

    // RenderTarget
    const uint32 NumRenderTargets = Initializer.PipelineFormats.NumRenderTargets;

    D3D12_RT_FORMAT_ARRAY& RenderTargetInfo = PipelineStream.RenderTargetInfo;
    RenderTargetInfo.NumRenderTargets = NumRenderTargets;

    for (uint32 Index = 0; Index < NumRenderTargets; Index++)
    {
        RenderTargetInfo.RTFormats[Index] = ConvertFormat(Initializer.PipelineFormats.RenderTargetFormats[Index]);
    }

    // DepthStencil
    PipelineStream.DepthBufferFormat = ConvertFormat(Initializer.PipelineFormats.DepthStencilFormat);

    // RasterizerState
    FD3D12RasterizerState* D3D12RasterizerState = static_cast<FD3D12RasterizerState*>(Initializer.RasterizerState);
    Check(D3D12RasterizerState != nullptr);

    D3D12_RASTERIZER_DESC& RasterizerDesc = PipelineStream.RasterizerDesc;
    RasterizerDesc = D3D12RasterizerState->GetDesc();

    // DepthStencilState
    FD3D12DepthStencilState* D3D12DepthStencilState = static_cast<FD3D12DepthStencilState*>(Initializer.DepthStencilState);
    Check(D3D12DepthStencilState != nullptr);

    D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc = PipelineStream.DepthStencilDesc;
    DepthStencilDesc = D3D12DepthStencilState->GetDesc();

    // BlendState
    FD3D12BlendState* D3D12BlendState = static_cast<FD3D12BlendState*>(Initializer.BlendState);
    Check(D3D12BlendState != nullptr);

    D3D12_BLEND_DESC& BlendStateDesc = PipelineStream.BlendStateDesc;
    BlendStateDesc = D3D12BlendState->GetDesc();

    // Topology
    PipelineStream.PrimitiveTopologyType = ConvertPrimitiveTopologyType(Initializer.PrimitiveTopologyType);

    // MSAA
    DXGI_SAMPLE_DESC& SamplerDesc = PipelineStream.SampleDesc;
    SamplerDesc.Count   = Initializer.SampleCount;
    SamplerDesc.Quality = Initializer.SampleQuality;

    // RootSignature
    if (ShadersWithRootSignature.IsEmpty())
    {
        FD3D12RootSignatureResourceCount ResourceCounts;
        ResourceCounts.Type = ERootSignatureType::Graphics;
        // TODO: Check if any shader actually uses the input assembler
        ResourceCounts.AllowInputAssembler = true;

        // NOTE: For now all constants are put in visibility_all
        uint32 Num32BitConstants = 0;
        for (FD3D12Shader* DxShader : BaseShaders)
        {
            uint32 Index = DxShader->GetShaderVisibility();
            ResourceCounts.ResourceCounts[Index]                   = DxShader->GetResourceCount();
            ResourceCounts.ResourceCounts[Index].Num32BitConstants = 0;
            Num32BitConstants = NMath::Max<uint32>(Num32BitConstants,ResourceCounts.ResourceCounts[Index].Num32BitConstants);
        }

        ResourceCounts.ResourceCounts[ShaderVisibility_All].Num32BitConstants = Num32BitConstants;

        FD3D12RootSignatureCache* RootSignatureCache = GetDevice()->GetRootSignatureCache();
        RootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureCache->GetOrCreateRootSignature(ResourceCounts));
    }
    else
    {
        // TODO: Maybe use all shaders and create one that fits all
        D3D12_SHADER_BYTECODE ByteCode = ShadersWithRootSignature.FirstElement()->GetByteCode();

        RootSignature = dbg_new FD3D12RootSignature(GetDevice());
        if (!RootSignature->Initialize(ByteCode.pShaderBytecode, ByteCode.BytecodeLength))
        {
            return false;
        }
        else
        {
            RootSignature->SetName("Custom Graphics RootSignature");
        }
    }

    Check(RootSignature != nullptr);

    PipelineStream.RootSignature = RootSignature->GetRootSignature();

    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
    FMemory::Memzero(&PipelineStreamDesc);

    PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
    PipelineStreamDesc.SizeInBytes                   = sizeof(FD3D12GraphicsPipelineStream);

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ComputePipelineState

FD3D12ComputePipelineState::FD3D12ComputePipelineState(FD3D12Device* InDevice, const TSharedRef<FD3D12ComputeShader>& InShader)
    : FRHIComputePipelineState()
    , FD3D12DeviceChild(InDevice)
    , PipelineState(nullptr)
    , Shader(InShader)
    , RootSignature(nullptr)
{ }

bool FD3D12ComputePipelineState::Init()
{
    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT) SComputePipelineStream
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

    PipelineStream.ComputeShader = Shader->GetByteCode();

    if (!Shader->HasRootSignature())
    {
        FD3D12RootSignatureResourceCount ResourceCounts;
        ResourceCounts.Type                                 = ERootSignatureType::Compute;
        ResourceCounts.AllowInputAssembler                  = false;
        ResourceCounts.ResourceCounts[ShaderVisibility_All] = Shader->GetResourceCount();

        FD3D12RootSignatureCache* RootSignatureCache = GetDevice()->GetRootSignatureCache();
        RootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureCache->GetOrCreateRootSignature(ResourceCounts));
    }
    else
    {
        D3D12_SHADER_BYTECODE ByteCode = Shader->GetByteCode();

        RootSignature = dbg_new FD3D12RootSignature(GetDevice());
        if (!RootSignature->Initialize(ByteCode.pShaderBytecode, ByteCode.BytecodeLength))
        {
            return false;
        }
        else
        {
            RootSignature->SetName("Custom Compute RootSignature");
        }
    }

    Check(RootSignature != nullptr);

    PipelineStream.RootSignature = RootSignature->GetRootSignature();

    // Create PipelineState
    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
    FMemory::Memzero(&PipelineStreamDesc);

    PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
    PipelineStreamDesc.SizeInBytes                   = sizeof(SComputePipelineStream);

    HRESULT Result = GetDevice()->GetD3D12Device2()->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&PipelineState));
    if (FAILED(Result))
    {
        D3D12_ERROR("[D3D12ComputePipelineState]: FAILED to Create ComputePipelineState");
        return false;
    }

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RootSignatureAssociation

struct FD3D12RootSignatureAssociation
{
    FD3D12RootSignatureAssociation(ID3D12RootSignature* InRootSignature, const TArray<WString>& InShaderExportNames)
        : ExportAssociation()
        , RootSignature(InRootSignature)
        , ShaderExportNames(InShaderExportNames)
        , ShaderExportNamesRef(InShaderExportNames.Size())
    {
        for (int32 i = 0; i < ShaderExportNames.Size(); i++)
        {
            ShaderExportNamesRef[i] = ShaderExportNames[i].CStr();
        }
    }

    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ExportAssociation;

    ID3D12RootSignature* RootSignature;

    TArray<WString> ShaderExportNames;
    TArray<LPCWSTR> ShaderExportNamesRef;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12HitGroup

struct FD3D12HitGroup
{
    FD3D12HitGroup(const WString& InHitGroupName, const WString& InClosestHit, const WString& InAnyHit, const WString& InIntersection)
        : Desc()
        , HitGroupName(InHitGroupName)
        , ClosestHit(InClosestHit)
        , AnyHit(InAnyHit)
        , Intersection(InIntersection)
    {
        FMemory::Memzero(&Desc);

        Desc.Type                   = D3D12_HIT_GROUP_TYPE_TRIANGLES;
        Desc.HitGroupExport         = HitGroupName.CStr();
        Desc.ClosestHitShaderImport = ClosestHit.CStr();

        if (AnyHit != L"")
        {
            Desc.AnyHitShaderImport = AnyHit.CStr();
        }

        if (Desc.Type != D3D12_HIT_GROUP_TYPE_TRIANGLES)
        {
            Desc.IntersectionShaderImport = Intersection.CStr();
        }
    }

    D3D12_HIT_GROUP_DESC Desc;

    WString HitGroupName;
    WString ClosestHit;
    WString AnyHit;
    WString Intersection;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Library

struct FD3D12Library
{
    FD3D12Library(D3D12_SHADER_BYTECODE ByteCode, const TArray<WString>& InExportNames)
        : ExportNames(InExportNames)
        , ExportDescs(InExportNames.Size())
        , Desc()
    {
        for (int32 i = 0; i < ExportDescs.Size(); i++)
        {
            D3D12_EXPORT_DESC& TempDesc = ExportDescs[i];
            TempDesc.Flags          = D3D12_EXPORT_FLAG_NONE;
            TempDesc.Name           = ExportNames[i].CStr();
            TempDesc.ExportToRename = nullptr;
        }

        Desc.DXILLibrary = ByteCode;
        Desc.pExports    = ExportDescs.Data();
        Desc.NumExports  = ExportDescs.Size();
    }

    TArray<WString>           ExportNames;
    TArray<D3D12_EXPORT_DESC> ExportDescs;
    D3D12_DXIL_LIBRARY_DESC   Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RayTracingPipelineStateStream

struct FD3D12RayTracingPipelineStateStream
{
    void AddLibrary(D3D12_SHADER_BYTECODE ByteCode, const TArray<WString>& ExportNames)
    {
        Libraries.Emplace(ByteCode, ExportNames);
    }

    void AddHitGroup(const WString& HitGroupName, const WString& ClosestHit, const WString& AnyHit, const WString& Intersection)
    {
        HitGroups.Emplace(HitGroupName, ClosestHit, AnyHit, Intersection);
    }

    void AddRootSignatureAssociation(ID3D12RootSignature* RootSignature, const TArray<WString>& ShaderExportNames)
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
            PayLoadExportNamesRef[i] = PayLoadExportNames[i].CStr();
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

    TArray<WString>                        PayLoadExportNames;
    TArray<LPCWSTR>                        PayLoadExportNamesRef;

    ID3D12RootSignature*                   GlobalRootSignature;
    TArray<D3D12_STATE_SUBOBJECT>          SubObjects;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RayTracingPipelineState

FD3D12RayTracingPipelineState::FD3D12RayTracingPipelineState(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , StateObject(nullptr)
{ }

bool FD3D12RayTracingPipelineState::Init(const FRHIRayTracingPipelineStateInitializer& Initializer)
{
    FD3D12RayTracingPipelineStateStream PipelineStream;
    TArray<FD3D12Shader*> Shaders;

    FD3D12RootSignatureCache* RootSignatureCache = GetDevice()->GetRootSignatureCache();
    Check(RootSignatureCache != nullptr);

    // Collect and add all RayGen-Shaders
    for (FRHIRayGenShader* RayGen : Initializer.RayGenShaders)
    {
        FD3D12RayGenShader* D3D12RayGen = static_cast<FD3D12RayGenShader*>(RayGen);
        Shaders.Emplace(D3D12RayGen);

        FD3D12RootSignatureResourceCount RayGenLocalResourceCounts;
        RayGenLocalResourceCounts.Type                                 = ERootSignatureType::RayTracingLocal;
        RayGenLocalResourceCounts.AllowInputAssembler                  = false;
        RayGenLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = D3D12RayGen->GetRTLocalResourceCount();

        HitLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureCache->GetOrCreateRootSignature(RayGenLocalResourceCounts));
        if (!HitLocalRootSignature)
        {
            return false;
        }

        WString RayGenIdentifier = CharToWide(D3D12RayGen->GetIdentifier());
        PipelineStream.AddLibrary(D3D12RayGen->GetByteCode(), { RayGenIdentifier });
        PipelineStream.AddRootSignatureAssociation(RayGenLocalRootSignature->GetRootSignature(), { RayGenIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(RayGenIdentifier);
    }

    // Collect and add all HitGroups
    WString HitGroupName;
    WString ClosestHitName;
    WString AnyHitName;
    WString IntersectionName;

    TArray<FRHIRayAnyHitShader*>     AnyHitShaders;
    TArray<FRHIRayClosestHitShader*> ClosestHitShaders;

    for (const FRHIRayTracingHitGroupInitializer& HitGroup : Initializer.HitGroups)
    {
        HitGroupName.Clear();
        ClosestHitName.Clear();
        AnyHitName.Clear();
        IntersectionName.Clear();

        for (FRHIRayTracingShader* HitGroupShader : HitGroup.Shaders)
        {
            FD3D12RayTracingShader* D3D12HitGroupShader = D3D12RayTracingShaderCast(HitGroupShader);
            if (HitGroupShader->GetShaderStage() == EShaderStage::RayClosestHit)
            {
                // TODO: Not the greatest way to handle this
                Check(ClosestHitName.IsEmpty());
                ClosestHitName = CharToWide(D3D12HitGroupShader->GetIdentifier());
                ClosestHitShaders.Emplace(static_cast<FRHIRayClosestHitShader*>(HitGroupShader));
            }
            else if (HitGroupShader->GetShaderStage() == EShaderStage::RayAnyHit)
            {
                // TODO: Not the greatest way to handle this
                Check(AnyHitName.IsEmpty());

                AnyHitName = CharToWide(D3D12HitGroupShader->GetIdentifier());
                AnyHitShaders.Emplace(static_cast<FRHIRayAnyHitShader*>(HitGroupShader));
            }
            else if (HitGroupShader->GetShaderStage() == EShaderStage::RayIntersection)
            {
                // TODO: Not the greatest way to handle this
                Check(IntersectionName.IsEmpty());
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

        FD3D12RootSignatureResourceCount AnyHitLocalResourceCounts;
        AnyHitLocalResourceCounts.Type                                 = ERootSignatureType::RayTracingLocal;
        AnyHitLocalResourceCounts.AllowInputAssembler                  = false;
        AnyHitLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = D3D12AnyHit->GetRTLocalResourceCount();

        HitLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureCache->GetOrCreateRootSignature(AnyHitLocalResourceCounts));
        if (!HitLocalRootSignature)
        {
            return false;
        }

        WString AnyHitIdentifier = CharToWide(D3D12AnyHit->GetIdentifier());
        PipelineStream.AddLibrary(D3D12AnyHit->GetByteCode(), { AnyHitIdentifier });
        PipelineStream.AddRootSignatureAssociation(HitLocalRootSignature->GetRootSignature(), { AnyHitIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(AnyHitIdentifier);
    }

    // Collect and add all ClosestHit shaders
    for (FRHIRayClosestHitShader* ClosestHit : ClosestHitShaders)
    {
        FD3D12RayClosestHitShader* D3D12ClosestHit = static_cast<FD3D12RayClosestHitShader*>(ClosestHit);
        Shaders.Emplace(D3D12ClosestHit);

        FD3D12RootSignatureResourceCount ClosestHitLocalResourceCounts;
        ClosestHitLocalResourceCounts.Type                                 = ERootSignatureType::RayTracingLocal;
        ClosestHitLocalResourceCounts.AllowInputAssembler                  = false;
        ClosestHitLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = D3D12ClosestHit->GetRTLocalResourceCount();

        HitLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureCache->GetOrCreateRootSignature(ClosestHitLocalResourceCounts));
        if (!HitLocalRootSignature)
        {
            return false;
        }

        WString ClosestHitIdentifier = CharToWide(D3D12ClosestHit->GetIdentifier());
        PipelineStream.AddLibrary(D3D12ClosestHit->GetByteCode(), { ClosestHitIdentifier });
        PipelineStream.AddRootSignatureAssociation(HitLocalRootSignature->GetRootSignature(), { ClosestHitIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(ClosestHitIdentifier);
    }

    // Collect and add all Miss shaders
    for (FRHIRayMissShader* Miss : Initializer.MissShaders)
    {
        FD3D12RayMissShader* D3D12MissShader = static_cast<FD3D12RayMissShader*>(Miss);
        Shaders.Emplace(D3D12MissShader);

        FD3D12RootSignatureResourceCount MissLocalResourceCounts;
        MissLocalResourceCounts.Type                                 = ERootSignatureType::RayTracingLocal;
        MissLocalResourceCounts.AllowInputAssembler                  = false;
        MissLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = D3D12MissShader->GetRTLocalResourceCount();

        MissLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureCache->GetOrCreateRootSignature(MissLocalResourceCounts));
        if (!MissLocalRootSignature)
        {
            return false;
        }

        WString MissIdentifier = CharToWide(D3D12MissShader->GetIdentifier());
        PipelineStream.AddLibrary(D3D12MissShader->GetByteCode(), { MissIdentifier });
        PipelineStream.AddRootSignatureAssociation(MissLocalRootSignature->GetRootSignature(), { MissIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(MissIdentifier);
    }

    PipelineStream.ShaderConfig.MaxAttributeSizeInBytes  = Initializer.MaxAttributeSizeInBytes;
    PipelineStream.ShaderConfig.MaxPayloadSizeInBytes    = Initializer.MaxPayloadSizeInBytes;
    PipelineStream.PipelineConfig.MaxTraceRecursionDepth = Initializer.MaxRecursionDepth;

    FShaderResourceCount CombinedResourceCount;
    for (FD3D12Shader* Shader : Shaders)
    {
        Check(Shader != nullptr);
        CombinedResourceCount.Combine(Shader->GetResourceCount());
    }

    FD3D12RootSignatureResourceCount GlobalResourceCounts;
    GlobalResourceCounts.Type                                 = ERootSignatureType::RayTracingGlobal;
    GlobalResourceCounts.AllowInputAssembler                  = false;
    GlobalResourceCounts.ResourceCounts[ShaderVisibility_All] = CombinedResourceCount;

    GlobalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureCache->GetOrCreateRootSignature(GlobalResourceCounts));
    if (!GlobalRootSignature)
    {
        return false;
    }

    PipelineStream.GlobalRootSignature = GlobalRootSignature->GetRootSignature();

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
        FDebug::DebugBreak();
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
    const auto MapItem = ShaderIdentifers.find(ExportName);
    if (MapItem == ShaderIdentifers.end())
    {
        WString WideExportName = CharToWide(ExportName);

        void* Result = StateObjectProperties->GetShaderIdentifier(WideExportName.CStr());
        if (!Result)
        {
            return nullptr;
        }

        FRayTracingShaderIdentifer Identifier;
        FMemory::Memcpy(Identifier.ShaderIdentifier, Result, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

        const auto NewIdentifier = ShaderIdentifers.insert(std::make_pair(ExportName, Identifier));
        return NewIdentifier.first->second.ShaderIdentifier;
    }
    else
    {
        return MapItem->second.ShaderIdentifier;
    }
}
