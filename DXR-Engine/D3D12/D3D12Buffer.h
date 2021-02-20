#pragma once
#include "RenderLayer/Resources.h"

#include "D3D12Resource.h"
#include "D3D12Views.h"

class D3D12BaseBuffer : public D3D12DeviceChild
{
public:
    D3D12BaseBuffer(D3D12Device* InDevice)
        : D3D12DeviceChild(InDevice)
        , Resource(nullptr)
    {
    }

    virtual void SetResource(D3D12Resource* InResource) { Resource = InResource; }

    UInt64 GetSizeInBytes() const { return Resource->GetDesc().Width; }
    
    D3D12Resource* GetResource() { return Resource.Get(); }

protected:
    TRef<D3D12Resource> Resource;
};

class D3D12BaseVertexBuffer : public VertexBuffer, public D3D12BaseBuffer
{
public:
    D3D12BaseVertexBuffer(D3D12Device* InDevice, UInt32 InNumVertices, UInt32 InStride, UInt32 InFlags)
        : VertexBuffer(InNumVertices, InStride, InFlags)
        , D3D12BaseBuffer(InDevice)
        , View()
    {
    }

    virtual void SetResource(D3D12Resource* InResource) override
    {
        D3D12BaseBuffer::SetResource(InResource);

        Memory::Memzero(&View);
        View.StrideInBytes  = GetStride();
        View.SizeInBytes    = GetNumVertices() * View.StrideInBytes;
        View.BufferLocation = D3D12BaseBuffer::Resource->GetGPUVirtualAddress();
    }

    const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return View; }

private:
    D3D12_VERTEX_BUFFER_VIEW View;
};

class D3D12BaseIndexBuffer : public IndexBuffer, public D3D12BaseBuffer
{
public:
    D3D12BaseIndexBuffer(D3D12Device* InDevice, EIndexFormat InIndexFormat, UInt32 InNumIndices, UInt32 InFlags)
        : IndexBuffer(InIndexFormat, InNumIndices, InFlags)
        , D3D12BaseBuffer(InDevice)
        , View()
    {
    }

    virtual void SetResource(D3D12Resource* InResource) override
    {
        D3D12BaseBuffer::SetResource(InResource);

        Memory::Memzero(&View);

        EIndexFormat IndexFormat = GetFormat();
        if (IndexFormat != EIndexFormat::Unknown)
        {
            View.Format         = IndexFormat == EIndexFormat::UInt16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            View.SizeInBytes    = GetNumIndicies() * GetStrideFromIndexFormat(IndexFormat);
            View.BufferLocation = D3D12BaseBuffer::Resource->GetGPUVirtualAddress();
        }
    }

    const D3D12_INDEX_BUFFER_VIEW& GetView() const { return View; }

private:
    D3D12_INDEX_BUFFER_VIEW View;
};

class D3D12BaseConstantBuffer : public ConstantBuffer, public D3D12BaseBuffer
{
public:
    D3D12BaseConstantBuffer(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap, UInt32 InSizeInBytes, UInt32 InFlags)
        : ConstantBuffer(InSizeInBytes, InFlags)
        , D3D12BaseBuffer(InDevice)
        , View(InDevice, InHeap)
    {
    }

    virtual void SetResource(D3D12Resource* InResource) override
    {
        D3D12BaseBuffer::SetResource(InResource);

        D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
        Memory::Memzero(&ViewDesc);

        ViewDesc.BufferLocation = D3D12BaseBuffer::Resource->GetGPUVirtualAddress();
        ViewDesc.SizeInBytes    = (UInt32)D3D12BaseBuffer::GetSizeInBytes();

        if (View.GetOfflineHandle() == 0)
        {
            if (!View.Init())
            {
                return;
            }
        }

        View.CreateView(D3D12BaseBuffer::Resource.Get(), ViewDesc);
    }

    D3D12ConstantBufferView& GetView() { return View; }
    const D3D12ConstantBufferView& GetView() const { return View; }

private:
    D3D12ConstantBufferView View;
};

