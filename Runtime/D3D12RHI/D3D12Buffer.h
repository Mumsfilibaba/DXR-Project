#pragma once
#include "D3D12Resource.h"
#include "D3D12Views.h"

#include "RHI/RHIResources.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<class CD3D12Buffer> CD3D12BufferRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Buffer

class CD3D12Buffer : public CRHIBuffer, public CD3D12DeviceObject
{
public:
    CD3D12Buffer(CD3D12Device* InDevice, const CRHIBufferDesc& InBufferDesc);
    ~CD3D12Buffer() = default;

    bool Initialize(class CD3D12CommandContext* CommandContext, ERHIResourceState InitalState, const SRHIResourceData* InitialData);

    virtual void SetName(const String& InName) override final;

    virtual void* GetNativeResource() const override final;

    virtual bool IsValid() const override final;

    void SetResource(const CD3D12ResourceRef& InResource);

    FORCEINLINE uint64 GetSizeInBytes() const
    {
        return static_cast<uint64>(Resource->GetDesc().Width);
    }

    FORCEINLINE CD3D12Resource* GetResource()
    {
        return Resource.Get();
    }

protected:
    CD3D12ResourceRef Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseVertexBuffer

class CD3D12BaseVertexBuffer : public CRHIBuffer, public CD3D12BaseBuffer
{
public:
    CD3D12BaseVertexBuffer(CD3D12Device* InDevice, uint32 InNumVertices, uint32 InStride, uint32 InFlags)
        : CRHIBuffer(InNumVertices, InStride, InFlags)
        , CD3D12BaseBuffer(InDevice)
        , View()
    {
    }

