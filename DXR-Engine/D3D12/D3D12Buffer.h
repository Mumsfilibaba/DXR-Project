#pragma once
#include "CoreRHI/RHIResources.h"

#include "D3D12Resource.h"
#include "D3D12Views.h"

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

class CD3D12BaseVertexBuffer : public CRHIVertexBuffer, public CD3D12BaseBuffer
{
public:
    CD3D12BaseVertexBuffer( CD3D12Device* InDevice, uint32 InNumVertices, uint32 InStride, uint32 InFlags )
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

class CD3D12BaseIndexBuffer : public CRHIIndexBuffer, public CD3D12BaseBuffer
{
public:
    CD3D12BaseIndexBuffer( CD3D12Device* InDevice, EIndexFormat InIndexFormat, uint32 InNumIndices, uint32 InFlags )
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

class CD3D12BaseConstantBuffer : public CRHIConstantBuffer, public CD3D12BaseBuffer
{
public:
    CD3D12BaseConstantBuffer( CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap, uint32 InSizeInBytes, uint32 InFlags )
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

    FORCEINLINE CD3D12ConstantBufferView& GetView()
    {
        return View;
    }

    FORCEINLINE const CD3D12ConstantBufferView& GetView() const
    {
        return View;
    }

private:
    CD3D12ConstantBufferView View;
};

class CD3D12BaseStructuredBuffer : public CRHIStructuredBuffer, public CD3D12BaseBuffer
{
public:
    CD3D12BaseStructuredBuffer( CD3D12Device* InDevice, uint32 InSizeInBytes, uint32 InStride, uint32 InFlags )
        : CRHIStructuredBuffer( InSizeInBytes, InStride, InFlags )
        , CD3D12BaseBuffer( InDevice )
    {
    }
};

template<typename TBaseBuffer>
class TD3D12BaseBuffer : public TBaseBuffer
{
public:
    template<typename... TBufferArgs>
    TD3D12BaseBuffer( CD3D12Device* InDevice, TBufferArgs&&... BufferArgs )
        : TBaseBuffer( InDevice, ::Forward<TBufferArgs>( BufferArgs )... )
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

class CD3D12VertexBuffer : public TD3D12BaseBuffer<CD3D12BaseVertexBuffer>
{
public:
    CD3D12VertexBuffer( CD3D12Device* InDevice, uint32 InNumVertices, uint32 InStride, uint32 InFlags )
        : TD3D12BaseBuffer<CD3D12BaseVertexBuffer>( InDevice, InNumVertices, InStride, InFlags )
    {
    }
};

class CD3D12IndexBuffer : public TD3D12BaseBuffer<CD3D12BaseIndexBuffer>
{
public:
    CD3D12IndexBuffer( CD3D12Device* InDevice, EIndexFormat InIndexFormat, uint32 InNumIndices, uint32 InFlags )
        : TD3D12BaseBuffer<CD3D12BaseIndexBuffer>( InDevice, InIndexFormat, InNumIndices, InFlags )
    {
    }
};

class CD3D12ConstantBuffer : public TD3D12BaseBuffer<CD3D12BaseConstantBuffer>
{
public:
    CD3D12ConstantBuffer( CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap, uint32 InSizeInBytes, uint32 InFlags )
        : TD3D12BaseBuffer<CD3D12BaseConstantBuffer>( InDevice, InHeap, InSizeInBytes, InFlags )
    {
    }
};

class CD3D12StructuredBuffer : public TD3D12BaseBuffer<CD3D12BaseStructuredBuffer>
{
public:
    CD3D12StructuredBuffer( CD3D12Device* InDevice, uint32 InNumElements, uint32 InStride, uint32 InFlags )
        : TD3D12BaseBuffer<CD3D12BaseStructuredBuffer>( InDevice, InNumElements, InStride, InFlags )
    {
    }
};

inline CD3D12BaseBuffer* D3D12BufferCast( CRHIBuffer* Buffer )
{
    if ( Buffer->AsVertexBuffer() )
    {
        return static_cast<CD3D12VertexBuffer*>(Buffer);
    }
    else if ( Buffer->AsIndexBuffer() )
    {
        return static_cast<CD3D12IndexBuffer*>(Buffer);
    }
    else if ( Buffer->AsConstantBuffer() )
    {
        return static_cast<CD3D12ConstantBuffer*>(Buffer);
    }
    else if ( Buffer->AsStructuredBuffer() )
    {
        return static_cast<CD3D12StructuredBuffer*>(Buffer);
    }
    else
    {
        return nullptr;
    }
}