class D3D12BaseStructuredBuffer : public StructuredBuffer, public D3D12BaseBuffer
{
public:
    D3D12BaseStructuredBuffer(D3D12Device* InDevice, UInt32 InSizeInBytes, UInt32 InStride, UInt32 InFlags)
        : StructuredBuffer(InSizeInBytes, InStride, InFlags)
        , D3D12BaseBuffer(InDevice)
    {
    }
};

template<typename TBaseBuffer>
class TD3D12BaseBuffer : public TBaseBuffer
{
public:
    template<typename... TBufferArgs>
    TD3D12BaseBuffer(D3D12Device* InDevice, TBufferArgs&&... BufferArgs)
        : TBaseBuffer(InDevice, ::Forward<TBufferArgs>(BufferArgs)...)
    {
    }

    virtual void* Map(UInt32 Offset, UInt32 InSize) override
    {
        return D3D12BaseBuffer::Resource->Map(Offset, InSize);
    }

    virtual void Unmap(UInt32 Offset, UInt32 InSize) override
    {
        D3D12BaseBuffer::Resource->Unmap(Offset, InSize);
    }

    virtual void SetName(const std::string& InName) override final
    {
        Resource::SetName(InName);
        D3D12BaseBuffer::Resource->SetName(InName);
    }

    virtual void* GetNativeResource() const override final
    {
        return reinterpret_cast<void*>(D3D12BaseBuffer::Resource->GetResource());
    }

    virtual Bool IsValid() const override
    {
        return D3D12BaseBuffer::Resource->GetResource() != nullptr;
    }
};

class D3D12VertexBuffer : public TD3D12BaseBuffer<D3D12BaseVertexBuffer>
{
public:
    D3D12VertexBuffer(D3D12Device* InDevice, UInt32 InNumVertices, UInt32 InStride, UInt32 InFlags)
        : TD3D12BaseBuffer<D3D12BaseVertexBuffer>(InDevice, InNumVertices, InStride, InFlags)
    {
    }
};

class D3D12IndexBuffer : public TD3D12BaseBuffer<D3D12BaseIndexBuffer>
{
public:
    D3D12IndexBuffer(D3D12Device* InDevice, EIndexFormat InIndexFormat, UInt32 InNumIndices, UInt32 InFlags)
        : TD3D12BaseBuffer<D3D12BaseIndexBuffer>(InDevice, InIndexFormat, InNumIndices, InFlags)
    {
    }
};

class D3D12ConstantBuffer : public TD3D12BaseBuffer<D3D12BaseConstantBuffer>
{
public:
    D3D12ConstantBuffer(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap, UInt32 InSizeInBytes, UInt32 InFlags)
        : TD3D12BaseBuffer<D3D12BaseConstantBuffer>(InDevice, InHeap, InSizeInBytes, InFlags)
    {
    }
};

class D3D12StructuredBuffer : public TD3D12BaseBuffer<D3D12BaseStructuredBuffer>
{
public:
    D3D12StructuredBuffer(D3D12Device* InDevice, UInt32 InNumElements, UInt32 InStride, UInt32 InFlags)
        : TD3D12BaseBuffer<D3D12BaseStructuredBuffer>(InDevice, InNumElements, InStride, InFlags)
    {
    }
};

inline D3D12BaseBuffer* D3D12BufferCast(Buffer* Buffer)
{
    if (Buffer->AsVertexBuffer())
    {
        return static_cast<D3D12VertexBuffer*>(Buffer);
    }
    else if (Buffer->AsIndexBuffer())
    {
        return static_cast<D3D12IndexBuffer*>(Buffer);
    }
    else if (Buffer->AsConstantBuffer())
    {
        return static_cast<D3D12ConstantBuffer*>(Buffer);
    }
    else if (Buffer->AsStructuredBuffer())
    {
        return static_cast<D3D12StructuredBuffer*>(Buffer);
    }
    else
    {
        return nullptr;
    }
}