#pragma once
#include "D3D12RootSignature.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12CommandList.h"
#include "D3D12RHIBuffer.h"
#include "D3D12RHIViews.h"
#include "D3D12RHISamplerState.h"

template <typename ViewType, D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint32 kDescriptorTableSize>
class TD3D12ViewCache
{
public:

    static FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapType()
    {
        return HeapType;
    }

    static FORCEINLINE uint32 GetDescriptorTableSize()
    {
        return kDescriptorTableSize;
    }

    TD3D12ViewCache()
        : ResourceViews()
        , HostDescriptors()
        , DeviceDescriptors()
        , CopyDescriptors()
        , Dirty()
    {
    }

    FORCEINLINE void SetView( ViewType* DescriptorView, EShaderVisibility Visibility, uint32 ShaderRegister )
    {
        D3D12_ERROR( DescriptorView != nullptr, "[D3D12]: Trying to bind a ResourceView that was nullptr, check input from DescriptorCache" );

        ViewType* CurrentDescriptorView = ResourceViews[Visibility][ShaderRegister];
        if ( DescriptorView != CurrentDescriptorView )
        {
            ResourceViews[Visibility][ShaderRegister] = DescriptorView;
            Dirty[Visibility] = true;
        }
    }

    void Reset( ViewType* DefaultView )
    {
        CMemory::Memzero( HostDescriptors, sizeof( HostDescriptors ) );
        CMemory::Memzero( DeviceDescriptors, sizeof( DeviceDescriptors ) );
        CMemory::Memzero( CopyDescriptors, sizeof( CopyDescriptors ) );

        for ( uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++ )
        {
            // Set each stage to be dirty
            Dirty[Stage] = true;
            
            // Set each descriptor to the default view
            for (uint32 Index = 0; Index < kDescriptorTableSize; Index++ )
            {
                ResourceViews[Stage][Index] = DefaultView;
            }
        }
    }

    FORCEINLINE uint32 CountNeededDescriptors() const
    {
        uint32 NumDescriptors = 0;
        for ( uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++ )
        {
            if ( Dirty[Stage] )
            {
                NumDescriptors += kDescriptorTableSize;
            }
        }

        return NumDescriptors;
    }

    void PrepareForCopy()
    {
        for ( uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++ )
        {
            if ( Dirty[Stage] )
            {
                for ( uint32 Index = 0; Index < kDescriptorTableSize; Index++ )
                {
                    ViewType* View = ResourceViews[Stage][Index];
                    Assert( View != nullptr );

                    CopyDescriptors[Stage][Index] = View->GetOfflineHandle();
                }
            }
        }
    }

    void SetAllocatedDescriptorHandles( D3D12_CPU_DESCRIPTOR_HANDLE HostStartHandle, D3D12_GPU_DESCRIPTOR_HANDLE DeviceStartHandle, uint64 IncreamentDescriptorSize )
    {
        for ( uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++ )
        {
            if ( Dirty[Stage] )
            {
                // We keep the host descriptors for when the descriptors are copied to the device
                HostDescriptors[Stage] = HostStartHandle;
                HostStartHandle.ptr += (uint64)(kDescriptorTableSize * IncreamentDescriptorSize);

                DeviceDescriptors[Stage] = DeviceStartHandle;
                DeviceStartHandle.ptr += (uint64)(kDescriptorTableSize * IncreamentDescriptorSize);
            }
        }
    }

    FORCEINLINE void InvalidateAll()
    {
        /* Invalidate all stage's descriptor table */
        for ( uint32 Index = 0; Index < D3D12_CACHED_DESCRIPTORS_NUM_STAGES; Index++ )
        {
            Dirty[Index] = true;
        }
    }

    ViewType* ResourceViews[D3D12_CACHED_DESCRIPTORS_NUM_STAGES][kDescriptorTableSize];

    /* Offline handles to the currently bound ResourceViews */
    D3D12_CPU_DESCRIPTOR_HANDLE CopyDescriptors[D3D12_CACHED_DESCRIPTORS_NUM_STAGES][kDescriptorTableSize];

