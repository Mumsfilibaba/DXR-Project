#pragma once
#include "D3D12Resource.h"
#include "D3D12RHIViews.h"

#include "RHI/RHIResources.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12BaseBuffer : public CD3D12DeviceChild
{
public:
    CD3D12BaseBuffer( CD3D12Device* InDevice )
        : CD3D12DeviceChild( InDevice )
        , Resource( nullptr )
    {
    }

    virtual void SetResource( CD3D12Resource* InResource )
    {
        Resource = InResource;
    }

    FORCEINLINE uint64 GetSizeInBytes() const
    {
        return static_cast<uint64>(Resource->GetDesc().Width);
    }

    FORCEINLINE CD3D12Resource* GetResource()
    {
        return Resource.Get();
    }

protected:
    TSharedRef<CD3D12Resource> Resource;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHIBaseVertexBuffer : public CRHIVertexBuffer, public CD3D12BaseBuffer
{
public:
    CD3D12RHIBaseVertexBuffer( CD3D12Device* InDevice, uint32 InNumVertices, uint32 InStride, uint32 InFlags )
        : CRHIVertexBuffer( InNumVertices, InStride, InFlags )
        , CD3D12BaseBuffer( InDevice )
        , View()
    {
    }

    virtual void SetResource( CD3D12Resource* InResource ) override
    {
        CD3D12BaseBuffer::SetResource( InResource );

        CMemory::Memzero( &View );
        View.StrideInBytes = GetStride();
        View.SizeInBytes = GetNumVertices() * View.StrideInBytes;
        View.BufferLocation = CD3D12BaseBuffer::Resource->GetGPUVirtualAddress();
    }

    FORCEINLINE const D3D12_VERTEX_BUFFER_VIEW& GetView() const
    {
        return View;
    }

private:
    D3D12_VERTEX_BUFFER_VIEW View;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHIBaseIndexBuffer : public CRHIIndexBuffer, public CD3D12BaseBuffer
{
public:
    CD3D12RHIBaseIndexBuffer( CD3D12Device* InDevice, EIndexFormat InIndexFormat, uint32 InNumIndices, uint32 InFlags )
        : CRHIIndexBuffer( InIndexFormat, InNumIndices, InFlags )
        , CD3D12BaseBuffer( InDevice )
        , View()
    {
    }

    virtual void SetResource( CD3D12Resource* InResource ) override
    {
        CD3D12BaseBuffer::SetResource( InResource );

        CMemory::Memzero( &View );

        EIndexFormat IndexFormat = GetFormat();
        if ( IndexFormat != EIndexFormat::Unknown )
        {
            View.Format = IndexFormat == EIndexFormat::uint16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            View.SizeInBytes = GetNumIndicies() * GetStrideFromIndexFormat( IndexFormat );
            View.BufferLocation = CD3D12BaseBuffer::Resource->GetGPUVirtualAddress();
        }
    }

    FORCEINLINE const D3D12_INDEX_BUFFER_VIEW& GetView() const
    {
        return View;
    }

private:
    D3D12_INDEX_BUFFER_VIEW View;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHIBaseConstantBuffer : public CRHIConstantBuffer, public CD3D12BaseBuffer
{
public:
    CD3D12RHIBaseConstantBuffer( CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap, uint32 InSizeInBytes, uint32 InFlags )
        : CRHIConstantBuffer( InSizeInBytes, InFlags )
        , CD3D12BaseBuffer( InDevice )
        , View( InDevice, InHeap )
    {
    }

    virtual void SetResource( CD3D12Resource* InResource ) override
    {
        CD3D12BaseBuffer::SetResource( InResource );

        D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
        CMemory::Memzero( &ViewDesc );

        ViewDesc.BufferLocation = CD3D12BaseBuffer::Resource->GetGPUVirtualAddress();
        ViewDesc.SizeInBytes = (uint32)CD3D12BaseBuffer::GetSizeInBytes();

        if ( View.GetOfflineHandle() == 0 )
        {
            if ( !View.Init() )
            {
                return;
            }
        }

        View.CreateView( CD3D12BaseBuffer::Resource.Get(), ViewDesc );
    }

    FORCEINLINE CD3D12RHIConstantBufferView& GetView()
    {
        return View;
    }

    FORCEINLINE const CD3D12RHIConstantBufferView& GetView() const
    {
        return View;
    }

private:
    CD3D12RHIConstantBufferView View;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHIBaseStructuredBuffer : public CRHIStructuredBuffer, public CD3D12BaseBuffer
{
public:
    CD3D12RHIBaseStructuredBuffer( CD3D12Device* InDevice, uint32 InSizeInBytes, uint32 InStride, uint32 InFlags )
        : CRHIStructuredBuffer( InSizeInBytes, InStride, InFlags )
        , CD3D12BaseBuffer( InDevice )
    {
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////

template<typename BaseBufferType>
class TD3D12RHIBaseBuffer : public BaseBufferType
{
public:
    template<typename... TBufferArgs>
    TD3D12RHIBaseBuffer( CD3D12Device* InDevice, TBufferArgs&&... BufferArgs )
        : BaseBufferType( InDevice, ::Forward<TBufferArgs>( BufferArgs )... )
    {
    }

    virtual void* Map( uint32 Offset, uint32 InSize ) override
    {
        if ( Offset != 0 || InSize != 0 )
        {
            D3D12_RANGE MapRange = { Offset, InSize };
            return CD3D12BaseBuffer::Resource->Map( 0, &MapRange );
        }
        else
        {
            return CD3D12BaseBuffer::Resource->Map( 0, nullptr );
        }
    }

    virtual void Unmap( uint32 Offset, uint32 InSize ) override
    {
        if ( Offset != 0 || InSize != 0 )
        {
            D3D12_RANGE MapRange = { Offset, InSize };
            CD3D12BaseBuffer::Resource->Unmap( 0, &MapRange );
        }
        else
        {
            CD3D12BaseBuffer::Resource->Unmap( 0, nullptr );
        }
    }

    virtual void SetName( const CString& InName ) override final
    {
        CRHIResource::SetName( InName );

        CD3D12BaseBuffer::Resource->SetName( InName );
    }

    virtual void* GetNativeResource() const override final
    {
        return reinterpret_cast<void*>(CD3D12BaseBuffer::Resource->GetResource());
    }

    virtual bool IsValid() const override
    {
        return CD3D12BaseBuffer::Resource->GetResource() != nullptr;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHIVertexBuffer : public TD3D12RHIBaseBuffer<CD3D12RHIBaseVertexBuffer>
{
public:
    CD3D12RHIVertexBuffer( CD3D12Device* InDevice, uint32 InNumVertices, uint32 InStride, uint32 InFlags )
        : TD3D12RHIBaseBuffer<CD3D12RHIBaseVertexBuffer>( InDevice, InNumVertices, InStride, InFlags )
    {
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHIIndexBuffer : public TD3D12RHIBaseBuffer<CD3D12RHIBaseIndexBuffer>
{
public:
    CD3D12RHIIndexBuffer( CD3D12Device* InDevice, EIndexFormat InIndexFormat, uint32 InNumIndices, uint32 InFlags )
        : TD3D12RHIBaseBuffer<CD3D12RHIBaseIndexBuffer>( InDevice, InIndexFormat, InNumIndices, InFlags )
    {
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHIConstantBuffer : public TD3D12RHIBaseBuffer<CD3D12RHIBaseConstantBuffer>
{
public:
    CD3D12RHIConstantBuffer( CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap, uint32 InSizeInBytes, uint32 InFlags )
        : TD3D12RHIBaseBuffer<CD3D12RHIBaseConstantBuffer>( InDevice, InHeap, InSizeInBytes, InFlags )
    {
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHIStructuredBuffer : public TD3D12RHIBaseBuffer<CD3D12RHIBaseStructuredBuffer>
{
public:
    CD3D12RHIStructuredBuffer( CD3D12Device* InDevice, uint32 InNumElements, uint32 InStride, uint32 InFlags )
        : TD3D12RHIBaseBuffer<CD3D12RHIBaseStructuredBuffer>( InDevice, InNumElements, InStride, InFlags )
    {
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////

inline CD3D12BaseBuffer* D3D12BufferCast( CRHIBuffer* Buffer )
{
    if ( Buffer )
    {
        if ( Buffer->AsVertexBuffer() )
        {
            return static_cast<CD3D12RHIVertexBuffer*>(Buffer);
        }
        else if ( Buffer->AsIndexBuffer() )
        {
            return static_cast<CD3D12RHIIndexBuffer*>(Buffer);
        }
        else if ( Buffer->AsConstantBuffer() )
        {
            return static_cast<CD3D12RHIConstantBuffer*>(Buffer);
        }
        else if ( Buffer->AsStructuredBuffer() )
        {
            return static_cast<CD3D12RHIStructuredBuffer*>(Buffer);
        }
    }
    
    return nullptr;
}