#include "Core/Application/Windows/WindowsWindow.h"

#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandAllocator.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Fence.h"
#include "D3D12RootSignature.h"
#include "D3D12Helpers.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12Shader.h"
#include "D3D12RHICore.h"
#include "D3D12RHIViews.h"
#include "D3D12RHIRayTracing.h"
#include "D3D12RHIPipelineState.h"
#include "D3D12RHITexture.h"
#include "D3D12RHIBuffer.h"
#include "D3D12RHISamplerState.h"
#include "D3D12RHIViewport.h"
#include "D3D12RHIGPUProfiler.h"

#include <algorithm>

CD3D12RHICore* GD3D12RenderLayer = nullptr;

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12RHITexture2D>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12RHITexture2DArray>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12RHITextureCube>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12RHITextureCubeArray>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12RHITexture3D>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
}

template<typename TD3D12Texture>
inline bool IsTextureCube()
{
    return false;
}

template<>
inline bool IsTextureCube<CD3D12RHITextureCube>()
{
    return true;
}

template<>
inline bool IsTextureCube<CD3D12RHITextureCubeArray>()
{
    return true;
}

CD3D12RHICore::CD3D12RHICore()
    : CRHICore( ERHIModule::D3D12 )
    , Device( nullptr )
    , DirectCmdContext( nullptr )
{
    GD3D12RenderLayer = this;
}

CD3D12RHICore::~CD3D12RHICore()
{
    DirectCmdContext.Reset();

    SafeDelete( RootSignatureCache );

    SafeRelease( ResourceOfflineDescriptorHeap );
    SafeRelease( RenderTargetOfflineDescriptorHeap );
    SafeRelease( DepthStencilOfflineDescriptorHeap );
    SafeRelease( SamplerOfflineDescriptorHeap );

    SafeDelete( Device );

    GD3D12RenderLayer = nullptr;
}

