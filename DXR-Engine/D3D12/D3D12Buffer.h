#pragma once
#include "RenderLayer/Resources.h"

#include "D3D12Resource.h"
#include "D3D12Views.h"

class D3D12Buffer : public D3D12Resource
{
public:
    D3D12Buffer(D3D12Device* InDevice)
        : D3D12Resource(InDevice)
    {
    }

    UInt64 GetAllocatedSize() const
    {
        return Desc.Width;
    }
};

class D3D12VertexBuffer : public VertexBuffer, public D3D12Buffer
{
    friend class D3D12RenderLayer;

public:
    D3D12VertexBuffer(D3D12Device* InDevice, UInt32 InSizeInBytes, UInt32 InStride, UInt32 InUsage)
        : VertexBuffer(InSizeInBytes, InStride, InUsage)
        , D3D12Buffer(InDevice)
        , VertexBufferView()
    {
    }

    virtual void* Map(UInt32 Offset, UInt32 Size) override
    {
        return D3D12Resource::Map(Offset, Size);
    }

    virtual void Unmap(UInt32 Offset, UInt32 Size) override
    {
        D3D12Resource::Unmap(Offset, Size);
    }

    virtual void SetName(const std::string& Name) override final
    {
        D3D12Resource::SetName(Name);
    }

    const D3D12_VERTEX_BUFFER_VIEW& GetView() const
    {
        return VertexBufferView;
    }

private:
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
};

class D3D12IndexBuffer : public IndexBuffer, public D3D12Buffer
{
    friend class D3D12RenderLayer;

public:
    D3D12IndexBuffer(D3D12Device* InDevice, UInt32 InSizeInBytes, EIndexFormat InIndexFormat, UInt32 InUsage)
        : IndexBuffer(InSizeInBytes, InIndexFormat, InUsage)
        , D3D12Buffer(InDevice)
        , IndexBufferView()
    {
    }

    virtual void* Map(UInt32 Offset, UInt32 Size) override
    {
        return D3D12Resource::Map(Offset, Size);
    }

    virtual void Unmap(UInt32 Offset, UInt32 Size) override
    {
        D3D12Resource::Unmap(Offset, Size);
    }

    virtual void SetName(const std::string& Name) override final
    {
        D3D12Resource::SetName(Name);
    }

    FORCEINLINE const D3D12_INDEX_BUFFER_VIEW& GetView() const
    {
        return IndexBufferView;
    }

private:
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
};

class D3D12ConstantBuffer : public ConstantBuffer, public D3D12Buffer
{
    friend class D3D12RenderLayer;

public:
    D3D12ConstantBuffer(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap, UInt32 InSizeInBytes)
        : ConstantBuffer(InSizeInBytes)
        , D3D12Buffer(InDevice)
        , View(InDevice, InHeap)
    {
    }

    virtual void* Map(UInt32 Offset, UInt32 Size) override
    {
        return D3D12Resource::Map(Offset, Size);
    }

    virtual void Unmap(UInt32 Offset, UInt32 Size) override
    {
        D3D12Resource::Unmap(Offset, Size);
    }

    virtual void SetName(const std::string& Name) override final
    {
        D3D12Resource::SetName(Name);
    }

    D3D12ConstantBufferView& GetView()
    {
        return View;
    }

    const D3D12ConstantBufferView& GetView() const
    {
        return View;
    }

private:
    D3D12ConstantBufferView View;
};

class D3D12StructuredBuffer : public StructuredBuffer, public D3D12Buffer
{
    friend class D3D12RenderLayer;

public:
    D3D12StructuredBuffer(D3D12Device* InDevice, UInt32 InSizeInBytes, UInt32 InStride, UInt32 InUsage)
        : StructuredBuffer(InSizeInBytes, InStride, InUsage)
        , D3D12Buffer(InDevice)
    {
    }

    virtual void* Map(UInt32 Offset, UInt32 Size) override
    {
        return D3D12Resource::Map(Offset, Size);
    }

    virtual void Unmap(UInt32 Offset, UInt32 Size) override
    {
        D3D12Resource::Unmap(Offset, Size);
    }

    virtual void SetName(const std::string& Name) override final
    {
        D3D12Resource::SetName(Name);
    }
};