    /* The beginning of each stage's descriptor table */
    D3D12_CPU_DESCRIPTOR_HANDLE HostDescriptors[D3D12_CACHED_DESCRIPTORS_NUM_STAGES];
    D3D12_GPU_DESCRIPTOR_HANDLE DeviceDescriptors[D3D12_CACHED_DESCRIPTORS_NUM_STAGES];

    /* Each bool informs the state of a stage's descriptor table */
    bool Dirty[D3D12_CACHED_DESCRIPTORS_NUM_STAGES];
};

using CD3D12ConstantBufferViewCache  = TD3D12ViewCache<CD3D12ConstantBufferView, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DEFAULT_CONSTANT_BUFFER_COUNT>;
using CD3D12ShaderResourceViewCache  = TD3D12ViewCache<CD3D12ShaderResourceView, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT>;
using CD3D12UnorderedAccessViewCache = TD3D12ViewCache<CD3D12UnorderedAccessView, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT>;
using CD3D12SamplerStateCache        = TD3D12ViewCache<CD3D12RHISamplerState, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DEFAULT_SAMPLER_STATE_COUNT>;

class CD3D12VertexBufferCache
{
public:
    CD3D12VertexBufferCache()
        : VertexBuffers()
        , VertexBufferViews()
        , NumVertexBuffers( 0 )
        , VertexBuffersDirty( false )
        , IndexBuffer( nullptr )
        , IndexBufferView()
        , IndexBufferDirty( false )
    {
        Reset();
    }

    FORCEINLINE void SetVertexBuffer( CD3D12RHIVertexBuffer* VertexBuffer, uint32 Slot )
    {
        D3D12_ERROR( Slot <= D3D12_MAX_VERTEX_BUFFER_SLOTS, "[D3D12]: Trying to bind a VertexBuffer to a slot (Slot=" + ToString( Slot ) + ") higher than the maximum (MaxVertexBufferCount=" + ToString( D3D12_MAX_VERTEX_BUFFER_SLOTS ) + ") " );

        if ( VertexBuffers[Slot] != VertexBuffer )
        {
            VertexBuffers[Slot] = VertexBuffer;
            NumVertexBuffers = NMath::Max( NumVertexBuffers, Slot + 1 );

            VertexBuffersDirty = true;
        }
    }

    FORCEINLINE void SetIndexBuffer( CD3D12RHIIndexBuffer* InIndexBuffer )
    {
        if ( IndexBuffer != InIndexBuffer )
        {
            IndexBuffer = InIndexBuffer;
            IndexBufferDirty = true;
        }
    }

    void CommitState( CD3D12CommandList& CmdList )
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
        if ( VertexBuffersDirty )
        {
            for ( uint32 i = 0; i < NumVertexBuffers; i++ )
            {
                CD3D12RHIVertexBuffer* VertexBuffer = VertexBuffers[i];
                if ( !VertexBuffer )
                {
                    VertexBufferViews[i].BufferLocation = 0;
                    VertexBufferViews[i].SizeInBytes = 0;
                    VertexBufferViews[i].StrideInBytes = 0;
                }
                else
                {
                    // TODO: Maybe save a ref so that we can ensure that the buffer
                    //       does not get deleted until command batch is finished
                    VertexBufferViews[i] = VertexBuffer->GetView();
                }
            }

            DxCmdList->IASetVertexBuffers( 0, NumVertexBuffers, VertexBufferViews );
            VertexBuffersDirty = false;
        }

        if ( IndexBufferDirty )
        {
            if ( !IndexBuffer )
            {
                IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
                IndexBufferView.BufferLocation = 0;
                IndexBufferView.SizeInBytes = 0;
            }
            else
            {
                IndexBufferView = IndexBuffer->GetView();
            }

            DxCmdList->IASetIndexBuffer( &IndexBufferView );
            IndexBufferDirty = false;
        }
    }

    FORCEINLINE void Reset()
    {
        CMemory::Memzero( VertexBuffers, sizeof( VertexBuffers ) );
    
        NumVertexBuffers = 0;
        VertexBuffersDirty = true;

        IndexBuffer = nullptr;
        IndexBufferDirty = true;
    }

