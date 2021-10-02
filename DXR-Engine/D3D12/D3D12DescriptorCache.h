#pragma once
#include "D3D12RootSignature.h"
#include "D3D12Views.h"
#include "D3D12Buffer.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12SamplerState.h"
#include "D3D12CommandList.h"

template <typename ViewType>
struct TD3D12ViewCache
{
    TD3D12ViewCache()
        : DescriptorViews()
        , Descriptors()
        , CopyDescriptors()
        , Dirty()
        , DescriptorRangeLengths()
        , TotalNumDescriptors( 0 )
    {
        Reset();
    }

    void Set( ViewType* DescriptorView, EShaderVisibility Visibility, uint32 ShaderRegister )
    {
        Assert( DescriptorView != nullptr );

        ViewType* CurrentDescriptorView = DescriptorViews[Visibility][ShaderRegister];
        if ( DescriptorView != CurrentDescriptorView )
        {
            DescriptorViews[Visibility][ShaderRegister] = DescriptorView;
            Dirty[Visibility] = true;

            uint32& RangeLength = DescriptorRangeLengths[Visibility];
            RangeLength = NMath::Max<uint32>( RangeLength, ShaderRegister + 1 );
        }
    }

    void Reset()
    {
        CMemory::Memzero( DescriptorViews, sizeof( DescriptorViews ) );
        CMemory::Memzero( Descriptors, sizeof( Descriptors ) );
        CMemory::Memzero( CopyDescriptors, sizeof( CopyDescriptors ) );
        CMemory::Memzero( DescriptorRangeLengths, sizeof( DescriptorRangeLengths ) );

        for ( uint32 i = 0; i < ShaderVisibility_Count; i++ )
        {
            Dirty[i] = true;
        }
    }

    uint32 CountNeededDescriptors() const
    {
        uint32 NumDescriptors = 0;
        for ( uint32 i = 0; i < ShaderVisibility_Count; i++ )
        {
            if ( Dirty[i] )
            {
                NumDescriptors += DescriptorRangeLengths[i];
            }
        }

        return NumDescriptors;
    }

    void PrepareForCopy( ViewType* DefaultView )
    {
        TotalNumDescriptors = 0;
        for ( uint32 i = 0; i < ShaderVisibility_Count; i++ )
        {
            if ( Dirty[i] )
            {
                uint32 NumDescriptors = DescriptorRangeLengths[i];
                uint32 Offset = TotalNumDescriptors;

                TotalNumDescriptors += NumDescriptors;
                Assert( TotalNumDescriptors <= D3D12_CACHED_DESCRIPTORS_COUNT );

                for ( uint32 d = 0; d < NumDescriptors; d++ )
                {
                    ViewType* View = DescriptorViews[i][d];
                    if ( !View )
                    {
                        DescriptorViews[i][d] = View = DefaultView;
                    }

                    CopyDescriptors[Offset + d] = View->GetOfflineHandle();
                }
            }
        }
    }

    void SetGPUHandles( D3D12_GPU_DESCRIPTOR_HANDLE StartHandle, uint64 DescriptorSize )
    {
        for ( uint32 i = 0; i < ShaderVisibility_Count; i++ )
        {
            if ( Dirty[i] )
            {
                Descriptors[i] = StartHandle;
                StartHandle.ptr += (uint64)DescriptorRangeLengths[i] * DescriptorSize;

                Dirty[i] = false;
            }
        }
    }

    FORCEINLINE void InvalidateAll()
    {
        for ( uint32 i = 0; i < D3D12_CACHED_DESCRIPTORS_NUM_STAGES; i++ )
        {
            Dirty[i] = true;
        }
    }

    ViewType* DescriptorViews[D3D12_CACHED_DESCRIPTORS_NUM_STAGES][D3D12_CACHED_DESCRIPTORS_COUNT];

    D3D12_GPU_DESCRIPTOR_HANDLE Descriptors[D3D12_CACHED_DESCRIPTORS_NUM_STAGES];
    D3D12_CPU_DESCRIPTOR_HANDLE CopyDescriptors[D3D12_CACHED_DESCRIPTORS_COUNT];

    bool Dirty[D3D12_CACHED_DESCRIPTORS_NUM_STAGES];

    uint32 DescriptorRangeLengths[D3D12_CACHED_DESCRIPTORS_NUM_STAGES];
    uint32 TotalNumDescriptors;
};

