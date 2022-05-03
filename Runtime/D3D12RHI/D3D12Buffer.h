#pragma once
#include "D3D12Resource.h"
#include "D3D12Views.h"

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

    uint64 GetSizeInBytes() const { return Resource ? static_cast<uint64>(Resource->GetDesc().Width) : 0; }

    CD3D12Resource* GetD3D12Resource() const { return Resource.Get(); }

protected:
    TSharedRef<CD3D12Resource> Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12VertexBuffer

class CD3D12VertexBuffer : public CRHIVertexBuffer, public CD3D12Buffer
{
public:

    CD3D12VertexBuffer(CD3D12Device* InDevice, const CRHIVertexBufferInitializer& Initializer)
        : CRHIVertexBuffer(Initializer)
        , CD3D12Buffer(InDevice)
        , View()
    { }

    FORCEINLINE const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return View; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIVertexBuffer Interface

    virtual void* GetRHIBaseResource() const override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseBuffer() override final
    {
        CD3D12Buffer* D3D12Buffer = static_cast<CD3D12Buffer*>(this);
        return reinterpret_cast<void*>(D3D12Buffer);
    }

    virtual void SetName(const String& InName) override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
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

private:
    D3D12_VERTEX_BUFFER_VIEW View;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12IndexBuffer

class CD3D12IndexBuffer : public CRHIIndexBuffer, public CD3D12Buffer
{
public:

    CD3D12IndexBuffer(CD3D12Device* InDevice, const CRHIIndexBufferInitializer& Initializer)
        : CRHIIndexBuffer(Initializer)
        , CD3D12Buffer(InDevice)
        , View()
    { }

    FORCEINLINE const D3D12_INDEX_BUFFER_VIEW& GetView() const { return View; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIIndexBuffer Interface

    virtual void* GetRHIBaseResource() const override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseBuffer() override final
    {
        CD3D12Buffer* D3D12Buffer = static_cast<CD3D12Buffer*>(this);
        return reinterpret_cast<void*>(D3D12Buffer);
    }

    virtual void SetName(const String& InName) override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
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

private:
    D3D12_INDEX_BUFFER_VIEW View;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12ConstantBuffer

class CD3D12ConstantBuffer : public CRHIConstantBuffer, public CD3D12Buffer
{
public:

    CD3D12ConstantBuffer(CD3D12Device* InDevice, CD3D12OfflineDescriptorHeap* InOfflineHeap, const CRHIConstantBufferInitializer& Initializer)
        : CRHIConstantBuffer(Initializer)
        , CD3D12Buffer(InDevice)
        , View(InDevice, InOfflineHeap)
    { }

    CD3D12ConstantBufferView& GetView() { return View; }

    const CD3D12ConstantBufferView& GetView() const { return View; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIConstantBuffer Interface

    virtual void* GetRHIBaseResource() const override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseBuffer() override final
    {
        CD3D12Buffer* D3D12Buffer = static_cast<CD3D12Buffer*>(this);
        return reinterpret_cast<void*>(D3D12Buffer);
    }

    virtual CRHIDescriptorHandle GetBindlessHandle() const override final 
    { 
        return CRHIDescriptorHandle();
    }

    virtual void SetName(const String& InName) override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }

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

private:
    CD3D12ConstantBufferView View;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12GenericBuffer

class CD3D12GenericBuffer : public CRHIGenericBuffer, public CD3D12Buffer
{
public:
    
    CD3D12GenericBuffer(CD3D12Device* InDevice, const CRHIGenericBufferInitializer& Initializer)
        : CRHIGenericBuffer(Initializer)
        , CD3D12Buffer(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIGenericBuffer Interface

    virtual void* GetRHIBaseResource() const override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseBuffer() override final
    {
        CD3D12Buffer* D3D12Buffer = static_cast<CD3D12Buffer*>(this);
        return reinterpret_cast<void*>(D3D12Buffer);
    }

    virtual void SetName(const String& InName) override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12BufferCast

inline CD3D12Buffer* D3D12BufferCast(CRHIBuffer* Buffer)
{
    return Buffer ? reinterpret_cast<CD3D12Buffer*>(Buffer->GetRHIBaseBuffer()) : nullptr;
}

inline CD3D12Resource* D3D12ResourceCast(CRHIBuffer* Buffer)
{
    return Buffer ? reinterpret_cast<CD3D12Resource*>(Buffer->GetRHIBaseResource()) : nullptr;
}