private:
    CD3D12RHIVertexBuffer* VertexBuffers[D3D12_MAX_VERTEX_BUFFER_SLOTS];
    D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[D3D12_MAX_VERTEX_BUFFER_SLOTS];
    uint32 NumVertexBuffers;
    bool   VertexBuffersDirty;

    CD3D12RHIIndexBuffer* IndexBuffer;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
    bool IndexBufferDirty;
};

class CD3D12RenderTargetState
{
public:
    CD3D12RenderTargetState()
        : RenderTargetViewHandles()
        , NumRenderTargets( 0 )
        , DepthStencilViewHandle( { 0 } )
        , Dirty( false )
    {
        Reset();
    }

    FORCEINLINE void SetRenderTargetView( CD3D12RenderTargetView* RenderTargetView, uint32 Slot )
    {
        D3D12_ERROR( Slot <= D3D12_MAX_RENDER_TARGET_COUNT, "[D3D12]: Trying to bind a RenderTarget to a slot (Slot=" + ToString( Slot ) + ") higher than the maximum (MaxRenderTargetCount=" + ToString( D3D12_MAX_RENDER_TARGET_COUNT ) + ") " );

        if ( RenderTargetView )
        {
            RenderTargetViewHandles[Slot] = RenderTargetView->GetOfflineHandle();
        }
        else
        {
            RenderTargetViewHandles[Slot] = { 0 };
        }

        NumRenderTargets = NMath::Max( NumRenderTargets, Slot + 1 );
        Dirty = true;
    }

    FORCEINLINE void SetDepthStencilView( CD3D12DepthStencilView* DepthStencilView )
    {
        if ( DepthStencilView )
        {
            DepthStencilViewHandle = DepthStencilView->GetOfflineHandle();
        }
        else
        {
            DepthStencilViewHandle = { 0 };
        }

        Dirty = true;
    }

    FORCEINLINE void Reset()
    {
        CMemory::Memzero( RenderTargetViewHandles, sizeof( RenderTargetViewHandles ) );
        DepthStencilViewHandle = { 0 };
        NumRenderTargets = 0;
    }

    FORCEINLINE void CommitState( CD3D12CommandList& CmdList )
    {
        if ( Dirty )
        {
            ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

            D3D12_CPU_DESCRIPTOR_HANDLE* DepthStencil = nullptr;
            if ( DepthStencilViewHandle.ptr )
            {
                DepthStencil = &DepthStencilViewHandle;
            }

            DxCmdList->OMSetRenderTargets( NumRenderTargets, RenderTargetViewHandles, false, DepthStencil );
            Dirty = false;
        }
    }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetViewHandles[D3D12_MAX_RENDER_TARGET_COUNT];
    uint32 NumRenderTargets;

    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilViewHandle;

    bool Dirty;
};

class CD3D12DescriptorCache : public CD3D12DeviceChild
{
public:
    CD3D12DescriptorCache( CD3D12Device* Device );
    ~CD3D12DescriptorCache();

    bool Init();

    /* Binds all descriptors that is necessary for the graphics pipeline */
    void CommitGraphicsDescriptors( CD3D12CommandList& CmdList, class CD3D12CommandBatch* CmdBatch, CD3D12RootSignature* RootSignature );
    
    /* Binds all descriptors that is necessary for the compute pipeline */
    void CommitComputeDescriptors( CD3D12CommandList& CmdList, class CD3D12CommandBatch* CmdBatch, CD3D12RootSignature* RootSignature );

    /* Resets the descriptor-cache to use the null views and removes references to bound buffers and render-targets */
    void Reset();