using CD3D12ConstantBufferViewCache = TD3D12ViewCache<CD3D12ConstantBufferView>;
using CD3D12ShaderResourceViewCache = TD3D12ViewCache<CD3D12ShaderResourceView>;
using CD3D12UnorderedAccessViewCache = TD3D12ViewCache<CD3D12UnorderedAccessView>;
using CD3D12SamplerStateCache = TD3D12ViewCache<CD3D12SamplerState>;

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

    FORCEINLINE void SetVertexBuffer( CD3D12VertexBuffer* VertexBuffer, uint32 Slot )
    {
        Assert( Slot < D3D12_MAX_VERTEX_BUFFER_SLOTS );

        if ( VertexBuffers[Slot] != VertexBuffer )
        {
            VertexBuffers[Slot] = VertexBuffer;
            NumVertexBuffers = NMath::Max( NumVertexBuffers, Slot + 1 );

            VertexBuffersDirty = true;
        }
    }

    FORCEINLINE void SetIndexBuffer( CD3D12IndexBuffer* InIndexBuffer )
    {
        if ( IndexBuffer != InIndexBuffer )
        {
            IndexBuffer = InIndexBuffer;
            IndexBufferDirty = true;
        }
    }

    FORCEINLINE void CommitState( CD3D12CommandList& CmdList )
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
        if ( VertexBuffersDirty )
        {
            for ( uint32 i = 0; i < NumVertexBuffers; i++ )
            {
                CD3D12VertexBuffer* VertexBuffer = VertexBuffers[i];
                if ( !VertexBuffer )
                {
                    VertexBufferViews[i].BufferLocation = 0;
                    VertexBufferViews[i].SizeInBytes = 0;
                    VertexBufferViews[i].StrideInBytes = 0;
                }
                else
                {
                    // TODO: Maybe save a ref so that we can ensure that the buffer
                    //       does not get deleted until commandbatch is finished
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
    CD3D12VertexBuffer* VertexBuffers[D3D12_MAX_VERTEX_BUFFER_SLOTS];
    D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[D3D12_MAX_VERTEX_BUFFER_SLOTS];
    uint32 NumVertexBuffers;
    bool   VertexBuffersDirty;

    CD3D12IndexBuffer* IndexBuffer;
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
        , DSVPtr( nullptr )
        , Dirty( false )
    {
        Reset();
    }

    FORCEINLINE void SetRenderTargetView( CD3D12RenderTargetView* RenderTargetView, uint32 Slot )
    {
        Assert( Slot < D3D12_MAX_RENDER_TARGET_COUNT );

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
            DSVPtr = &DepthStencilViewHandle;
        }
        else
        {
            DepthStencilViewHandle = { 0 };
            DSVPtr = nullptr;
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
            DxCmdList->OMSetRenderTargets( NumRenderTargets, RenderTargetViewHandles, FALSE, DSVPtr );
            Dirty = false;
        }
    }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetViewHandles[D3D12_MAX_RENDER_TARGET_COUNT];
    uint32 NumRenderTargets;

    D3D12_CPU_DESCRIPTOR_HANDLE  DepthStencilViewHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE* DSVPtr;

    bool Dirty;
};

class CD3D12DescriptorCache : public CD3D12DeviceChild
{
public:
    CD3D12DescriptorCache( CD3D12Device* Device );
    ~CD3D12DescriptorCache();

    bool Init();

    void CommitGraphicsDescriptors( CD3D12CommandList& CmdList, class CD3D12CommandBatch* CmdBatch, CD3D12RootSignature* RootSignature );
    void CommitComputeDescriptors( CD3D12CommandList& CmdList, class CD3D12CommandBatch* CmdBatch, CD3D12RootSignature* RootSignature );

    void Reset();

    FORCEINLINE void SetVertexBuffer( CD3D12VertexBuffer* VertexBuffer, uint32 Slot )
    {
        VertexBufferCache.SetVertexBuffer( VertexBuffer, Slot );
    }

    FORCEINLINE void SetIndexBuffer( CD3D12IndexBuffer* IndexBuffer )
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

        ConstantBufferViewCache.Set( Descriptor, Visibility, ShaderRegister );
    }

    FORCEINLINE void SetShaderResourceView( CD3D12ShaderResourceView* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister )
    {
        if ( !Descriptor )
        {
            Descriptor = NullSRV;
        }

        ShaderResourceViewCache.Set( Descriptor, Visibility, ShaderRegister );
    }

    FORCEINLINE void SetUnorderedAccessView( CD3D12UnorderedAccessView* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister )
    {
        if ( !Descriptor )
        {
            Descriptor = NullUAV;
        }

        UnorderedAccessViewCache.Set( Descriptor, Visibility, ShaderRegister );
    }

    FORCEINLINE void SetSamplerState( CD3D12SamplerState* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister )
    {
        if ( !Descriptor )
        {
            Descriptor = NullSampler;
        }

        SamplerStateCache.Set( Descriptor, Visibility, ShaderRegister );
    }

private:
    void CopyDescriptorsAndSetHeaps( ID3D12GraphicsCommandList* CmdList, CD3D12OnlineDescriptorHeap* ResourceHeap, CD3D12OnlineDescriptorHeap* SamplerHeap );

    CD3D12ConstantBufferView* NullCBV = nullptr;
    CD3D12ShaderResourceView* NullSRV = nullptr;
    CD3D12UnorderedAccessView* NullUAV = nullptr;
    CD3D12SamplerState* NullSampler = nullptr;

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
    {
        Reset();
    }

    void Set32BitShaderConstants( uint32* InConstants, uint32 InNumConstants )
    {
        Assert( InNumConstants <= D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT );

        CMemory::Memcpy( Constants, InConstants, sizeof( uint32 ) * InNumConstants );
        NumConstants = InNumConstants;
    }

    void CommitGraphics( CD3D12CommandList& CmdList, CD3D12RootSignature* RootSignature )
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
        int32 RootIndex = RootSignature->Get32BitConstantsIndex();
        if ( RootIndex >= 0 )
        {
            DxCmdList->SetGraphicsRoot32BitConstants( RootIndex, NumConstants, Constants, 0 );
        }
    }

    void CommitCompute( CD3D12CommandList& CmdList, CD3D12RootSignature* RootSignature )
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
        int32 RootIndex = RootSignature->Get32BitConstantsIndex();
        if ( RootIndex >= 0 )
        {
            DxCmdList->SetComputeRoot32BitConstants( RootIndex, NumConstants, Constants, 0 );
        }
    }

    void Reset()
    {
        CMemory::Memzero( Constants, sizeof( Constants ) );
        NumConstants = 0;
    }

private:
    uint32 Constants[D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT];
    uint32 NumConstants;
};