bool CD3D12RHICore::Init( bool EnableDebug )
{
    // NOTE: GPUBasedValidation does not work with ray tracing since it causes Device Removed (2021-02-25)
    bool GPUBasedValidationOn =
    #if ENABLE_API_GPU_DEBUGGING
        EnableDebug;
#else
        false;
#endif
    bool DREDOn =
    #if ENABLE_API_GPU_BREADCRUMBS
        EnableDebug;
#else
        false;
#endif

    Device = DBG_NEW CD3D12Device( EnableDebug, GPUBasedValidationOn, DREDOn );
    if ( !Device->Init() )
    {
        return false;
    }

    RootSignatureCache = DBG_NEW CD3D12RootSignatureCache( Device );
    if ( !RootSignatureCache->Init() )
    {
        return false;
    }

    ResourceOfflineDescriptorHeap = DBG_NEW CD3D12OfflineDescriptorHeap( Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
    if ( !ResourceOfflineDescriptorHeap->Init() )
    {
        return false;
    }

    RenderTargetOfflineDescriptorHeap = DBG_NEW CD3D12OfflineDescriptorHeap( Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
    if ( !RenderTargetOfflineDescriptorHeap->Init() )
    {
        return false;
    }

    DepthStencilOfflineDescriptorHeap = DBG_NEW CD3D12OfflineDescriptorHeap( Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV );
    if ( !DepthStencilOfflineDescriptorHeap->Init() )
    {
        return false;
    }

    SamplerOfflineDescriptorHeap = DBG_NEW CD3D12OfflineDescriptorHeap( Device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER );
    if ( !SamplerOfflineDescriptorHeap->Init() )
    {
        return false;
    }

    DirectCmdContext = CD3D12RHICommandContext::Make( Device );
    if ( !DirectCmdContext )
    {
        return false;
    }

    return true;
}

template<typename TD3D12Texture>
TD3D12Texture* CD3D12RHICore::CreateTexture(
    EFormat Format,
    uint32 SizeX, uint32 SizeY, uint32 SizeZ,
    uint32 NumMips,
    uint32 NumSamples,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData,
    const SClearValue& OptimalClearValue )
{
    TSharedRef<TD3D12Texture> NewTexture = DBG_NEW TD3D12Texture( Device, Format, SizeX, SizeY, SizeZ, NumMips, NumSamples, Flags, OptimalClearValue );

    D3D12_RESOURCE_DESC Desc;
    CMemory::Memzero( &Desc );

    Desc.Dimension = GetD3D12TextureResourceDimension<TD3D12Texture>();
    Desc.Flags = ConvertTextureFlags( Flags );
    Desc.Format = ConvertFormat( Format );
    Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    Desc.Width = SizeX;
    Desc.Height = SizeY;
    Desc.DepthOrArraySize = (UINT16)SizeZ;
    Desc.MipLevels = (UINT16)NumMips;
    Desc.Alignment = 0;
    Desc.SampleDesc.Count = NumSamples;

    if ( NumSamples > 1 )
    {
        const int32 Quality = Device->GetMultisampleQuality( Desc.Format, NumSamples );
        Desc.SampleDesc.Quality = Quality - 1;
    }
    else
    {
        Desc.SampleDesc.Quality = 0;
    }

    D3D12_CLEAR_VALUE* ClearValuePtr = nullptr;
    D3D12_CLEAR_VALUE  ClearValue;
    if ( Flags & TextureFlag_RTV || Flags & TextureFlag_DSV )
    {
        ClearValue.Format = (OptimalClearValue.GetFormat() != EFormat::Unknown) ? ConvertFormat( OptimalClearValue.GetFormat() ) : Desc.Format;
        if ( OptimalClearValue.GetType() == SClearValue::EType::DepthStencil )
        {
            ClearValue.DepthStencil.Depth = OptimalClearValue.AsDepthStencil().Depth;
            ClearValue.DepthStencil.Stencil = OptimalClearValue.AsDepthStencil().Stencil;
            ClearValuePtr = &ClearValue;
        }
        else if ( OptimalClearValue.GetType() == SClearValue::EType::Color )
        {
            CMemory::Memcpy( ClearValue.Color, OptimalClearValue.AsColor().Elements, sizeof( float[4] ) );
            ClearValuePtr = &ClearValue;
        }
    }

    TSharedRef<CD3D12Resource> Resource = DBG_NEW CD3D12Resource( Device, Desc, D3D12_HEAP_TYPE_DEFAULT );
    if ( !Resource->Init( D3D12_RESOURCE_STATE_COMMON, ClearValuePtr ) )
    {
        return nullptr;
    }
    else
    {
        NewTexture->SetResource( Resource.ReleaseOwnership() );
    }

    if ( (Flags & TextureFlag_SRV) && !(Flags & TextureFlag_NoDefaultSRV) )
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc;
        CMemory::Memzero( &ViewDesc );

        // TODO: Handle typeless
        ViewDesc.Format = CastShaderResourceFormat( Desc.Format );
        ViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        if ( Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D )
        {
            if ( SizeZ > 6 && IsTextureCube<TD3D12Texture>() )
            {
                ViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                ViewDesc.TextureCubeArray.MipLevels = NumMips;
                ViewDesc.TextureCubeArray.MostDetailedMip = 0;
                ViewDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
                ViewDesc.TextureCubeArray.First2DArrayFace = 0;
                ViewDesc.TextureCubeArray.NumCubes = SizeZ / TEXTURE_CUBE_FACE_COUNT;
            }
            else if ( IsTextureCube<TD3D12Texture>() )
            {
                ViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                ViewDesc.TextureCube.MipLevels = NumMips;
                ViewDesc.TextureCube.MostDetailedMip = 0;
                ViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
            }
            else if ( SizeZ > 1 )
            {
                ViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                ViewDesc.Texture2DArray.MipLevels = NumMips;
                ViewDesc.Texture2DArray.MostDetailedMip = 0;
                ViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
                ViewDesc.Texture2DArray.PlaneSlice = 0;
                ViewDesc.Texture2DArray.ArraySize = SizeZ;
                ViewDesc.Texture2DArray.FirstArraySlice = 0;
            }
            else
            {
                ViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                ViewDesc.Texture2D.MipLevels = NumMips;
                ViewDesc.Texture2D.MostDetailedMip = 0;
                ViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                ViewDesc.Texture2D.PlaneSlice = 0;
            }
        }
        else if ( Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D )
        {
            ViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture3D.MipLevels = NumMips;
            ViewDesc.Texture3D.MostDetailedMip = 0;
            ViewDesc.Texture3D.ResourceMinLODClamp = 0.0f;
        }
        else
        {
            Assert( false );
            return nullptr;
        }

        TSharedRef<CD3D12RHIShaderResourceView> SRV = DBG_NEW CD3D12RHIShaderResourceView( Device, ResourceOfflineDescriptorHeap );
        if ( !SRV->Init() )
        {
            return nullptr;
        }

        if ( !SRV->CreateView( NewTexture->GetResource(), ViewDesc ) )
        {
            return nullptr;
        }

        NewTexture->SetShaderResourceView( SRV.ReleaseOwnership() );
    }

    // TODO: Fix for other resources that Texture2D?
    const bool IsTexture2D = (Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) && (SizeZ == 1);
    if ( Flags & TextureFlag_RTV && !(Flags & TextureFlag_NoDefaultRTV) && IsTexture2D )
    {
        CD3D12RHITexture2D* NewTexture2D = static_cast<CD3D12RHITexture2D*>(NewTexture->AsTexture2D());

        D3D12_RENDER_TARGET_VIEW_DESC ViewDesc;
        CMemory::Memzero( &ViewDesc );

        // TODO: Handle typeless
        ViewDesc.Format = Desc.Format;
        ViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice = 0;
        ViewDesc.Texture2D.PlaneSlice = 0;

        TSharedRef<CD3D12RenderTargetView> RTV = DBG_NEW CD3D12RenderTargetView( Device, RenderTargetOfflineDescriptorHeap );
        if ( !RTV->Init() )
        {
            return nullptr;
        }

        if ( !RTV->CreateView( NewTexture->GetResource(), ViewDesc ) )
        {
            return nullptr;
        }

        NewTexture2D->SetRenderTargetView( RTV.ReleaseOwnership() );
    }

    if ( Flags & TextureFlag_DSV && !(Flags & TextureFlag_NoDefaultDSV) && IsTexture2D )
    {
        CD3D12RHITexture2D* NewTexture2D = static_cast<CD3D12RHITexture2D*>(NewTexture->AsTexture2D());

        D3D12_DEPTH_STENCIL_VIEW_DESC ViewDesc;
        CMemory::Memzero( &ViewDesc );

        // TODO: Handle typeless
        ViewDesc.Format = Desc.Format;
        ViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice = 0;

        TSharedRef<CD3D12DepthStencilView> DSV = DBG_NEW CD3D12DepthStencilView( Device, DepthStencilOfflineDescriptorHeap );
        if ( !DSV->Init() )
        {
            return nullptr;
        }

        if ( !DSV->CreateView( NewTexture->GetResource(), ViewDesc ) )
        {
            return nullptr;
        }

        NewTexture2D->SetDepthStencilView( DSV.ReleaseOwnership() );
    }

    if ( Flags & TextureFlag_UAV && !(Flags & TextureFlag_NoDefaultUAV) && IsTexture2D )
    {
        CD3D12RHITexture2D* NewTexture2D = static_cast<CD3D12RHITexture2D*>(NewTexture->AsTexture2D());

        D3D12_UNORDERED_ACCESS_VIEW_DESC ViewDesc;
        CMemory::Memzero( &ViewDesc );

        // TODO: Handle typeless
        ViewDesc.Format = Desc.Format;
        ViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice = 0;
        ViewDesc.Texture2D.PlaneSlice = 0;

        TSharedRef<CD3D12RHIUnorderedAccessView> UAV = DBG_NEW CD3D12RHIUnorderedAccessView( Device, ResourceOfflineDescriptorHeap );
        if ( !UAV->Init() )
        {
            return nullptr;
        }

        if ( !UAV->CreateView( nullptr, NewTexture->GetResource(), ViewDesc ) )
        {
            return nullptr;
        }

        NewTexture2D->SetUnorderedAccessView( UAV.ReleaseOwnership() );
    }

    if ( InitialData )
    {
        // TODO: Support other types than texture 2D

        CRHITexture2D* Texture2D = NewTexture->AsTexture2D();
        if ( !Texture2D )
        {
            return nullptr;
        }

        DirectCmdContext->Begin();

        DirectCmdContext->TransitionTexture( Texture2D, EResourceState::Common, EResourceState::CopyDest );
        DirectCmdContext->UpdateTexture2D( Texture2D, SizeX, SizeY, 0, InitialData->GetData() );

        // NOTE: Transition into InitialState
        DirectCmdContext->TransitionTexture( Texture2D, EResourceState::CopyDest, InitialState );

        DirectCmdContext->End();
    }
    else
    {
        if ( InitialState != EResourceState::Common )
        {
            DirectCmdContext->Begin();
            DirectCmdContext->TransitionTexture( NewTexture.Get(), EResourceState::Common, InitialState );
            DirectCmdContext->End();
        }
    }

    return NewTexture.ReleaseOwnership();
}

CRHITexture2D* CD3D12RHICore::CreateTexture2D(
    EFormat Format,
    uint32 Width,
    uint32 Height,
    uint32 NumMips,
    uint32 NumSamples,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData,
    const SClearValue& OptimalClearValue )
{
    return CreateTexture<CD3D12RHITexture2D>( Format, Width, Height, 1, NumMips, NumSamples, Flags, InitialState, InitialData, OptimalClearValue );
}

CRHITexture2DArray* CD3D12RHICore::CreateTexture2DArray(
    EFormat Format,
    uint32 Width,
    uint32 Height,
    uint32 NumMips,
    uint32 NumSamples,
    uint32 NumArraySlices,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData,
    const SClearValue& OptimalClearValue )
{
    return CreateTexture<CD3D12RHITexture2DArray>( Format, Width, Height, NumArraySlices, NumMips, NumSamples, Flags, InitialState, InitialData, OptimalClearValue );
}

CRHITextureCube* CD3D12RHICore::CreateTextureCube(
    EFormat Format,
    uint32 Size,
    uint32 NumMips,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData,
    const SClearValue& OptimalClearValue )
{
    return CreateTexture<CD3D12RHITextureCube>( Format, Size, Size, TEXTURE_CUBE_FACE_COUNT, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue );
}

CRHITextureCubeArray* CD3D12RHICore::CreateTextureCubeArray(
    EFormat Format,
    uint32 Size,
    uint32 NumMips,
    uint32 NumArraySlices,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData,
    const SClearValue& OptimalClearValue )
{
    const uint32 ArraySlices = NumArraySlices * TEXTURE_CUBE_FACE_COUNT;
    return CreateTexture<CD3D12RHITextureCubeArray>( Format, Size, Size, ArraySlices, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue );
}

CRHITexture3D* CD3D12RHICore::CreateTexture3D(
    EFormat Format,
    uint32 Width,
    uint32 Height,
    uint32 Depth,
    uint32 NumMips,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData,
    const SClearValue& OptimalClearValue )
{
    return CreateTexture<CD3D12RHITexture3D>( Format, Width, Height, Depth, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue );
}

CRHISamplerState* CD3D12RHICore::CreateSamplerState( const SSamplerStateCreateInfo& CreateInfo )
{
    D3D12_SAMPLER_DESC Desc;
    CMemory::Memzero( &Desc );

    Desc.AddressU = ConvertSamplerMode( CreateInfo.AddressU );
    Desc.AddressV = ConvertSamplerMode( CreateInfo.AddressV );
    Desc.AddressW = ConvertSamplerMode( CreateInfo.AddressW );
    Desc.ComparisonFunc = ConvertComparisonFunc( CreateInfo.ComparisonFunc );
    Desc.Filter = ConvertSamplerFilter( CreateInfo.Filter );
    Desc.MaxAnisotropy = CreateInfo.MaxAnisotropy;
    Desc.MaxLOD = CreateInfo.MaxLOD;
    Desc.MinLOD = CreateInfo.MinLOD;
    Desc.MipLODBias = CreateInfo.MipLODBias;

    CMemory::Memcpy( Desc.BorderColor, CreateInfo.BorderColor.Elements, sizeof( Desc.BorderColor ) );

    TSharedRef<CD3D12RHISamplerState> Sampler = DBG_NEW CD3D12RHISamplerState( Device, SamplerOfflineDescriptorHeap );
    if ( !Sampler->Init( Desc ) )
    {
        return nullptr;
    }
    else
    {
        return Sampler.ReleaseOwnership();
    }
}

template<typename TD3D12Buffer>
bool CD3D12RHICore::FinalizeBufferResource( TD3D12Buffer* Buffer, uint32 SizeInBytes, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData )
{
    D3D12_RESOURCE_DESC Desc;
    CMemory::Memzero( &Desc );

    Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags = ConvertBufferFlags( Flags );
    Desc.Format = DXGI_FORMAT_UNKNOWN;
    Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width = SizeInBytes;
    Desc.Height = 1;
    Desc.DepthOrArraySize = 1;
    Desc.MipLevels = 1;
    Desc.Alignment = 0;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;

    D3D12_HEAP_TYPE       DxHeapType = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES DxInitialState = D3D12_RESOURCE_STATE_COMMON;
    if ( Flags & BufferFlag_Upload )
    {
        DxHeapType = D3D12_HEAP_TYPE_UPLOAD;
        DxInitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    TSharedRef<CD3D12Resource> Resource = DBG_NEW CD3D12Resource( Device, Desc, DxHeapType );
    if ( !Resource->Init( DxInitialState, nullptr ) )
    {
        return false;
    }
    else
    {
        Buffer->SetResource( Resource.ReleaseOwnership() );
    }

    if ( InitialData )
    {
        if ( Buffer->IsUpload() )
        {
            Assert( Buffer->GetSizeInBytes() <= SizeInBytes );

            void* HostData = Buffer->Map( 0, 0 );
            if ( !HostData )
            {
                return false;
            }

            CMemory::Memcpy( HostData, InitialData->GetData(), InitialData->GetSizeInBytes() );
            Buffer->Unmap( 0, 0 );
        }
        else
        {
            DirectCmdContext->Begin();

            DirectCmdContext->TransitionBuffer( Buffer, EResourceState::Common, EResourceState::CopyDest );
            DirectCmdContext->UpdateBuffer( Buffer, 0, InitialData->GetSizeInBytes(), InitialData->GetData() );

            // NOTE: Transfer to the initialstate
            DirectCmdContext->TransitionBuffer( Buffer, EResourceState::CopyDest, InitialState );

            DirectCmdContext->End();
        }
    }
    else
    {
        if ( InitialState != EResourceState::Common && !Buffer->IsUpload() )
        {
            DirectCmdContext->Begin();
            DirectCmdContext->TransitionBuffer( Buffer, EResourceState::Common, InitialState );
            DirectCmdContext->End();
        }
    }

    return true;
}

CRHIVertexBuffer* CD3D12RHICore::CreateVertexBuffer( uint32 Stride, uint32 NumVertices, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData )
{
    const uint32 SizeInBytes = NumVertices * Stride;

    TSharedRef<CD3D12RHIVertexBuffer> NewBuffer = DBG_NEW CD3D12RHIVertexBuffer( Device, NumVertices, Stride, Flags );
    if ( !FinalizeBufferResource<CD3D12RHIVertexBuffer>( NewBuffer.Get(), SizeInBytes, Flags, InitialState, InitialData ) )
    {
        LOG_ERROR( "[D3D12RenderLayer]: Failed to create VertexBuffer" );
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIIndexBuffer* CD3D12RHICore::CreateIndexBuffer( EIndexFormat Format, uint32 NumIndices, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData )
{
    const uint32 SizeInBytes = NumIndices * GetStrideFromIndexFormat( Format );
    const uint32 AlignedSizeInBytes = NMath::AlignUp<uint32>( SizeInBytes, sizeof( uint32 ) );

    TSharedRef<CD3D12RHIIndexBuffer> NewBuffer = DBG_NEW CD3D12RHIIndexBuffer( Device, Format, NumIndices, Flags );
    if ( !FinalizeBufferResource<CD3D12RHIIndexBuffer>( NewBuffer.Get(), AlignedSizeInBytes, Flags, InitialState, InitialData ) )
    {
        LOG_ERROR( "[D3D12RenderLayer]: Failed to create IndexBuffer" );
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIConstantBuffer* CD3D12RHICore::CreateConstantBuffer( uint32 Size, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData )
{
    Assert( !(Flags & BufferFlag_UAV) && !(Flags & BufferFlag_SRV) );

    const uint32 AlignedSizeInBytes = NMath::AlignUp<uint32>( Size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT );

    TSharedRef<CD3D12RHIConstantBuffer> NewBuffer = DBG_NEW CD3D12RHIConstantBuffer( Device, ResourceOfflineDescriptorHeap, Size, Flags );
    if ( !FinalizeBufferResource<CD3D12RHIConstantBuffer>( NewBuffer.Get(), AlignedSizeInBytes, Flags, InitialState, InitialData ) )
    {
        LOG_ERROR( "[D3D12RenderLayer]: Failed to create ConstantBuffer" );
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIStructuredBuffer* CD3D12RHICore::CreateStructuredBuffer( uint32 Stride, uint32 NumElements, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData )
{
    const uint32 SizeInBytes = NumElements * Stride;

    TSharedRef<CD3D12RHIStructuredBuffer> NewBuffer = DBG_NEW CD3D12RHIStructuredBuffer( Device, NumElements, Stride, Flags );
    if ( !FinalizeBufferResource<CD3D12RHIStructuredBuffer>( NewBuffer.Get(), SizeInBytes, Flags, InitialState, InitialData ) )
    {
        LOG_ERROR( "[D3D12RenderLayer]: Failed to create StructuredBuffer" );
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIRayTracingGeometry* CD3D12RHICore::CreateRayTracingGeometry( uint32 Flags, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer )
{
    CD3D12RHIVertexBuffer* DxVertexBuffer = static_cast<CD3D12RHIVertexBuffer*>(VertexBuffer);
    CD3D12RHIIndexBuffer* DxIndexBuffer = static_cast<CD3D12RHIIndexBuffer*>(IndexBuffer);

    TSharedRef<CD3D12RHIRayTracingGeometry> Geometry = DBG_NEW CD3D12RHIRayTracingGeometry( Device, Flags );
    Geometry->VertexBuffer = MakeSharedRef<CD3D12RHIVertexBuffer>( DxVertexBuffer );
    Geometry->IndexBuffer = MakeSharedRef<CD3D12RHIIndexBuffer>( DxIndexBuffer );

    DirectCmdContext->Begin();

    if ( !Geometry->Build( *DirectCmdContext, false ) )
    {
        CDebug::DebugBreak();
        Geometry.Reset();
    }

    DirectCmdContext->End();

    return Geometry.ReleaseOwnership();
}

CRHIRayTracingScene* CD3D12RHICore::CreateRayTracingScene( uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances )
{
    TSharedRef<CD3D12RHIRayTracingScene> Scene = DBG_NEW CD3D12RHIRayTracingScene( Device, Flags );

    DirectCmdContext->Begin();

    if ( !Scene->Build( *DirectCmdContext, Instances, NumInstances, false ) )
    {
        CDebug::DebugBreak();
        Scene.Reset();
    }

    DirectCmdContext->End();

    return Scene.ReleaseOwnership();
}

CRHIShaderResourceView* CD3D12RHICore::CreateShaderResourceView( const SShaderResourceViewCreateInfo& CreateInfo )
{
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    CMemory::Memzero( &Desc );

    // TODO: Expose in ShaderResourceViewCreateInfo
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    CD3D12Resource* Resource = nullptr;
    if ( CreateInfo.Type == SShaderResourceViewCreateInfo::EType::Texture2D )
    {
        CRHITexture2D* Texture = CreateInfo.Texture2D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsSRV() && CreateInfo.Texture2D.Format != EFormat::Unknown );

        Desc.Format = ConvertFormat( CreateInfo.Texture2D.Format );
        if ( !Texture->IsMultiSampled() )
        {
            Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipLevels = CreateInfo.Texture2D.NumMips;
            Desc.Texture2D.MostDetailedMip = CreateInfo.Texture2D.Mip;
            Desc.Texture2D.ResourceMinLODClamp = CreateInfo.Texture2D.MinMipBias;
            Desc.Texture2D.PlaneSlice = 0;
        }
        else
        {
            Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        }
    }
    else if ( CreateInfo.Type == SShaderResourceViewCreateInfo::EType::Texture2DArray )
    {
        CRHITexture2DArray* Texture = CreateInfo.Texture2DArray.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsSRV() && CreateInfo.Texture2DArray.Format != EFormat::Unknown );

        Desc.Format = ConvertFormat( CreateInfo.Texture2DArray.Format );
        if ( !Texture->IsMultiSampled() )
        {
            Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipLevels = CreateInfo.Texture2DArray.NumMips;
            Desc.Texture2DArray.MostDetailedMip = CreateInfo.Texture2DArray.Mip;
            Desc.Texture2DArray.ResourceMinLODClamp = CreateInfo.Texture2DArray.MinMipBias;
            Desc.Texture2DArray.ArraySize = CreateInfo.Texture2DArray.NumArraySlices;
            Desc.Texture2DArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
            Desc.Texture2DArray.PlaneSlice = 0;
        }
        else
        {
            Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize = CreateInfo.Texture2DArray.NumArraySlices;
            Desc.Texture2DMSArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
        }
    }
    else if ( CreateInfo.Type == SShaderResourceViewCreateInfo::EType::TextureCube )
    {
        CRHITextureCube* Texture = CreateInfo.TextureCube.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsSRV() && CreateInfo.TextureCube.Format != EFormat::Unknown );

        Desc.Format = ConvertFormat( CreateInfo.Texture2D.Format );
        Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        Desc.TextureCube.MipLevels = CreateInfo.TextureCube.NumMips;
        Desc.TextureCube.MostDetailedMip = CreateInfo.TextureCube.Mip;
        Desc.TextureCube.ResourceMinLODClamp = CreateInfo.TextureCube.MinMipBias;
    }
    else if ( CreateInfo.Type == SShaderResourceViewCreateInfo::EType::TextureCubeArray )
    {
        CRHITextureCubeArray* Texture = CreateInfo.TextureCubeArray.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsSRV() && CreateInfo.TextureCubeArray.Format != EFormat::Unknown );

        Desc.Format = ConvertFormat( CreateInfo.Texture2D.Format );
        Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        Desc.TextureCubeArray.MipLevels = CreateInfo.TextureCubeArray.NumMips;
        Desc.TextureCubeArray.MostDetailedMip = CreateInfo.TextureCubeArray.Mip;
        Desc.TextureCubeArray.ResourceMinLODClamp = CreateInfo.TextureCubeArray.MinMipBias;
        // ArraySlice * 6 to get the first Texture2D face
        Desc.TextureCubeArray.First2DArrayFace = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT;
        Desc.TextureCubeArray.NumCubes = CreateInfo.TextureCubeArray.NumArraySlices;
    }
    else if ( CreateInfo.Type == SShaderResourceViewCreateInfo::EType::Texture3D )
    {
        CRHITexture3D* Texture = CreateInfo.Texture3D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsSRV() && CreateInfo.Texture3D.Format != EFormat::Unknown );

        Desc.Format = ConvertFormat( CreateInfo.Texture3D.Format );
        Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipLevels = CreateInfo.Texture3D.NumMips;
        Desc.Texture3D.MostDetailedMip = CreateInfo.Texture3D.Mip;
        Desc.Texture3D.ResourceMinLODClamp = CreateInfo.Texture3D.MinMipBias;
    }
    else if ( CreateInfo.Type == SShaderResourceViewCreateInfo::EType::VertexBuffer )
    {
        CRHIVertexBuffer* Buffer = CreateInfo.VertexBuffer.Buffer;
        CD3D12BaseBuffer* DxBuffer = D3D12BufferCast( Buffer );
        Resource = DxBuffer->GetResource();

        Assert( Buffer->IsSRV() );

        Desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = CreateInfo.VertexBuffer.FirstVertex;
        Desc.Buffer.NumElements = CreateInfo.VertexBuffer.NumVertices;
        Desc.Format = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }
    else if ( CreateInfo.Type == SShaderResourceViewCreateInfo::EType::IndexBuffer )
    {
        CRHIIndexBuffer* Buffer = CreateInfo.IndexBuffer.Buffer;
        CD3D12BaseBuffer* DxBuffer = D3D12BufferCast( Buffer );
        Resource = DxBuffer->GetResource();

        Assert( Buffer->IsSRV() );
        Assert( Buffer->GetFormat() != EIndexFormat::uint16 );

        Desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = CreateInfo.IndexBuffer.FirstIndex;
        Desc.Buffer.NumElements = CreateInfo.IndexBuffer.NumIndices;
        Desc.Format = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }
    else if ( CreateInfo.Type == SShaderResourceViewCreateInfo::EType::StructuredBuffer )
    {
        CRHIStructuredBuffer* Buffer = CreateInfo.StructuredBuffer.Buffer;
        CD3D12BaseBuffer* DxBuffer = D3D12BufferCast( Buffer );
        Resource = DxBuffer->GetResource();

        Assert( Buffer->IsSRV() );

        Desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = CreateInfo.StructuredBuffer.FirstElement;
        Desc.Buffer.NumElements = CreateInfo.StructuredBuffer.NumElements;
        Desc.Format = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }

    Assert( Resource != nullptr );

    TSharedRef<CD3D12RHIShaderResourceView> DxView = DBG_NEW CD3D12RHIShaderResourceView( Device, ResourceOfflineDescriptorHeap );
    if ( !DxView->Init() )
    {
        return nullptr;
    }

    if ( DxView->CreateView( Resource, Desc ) )
    {
        return DxView.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

CRHIUnorderedAccessView* CD3D12RHICore::CreateUnorderedAccessView( const SUnorderedAccessViewCreateInfo& CreateInfo )
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
    CMemory::Memzero( &Desc );

    CD3D12Resource* Resource = nullptr;
    if ( CreateInfo.Type == SUnorderedAccessViewCreateInfo::EType::Texture2D )
    {
        CRHITexture2D* Texture = CreateInfo.Texture2D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsUAV() && CreateInfo.Texture2D.Format != EFormat::Unknown );

        Desc.Format = ConvertFormat( CreateInfo.Texture2D.Format );
        Desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        Desc.Texture2D.MipSlice = CreateInfo.Texture2D.Mip;
        Desc.Texture2D.PlaneSlice = 0;
    }
    else if ( CreateInfo.Type == SUnorderedAccessViewCreateInfo::EType::Texture2DArray )
    {
        CRHITexture2DArray* Texture = CreateInfo.Texture2DArray.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsUAV() && CreateInfo.Texture2DArray.Format != EFormat::Unknown );

        Desc.Format = ConvertFormat( CreateInfo.Texture2DArray.Format );
        Desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice = CreateInfo.Texture2DArray.Mip;
        Desc.Texture2DArray.ArraySize = CreateInfo.Texture2DArray.NumArraySlices;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
        Desc.Texture2DArray.PlaneSlice = 0;
    }
    else if ( CreateInfo.Type == SUnorderedAccessViewCreateInfo::EType::TextureCube )
    {
        CRHITextureCube* Texture = CreateInfo.TextureCube.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsUAV() && CreateInfo.TextureCube.Format != EFormat::Unknown );

        Desc.Format = ConvertFormat( CreateInfo.TextureCube.Format );
        Desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice = CreateInfo.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize = TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.FirstArraySlice = 0;
        Desc.Texture2DArray.PlaneSlice = 0;
    }
    else if ( CreateInfo.Type == SUnorderedAccessViewCreateInfo::EType::TextureCubeArray )
    {
        CRHITextureCubeArray* Texture = CreateInfo.TextureCubeArray.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsUAV() && CreateInfo.TextureCubeArray.Format != EFormat::Unknown );

        Desc.Format = ConvertFormat( CreateInfo.TextureCubeArray.Format );
        Desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice = CreateInfo.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize = CreateInfo.TextureCubeArray.NumArraySlices * TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.PlaneSlice = 0;
    }
    else if ( CreateInfo.Type == SUnorderedAccessViewCreateInfo::EType::Texture3D )
    {
        CRHITexture3D* Texture = CreateInfo.Texture3D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsUAV() && CreateInfo.Texture3D.Format != EFormat::Unknown );

        Desc.Format = ConvertFormat( CreateInfo.Texture3D.Format );
        Desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipSlice = CreateInfo.Texture3D.Mip;
        Desc.Texture3D.FirstWSlice = CreateInfo.Texture3D.DepthSlice;
        Desc.Texture3D.WSize = CreateInfo.Texture3D.NumDepthSlices;
    }
    else if ( CreateInfo.Type == SUnorderedAccessViewCreateInfo::EType::VertexBuffer )
    {
        CRHIVertexBuffer* Buffer = CreateInfo.VertexBuffer.Buffer;
        CD3D12BaseBuffer* DxBuffer = D3D12BufferCast( Buffer );
        Resource = DxBuffer->GetResource();

        Assert( Buffer->IsUAV() );

        Desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = CreateInfo.VertexBuffer.FirstVertex;
        Desc.Buffer.NumElements = CreateInfo.VertexBuffer.NumVertices;
        Desc.Format = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }
    else if ( CreateInfo.Type == SUnorderedAccessViewCreateInfo::EType::IndexBuffer )
    {
        CRHIIndexBuffer* Buffer = CreateInfo.IndexBuffer.Buffer;
        CD3D12BaseBuffer* DxBuffer = D3D12BufferCast( Buffer );
        Resource = DxBuffer->GetResource();

        Assert( Buffer->IsUAV() );

        Desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = CreateInfo.IndexBuffer.FirstIndex;
        Desc.Buffer.NumElements = CreateInfo.IndexBuffer.NumIndices;

        // TODO: What if the index type is 16-bit?
        Assert( Buffer->GetFormat() != EIndexFormat::uint16 );

        Desc.Format = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }
    else if ( CreateInfo.Type == SUnorderedAccessViewCreateInfo::EType::StructuredBuffer )
    {
        CRHIStructuredBuffer* Buffer = CreateInfo.StructuredBuffer.Buffer;
        CD3D12BaseBuffer* DxBuffer = D3D12BufferCast( Buffer );
        Resource = DxBuffer->GetResource();

        Assert( Buffer->IsUAV() );

        Desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = CreateInfo.StructuredBuffer.FirstElement;
        Desc.Buffer.NumElements = CreateInfo.StructuredBuffer.NumElements;
        Desc.Format = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }

    TSharedRef<CD3D12RHIUnorderedAccessView> DxView = DBG_NEW CD3D12RHIUnorderedAccessView( Device, ResourceOfflineDescriptorHeap );
    if ( !DxView->Init() )
    {
        return nullptr;
    }

    Assert( Resource != nullptr );

    // TODO: Expose counterresource
    if ( DxView->CreateView( nullptr, Resource, Desc ) )
    {
        return DxView.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

CRHIRenderTargetView* CD3D12RHICore::CreateRenderTargetView( const SRenderTargetViewCreateInfo& CreateInfo )
{
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
    CMemory::Memzero( &Desc );

    CD3D12Resource* Resource = nullptr;

    Desc.Format = ConvertFormat( CreateInfo.Format );
    Assert( CreateInfo.Format != EFormat::Unknown );

    if ( CreateInfo.Type == SRenderTargetViewCreateInfo::EType::Texture2D )
    {
        CRHITexture2D* Texture = CreateInfo.Texture2D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsRTV() );

        if ( Texture->IsMultiSampled() )
        {
            Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipSlice = CreateInfo.Texture2D.Mip;
            Desc.Texture2D.PlaneSlice = 0;
        }
        else
        {
            Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        }
    }
    else if ( CreateInfo.Type == SRenderTargetViewCreateInfo::EType::Texture2DArray )
    {
        CRHITexture2DArray* Texture = CreateInfo.Texture2DArray.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsRTV() );

        if ( Texture->IsMultiSampled() )
        {
            Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice = CreateInfo.Texture2DArray.Mip;
            Desc.Texture2DArray.ArraySize = CreateInfo.Texture2DArray.NumArraySlices;
            Desc.Texture2DArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
            Desc.Texture2DArray.PlaneSlice = 0;
        }
        else
        {
            Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize = CreateInfo.Texture2DArray.NumArraySlices;
            Desc.Texture2DMSArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
        }
    }
    else if ( CreateInfo.Type == SRenderTargetViewCreateInfo::EType::TextureCube )
    {
        CRHITextureCube* Texture = CreateInfo.TextureCube.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsRTV() );

        Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice = CreateInfo.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize = 1;
        Desc.Texture2DArray.FirstArraySlice = GetCubeFaceIndex( CreateInfo.TextureCube.CubeFace );
        Desc.Texture2DArray.PlaneSlice = 0;
    }
    else if ( CreateInfo.Type == SRenderTargetViewCreateInfo::EType::TextureCubeArray )
    {
        CRHITextureCubeArray* Texture = CreateInfo.TextureCubeArray.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsRTV() );

        Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice = CreateInfo.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize = 1;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT + GetCubeFaceIndex( CreateInfo.TextureCube.CubeFace );
        Desc.Texture2DArray.PlaneSlice = 0;
    }
    else if ( CreateInfo.Type == SRenderTargetViewCreateInfo::EType::Texture3D )
    {
        CRHITexture3D* Texture = CreateInfo.Texture3D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsRTV() );

        Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipSlice = CreateInfo.Texture3D.Mip;
        Desc.Texture3D.FirstWSlice = CreateInfo.Texture3D.DepthSlice;
        Desc.Texture3D.WSize = CreateInfo.Texture3D.NumDepthSlices;
    }

    TSharedRef<CD3D12RenderTargetView> DxView = DBG_NEW CD3D12RenderTargetView( Device, RenderTargetOfflineDescriptorHeap );
    if ( !DxView->Init() )
    {
        return nullptr;
    }

    Assert( Resource != nullptr );

    if ( !DxView->CreateView( Resource, Desc ) )
    {
        return nullptr;
    }
    else
    {
        return DxView.ReleaseOwnership();
    }
}