    FORCEINLINE void SetVertexBuffer( CD3D12RHIVertexBuffer* VertexBuffer, uint32 Slot )
    {
        VertexBufferCache.SetVertexBuffer( VertexBuffer, Slot );
    }

    FORCEINLINE void SetIndexBuffer( CD3D12RHIIndexBuffer* IndexBuffer )
    {
        VertexBufferCache.SetIndexBuffer( IndexBuffer );
    }

    FORCEINLINE void SetRenderTargetView( CD3D12RenderTargetView* RenderTargetView, uint32 Slot )
    {
        RenderTargetCache.SetRenderTargetView( RenderTargetView, Slot );
    }

    FORCEINLINE void SetDepthStencilView( CD3D12DepthStencilView* DepthStencilView )
    {
        RenderTargetCache.SetDepthStencilView( DepthStencilView );
    }

    FORCEINLINE void SetConstantBufferView( CD3D12ConstantBufferView* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister )
    {
        if ( !Descriptor )
        {
            Descriptor = NullCBV;
        }

        ConstantBufferViewCache.SetView( Descriptor, Visibility, ShaderRegister );
    }

    FORCEINLINE void SetShaderResourceView( CD3D12ShaderResourceView* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister )
    {
        if ( !Descriptor )
        {
            Descriptor = NullSRV;
        }

        ShaderResourceViewCache.SetView( Descriptor, Visibility, ShaderRegister );
    }

    FORCEINLINE void SetUnorderedAccessView( CD3D12UnorderedAccessView* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister )
    {
        if ( !Descriptor )
        {
            Descriptor = NullUAV;
        }

        UnorderedAccessViewCache.SetView( Descriptor, Visibility, ShaderRegister );
    }

    FORCEINLINE void SetSamplerState( CD3D12RHISamplerState* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister )
    {
        if ( !Descriptor )
        {
            Descriptor = NullSampler;
        }

        SamplerStateCache.SetView( Descriptor, Visibility, ShaderRegister );
    }

private:

    /* Allocates necessary descriptors, binds them to the caches, and then binds the current descriptor heaps*/
    void AllocateDescriptorsAndSetHeaps( ID3D12GraphicsCommandList* CmdList, CD3D12OnlineDescriptorHeap* ResourceHeap, CD3D12OnlineDescriptorHeap* SamplerHeap );

    template<typename TResourveViewCache>
    void CopyAndBindComputeDescriptors( ID3D12Device* DxDevice, ID3D12GraphicsCommandList* DxCmdList, TResourveViewCache& ResourceViewCache, int32 ParameterIndex )
    {
        const EShaderVisibility ShaderVisibility = ShaderVisibility_All; 
        if ( ParameterIndex >= 0 && ResourceViewCache.Dirty[ShaderVisibility] )
        {
            const UINT DestRangeSize = TResourveViewCache::GetDescriptorTableSize();
            const UINT NumSrcRanges  = TResourveViewCache::GetDescriptorTableSize();

            const D3D12_CPU_DESCRIPTOR_HANDLE* SrcHostStarts = ResourceViewCache.CopyDescriptors[ShaderVisibility];

            D3D12_CPU_DESCRIPTOR_HANDLE HostHandle = ResourceViewCache.HostDescriptors[ShaderVisibility];
            DxDevice->CopyDescriptors( 1, &HostHandle, &DestRangeSize, NumSrcRanges, SrcHostStarts, RangeSizes, TResourveViewCache::GetDescriptorHeapType());

            D3D12_GPU_DESCRIPTOR_HANDLE DeviceHandle = ResourceViewCache.DeviceDescriptors[ShaderVisibility];
            DxCmdList->SetComputeRootDescriptorTable( ParameterIndex, DeviceHandle );

            // When the descriptors are copied and bound, then the descriptors are not dirty anymore
            ResourceViewCache.Dirty[ShaderVisibility] = false;
        }
    }

