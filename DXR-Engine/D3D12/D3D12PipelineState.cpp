#include "D3D12PipelineState.h"
#include "D3D12ShaderCompiler.h"

D3D12GraphicsPipelineState::D3D12GraphicsPipelineState( D3D12Device* InDevice )
    : D3D12DeviceChild( InDevice )
    , PipelineState( nullptr )
    , RootSignature( nullptr )
{
}

bool D3D12GraphicsPipelineState::Init( const GraphicsPipelineStateCreateInfo& CreateInfo )
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

    D3D12_INPUT_LAYOUT_DESC& InputLayoutDesc = PipelineStream.InputLayout;

    D3D12InputLayoutState* DxInputLayoutState = static_cast<D3D12InputLayoutState*>(CreateInfo.InputLayoutState);
    if ( !DxInputLayoutState )
    {
        InputLayoutDesc.pInputElementDescs = nullptr;
        InputLayoutDesc.NumElements = 0;
    }
    else
    {
        InputLayoutDesc = DxInputLayoutState->GetDesc();
    }

    TArray<D3D12BaseShader*> ShadersWithRootSignature;
    TArray<D3D12BaseShader*> BaseShaders;

    // VertexShader
    D3D12VertexShader* DxVertexShader = static_cast<D3D12VertexShader*>(CreateInfo.ShaderState.VertexShader);
    Assert( DxVertexShader != nullptr );

    if ( DxVertexShader->HasRootSignature() )
    {
        ShadersWithRootSignature.Emplace( DxVertexShader );
    }

    D3D12_SHADER_BYTECODE& VertexShader = PipelineStream.VertexShader;
    VertexShader = DxVertexShader->GetByteCode();
    BaseShaders.Emplace( DxVertexShader );

    // PixelShader
    D3D12PixelShader* DxPixelShader = static_cast<D3D12PixelShader*>(CreateInfo.ShaderState.PixelShader);

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
    D3D12RasterizerState* DxRasterizerState = static_cast<D3D12RasterizerState*>(CreateInfo.RasterizerState);
    Assert( DxRasterizerState != nullptr );

    D3D12_RASTERIZER_DESC& RasterizerDesc = PipelineStream.RasterizerDesc;
    RasterizerDesc = DxRasterizerState->GetDesc();

    // DepthStencilState
    D3D12DepthStencilState* DxDepthStencilState = static_cast<D3D12DepthStencilState*>(CreateInfo.DepthStencilState);
    Assert( DxDepthStencilState != nullptr );

    D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc = PipelineStream.DepthStencilDesc;
    DepthStencilDesc = DxDepthStencilState->GetDesc();

    // BlendState
    D3D12BlendState* DxBlendState = static_cast<D3D12BlendState*>(CreateInfo.BlendState);
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
        D3D12RootSignatureResourceCount ResourceCounts;
        ResourceCounts.Type = ERootSignatureType::Graphics;
        // TODO: Check if any shader actually uses the input assembler
        ResourceCounts.AllowInputAssembler = true;

        // NOTE: For now all constants are put in visibility_all
        uint32 Num32BitConstants = 0;
        for ( D3D12BaseShader* DxShader : BaseShaders )
        {
            uint32 Index = DxShader->GetShaderVisibility();
            ResourceCounts.ResourceCounts[Index] = DxShader->GetResourceCount();
            Num32BitConstants = NMath::Max<uint32>( Num32BitConstants, ResourceCounts.ResourceCounts[Index].Num32BitConstants );
            ResourceCounts.ResourceCounts[Index].Num32BitConstants = 0;
        }

        ResourceCounts.ResourceCounts[ShaderVisibility_All].Num32BitConstants = Num32BitConstants;

        RootSignature = MakeSharedRef<D3D12RootSignature>( D3D12RootSignatureCache::Get().GetOrCreateRootSignature( ResourceCounts ) );
    }
    else
    {
        // TODO: Maybe use all shaders and create one that fits all
        D3D12_SHADER_BYTECODE ByteCode = ShadersWithRootSignature.FirstElement()->GetByteCode();

        RootSignature = DBG_NEW D3D12RootSignature( GetDevice() );
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
    Memory::Memzero( &PipelineStreamDesc );

    PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
    PipelineStreamDesc.SizeInBytes = sizeof( GraphicsPipelineStream );

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

D3D12ComputePipelineState::D3D12ComputePipelineState( D3D12Device* InDevice, const TSharedRef<D3D12ComputeShader>& InShader )
    : ComputePipelineState()
    , D3D12DeviceChild( InDevice )
    , PipelineState( nullptr )
    , Shader( InShader )
    , RootSignature( nullptr )
{
}

bool D3D12ComputePipelineState::Init()
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

    PipelineStream.ComputeShader = Shader->GetByteCode();

    if ( !Shader->HasRootSignature() )
    {
        D3D12RootSignatureResourceCount ResourceCounts;
        ResourceCounts.Type = ERootSignatureType::Compute;
        ResourceCounts.AllowInputAssembler = false;
        ResourceCounts.ResourceCounts[ShaderVisibility_All] = Shader->GetResourceCount();

        RootSignature = MakeSharedRef<D3D12RootSignature>( D3D12RootSignatureCache::Get().GetOrCreateRootSignature( ResourceCounts ) );
    }
    else
    {
        D3D12_SHADER_BYTECODE ByteCode = Shader->GetByteCode();

        RootSignature = DBG_NEW D3D12RootSignature( GetDevice() );
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
    Memory::Memzero( &PipelineStreamDesc, sizeof( D3D12_PIPELINE_STATE_STREAM_DESC ) );

    PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
    PipelineStreamDesc.SizeInBytes = sizeof( ComputePipelineStream );

    HRESULT Result = GetDevice()->CreatePipelineState( &PipelineStreamDesc, IID_PPV_ARGS( &PipelineState ) );
    if ( FAILED( Result ) )
    {
        LOG_ERROR( "[D3D12ComputePipelineState]: FAILED to Create ComputePipelineState" );
        return false;
    }

    return true;
}

struct D3D12RootSignatureAssociation
{
    D3D12RootSignatureAssociation( ID3D12RootSignature* InRootSignature, const TArray<std::wstring>& InShaderExportNames )
        : ExportAssociation()
        , RootSignature( InRootSignature )
        , ShaderExportNames( InShaderExportNames )
        , ShaderExportNamesRef( InShaderExportNames.Size() )
    {
        for ( uint32 i = 0; i < ShaderExportNames.Size(); i++ )
        {
            ShaderExportNamesRef[i] = ShaderExportNames[i].c_str();
        }
    }

    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ExportAssociation;
    ID3D12RootSignature* RootSignature;
    TArray<std::wstring> ShaderExportNames;
    TArray<LPCWSTR>      ShaderExportNamesRef;
};

struct D3D12HitGroup
{
    D3D12HitGroup( std::wstring InHitGroupName, std::wstring InClosestHit, std::wstring InAnyHit, std::wstring InIntersection )
        : Desc()
        , HitGroupName( InHitGroupName )
        , ClosestHit( InClosestHit )
        , AnyHit( InAnyHit )
        , Intersection( InIntersection )
    {
        Memory::Memzero( &Desc );

        Desc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
        Desc.HitGroupExport = HitGroupName.c_str();
        Desc.ClosestHitShaderImport = ClosestHit.c_str();

        if ( AnyHit != L"" )
        {
            Desc.AnyHitShaderImport = AnyHit.c_str();
        }

        if ( Desc.Type != D3D12_HIT_GROUP_TYPE_TRIANGLES )
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
    D3D12Library( D3D12_SHADER_BYTECODE ByteCode, const TArray<std::wstring>& InExportNames )
        : ExportNames( InExportNames )
        , ExportDescs( InExportNames.Size() )
        , Desc()
    {
        for ( uint32 i = 0; i < ExportDescs.Size(); i++ )
        {
            D3D12_EXPORT_DESC& TempDesc = ExportDescs[i];
            TempDesc.Flags = D3D12_EXPORT_FLAG_NONE;
            TempDesc.Name = ExportNames[i].c_str();
            TempDesc.ExportToRename = nullptr;
        }

        Desc.DXILLibrary = ByteCode;
        Desc.pExports = ExportDescs.Data();
        Desc.NumExports = ExportDescs.Size();
    }

    TArray<std::wstring>      ExportNames;
    TArray<D3D12_EXPORT_DESC> ExportDescs;
    D3D12_DXIL_LIBRARY_DESC   Desc;
};

struct D3D12RayTracingPipelineStateStream
{
    void AddLibrary( D3D12_SHADER_BYTECODE ByteCode, const TArray<std::wstring>& ExportNames )
    {
        Libraries.Emplace( ByteCode, ExportNames );
    }

    void AddHitGroup( std::wstring HitGroupName, std::wstring ClosestHit, std::wstring AnyHit, std::wstring Intersection )
    {
        HitGroups.Emplace( HitGroupName, ClosestHit, AnyHit, Intersection );
    }

    void AddRootSignatureAssociation( ID3D12RootSignature* RootSignature, const TArray<std::wstring>& ShaderExportNames )
    {
        RootSignatureAssociations.Emplace( RootSignature, ShaderExportNames );
    }

    void Generate()
    {
        uint32 NumSubObjects = Libraries.Size() + HitGroups.Size() + (RootSignatureAssociations.Size() * 2) + 4;
        SubObjects.Resize( NumSubObjects );

        uint32 SubObjectIndex = 0;
        for ( D3D12Library& Lib : Libraries )
        {
            D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
            SubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            SubObject.pDesc = &Lib.Desc;
        }

        for ( D3D12HitGroup& HitGroup : HitGroups )
        {
            D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
            SubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            SubObject.pDesc = &HitGroup.Desc;
        }

        for ( D3D12RootSignatureAssociation& Association : RootSignatureAssociations )
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
        for ( uint32 i = 0; i < PayLoadExportNames.Size(); i++ )
        {
            PayLoadExportNamesRef[i] = PayLoadExportNames[i].c_str();
        }

        ShaderConfigAssociation.pExports = PayLoadExportNamesRef.Data();
        ShaderConfigAssociation.NumExports = PayLoadExportNamesRef.Size();
        ShaderConfigAssociation.pSubobjectToAssociate = &SubObjects[SubObjectIndex - 1];

        D3D12_STATE_SUBOBJECT& ShaderConfigAssociationSubObject = SubObjects[SubObjectIndex++];
        ShaderConfigAssociationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
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

    ID3D12RootSignature* GlobalRootSignature;
    TArray<D3D12_STATE_SUBOBJECT> SubObjects;
};

D3D12RayTracingPipelineState::D3D12RayTracingPipelineState( D3D12Device* InDevice )
    : D3D12DeviceChild( InDevice )
    , StateObject( nullptr )
{
}

bool D3D12RayTracingPipelineState::Init( const RayTracingPipelineStateCreateInfo& CreateInfo )
{
    D3D12RayTracingPipelineStateStream PipelineStream;

    TArray<D3D12BaseShader*> Shaders;
    D3D12RayGenShader* RayGen = static_cast<D3D12RayGenShader*>(CreateInfo.RayGen);
    Shaders.Emplace( RayGen );

    D3D12RootSignatureResourceCount RayGenLocalResourceCounts;
    RayGenLocalResourceCounts.Type = ERootSignatureType::RayTracingLocal;
    RayGenLocalResourceCounts.AllowInputAssembler = false;
    RayGenLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = RayGen->GetRTLocalResourceCount();

    RayGenLocalRootSignature = MakeSharedRef<D3D12RootSignature>( D3D12RootSignatureCache::Get().GetOrCreateRootSignature( RayGenLocalResourceCounts ) );
    if ( !RayGenLocalRootSignature )
    {
        return false;
    }

    std::wstring RayGenIdentifier = CharToWide( CString( RayGen->GetIdentifier().c_str(), RayGen->GetIdentifier().length() ) ).CStr();
    PipelineStream.AddLibrary( RayGen->GetByteCode(), { RayGenIdentifier } );
    PipelineStream.AddRootSignatureAssociation( RayGenLocalRootSignature->GetRootSignature(), { RayGenIdentifier } );
    PipelineStream.PayLoadExportNames.Emplace( RayGenIdentifier );

    std::wstring HitGroupName;
    std::wstring ClosestHitName;
    std::wstring AnyHitName;

    for ( const RayTracingHitGroup& HitGroup : CreateInfo.HitGroups )
    {
        D3D12RayAnyHitShader* DxAnyHit = static_cast<D3D12RayAnyHitShader*>(HitGroup.AnyHit);
        D3D12RayClosestHitShader* DxClosestHit = static_cast<D3D12RayClosestHitShader*>(HitGroup.ClosestHit);

        Assert( DxClosestHit != nullptr );

        HitGroupName = CharToWide( CString( HitGroup.Name.c_str(), HitGroup.Name.length() ) ).CStr();
        ClosestHitName = CharToWide( CString( DxClosestHit->GetIdentifier().c_str(), DxClosestHit->GetIdentifier().length() ) ).CStr();
        AnyHitName = DxAnyHit ? CharToWide( CString( DxAnyHit->GetIdentifier().c_str(), DxAnyHit->GetIdentifier().length() ) ).CStr() : L"";

        PipelineStream.AddHitGroup( HitGroupName, ClosestHitName, AnyHitName, L"" );
    }

    for ( RayAnyHitShader* AnyHit : CreateInfo.AnyHitShaders )
    {
        D3D12RayAnyHitShader* DxAnyHit = static_cast<D3D12RayAnyHitShader*>(AnyHit);
        Shaders.Emplace( DxAnyHit );

        D3D12RootSignatureResourceCount AnyHitLocalResourceCounts;
        AnyHitLocalResourceCounts.Type = ERootSignatureType::RayTracingLocal;
        AnyHitLocalResourceCounts.AllowInputAssembler = false;
        AnyHitLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = DxAnyHit->GetRTLocalResourceCount();

        HitLocalRootSignature = MakeSharedRef<D3D12RootSignature>( D3D12RootSignatureCache::Get().GetOrCreateRootSignature( AnyHitLocalResourceCounts ) );
        if ( !HitLocalRootSignature )
        {
            return false;
        }

        std::wstring AnyHitIdentifier = CharToWide( CString( DxAnyHit->GetIdentifier().c_str(), DxAnyHit->GetIdentifier().length() ) ).CStr();
        PipelineStream.AddLibrary( DxAnyHit->GetByteCode(), { AnyHitIdentifier } );
        PipelineStream.AddRootSignatureAssociation( HitLocalRootSignature->GetRootSignature(), { AnyHitIdentifier } );
        PipelineStream.PayLoadExportNames.Emplace( AnyHitIdentifier );
    }

    for ( RayClosestHitShader* ClosestHit : CreateInfo.ClosestHitShaders )
    {
        D3D12RayClosestHitShader* DxClosestHit = static_cast<D3D12RayClosestHitShader*>(ClosestHit);
        Shaders.Emplace( DxClosestHit );

        D3D12RootSignatureResourceCount ClosestHitLocalResourceCounts;
        ClosestHitLocalResourceCounts.Type = ERootSignatureType::RayTracingLocal;
        ClosestHitLocalResourceCounts.AllowInputAssembler = false;
        ClosestHitLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = DxClosestHit->GetRTLocalResourceCount();

        HitLocalRootSignature = MakeSharedRef<D3D12RootSignature>( D3D12RootSignatureCache::Get().GetOrCreateRootSignature( ClosestHitLocalResourceCounts ) );
        if ( !HitLocalRootSignature )
        {
            return false;
        }

        std::wstring ClosestHitIdentifier = CharToWide( CString( DxClosestHit->GetIdentifier().c_str(), DxClosestHit->GetIdentifier().length() ) ).CStr();
        PipelineStream.AddLibrary( DxClosestHit->GetByteCode(), { ClosestHitIdentifier } );
        PipelineStream.AddRootSignatureAssociation( HitLocalRootSignature->GetRootSignature(), { ClosestHitIdentifier } );
        PipelineStream.PayLoadExportNames.Emplace( ClosestHitIdentifier );
    }

    for ( RayMissShader* Miss : CreateInfo.MissShaders )
    {
        D3D12RayMissShader* DxMiss = static_cast<D3D12RayMissShader*>(Miss);
        Shaders.Emplace( DxMiss );

        D3D12RootSignatureResourceCount MissLocalResourceCounts;
        MissLocalResourceCounts.Type = ERootSignatureType::RayTracingLocal;
        MissLocalResourceCounts.AllowInputAssembler = false;
        MissLocalResourceCounts.ResourceCounts[ShaderVisibility_All] = DxMiss->GetRTLocalResourceCount();

        MissLocalRootSignature = MakeSharedRef<D3D12RootSignature>( D3D12RootSignatureCache::Get().GetOrCreateRootSignature( MissLocalResourceCounts ) );
        if ( !MissLocalRootSignature )
        {
            return false;
        }

        std::wstring MissIdentifier = CharToWide( CString( DxMiss->GetIdentifier().c_str(), DxMiss->GetIdentifier().length() ) ).CStr();
        PipelineStream.AddLibrary( DxMiss->GetByteCode(), { MissIdentifier } );
        PipelineStream.AddRootSignatureAssociation( MissLocalRootSignature->GetRootSignature(), { MissIdentifier } );
        PipelineStream.PayLoadExportNames.Emplace( MissIdentifier );
    }

    PipelineStream.ShaderConfig.MaxAttributeSizeInBytes = CreateInfo.MaxAttributeSizeInBytes;
    PipelineStream.ShaderConfig.MaxPayloadSizeInBytes = CreateInfo.MaxPayloadSizeInBytes;
    PipelineStream.PipelineConfig.MaxTraceRecursionDepth = CreateInfo.MaxRecursionDepth;

    ShaderResourceCount CombinedResourceCount;
    for ( D3D12BaseShader* Shader : Shaders )
    {
        Assert( Shader != nullptr );
        CombinedResourceCount.Combine( Shader->GetResourceCount() );
    }

    D3D12RootSignatureResourceCount GlobalResourceCounts;
    GlobalResourceCounts.Type = ERootSignatureType::RayTracingGlobal;
    GlobalResourceCounts.AllowInputAssembler = false;
    GlobalResourceCounts.ResourceCounts[ShaderVisibility_All] = CombinedResourceCount;

    GlobalRootSignature = MakeSharedRef<D3D12RootSignature>( D3D12RootSignatureCache::Get().GetOrCreateRootSignature( GlobalResourceCounts ) );
    if ( !GlobalRootSignature )
    {
        return false;
    }

    PipelineStream.GlobalRootSignature = GlobalRootSignature->GetRootSignature();

    PipelineStream.Generate();

    D3D12_STATE_OBJECT_DESC RayTracingPipeline;
    Memory::Memzero( &RayTracingPipeline );

    RayTracingPipeline.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    RayTracingPipeline.pSubobjects = PipelineStream.SubObjects.Data();
    RayTracingPipeline.NumSubobjects = PipelineStream.SubObjects.Size();

    TComPtr<ID3D12StateObject> TempStateObject;
    HRESULT Result = GetDevice()->GetDXRDevice()->CreateStateObject( &RayTracingPipeline, IID_PPV_ARGS( &TempStateObject ) );
    if ( FAILED( Result ) )
    {
        Debug::DebugBreak();
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

void* D3D12RayTracingPipelineState::GetShaderIdentifer( const std::string& ExportName )
{
    auto MapItem = ShaderIdentifers.find( ExportName );
    if ( MapItem == ShaderIdentifers.end() )
    {
        std::wstring WideExportName = CharToWide( CString( ExportName.c_str(), ExportName.length() ) ).CStr();

        void* Result = StateObjectProperties->GetShaderIdentifier( WideExportName.c_str() );
        if ( !Result )
        {
            return nullptr;
        }

        RayTracingShaderIdentifer Identifier;
        Memory::Memcpy( Identifier.ShaderIdentifier, Result, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES );

        auto NewIdentifier = ShaderIdentifers.insert( std::make_pair( ExportName, Identifier ) );
        return NewIdentifier.first->second.ShaderIdentifier;
    }
    else
    {
        return MapItem->second.ShaderIdentifier;
    }
}