CRHIDepthStencilView* CD3D12RHICore::CreateDepthStencilView( const SDepthStencilViewCreateInfo& CreateInfo )
{
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
    CMemory::Memzero( &Desc );

    CD3D12Resource* Resource = nullptr;

    Desc.Format = ConvertFormat( CreateInfo.Format );
    Assert( CreateInfo.Format != EFormat::Unknown );

    if ( CreateInfo.Type == SDepthStencilViewCreateInfo::EType::Texture2D )
    {
        CRHITexture2D* Texture = CreateInfo.Texture2D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsDSV() );

        if ( Texture->IsMultiSampled() )
        {
            Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipSlice = CreateInfo.Texture2D.Mip;
        }
        else
        {
            Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        }
    }
    else if ( CreateInfo.Type == SDepthStencilViewCreateInfo::EType::Texture2DArray )
    {
        CRHITexture2DArray* Texture = CreateInfo.Texture2DArray.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsDSV() );

        if ( Texture->IsMultiSampled() )
        {
            Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice = CreateInfo.Texture2DArray.Mip;
            Desc.Texture2DArray.ArraySize = CreateInfo.Texture2DArray.NumArraySlices;
            Desc.Texture2DArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
        }
        else
        {
            Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize = CreateInfo.Texture2DArray.NumArraySlices;
            Desc.Texture2DMSArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
        }
    }
    else if ( CreateInfo.Type == SDepthStencilViewCreateInfo::EType::TextureCube )
    {
        CRHITextureCube* Texture = CreateInfo.TextureCube.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsDSV() );

        Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice = CreateInfo.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize = 1;
        Desc.Texture2DArray.FirstArraySlice = GetCubeFaceIndex( CreateInfo.TextureCube.CubeFace );
    }
    else if ( CreateInfo.Type == SDepthStencilViewCreateInfo::EType::TextureCubeArray )
    {
        CRHITextureCubeArray* Texture = CreateInfo.TextureCubeArray.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
        Resource = DxTexture->GetResource();

        Assert( Texture->IsDSV() );

        Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice = CreateInfo.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize = 1;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT + GetCubeFaceIndex( CreateInfo.TextureCube.CubeFace );
    }

    TSharedRef<CD3D12DepthStencilView> DxView = DBG_NEW CD3D12DepthStencilView( Device, DepthStencilOfflineDescriptorHeap );
    if ( !DxView->Init() )
    {
        return nullptr;
    }

    if ( !DxView->CreateView( Resource, Desc ) )
    {
        return nullptr;
    }
    else
    {
        return DxView.ReleaseOwnership();
    }
}