    template<typename TResourveViewCache>
    void CopyAndBindGraphicsDescriptors( ID3D12Device* DxDevice, ID3D12GraphicsCommandList* DxCmdList, TResourveViewCache& ResourceViewCache, int32 ParameterIndex, EShaderVisibility ShaderVisibility )
    {
        if ( ParameterIndex >= 0 && ResourceViewCache.Dirty[ShaderVisibility] )
        {
            const UINT DestRangeSize = TResourveViewCache::GetDescriptorTableSize();
            const UINT NumSrcRanges  = TResourveViewCache::GetDescriptorTableSize();

            const D3D12_CPU_DESCRIPTOR_HANDLE* SrcHostStarts = ResourceViewCache.CopyDescriptors[ShaderVisibility];

            D3D12_CPU_DESCRIPTOR_HANDLE HostHandle = ResourceViewCache.HostDescriptors[ShaderVisibility];
            DxDevice->CopyDescriptors( 1, &HostHandle, &DestRangeSize, NumSrcRanges, SrcHostStarts, RangeSizes, TResourveViewCache::GetDescriptorHeapType() );

            D3D12_GPU_DESCRIPTOR_HANDLE DeviceHandle = ResourceViewCache.DeviceDescriptors[ShaderVisibility];
            DxCmdList->SetGraphicsRootDescriptorTable( ParameterIndex, DeviceHandle );

            // When the descriptors are copied and bound, then the descriptors are not dirty anymore
            ResourceViewCache.Dirty[ShaderVisibility] = false;
        }
    }

    CD3D12ConstantBufferView*  NullCBV     = nullptr;
    CD3D12ShaderResourceView*  NullSRV     = nullptr;
    CD3D12UnorderedAccessView* NullUAV     = nullptr;
    CD3D12RHISamplerState*        NullSampler = nullptr;

    CD3D12VertexBufferCache        VertexBufferCache;
    CD3D12RenderTargetState        RenderTargetCache;
    CD3D12ShaderResourceViewCache  ShaderResourceViewCache;
    CD3D12UnorderedAccessViewCache UnorderedAccessViewCache;
    CD3D12ConstantBufferViewCache  ConstantBufferViewCache;
    CD3D12SamplerStateCache        SamplerStateCache;

    ID3D12DescriptorHeap* PreviousDescriptorHeaps[2] = { nullptr, nullptr };

    UINT RangeSizes[D3D12_CACHED_DESCRIPTORS_COUNT];
};

class CD3D12ShaderConstantsCache
{
public:

    CD3D12ShaderConstantsCache()
        : Constants()
        , NumConstants()
    {
        Reset();
    }

    FORCEINLINE void Set32BitShaderConstants( uint32* InConstants, uint32 InNumConstants )
    {
        D3D12_ERROR( InNumConstants <= D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT, "[D3D12]: Trying to set a number of shader-constants (NumConstants=" + ToString( InNumConstants ) + ") higher than the maximum (MaxShaderConstants=" + ToString( D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT ) + ") " );

        CMemory::Memcpy( Constants, InConstants, sizeof( uint32 ) * InNumConstants );
        NumConstants = InNumConstants;
    }

    FORCEINLINE void CommitGraphics( CD3D12CommandList& CmdList, CD3D12RootSignature* RootSignature )
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

        int32 RootIndex = RootSignature->Get32BitConstantsIndex();
        if ( RootIndex >= 0 )
        {
            DxCmdList->SetGraphicsRoot32BitConstants( RootIndex, NumConstants, Constants, 0 );
        }
    }

    FORCEINLINE void CommitCompute( CD3D12CommandList& CmdList, CD3D12RootSignature* RootSignature )
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
        
        int32 RootIndex = RootSignature->Get32BitConstantsIndex();
        if ( RootIndex >= 0 )
        {
            DxCmdList->SetComputeRoot32BitConstants( RootIndex, NumConstants, Constants, 0 );
        }
    }

    FORCEINLINE void Reset()
    {
        CMemory::Memzero( Constants, sizeof( Constants ) );
        NumConstants = 0;
    }

private:
    uint32 Constants[D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT];
    uint32 NumConstants;
};