#pragma once
#include "D3D12Resource.h"
#include "D3D12RHIViews.h"

#include "RHI/RHIResources.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Buffer

class CD3D12Buffer : public CD3D12DeviceChild
{
public:
    CD3D12Buffer(CD3D12Device* InDevice)
        : CD3D12DeviceChild(InDevice)
        , Resource(nullptr)
    { }

    /** @brief : Set the native resource, this function takes ownership of the current reference, call AddRef to use it in more places */
    virtual void SetResource(CD3D12Resource* InResource) { Resource = InResource; }

    FORCEINLINE uint64 GetSizeInBytes() const
    {
        return static_cast<uint64>(Resource->GetDesc().Width);
    }

    FORCEINLINE CD3D12Resource* GetResource() const
    {
        return Resource.Get();
    }

protected:
    TSharedRef<CD3D12Resource> Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12VertexBuffer

class CD3D12VertexBuffer : public CRHIVertexBuffer, public CD3D12Buffer
{
public:

    CD3D12VertexBuffer(CD3D12Device* InDevice, EBufferUsageFlags InFlags, uint32 InNumVertices, uint32 InStride)
        : CRHIVertexBuffer(InFlags, InNumVertices, InStride)
        , CD3D12Buffer(InDevice)
        , View()
    { }

    FORCEINLINE const D3D12_VERTEX_BUFFER_VIEW& GetView() const
    {
        return View;
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIVertexBuffer Interface

    virtual void* GetRHIResourceHandle() const override final
    {
        CD3D12Resource* D3D12Resource = GetResource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseBuffer() override final
    {
        CD3D12Buffer* D3D12Buffer = static_cast<CD3D12Buffer*>(this);
        return reinterpret_cast<void*>(D3D12Buffer);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CD3D12Buffer Interface

    virtual void SetResource(CD3D12Resource* InResource) override final
    {
        CD3D12Buffer::SetResource(InResource);

        CMemory::Memzero(&View);
        View.StrideInBytes  = GetStride();
        View.SizeInBytes    = GetNumVertices() * View.StrideInBytes;
        View.BufferLocation = CD3D12Buffer::Resource->GetGPUVirtualAddress();
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Deprecated

    virtual void* Map(uint32 Offset, uint32 InSize) override
    {
        D3D12_ERROR((GetFlags() & EBufferUsageFlags::Dynamic) != EBufferUsageFlags::None, "Map is only supported on dynamic buffers");

        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
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
        D3D12_ERROR((GetFlags() & EBufferUsageFlags::Dynamic) != EBufferUsageFlags::None, "Unmap is only supported on dynamic buffers");

        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
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

    virtual void SetName(const String& InName) override final
    {
        CRHIResource::SetName(InName);

        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
        if (DxResource)
        {
            DxResource->SetName(InName);
        }
    }

    virtual void* GetNativeResource() const override final
    {
        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
        return DxResource ? reinterpret_cast<void*>(DxResource->GetResource()) : nullptr;
    }

    virtual bool IsValid() const override
    {
        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
        return DxResource ? (CD3D12Buffer::Resource->GetResource() != nullptr) : false;
    }

private:
    D3D12_VERTEX_BUFFER_VIEW View;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12IndexBuffer

class CD3D12IndexBuffer : public CRHIIndexBuffer, public CD3D12Buffer
{
public:

    CD3D12IndexBuffer(CD3D12Device* InDevice, EBufferUsageFlags InFlags, EIndexFormat InIndexFormat, uint32 InNumIndices)
        : CRHIIndexBuffer(InFlags, InIndexFormat, InNumIndices)
        , CD3D12Buffer(InDevice)
        , View()
    { }

    FORCEINLINE const D3D12_INDEX_BUFFER_VIEW& GetView() const
    {
        return View;
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIIndexBuffer Interface

    virtual void* GetRHIResourceHandle() const override final
    {
        CD3D12Resource* D3D12Resource = GetResource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseBuffer() override final
    {
        CD3D12Buffer* D3D12Buffer = static_cast<CD3D12Buffer*>(this);
        return reinterpret_cast<void*>(D3D12Buffer);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CD3D12Buffer Interface

    virtual void SetResource(CD3D12Resource* InResource) override final
    {
        CD3D12Buffer::SetResource(InResource);

        CMemory::Memzero(&View);

        EIndexFormat IndexFormat = GetFormat();
        if (IndexFormat != EIndexFormat::Unknown)
        {
            View.Format         = (IndexFormat == EIndexFormat::uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            View.SizeInBytes    = GetNumIndicies() * GetStrideFromIndexFormat(IndexFormat);
            View.BufferLocation = CD3D12Buffer::Resource->GetGPUVirtualAddress();
        }
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Deprecated

    virtual void* Map(uint32 Offset, uint32 InSize) override
    {
        D3D12_ERROR((GetFlags() & EBufferUsageFlags::Dynamic) != EBufferUsageFlags::None, "Map is only supported on dynamic buffers");

        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
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
        D3D12_ERROR((GetFlags() & EBufferUsageFlags::Dynamic) != EBufferUsageFlags::None, "Unmap is only supported on dynamic buffers");

        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
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

    virtual void SetName(const String& InName) override final
    {
        CRHIResource::SetName(InName);

        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
        if (DxResource)
        {
            DxResource->SetName(InName);
        }
    }

    virtual void* GetNativeResource() const override final
    {
        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
        return DxResource ? reinterpret_cast<void*>(DxResource->GetResource()) : nullptr;
    }

    virtual bool IsValid() const override
    {
        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
        return DxResource ? (CD3D12Buffer::Resource->GetResource() != nullptr) : false;
    }

private:
    D3D12_INDEX_BUFFER_VIEW View;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12ConstantBuffer

class CD3D12ConstantBuffer : public CRHIConstantBuffer, public CD3D12Buffer
{
public:

    CD3D12ConstantBuffer(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InHeap, EBufferUsageFlags InFlags, uint32 InSizeInBytes)
        : CRHIConstantBuffer(InFlags, InSizeInBytes)
        , CD3D12Buffer(InDevice)
        , View(InDevice, InHeap)
    { }

    FORCEINLINE CD3D12RHIConstantBufferView& GetView()
    {
        return View;
    }

    FORCEINLINE const CD3D12RHIConstantBufferView& GetView() const
    {
        return View;
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIConstantBuffer Interface

    virtual void* GetRHIResourceHandle() const override final
    {
        CD3D12Resource* D3D12Resource = GetResource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseBuffer() override final
    {
        CD3D12Buffer* D3D12Buffer = static_cast<CD3D12Buffer*>(this);
        return reinterpret_cast<void*>(D3D12Buffer);
    }

    virtual CRHIDescriptorHandle GetBindlessHandle() const override final { return CRHIDescriptorHandle(); }

public:
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CD3D12Buffer Interface

    virtual void SetResource(CD3D12Resource* InResource) override final
    {
        CD3D12Buffer::SetResource(InResource);

        D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
        CMemory::Memzero(&ViewDesc);

        ViewDesc.BufferLocation = CD3D12Buffer::Resource->GetGPUVirtualAddress();
        ViewDesc.SizeInBytes    = (uint32)CD3D12Buffer::GetSizeInBytes();

        if (View.GetOfflineHandle() == 0)
        {
            if (!View.AllocateHandle())
            {
                return;
            }
        }

        View.CreateView(CD3D12Buffer::Resource.Get(), ViewDesc);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Deprecated

    virtual void* Map(uint32 Offset, uint32 InSize) override
    {
        D3D12_ERROR((GetFlags() & EBufferUsageFlags::Dynamic) != EBufferUsageFlags::None, "Map is only supported on dynamic buffers");

        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
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
        D3D12_ERROR((GetFlags() & EBufferUsageFlags::Dynamic) != EBufferUsageFlags::None, "Unmap is only supported on dynamic buffers");

        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
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

    virtual void SetName(const String& InName) override final
    {
        CRHIResource::SetName(InName);

        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
        if (DxResource)
        {
            DxResource->SetName(InName);
        }
    }

    virtual void* GetNativeResource() const override final
    {
        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
        return DxResource ? reinterpret_cast<void*>(DxResource->GetResource()) : nullptr;
    }

    virtual bool IsValid() const override
    {
        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
        return DxResource ? (CD3D12Buffer::Resource->GetResource() != nullptr) : false;
    }

private:
    CD3D12RHIConstantBufferView View;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12GenericBuffer

class CD3D12GenericBuffer : public CRHIGenericBuffer, public CD3D12Buffer
{
public:
    
    CD3D12GenericBuffer(CD3D12Device* InDevice, EBufferUsageFlags InFlags, uint32 InSizeInBytes, uint32 InStride)
        : CRHIGenericBuffer(InFlags, InSizeInBytes, InStride)
        , CD3D12Buffer(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIGenericBuffer Interface

    virtual void* GetRHIResourceHandle() const override final
    {
        CD3D12Resource* D3D12Resource = GetResource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseBuffer() override final
    {
        CD3D12Buffer* D3D12Buffer = static_cast<CD3D12Buffer*>(this);
        return reinterpret_cast<void*>(D3D12Buffer);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Deprecated

    virtual void* Map(uint32 Offset, uint32 InSize) override
    {
        D3D12_ERROR((GetFlags() & EBufferUsageFlags::Dynamic) != EBufferUsageFlags::None, "Map is only supported on dynamic buffers");

        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
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
        D3D12_ERROR((GetFlags() & EBufferUsageFlags::Dynamic) != EBufferUsageFlags::None, "Unmap is only supported on dynamic buffers");

        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
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

    virtual void SetName(const String& InName) override final
    {
        CRHIResource::SetName(InName);

        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
        if (DxResource)
        {
            DxResource->SetName(InName);
        }
    }

    virtual void* GetNativeResource() const override final
    {
        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
        return DxResource ? reinterpret_cast<void*>(DxResource->GetResource()) : nullptr;
    }

    virtual bool IsValid() const override
    {
        CD3D12Resource* DxResource = CD3D12Buffer::Resource.Get();
        return DxResource ? (CD3D12Buffer::Resource->GetResource() != nullptr) : false;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12BufferCast

inline CD3D12Buffer* D3D12BufferCast(CRHIBuffer* Buffer)
{
    return Buffer ? reinterpret_cast<CD3D12Buffer*>(Buffer->GetRHIBaseBuffer()) : nullptr;
}