CRHIComputeShader* CD3D12RHICore::CreateComputeShader( const TArray<uint8>& ShaderCode )
{
    TSharedRef<CD3D12RHIComputeShader> Shader = DBG_NEW CD3D12RHIComputeShader( Device, ShaderCode );
    if ( !Shader->Init() )
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIVertexShader* CD3D12RHICore::CreateVertexShader( const TArray<uint8>& ShaderCode )
{
    TSharedRef<CD3D12RHIVertexShader> Shader = DBG_NEW CD3D12RHIVertexShader( Device, ShaderCode );
    if ( !CD3D12BaseShader::GetShaderReflection( Shader.Get() ) )
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIHullShader* CD3D12RHICore::CreateHullShader( const TArray<uint8>& ShaderCode )
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE( ShaderCode );
    return nullptr;
}

CRHIDomainShader* CD3D12RHICore::CreateDomainShader( const TArray<uint8>& ShaderCode )
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE( ShaderCode );
    return nullptr;
}

CRHIGeometryShader* CD3D12RHICore::CreateGeometryShader( const TArray<uint8>& ShaderCode )
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE( ShaderCode );
    return nullptr;
}

CRHIMeshShader* CD3D12RHICore::CreateMeshShader( const TArray<uint8>& ShaderCode )
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE( ShaderCode );
    return nullptr;
}

