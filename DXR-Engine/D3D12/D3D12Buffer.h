#pragma once
#include "RenderLayer/Resources.h"

#include "D3D12Resource.h"
#include "D3D12Views.h"

class D3D12Buffer : public D3D12Resource
{
public:
    D3D12Buffer::D3D12Buffer(D3D12Device* InDevice)
        : D3D12Resource(InDevice)
    {
    }

    FORCEINLINE UInt64 GetAllocatedSize() const
    {
        return Desc.Width;
    }
};

class D3D12VertexBuffer : public VertexBuffer, public D3D12Buffer
{
    friend class D3D12RenderLayer;

public:
    D3D12VertexBuffer::D3D12VertexBuffer(D3D12Device* InDevice, UInt32 InSizeInBytes, UInt32 InStride, UInt32 InUsage)
        : VertexBuffer(InSizeInBytes, InStride, InUsage)
        , D3D12Buffer(InDevice)
        , VertexBufferView()
    {
    }

    virtual Void* Map(const Range* MappedRange) override
    {
        return D3D12Resource::Map(MappedRange);
    }

    virtual void Unmap(const Range* WrittenRange) override
    {
        D3D12Resource::Unmap(WrittenRange);
    }

    virtual void SetName(const std::string& Name) override final
    {
        D3D12Resource::SetName(Name);
    }

    virtual UInt64 GetRequiredAlignment() const override final
    {
        return 1;
    }

    FORCEINLINE const D3D12_VERTEX_BUFFER_VIEW& GetView() const
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

    virtual Void* Map(const Range* MappedRange) override
    {
        return D3D12Resource::Map(MappedRange);
    }

    virtual void Unmap(const Range* WrittenRange) override
    {
        D3D12Resource::Unmap(WrittenRange);
    }

    virtual void SetName(const std::string& Name) override final
    {
        D3D12Resource::SetName(Name);
    }

    virtual UInt64 GetRequiredAlignment() const override final
    {
        return 1;
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
    D3D12ConstantBuffer(D3D12Device* InDevice, UInt32 InSizeInBytes, UInt32 InUsage)
        : ConstantBuffer(InSizeInBytes, InUsage)
        , D3D12Buffer(InDevice)
        , View(nullptr)
    {
    }

    ~D3D12ConstantBuffer()
    {
        SAFEDELETE(View);
    }

    virtual Void* Map(const Range* MappedRange) override
    {
        return D3D12Resource::Map(MappedRange);
    }

    virtual void Unmap(const Range* WrittenRange) override
    {
        D3D12Resource::Unmap(WrittenRange);
    }

    virtual void SetName(const std::string& Name) override final
    {
        D3D12Resource::SetName(Name);
    }

    virtual UInt64 GetRequiredAlignment() const override final
    {
        return D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    }

    FORCEINLINE D3D12ConstantBufferView* GetView() const
    {
        return View;
    }

private:
    D3D12ConstantBufferView* View;
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

    virtual Void* Map(const Range* MappedRange) override
    {
        return D3D12Resource::Map(MappedRange);
    }

    virtual void Unmap(const Range* WrittenRange) override
    {
        D3D12Resource::Unmap(WrittenRange);
    }

    virtual void SetName(const std::string& Name) override final
    {
        D3D12Resource::SetName(Name);
    }

    virtual UInt64 GetRequiredAlignment() const override final
    {
        return 1;
    }
};

inline D3D12Buffer* D3D12BufferCast(Buffer* Buffer)
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

inline const D3D12Buffer* D3D12BufferCast(const Buffer* Buffer)
{
    if (Buffer->AsVertexBuffer())
    {
        return static_cast<const D3D12VertexBuffer*>(Buffer);
    }
    else if (Buffer->AsIndexBuffer())
    {
        return static_cast<const D3D12IndexBuffer*>(Buffer);
    }
    else if (Buffer->AsConstantBuffer())
    {
        return static_cast<const D3D12ConstantBuffer*>(Buffer);
    }
    else if (Buffer->AsStructuredBuffer())
    {
        return static_cast<const D3D12StructuredBuffer*>(Buffer);
    }
    else
    {
        return nullptr;
    }
}
