#include "D3D12ShaderCompiler.h"
#include "D3D12RHIPipelineState.h"

CD3D12RHIGraphicsPipelineState::CD3D12RHIGraphicsPipelineState( CD3D12Device* InDevice )
    : CD3D12DeviceChild( InDevice )
    , PipelineState( nullptr )
    , RootSignature( nullptr )
{
}

bool CD3D12RHIGraphicsPipelineState::Init( const SGraphicsPipelineStateCreateInfo& CreateInfo )
{
    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT) SGraphicsPipelineStream
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

    CD3D12RHIInputLayoutState* DxInputLayoutState = static_cast<CD3D12RHIInputLayoutState*>(CreateInfo.InputLayoutState);
    if ( !DxInputLayoutState )
    {
        InputLayoutDesc.pInputElementDescs = nullptr;
        InputLayoutDesc.NumElements = 0;
    }
    else
    {
        InputLayoutDesc = DxInputLayoutState->GetDesc();
    }

    TArray<CD3D12BaseShader*> ShadersWithRootSignature;
    TArray<CD3D12BaseShader*> BaseShaders;

    // VertexShader
    CD3D12RHIVertexShader* DxVertexShader = static_cast<CD3D12RHIVertexShader*>(CreateInfo.ShaderState.VertexShader);
    Assert( DxVertexShader != nullptr );

    if ( DxVertexShader->HasRootSignature() )
    {
        ShadersWithRootSignature.Emplace( DxVertexShader );
    }

    D3D12_SHADER_BYTECODE& VertexShader = PipelineStream.VertexShader;
    VertexShader = DxVertexShader->GetByteCode();
    BaseShaders.Emplace( DxVertexShader );

    // PixelShader
    CD3D12RHIPixelShader* DxPixelShader = static_cast<CD3D12RHIPixelShader*>(CreateInfo.ShaderState.PixelShader);

    D3D12_SHADER_BYTECODE& PixelShader = PipelineStream.PixelShader;
    if ( DxPixelShader )
    {
        PixelShader = DxPixelShader->GetByteCode();
        BaseShaders.Emplace( DxPixelShader );

        if ( DxPixelShader->HasRootSignature() )
        {
            ShadersWithRootSignature.Emplace( DxPixelShader );
        }
    }
    else
    {
        PixelShader.pShaderBytecode = nullptr;
        PixelShader.BytecodeLength = 0;
    }

    // RenderTarget
    const uint32 NumRenderTargets = CreateInfo.PipelineFormats.NumRenderTargets;

    D3D12_RT_FORMAT_ARRAY& RenderTargetInfo = PipelineStream.RenderTargetInfo;
    RenderTargetInfo.NumRenderTargets = NumRenderTargets;
    for ( uint32 Index = 0; Index < NumRenderTargets; Index++ )
    {
        RenderTargetInfo.RTFormats[Index] = ConvertFormat( CreateInfo.PipelineFormats.RenderTargetFormats[Index] );
    }

    // DepthStencil
    PipelineStream.DepthBufferFormat = ConvertFormat( CreateInfo.PipelineFormats.DepthStencilFormat );

    // RasterizerState
    CD3D12RHIRasterizerState* DxRasterizerState = static_cast<CD3D12RHIRasterizerState*>(CreateInfo.RasterizerState);
    Assert( DxRasterizerState != nullptr );

    D3D12_RASTERIZER_DESC& RasterizerDesc = PipelineStream.RasterizerDesc;
    RasterizerDesc = DxRasterizerState->GetDesc();

    // DepthStencilState
    CD3D12RHIDepthStencilState* DxDepthStencilState = static_cast<CD3D12RHIDepthStencilState*>(CreateInfo.DepthStencilState);
    Assert( DxDepthStencilState != nullptr );

    D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc = PipelineStream.DepthStencilDesc;
    DepthStencilDesc = DxDepthStencilState->GetDesc();

    // BlendState
    CD3D12RHIBlendState* DxBlendState = static_cast<CD3D12RHIBlendState*>(CreateInfo.BlendState);
    Assert( DxBlendState != nullptr );

    D3D12_BLEND_DESC& BlendStateDesc = PipelineStream.BlendStateDesc;
    BlendStateDesc = DxBlendState->GetDesc();

    // Topology
    PipelineStream.PrimitiveTopologyType = ConvertPrimitiveTopologyType( CreateInfo.PrimitiveTopologyType );

    // MSAA
    DXGI_SAMPLE_DESC& SamplerDesc = PipelineStream.SampleDesc;
    SamplerDesc.Count = CreateInfo.SampleCount;
    SamplerDesc.Quality = CreateInfo.SampleQuality;

    // RootSignature
    if ( ShadersWithRootSignature.IsEmpty() )
    {
        SD3D12RootSignatureResourceCount ResourceCounts;
        ResourceCounts.Type = ERootSignatureType::Graphics;
        // TODO: Check if any shader actually uses the input assembler
        ResourceCounts.AllowInputAssembler = true;

        // NOTE: For now all constants are put in visibility_all
        uint32 Num32BitConstants = 0;
        for ( CD3D12BaseShader* DxShader : BaseShaders )
        {
            uint32 Index = DxShader->GetShaderVisibility();
            ResourceCounts.ResourceCounts[Index] = DxShader->GetResourceCount();
            Num32BitConstants = NMath::Max<uint32>( Num32BitConstants, ResourceCounts.ResourceCounts[Index].Num32BitConstants );
            ResourceCounts.ResourceCounts[Index].Num32BitConstants = 0;
        }

        ResourceCounts.ResourceCounts[ShaderVisibility_All].Num32BitConstants = Num32BitConstants;

        RootSignature = MakeSharedRef<CD3D12RootSignature>( CD3D12RootSignatureCache::Get().GetOrCreateRootSignature( ResourceCounts ) );
    }
    else
    {
        // TODO: Maybe use all shaders and create one that fits all
        D3D12_SHADER_BYTECODE ByteCode = ShadersWithRootSignature.FirstElement()->GetByteCode();

        RootSignature = DBG_NEW CD3D12RootSignature( GetDevice() );
        if ( !RootSignature->Init( ByteCode.pShaderBytecode, ByteCode.BytecodeLength ) )
        {
            return false;
        }
        else
        {
            RootSignature->SetName( "Custom Graphics RootSignature" );
        }
    }

    Assert( RootSignature != nullptr );

    PipelineStream.RootSignature = RootSignature->GetRootSignature();

    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
    CMemory::Memzero( &PipelineStreamDesc );

    PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
    PipelineStreamDesc.SizeInBytes = sizeof( SGraphicsPipelineStream );

    TComPtr<ID3D12PipelineState> NewPipelineState;
    HRESULT Result = GetDevice()->CreatePipelineState( &PipelineStreamDesc, IID_PPV_ARGS( &NewPipelineState ) );
    if ( FAILED( Result ) )
    {
        LOG_ERROR( "[D3D12GraphicsPipelineState]: FAILED to Create GraphicsPipelineState" );
        return false;
    }

    PipelineState = NewPipelineState;
    return true;
}