CRHIAmplificationShader* CD3D12RHICore::CreateAmplificationShader( const TArray<uint8>& ShaderCode )
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE( ShaderCode );
    return nullptr;
}

CRHIPixelShader* CD3D12RHICore::CreatePixelShader( const TArray<uint8>& ShaderCode )
{
    TSharedRef<CD3D12RHIPixelShader> Shader = DBG_NEW CD3D12RHIPixelShader( Device, ShaderCode );
    if ( !CD3D12BaseShader::GetShaderReflection( Shader.Get() ) )
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIRayGenShader* CD3D12RHICore::CreateRayGenShader( const TArray<uint8>& ShaderCode )
{
    TSharedRef<CD3D12RHIRayGenShader> Shader = DBG_NEW CD3D12RHIRayGenShader( Device, ShaderCode );
    if ( !CD3D12RHIBaseRayTracingShader::GetRayTracingShaderReflection( Shader.Get() ) )
    {
        LOG_ERROR( "[D3D12RenderLayer]: Failed to retrive Shader Identifier" );
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayAnyHitShader* CD3D12RHICore::CreateRayAnyHitShader( const TArray<uint8>& ShaderCode )
{
    TSharedRef<CD3D12RHIRayAnyHitShader> Shader = DBG_NEW CD3D12RHIRayAnyHitShader( Device, ShaderCode );
    if ( !CD3D12RHIBaseRayTracingShader::GetRayTracingShaderReflection( Shader.Get() ) )
    {
        LOG_ERROR( "[D3D12RenderLayer]: Failed to retrive Shader Identifier" );
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayClosestHitShader* CD3D12RHICore::CreateRayClosestHitShader( const TArray<uint8>& ShaderCode )
{
    TSharedRef<CD3D12RayClosestHitShader> Shader = DBG_NEW CD3D12RayClosestHitShader( Device, ShaderCode );
    if ( !CD3D12RHIBaseRayTracingShader::GetRayTracingShaderReflection( Shader.Get() ) )
    {
        LOG_ERROR( "[D3D12RenderLayer]: Failed to retrive Shader Identifier" );
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayMissShader* CD3D12RHICore::CreateRayMissShader( const TArray<uint8>& ShaderCode )
{
    TSharedRef<CD3D12RHIRayMissShader> Shader = DBG_NEW CD3D12RHIRayMissShader( Device, ShaderCode );
    if ( !CD3D12RHIBaseRayTracingShader::GetRayTracingShaderReflection( Shader.Get() ) )
    {
        LOG_ERROR( "[D3D12RenderLayer]: Failed to retrive Shader Identifier" );
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIDepthStencilState* CD3D12RHICore::CreateDepthStencilState( const SDepthStencilStateCreateInfo& CreateInfo )
{
    D3D12_DEPTH_STENCIL_DESC Desc;
    CMemory::Memzero( &Desc );

    Desc.DepthEnable = CreateInfo.DepthEnable;
    Desc.DepthFunc = ConvertComparisonFunc( CreateInfo.DepthFunc );
    Desc.DepthWriteMask = ConvertDepthWriteMask( CreateInfo.DepthWriteMask );
    Desc.StencilEnable = CreateInfo.StencilEnable;
    Desc.StencilReadMask = CreateInfo.StencilReadMask;
    Desc.StencilWriteMask = CreateInfo.StencilWriteMask;
    Desc.FrontFace = ConvertDepthStencilOp( CreateInfo.FrontFace );
    Desc.BackFace = ConvertDepthStencilOp( CreateInfo.BackFace );

    return DBG_NEW CD3D12RHIDepthStencilState( Device, Desc );
}

CRHIRasterizerState* CD3D12RHICore::CreateRasterizerState( const SRasterizerStateCreateInfo& CreateInfo )
{
    D3D12_RASTERIZER_DESC Desc;
    CMemory::Memzero( &Desc );

    Desc.AntialiasedLineEnable = CreateInfo.AntialiasedLineEnable;
    Desc.ConservativeRaster = (CreateInfo.EnableConservativeRaster) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    Desc.CullMode = ConvertCullMode( CreateInfo.CullMode );
    Desc.DepthBias = CreateInfo.DepthBias;
    Desc.DepthBiasClamp = CreateInfo.DepthBiasClamp;
    Desc.DepthClipEnable = CreateInfo.DepthClipEnable;
    Desc.SlopeScaledDepthBias = CreateInfo.SlopeScaledDepthBias;
    Desc.FillMode = ConvertFillMode( CreateInfo.FillMode );
    Desc.ForcedSampleCount = CreateInfo.ForcedSampleCount;
    Desc.FrontCounterClockwise = CreateInfo.FrontCounterClockwise;
    Desc.MultisampleEnable = CreateInfo.MultisampleEnable;

    return DBG_NEW CD3D12RHIRasterizerState( Device, Desc );
}

CRHIBlendState* CD3D12RHICore::CreateBlendState( const SBlendStateCreateInfo& CreateInfo )
{
    D3D12_BLEND_DESC Desc;
    CMemory::Memzero( &Desc );

    Desc.AlphaToCoverageEnable = CreateInfo.AlphaToCoverageEnable;
    Desc.IndependentBlendEnable = CreateInfo.IndependentBlendEnable;
    for ( uint32 i = 0; i < 8; i++ )
    {
        Desc.RenderTarget[i].BlendEnable = CreateInfo.RenderTarget[i].BlendEnable;
        Desc.RenderTarget[i].BlendOp = ConvertBlendOp( CreateInfo.RenderTarget[i].BlendOp );
        Desc.RenderTarget[i].BlendOpAlpha = ConvertBlendOp( CreateInfo.RenderTarget[i].BlendOpAlpha );
        Desc.RenderTarget[i].DestBlend = ConvertBlend( CreateInfo.RenderTarget[i].DestBlend );
        Desc.RenderTarget[i].DestBlendAlpha = ConvertBlend( CreateInfo.RenderTarget[i].DestBlendAlpha );
        Desc.RenderTarget[i].SrcBlend = ConvertBlend( CreateInfo.RenderTarget[i].SrcBlend );
        Desc.RenderTarget[i].SrcBlendAlpha = ConvertBlend( CreateInfo.RenderTarget[i].SrcBlendAlpha );
        Desc.RenderTarget[i].LogicOpEnable = CreateInfo.RenderTarget[i].LogicOpEnable;
        Desc.RenderTarget[i].LogicOp = ConvertLogicOp( CreateInfo.RenderTarget[i].LogicOp );
        Desc.RenderTarget[i].RenderTargetWriteMask = ConvertRenderTargetWriteState( CreateInfo.RenderTarget[i].RenderTargetWriteMask );
    }

    return DBG_NEW CD3D12RHIBlendState( Device, Desc );
}

CRHIInputLayoutState* CD3D12RHICore::CreateInputLayout( const SInputLayoutStateCreateInfo& CreateInfo )
{
    return DBG_NEW CD3D12RHIInputLayoutState( Device, CreateInfo );
}

CRHIGraphicsPipelineState* CD3D12RHICore::CreateGraphicsPipelineState( const SGraphicsPipelineStateCreateInfo& CreateInfo )
{
    TSharedRef<CD3D12RHIGraphicsPipelineState> NewPipelineState = DBG_NEW CD3D12RHIGraphicsPipelineState( Device );
    if ( !NewPipelineState->Init( CreateInfo ) )
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

CRHIComputePipelineState* CD3D12RHICore::CreateComputePipelineState( const SComputePipelineStateCreateInfo& Info )
{
    Assert( Info.Shader != nullptr );

    TSharedRef<CD3D12RHIComputeShader> Shader = MakeSharedRef<CD3D12RHIComputeShader>( Info.Shader );
    TSharedRef<CD3D12RHIComputePipelineState> NewPipelineState = DBG_NEW CD3D12RHIComputePipelineState( Device, Shader );
    if ( !NewPipelineState->Init() )
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

CRHIRayTracingPipelineState* CD3D12RHICore::CreateRayTracingPipelineState( const SRayTracingPipelineStateCreateInfo& CreateInfo )
{
    TSharedRef<CD3D12RHIRayTracingPipelineState> NewPipelineState = DBG_NEW CD3D12RHIRayTracingPipelineState( Device );
    if ( NewPipelineState->Init( CreateInfo ) )
    {
        return NewPipelineState.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

CGPUProfiler* CD3D12RHICore::CreateProfiler()
{
    return CD3D12GPUProfiler::Create( Device );
}

CRHIViewport* CD3D12RHICore::CreateViewport( CCoreWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat )
{
    UNREFERENCED_VARIABLE( DepthFormat );

    // TODO: Take DepthFormat into account

    TSharedRef<CWindowsWindow> WinWindow = MakeSharedRef<CWindowsWindow>( Window );
    if ( Width == 0 )
    {
        Width = WinWindow->GetWidth();
    }

    if ( Height == 0 )
    {
        Height = WinWindow->GetHeight();
    }

    TSharedRef<CD3D12RHIViewport> Viewport = DBG_NEW CD3D12RHIViewport( Device, DirectCmdContext.Get(), WinWindow->GetHandle(), ColorFormat, Width, Height );
    if ( Viewport->Init() )
    {
        return Viewport.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

bool CD3D12RHICore::UAVSupportsFormat( EFormat Format ) const
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData;
    CMemory::Memzero( &FeatureData, sizeof( D3D12_FEATURE_DATA_D3D12_OPTIONS ) );

    HRESULT Result = Device->GetDevice()->CheckFeatureSupport( D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof( D3D12_FEATURE_DATA_D3D12_OPTIONS ) );
    if ( SUCCEEDED( Result ) )
    {
        if ( FeatureData.TypedUAVLoadAdditionalFormats )
        {
            D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport =
            {
                ConvertFormat( Format ),
                D3D12_FORMAT_SUPPORT1_NONE,
                D3D12_FORMAT_SUPPORT2_NONE
            };

            Result = Device->GetDevice()->CheckFeatureSupport( D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof( FormatSupport ) );
            if ( FAILED( Result ) || (FormatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) == 0 )
            {
                return false;
            }
        }
    }

    return true;
}

void CD3D12RHICore::CheckRayTracingSupport( SRayTracingSupport& OutSupport ) const
{
    D3D12_RAYTRACING_TIER Tier = Device->GetRayTracingTier();
    if ( Tier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED )
    {
        if ( Tier == D3D12_RAYTRACING_TIER_1_1 )
        {
            OutSupport.Tier = ERayTracingTier::Tier1_1;
        }
        else if ( Tier == D3D12_RAYTRACING_TIER_1_0 )
        {
            OutSupport.Tier = ERayTracingTier::Tier1;
        }

        OutSupport.MaxRecursionDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;
    }
    else
    {
        OutSupport.Tier = ERayTracingTier::NotSupported;
    }
}

void CD3D12RHICore::CheckShadingRateSupport( SShadingRateSupport& OutSupport ) const
{
    D3D12_VARIABLE_SHADING_RATE_TIER Tier = Device->GetVariableRateShadingTier();
    if ( Tier == D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED )
    {
        OutSupport.Tier = EShadingRateTier::NotSupported;
    }
    else if ( Tier == D3D12_VARIABLE_SHADING_RATE_TIER_1 )
    {
        OutSupport.Tier = EShadingRateTier::Tier1;
    }
    else if ( Tier == D3D12_VARIABLE_SHADING_RATE_TIER_2 )
    {
        OutSupport.Tier = EShadingRateTier::Tier2;
    }

    OutSupport.ShadingRateImageTileSize = Device->GetVariableRateShadingTileSize();
}