    // Set the resource and construct the view
    virtual void SetResource(CD3D12Resource* InResource) override final
    {
        CD3D12BaseBuffer::SetResource(InResource);

        CMemory::Memzero(&View);
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseIndexBuffer

class CD3D12BaseIndexBuffer : public CRHIBuffer, public CD3D12BaseBuffer
{
public:
    CD3D12BaseIndexBuffer(CD3D12Device* InDevice, ERHIIndexFormat InIndexFormat, uint32 InNumIndices, uint32 InFlags)
        : CRHIBuffer(InIndexFormat, InNumIndices, InFlags)
        , CD3D12BaseBuffer(InDevice)
        , View()
    {
    }

    // Set the resource and construct the view
    virtual void SetResource(CD3D12Resource* InResource) override final
    {
        CD3D12BaseBuffer::SetResource(InResource);

        CMemory::Memzero(&View);

        ERHIIndexFormat IndexFormat = GetFormat();
        if (IndexFormat != ERHIIndexFormat::Unknown)
        {
            View.Format = IndexFormat == ERHIIndexFormat::uint16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            View.SizeInBytes = GetNumIndicies() * GetStrideFromIndexFormat(IndexFormat);
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseConstantBuffer

class CD3D12BaseConstantBuffer : public CRHIBuffer, public CD3D12BaseBuffer
{
public:
    CD3D12BaseConstantBuffer(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap, uint32 InSizeInBytes, uint32 InFlags)
        : CRHIBuffer(InSizeInBytes, InFlags)
        , CD3D12BaseBuffer(InDevice)
        , View(InDevice, InHeap)
    {
    }

    virtual void SetResource(CD3D12Resource* InResource, ERHIResourceState InitalState, const SRHIResourceData* InitialData) override final
    {
        CD3D12BaseBuffer::SetResource(InResource);

        D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
        CMemory::Memzero(&ViewDesc);

        ViewDesc.BufferLocation = CD3D12BaseBuffer::Resource->GetGPUVirtualAddress();
        ViewDesc.SizeInBytes = (uint32)CD3D12BaseBuffer::GetSizeInBytes();

        if (View.GetOfflineHandle() == 0)
        {
            if (!View.AllocateHandle())
            {
                return;
            }
        }

        View.CreateView(CD3D12BaseBuffer::Resource.Get(), ViewDesc);
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseStructuredBuffer

class CD3D12BaseStructuredBuffer : public CRHIBuffer, public CD3D12BaseBuffer
{
public:
    CD3D12BaseStructuredBuffer(CD3D12Device* InDevice, uint32 InSizeInBytes, uint32 InStride, uint32 InFlags)
        : CRHIBuffer(InSizeInBytes, InStride, InFlags)
        , CD3D12BaseBuffer(InDevice)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TD3D12BaseBuffer

template<typename BaseBufferType>
class TD3D12BaseBuffer : public BaseBufferType
{
public:

    template<typename... TBufferArgs>
    TD3D12BaseBuffer(CD3D12Device* InDevice, TBufferArgs&&... BufferArgs)
        : BaseBufferType(InDevice, Forward<TBufferArgs>(BufferArgs)...)
    {
    }

    virtual void* Map(uint32 Offset, uint32 InSize) override
    {
        D3D12_ERROR(IsDynamic(), "Map is only supported on dynamic buffers");

        CD3D12Resource* DxResource = CD3D12BaseBuffer::Resource.Get();
        if (DxResource)
        {
            if (Offset != 0 || InSize != 0)
            {
                D3D12_RANGE MapRange = { Offset, InSize };
                return DxResource->Map(0, &MapRange);
            }
            else
            {
                return DxResource->Map(0, nullptr);
            }
        }

        return nullptr;
    }

    virtual void Unmap(uint32 Offset, uint32 InSize) override
    {
        D3D12_ERROR(IsDynamic(), "Unmap is only supported on dynamic buffers");

        CD3D12Resource* DxResource = CD3D12BaseBuffer::Resource.Get();
        if (DxResource)
        {
            if (Offset != 0 || InSize != 0)
            {
                D3D12_RANGE MapRange = { Offset, InSize };
                DxResource->Unmap(0, &MapRange);
            }
            else
            {
                DxResource->Unmap(0, nullptr);
            }
        }
    }


};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12VertexBuffer

class CD3D12VertexBuffer : public TD3D12BaseBuffer<CD3D12BaseVertexBuffer>
{
public:
    CD3D12VertexBuffer(CD3D12Device* InDevice, uint32 InNumVertices, uint32 InStride, uint32 InFlags)
        : TD3D12BaseBuffer<CD3D12BaseVertexBuffer>(InDevice, InNumVertices, InStride, InFlags)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12IndexBuffer

class CD3D12IndexBuffer : public TD3D12BaseBuffer<CD3D12BaseIndexBuffer>
{
public:
    CD3D12IndexBuffer(CD3D12Device* InDevice, ERHIIndexFormat InIndexFormat, uint32 InNumIndices, uint32 InFlags)
        : TD3D12BaseBuffer<CD3D12BaseIndexBuffer>(InDevice, InIndexFormat, InNumIndices, InFlags)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12ConstantBuffer

class CD3D12ConstantBuffer : public TD3D12BaseBuffer<CD3D12BaseConstantBuffer>
{
public:
    CD3D12ConstantBuffer(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap, uint32 InSizeInBytes, uint32 InFlags)
        : TD3D12BaseBuffer<CD3D12BaseConstantBuffer>(InDevice, InHeap, InSizeInBytes, InFlags)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12StructuredBuffer

class CD3D12StructuredBuffer : public TD3D12BaseBuffer<CD3D12BaseStructuredBuffer>
{
public:
    CD3D12StructuredBuffer(CD3D12Device* InDevice, uint32 InNumElements, uint32 InStride, uint32 InFlags)
        : TD3D12BaseBuffer<CD3D12BaseStructuredBuffer>(InDevice, InNumElements, InStride, InFlags)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12BufferCast

inline CD3D12BaseBuffer* D3D12BufferCast(CRHIBuffer* Buffer)
{
    if (Buffer)
    {
        if (Buffer->AsVertexBuffer())
        {
            return static_cast<CD3D12VertexBuffer*>(Buffer);
        }
        else if (Buffer->AsIndexBuffer())
        {
            return static_cast<CD3D12IndexBuffer*>(Buffer);
        }
        else if (Buffer->AsConstantBuffer())
        {
            return static_cast<CD3D12ConstantBuffer*>(Buffer);
        }
        else if (Buffer->AsStructuredBuffer())
        {
            return static_cast<CD3D12StructuredBuffer*>(Buffer);
        }
    }

    return nullptr;
}