CD3D12RHIComputePipelineState::CD3D12RHIComputePipelineState( CD3D12Device* InDevice, const TSharedRef<CD3D12RHIComputeShader>& InShader )
    : CRHIComputePipelineState()
    , CD3D12DeviceChild( InDevice )
    , PipelineState( nullptr )
    , Shader( InShader )
    , RootSignature( nullptr )
{
}

bool CD3D12RHIComputePipelineState::Init()
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

    if ( !Shader->HasRootSignature() )
    {
        SD3D12RootSignatureResourceCount ResourceCounts;
        ResourceCounts.Type = ERootSignatureType::Compute;
        ResourceCounts.AllowInputAssembler = false;
        ResourceCounts.ResourceCounts[ShaderVisibility_All] = Shader->GetResourceCount();

        RootSignature = MakeSharedRef<CD3D12RootSignature>( CD3D12RootSignatureCache::Get().GetOrCreateRootSignature( ResourceCounts ) );
    }
    else
    {
        D3D12_SHADER_BYTECODE ByteCode = Shader->GetByteCode();

        RootSignature = DBG_NEW CD3D12RootSignature( GetDevice() );
        if ( !RootSignature->Init( ByteCode.pShaderBytecode, ByteCode.BytecodeLength ) )
        {
            return false;
        }
        else
        {
            RootSignature->SetName( "Custom Compute RootSignature" );
        }
    }

    Assert( RootSignature != nullptr );

    PipelineStream.RootSignature = RootSignature->GetRootSignature();

    // Create PipelineState
    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
    CMemory::Memzero( &PipelineStreamDesc, sizeof( D3D12_PIPELINE_STATE_STREAM_DESC ) );

    PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
    PipelineStreamDesc.SizeInBytes = sizeof( SComputePipelineStream );

    HRESULT Result = GetDevice()->CreatePipelineState( &PipelineStreamDesc, IID_PPV_ARGS( &PipelineState ) );
    if ( FAILED( Result ) )
    {
        LOG_ERROR( "[D3D12ComputePipelineState]: FAILED to Create ComputePipelineState" );
        return false;
    }

    return true;
}

struct SD3D12RootSignatureAssociation
{
    SD3D12RootSignatureAssociation( ID3D12RootSignature* InRootSignature, const TArray<WString>& InShaderExportNames )
        : ExportAssociation()
        , RootSignature( InRootSignature )
        , ShaderExportNames( InShaderExportNames )
        , ShaderExportNamesRef( InShaderExportNames.Size() )
    {
        for ( int32 i = 0; i < ShaderExportNames.Size(); i++ )
        {
            ShaderExportNamesRef[i] = ShaderExportNames[i].CStr();
        }
    }

    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ExportAssociation;

    ID3D12RootSignature* RootSignature;

    TArray<WString> ShaderExportNames;
    TArray<LPCWSTR> ShaderExportNamesRef;
};

struct SD3D12HitGroup
{
    SD3D12HitGroup( const WString& InHitGroupName, const WString& InClosestHit, const WString& InAnyHit, const WString& InIntersection )
        : Desc()
        , HitGroupName( InHitGroupName )
        , ClosestHit( InClosestHit )
        , AnyHit( InAnyHit )
        , Intersection( InIntersection )
    {
        CMemory::Memzero( &Desc );

        Desc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
        Desc.HitGroupExport = HitGroupName.CStr();
        Desc.ClosestHitShaderImport = ClosestHit.CStr();

        if ( AnyHit != L"" )
        {
            Desc.AnyHitShaderImport = AnyHit.CStr();
        }

        if ( Desc.Type != D3D12_HIT_GROUP_TYPE_TRIANGLES )
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

struct SD3D12Library
{
    SD3D12Library( D3D12_SHADER_BYTECODE ByteCode, const TArray<WString>& InExportNames )
        : ExportNames( InExportNames )
        , ExportDescs( InExportNames.Size() )
        , Desc()
    {
        for ( int32 i = 0; i < ExportDescs.Size(); i++ )
        {
            D3D12_EXPORT_DESC& TempDesc = ExportDescs[i];
            TempDesc.Flags = D3D12_EXPORT_FLAG_NONE;
            TempDesc.Name = ExportNames[i].CStr();
            TempDesc.ExportToRename = nullptr;
        }

        Desc.DXILLibrary = ByteCode;
        Desc.pExports = ExportDescs.Data();
        Desc.NumExports = ExportDescs.Size();
    }

    TArray<WString>           ExportNames;
    TArray<D3D12_EXPORT_DESC> ExportDescs;
    D3D12_DXIL_LIBRARY_DESC   Desc;
};

struct SD3D12RayTracingPipelineStateStream
{
    void AddLibrary( D3D12_SHADER_BYTECODE ByteCode, const TArray<WString>& ExportNames )
    {
        Libraries.Emplace( ByteCode, ExportNames );
    }

    void AddHitGroup( const WString& HitGroupName, const WString& ClosestHit, const WString& AnyHit, const WString& Intersection )
    {
        HitGroups.Emplace( HitGroupName, ClosestHit, AnyHit, Intersection );
    }

    void AddRootSignatureAssociation( ID3D12RootSignature* RootSignature, const TArray<WString>& ShaderExportNames )
    {
        RootSignatureAssociations.Emplace( RootSignature, ShaderExportNames );
    }

    void Generate()
    {
        uint32 NumSubObjects = Libraries.Size() + HitGroups.Size() + (RootSignatureAssociations.Size() * 2) + 4;
        SubObjects.Resize( NumSubObjects );

        uint32 SubObjectIndex = 0;
        for ( SD3D12Library& Lib : Libraries )
        {
            D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
            SubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            SubObject.pDesc = &Lib.Desc;
        }

        for ( SD3D12HitGroup& HitGroup : HitGroups )
        {
            D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
            SubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            SubObject.pDesc = &HitGroup.Desc;
        }

        for ( SD3D12RootSignatureAssociation& Association : RootSignatureAssociations )
        {
            D3D12_STATE_SUBOBJECT& LocalRootSubObject = SubObjects[SubObjectIndex++];
            LocalRootSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
            LocalRootSubObject.pDesc = &Association.RootSignature;

            Association.ExportAssociation.pExports = Association.ShaderExportNamesRef.Data();
            Association.ExportAssociation.NumExports = Association.ShaderExportNamesRef.Size();
            Association.ExportAssociation.pSubobjectToAssociate = &SubObjects[SubObjectIndex - 1];

            D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
            SubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            SubObject.pDesc = &Association.ExportAssociation;
        }

        D3D12_STATE_SUBOBJECT& GlobalRootSubObject = SubObjects[SubObjectIndex++];
        GlobalRootSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        GlobalRootSubObject.pDesc = &GlobalRootSignature;

        D3D12_STATE_SUBOBJECT& PipelineConfigSubObject = SubObjects[SubObjectIndex++];
        PipelineConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        PipelineConfigSubObject.pDesc = &PipelineConfig;

        D3D12_STATE_SUBOBJECT& ShaderConfigObject = SubObjects[SubObjectIndex++];
        ShaderConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        ShaderConfigObject.pDesc = &ShaderConfig;

        PayLoadExportNamesRef.Resize( PayLoadExportNames.Size() );
        for ( int32 i = 0; i < PayLoadExportNames.Size(); i++ )
        {
            PayLoadExportNamesRef[i] = PayLoadExportNames[i].CStr();
        }

        ShaderConfigAssociation.pExports = PayLoadExportNamesRef.Data();
        ShaderConfigAssociation.NumExports = PayLoadExportNamesRef.Size();
        ShaderConfigAssociation.pSubobjectToAssociate = &SubObjects[SubObjectIndex - 1];

        D3D12_STATE_SUBOBJECT& ShaderConfigAssociationSubObject = SubObjects[SubObjectIndex++];
        ShaderConfigAssociationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        ShaderConfigAssociationSubObject.pDesc = &ShaderConfigAssociation;
    }

    TArray<SD3D12Library>  Libraries;
    TArray<SD3D12HitGroup> HitGroups;
    TArray<SD3D12RootSignatureAssociation> RootSignatureAssociations;

    D3D12_RAYTRACING_PIPELINE_CONFIG       PipelineConfig;
    D3D12_RAYTRACING_SHADER_CONFIG         ShaderConfig;
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ShaderConfigAssociation;

    TArray<WString> PayLoadExportNames;
    TArray<LPCWSTR> PayLoadExportNamesRef;

    ID3D12RootSignature* GlobalRootSignature;
    TArray<D3D12_STATE_SUBOBJECT> SubObjects;
};

CD3D12RHIRayTracingPipelineState::CD3D12RHIRayTracingPipelineState( CD3D12Device* InDevice )
    : CD3D12DeviceChild( InDevice )
    , StateObject( nullptr )
{
}

bool CD3D12RHIRayTracingPipelineState::Init( const SRayTracingPipelineStateCreateInfo& CreateInfo )
{
    SD3D12RayTracingPipelineStateStream PipelineStream;

    TArray<CD3D12BaseShader*> Shaders;
    CD3D12RHIRayGenShader* RayGen = static_cast<CD3D12RHIRayGenShader*>(CreateInfo.RayGen);
    Shaders.Emplace( RayGen );

    SD3D12RootSignatureResourceCount RayGenLocalResourceCounts;
    RayGenLocalResourceCounts.Type = ERootSignatureType::RayTracingLocal;
    RayGenLocalResourceCounts.AllowInputAssembler = false;
    RayGenLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = RayGen->GetRTLocalResourceCount();

    RayGenLocalRootSignature = MakeSharedRef<CD3D12RootSignature>( CD3D12RootSignatureCache::Get().GetOrCreateRootSignature( RayGenLocalResourceCounts ) );
    if ( !RayGenLocalRootSignature )
    {
        return false;
    }

    WString RayGenIdentifier = CharToWide( RayGen->GetIdentifier() );
    PipelineStream.AddLibrary( RayGen->GetByteCode(), { RayGenIdentifier } );
    PipelineStream.AddRootSignatureAssociation( RayGenLocalRootSignature->GetRootSignature(), { RayGenIdentifier } );
    PipelineStream.PayLoadExportNames.Emplace( RayGenIdentifier );

    WString HitGroupName;
    WString ClosestHitName;
    WString AnyHitName;

    for ( const SRayTracingHitGroup& HitGroup : CreateInfo.HitGroups )
    {
        CD3D12RHIRayAnyHitShader* DxAnyHit = static_cast<CD3D12RHIRayAnyHitShader*>(HitGroup.AnyHit);
        CD3D12RayClosestHitShader* DxClosestHit = static_cast<CD3D12RayClosestHitShader*>(HitGroup.ClosestHit);

        Assert( DxClosestHit != nullptr );

        HitGroupName = CharToWide( HitGroup.Name );
        ClosestHitName = CharToWide( DxClosestHit->GetIdentifier() );
        AnyHitName = DxAnyHit ? CharToWide( DxAnyHit->GetIdentifier() ) : L"";

        PipelineStream.AddHitGroup( HitGroupName, ClosestHitName, AnyHitName, L"" );
    }

    for ( CRHIRayAnyHitShader* AnyHit : CreateInfo.AnyHitShaders )
    {
        CD3D12RHIRayAnyHitShader* DxAnyHit = static_cast<CD3D12RHIRayAnyHitShader*>(AnyHit);
        Shaders.Emplace( DxAnyHit );

        SD3D12RootSignatureResourceCount AnyHitLocalResourceCounts;
        AnyHitLocalResourceCounts.Type = ERootSignatureType::RayTracingLocal;
        AnyHitLocalResourceCounts.AllowInputAssembler = false;
        AnyHitLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = DxAnyHit->GetRTLocalResourceCount();

        HitLocalRootSignature = MakeSharedRef<CD3D12RootSignature>( CD3D12RootSignatureCache::Get().GetOrCreateRootSignature( AnyHitLocalResourceCounts ) );
        if ( !HitLocalRootSignature )
        {
            return false;
        }

        WString AnyHitIdentifier = CharToWide( DxAnyHit->GetIdentifier() );
        PipelineStream.AddLibrary( DxAnyHit->GetByteCode(), { AnyHitIdentifier } );
        PipelineStream.AddRootSignatureAssociation( HitLocalRootSignature->GetRootSignature(), { AnyHitIdentifier } );
        PipelineStream.PayLoadExportNames.Emplace( AnyHitIdentifier );
    }

    for ( CRHIRayClosestHitShader* ClosestHit : CreateInfo.ClosestHitShaders )
    {
        CD3D12RayClosestHitShader* DxClosestHit = static_cast<CD3D12RayClosestHitShader*>(ClosestHit);
        Shaders.Emplace( DxClosestHit );

        SD3D12RootSignatureResourceCount ClosestHitLocalResourceCounts;
        ClosestHitLocalResourceCounts.Type = ERootSignatureType::RayTracingLocal;
        ClosestHitLocalResourceCounts.AllowInputAssembler = false;
        ClosestHitLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = DxClosestHit->GetRTLocalResourceCount();

        HitLocalRootSignature = MakeSharedRef<CD3D12RootSignature>( CD3D12RootSignatureCache::Get().GetOrCreateRootSignature( ClosestHitLocalResourceCounts ) );
        if ( !HitLocalRootSignature )
        {
            return false;
        }

        WString ClosestHitIdentifier = CharToWide( DxClosestHit->GetIdentifier() );
        PipelineStream.AddLibrary( DxClosestHit->GetByteCode(), { ClosestHitIdentifier } );
        PipelineStream.AddRootSignatureAssociation( HitLocalRootSignature->GetRootSignature(), { ClosestHitIdentifier } );
        PipelineStream.PayLoadExportNames.Emplace( ClosestHitIdentifier );
    }

    for ( CRHIRayMissShader* Miss : CreateInfo.MissShaders )
    {
        CD3D12RHIRayMissShader* DxMiss = static_cast<CD3D12RHIRayMissShader*>(Miss);
        Shaders.Emplace( DxMiss );

        SD3D12RootSignatureResourceCount MissLocalResourceCounts;
        MissLocalResourceCounts.Type = ERootSignatureType::RayTracingLocal;
        MissLocalResourceCounts.AllowInputAssembler = false;
        MissLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = DxMiss->GetRTLocalResourceCount();

        MissLocalRootSignature = MakeSharedRef<CD3D12RootSignature>( CD3D12RootSignatureCache::Get().GetOrCreateRootSignature( MissLocalResourceCounts ) );
        if ( !MissLocalRootSignature )
        {
            return false;
        }

        WString MissIdentifier = CharToWide( DxMiss->GetIdentifier() );
        PipelineStream.AddLibrary( DxMiss->GetByteCode(), { MissIdentifier } );
        PipelineStream.AddRootSignatureAssociation( MissLocalRootSignature->GetRootSignature(), { MissIdentifier } );
        PipelineStream.PayLoadExportNames.Emplace( MissIdentifier );
    }

    PipelineStream.ShaderConfig.MaxAttributeSizeInBytes = CreateInfo.MaxAttributeSizeInBytes;
    PipelineStream.ShaderConfig.MaxPayloadSizeInBytes = CreateInfo.MaxPayloadSizeInBytes;
    PipelineStream.PipelineConfig.MaxTraceRecursionDepth = CreateInfo.MaxRecursionDepth;

    SShaderResourceCount CombinedResourceCount;
    for ( CD3D12BaseShader* Shader : Shaders )
    {
        Assert( Shader != nullptr );
        CombinedResourceCount.Combine( Shader->GetResourceCount() );
    }

    SD3D12RootSignatureResourceCount GlobalResourceCounts;
    GlobalResourceCounts.Type = ERootSignatureType::RayTracingGlobal;
    GlobalResourceCounts.AllowInputAssembler = false;
    GlobalResourceCounts.ResourceCounts[ShaderVisibility_All] = CombinedResourceCount;

    GlobalRootSignature = MakeSharedRef<CD3D12RootSignature>( CD3D12RootSignatureCache::Get().GetOrCreateRootSignature( GlobalResourceCounts ) );
    if ( !GlobalRootSignature )
    {
        return false;
    }

    PipelineStream.GlobalRootSignature = GlobalRootSignature->GetRootSignature();

    PipelineStream.Generate();

    D3D12_STATE_OBJECT_DESC RayTracingPipeline;
    CMemory::Memzero( &RayTracingPipeline );

    RayTracingPipeline.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    RayTracingPipeline.pSubobjects = PipelineStream.SubObjects.Data();
    RayTracingPipeline.NumSubobjects = PipelineStream.SubObjects.Size();

    TComPtr<ID3D12StateObject> TempStateObject;
    HRESULT Result = GetDevice()->GetDXRDevice()->CreateStateObject( &RayTracingPipeline, IID_PPV_ARGS( &TempStateObject ) );
    if ( FAILED( Result ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    TComPtr<ID3D12StateObjectProperties> TempStateObjectProperties;
    Result = TempStateObject->QueryInterface( IID_PPV_ARGS( &TempStateObjectProperties ) );
    if ( FAILED( Result ) )
    {
        LOG_ERROR( "[D3D12RayTracingPipelineState] Failed to retrive ID3D12StateObjectProperties" );
        return false;
    }

    StateObject = TempStateObject;
    StateObjectProperties = TempStateObjectProperties;

    return true;
}

void* CD3D12RHIRayTracingPipelineState::GetShaderIdentifer( const CString& ExportName )
{
    auto MapItem = ShaderIdentifers.find( ExportName );
    if ( MapItem == ShaderIdentifers.end() )
    {
        WString WideExportName = CharToWide( ExportName );

        void* Result = StateObjectProperties->GetShaderIdentifier( WideExportName.CStr() );
        if ( !Result )
        {
            return nullptr;
        }

        SRayTracingShaderIdentifer Identifier;
        CMemory::Memcpy( Identifier.ShaderIdentifier, Result, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES );

        auto NewIdentifier = ShaderIdentifers.insert( std::make_pair( ExportName, Identifier ) );
        return NewIdentifier.first->second.ShaderIdentifier;
    }
    else
    {
        return MapItem->second.ShaderIdentifier;
    }
}
