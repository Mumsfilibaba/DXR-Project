#pragma once
#include "RHI/RHI.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIRayTracing.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

inline FRHIShaderResourceViewDesc GetDefaultShaderResourceViewDescForTexture(const FRHITextureDesc& TextureDesc)
{
    const EFormat Format    = TextureDesc.Format;
    const uint8   NumMips   = static_cast<uint8>(TextureDesc.NumMipLevels);
    const uint16  NumSlices = static_cast<uint16>(TextureDesc.NumArraySlices);

    if (TextureDesc.IsTexture1D())
    {
        return FRHIShaderResourceViewDesc::CreateTexture1D(Format, 0, NumMips);
    }

    if (TextureDesc.IsTexture1DArray())
    {
        return FRHIShaderResourceViewDesc::CreateTexture1DArray(Format, 0, NumMips, 0, NumSlices);
    }

    if (TextureDesc.IsTexture2D())
    {
        return FRHIShaderResourceViewDesc::CreateTexture2D(Format, 0, NumMips);
    }

    if (TextureDesc.IsTexture2DArray())
    {
        return FRHIShaderResourceViewDesc::CreateTexture2DArray(Format, 0, NumMips, 0, NumSlices);
    }

    if (TextureDesc.IsTextureCube())
    {
        return FRHIShaderResourceViewDesc::CreateTextureCube(Format, 0, NumMips);
    }

    if (TextureDesc.IsTextureCubeArray())
    {
        return FRHIShaderResourceViewDesc::CreateTextureCubeArray(Format, 0, NumMips, 0, NumSlices);
    }

    if (TextureDesc.IsTexture3D())
    {
        return FRHIShaderResourceViewDesc::CreateTexture3D(Format, 0, NumMips);
    }

    return FRHIShaderResourceViewDesc{};
}

inline FRHIUnorderedAccessViewDesc GetDefaultUnorderedAccessViewDescForTexture(const FRHITextureDesc& TextureDesc)
{
    const EFormat Format    = TextureDesc.Format;
    const uint16  NumSlices = static_cast<uint16>(TextureDesc.NumArraySlices);

    if (TextureDesc.IsTexture1D())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture1D(Format, 0);
    }

    if (TextureDesc.IsTexture1DArray())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture1DArray(Format, 0, 0, NumSlices);
    }

    if (TextureDesc.IsTexture2D())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture2D(Format, 0);
    }

    if (TextureDesc.IsTexture2DArray())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture2DArray(Format, 0, 0, NumSlices);
    }

    if (TextureDesc.IsTextureCube() || TextureDesc.IsTextureCubeArray())
    {
        const uint16 ArraySize = static_cast<uint16>(RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices));
        return FRHIUnorderedAccessViewDesc::CreateTexture2DArray(Format, 0, 0, ArraySize);
    }

    if (TextureDesc.IsTexture3D())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture3D(Format, 0, 0, static_cast<uint16>(TextureDesc.Extent.Z));
    }

    return FRHIUnorderedAccessViewDesc{};
}

inline FRHIRenderTargetViewDesc GetDefaultRenderTargetViewDescForTexture(const FRHITextureDesc& TextureDesc)
{
    const EFormat Format    = TextureDesc.Format;
    const uint16  NumSlices = static_cast<uint16>(TextureDesc.NumArraySlices);

    if (TextureDesc.IsTexture1D())
    {
        return FRHIRenderTargetViewDesc::CreateTexture1D(Format, 0);
    }

    if (TextureDesc.IsTexture1DArray())
    {
        return FRHIRenderTargetViewDesc::CreateTexture1DArray(Format, 0, 0, NumSlices);
    }

    if (TextureDesc.IsTexture2D())
    {
        return FRHIRenderTargetViewDesc::CreateTexture2D(Format, 0);
    }

    if (TextureDesc.IsTexture2DArray() || TextureDesc.IsTextureCube() || TextureDesc.IsTextureCubeArray())
    {
        const uint16 ArraySize = static_cast<uint16>(RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices));
        return FRHIRenderTargetViewDesc::CreateTexture2DArray(Format, 0, 0, ArraySize);
    }

    if (TextureDesc.IsTexture3D())
    {
        return FRHIRenderTargetViewDesc::CreateTexture3D(Format, 0, 0, static_cast<uint16>(TextureDesc.Extent.Z));
    }

    return FRHIRenderTargetViewDesc{};
}

inline FRHIDepthStencilViewDesc GetDefaultDepthStencilViewDescForTexture(const FRHITextureDesc& TextureDesc)
{
    const EFormat Format    = TextureDesc.ClearValue.Format != EFormat::Unknown ? TextureDesc.ClearValue.Format : TextureDesc.Format;
    const uint16  NumSlices = static_cast<uint16>(TextureDesc.NumArraySlices);

    if (TextureDesc.IsTexture1D())
    {
        return FRHIDepthStencilViewDesc::CreateTexture1D(Format, 0);
    }

    if (TextureDesc.IsTexture1DArray())
    {
        return FRHIDepthStencilViewDesc::CreateTexture1DArray(Format, 0, 0, NumSlices);
    }

    if (TextureDesc.IsTexture2D())
    {
        return FRHIDepthStencilViewDesc::CreateTexture2D(Format, 0);
    }

    if (TextureDesc.IsTexture2DArray() || TextureDesc.IsTextureCube() || TextureDesc.IsTextureCubeArray())
    {
        const uint16 ArraySize = static_cast<uint16>(RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices));
        return FRHIDepthStencilViewDesc::CreateTexture2DArray(Format, 0, 0, ArraySize);
    }

    return FRHIDepthStencilViewDesc{};
}

class FNullBufferRHI : public FRHIBuffer
{
public:
    FNullBufferRHI(const FRHIBufferDesc& InBufferDesc)
        : FRHIBuffer(InBufferDesc)
    {
    }

    virtual void* GetRHINativeResource() const override final
    {
        return nullptr;
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final
    {
        return FRHIDescriptorHandle();
    }

    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final
    {
        return nullptr;
    }

    virtual void Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final
    {
    }

    virtual void SetDebugName(const String& InDebugName) override final
    {
        DebugName = InDebugName;
    }

    virtual void GetDebugName(String& OutDebugName) const override final
    {
        OutDebugName = DebugName;
    }

private:
    String DebugName;
};

struct FNullShaderResourceViewRHI : public FRHIShaderResourceView
{
    FNullShaderResourceViewRHI(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc)
        : FRHIShaderResourceView(InResource, InRHIDesc)
    {
    }

    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final
    {
        return FRHIDescriptorHandle();
    }
};

struct FNullUnorderedAccessViewRHI : public FRHIUnorderedAccessView
{
    FNullUnorderedAccessViewRHI(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc)
        : FRHIUnorderedAccessView(InResource, InRHIDesc)
    {
    }

    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final
    {
        return FRHIDescriptorHandle();
    }
};

struct FNullRenderTargetViewRHI : public FRHIRenderTargetView
{
    FNullRenderTargetViewRHI(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InRHIDesc)
        : FRHIRenderTargetView(InResource, InRHIDesc)
    {
    }

    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }
};

struct FNullDepthStencilViewRHI : public FRHIDepthStencilView
{
    FNullDepthStencilViewRHI(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InRHIDesc)
        : FRHIDepthStencilView(InResource, InRHIDesc)
    {
    }

    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }

    EDepthStencilViewFlags GetFlags() const
    {
        return Desc.Flags;
    }
};

class FNullTextureRHI : public FRHITexture
{
public:
    FNullTextureRHI(const FRHITextureDesc& InTextureDesc)
        : FRHITexture(InTextureDesc)
        , ShaderResourceView(nullptr)
        , UnorderedAccessView(nullptr)
        , RenderTargetView(nullptr)
        , DepthStencilView(nullptr)
    {
        if (Desc.IsShaderResourceTexture() && !Desc.IsNoDefaultSRV())
        {
            ShaderResourceView = new FNullShaderResourceViewRHI(this, GetDefaultShaderResourceViewDescForTexture(Desc));
        }

        if (Desc.IsUnorderedAccessTexture() && !Desc.IsNoDefaultUAV())
        {
            UnorderedAccessView = new FNullUnorderedAccessViewRHI(this, GetDefaultUnorderedAccessViewDescForTexture(Desc));
        }

        if (Desc.IsRenderTarget() && !Desc.IsNoDefaultRTV())
        {
            RenderTargetView = new FNullRenderTargetViewRHI(this, GetDefaultRenderTargetViewDescForTexture(Desc));
        }

        if (Desc.IsDepthStencil() && !Desc.IsNoDefaultDSV())
        {
            DepthStencilView = new FNullDepthStencilViewRHI(this, GetDefaultDepthStencilViewDescForTexture(Desc));
        }
    }

    virtual void* GetRHINativeResource() const override final
    {
        return nullptr;
    }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final
    {
        return FRHIDescriptorHandle();
    }

    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final
    {
        return FRHIDescriptorHandle();
    }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final
    {
        return ShaderResourceView.Get();
    }

    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final
    {
        return UnorderedAccessView.Get();
    }

    virtual FRHIRenderTargetView* GetRenderTargetView() const override final
    {
        return RenderTargetView.Get();
    }

    virtual FRHIDepthStencilView* GetDepthStencilView() const override final
    {
        return DepthStencilView.Get();
    }

    virtual void SetDebugName(const String& InDebugName) override final
    {
        DebugName = InDebugName;
    }

    virtual void GetDebugName(String& OutDebugName) const override final
    {
        OutDebugName = DebugName;
    }

    void Resize(uint32 InWidth, uint32 InHeight)
    {
        Desc.Extent.X = int32(InWidth);
        Desc.Extent.Y = int32(InHeight);
    }

    void UpdateSwapChainTexture(EFormat InFormat, uint32 InWidth, uint32 InHeight)
    {
        Desc.Format = InFormat;
        Resize(InWidth, InHeight);
    }

private:
    TSharedRef<FNullShaderResourceViewRHI>  ShaderResourceView;
    TSharedRef<FNullUnorderedAccessViewRHI> UnorderedAccessView;
    TSharedRef<FNullRenderTargetViewRHI>    RenderTargetView;
    TSharedRef<FNullDepthStencilViewRHI>    DepthStencilView;
    String                                  DebugName;
};

class FNullRayTracingGeometryRHI : public FRHIGeometryAccelerationStructure
{
public:
    FNullRayTracingGeometryRHI(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
        : FRHIGeometryAccelerationStructure(InGeometryDesc)
    {
    }

    virtual void* GetRHINativeResource() const override final
    {
        return nullptr;
    }

    virtual void SetDebugName(const String& InName) override final
    {
        DebugName = InName;
    }

    virtual void GetDebugName(String& OutDebugName) const override final
    {
        OutDebugName = DebugName;
    }

private:
    String DebugName;
};

class FNullRayTracingSceneRHI : public FRHISceneAccelerationStructure
{
public:
    FNullRayTracingSceneRHI(const FRHISceneAccelerationStructureDesc& InSceneDesc)
        : FRHISceneAccelerationStructure(InSceneDesc)
        , View(new FNullShaderResourceViewRHI(this, FRHIShaderResourceViewDesc::CreateAccelerationStructure()))
    {
    }

    virtual void* GetRHINativeResource() const override final
    {
        return nullptr;
    }
    
    virtual FRHIShaderResourceView* GetShaderResourceView() const override final
    {
        return View.Get();
    }
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final
    {
        return FRHIDescriptorHandle();
    }

    virtual void SetDebugName(const String& InName) override final
    {
        DebugName = InName;
    }

    virtual void GetDebugName(String& OutDebugName) const override final
    {
        OutDebugName = DebugName;
    }

private:
    TSharedRef<FNullShaderResourceViewRHI> View;
    String                                 DebugName;
};

struct FNullSamplerStateRHI : public FRHISamplerState
{
    FNullSamplerStateRHI(const FRHISamplerStateDesc& InSamplerDesc)
        : FRHISamplerState(InSamplerDesc)
    {
    }

    virtual void* GetRHINativeSampler() const override final
    {
        return nullptr;
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final
    {
        return FRHIDescriptorHandle();
    }
};

class FNullSwapChainRHI : public FRHISwapChain
{
public:
    static constexpr uint32 kNumBackBuffers = 2;

public:
    FNullSwapChainRHI(const FRHISwapChainDesc& InSwapChainDesc)
        : FRHISwapChain(InSwapChainDesc)
        , BackBufferIndex(0)
    {
        if (Desc.ColorSpace == EColorSpace::Unknown)
        {
            Desc.ColorSpace = EColorSpace::RGB_Full_G22_None_P709;
        }

        if (Desc.ColorFormat == EFormat::Unknown)
        {
            Desc.ColorFormat = (RHI::DefaultSwapChainFormat != EFormat::Unknown) ? RHI::DefaultSwapChainFormat : EFormat::B8G8R8A8_Unorm;
        }

        AllocateBackBuffers();
    }

    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }

    virtual void* GetRHINativeResourceFromIndex(uint32 /*Index*/) const override final
    {
        return nullptr;
    }

    virtual void* GetRHINativeRenderTargetViewFromIndex(uint32 /*Index*/) const override final
    {
        return nullptr;
    }

    virtual void* GetRHINativeUnorderedAccessViewFromIndex(uint32 /*Index*/) const override final
    {
        return nullptr;
    }

    virtual void* GetRHINativeShaderResourceViewFromIndex(uint32 /*Index*/) const override final
    {
        return nullptr;
    }

    virtual FRHITexture* GetBackBuffer() const override final
    {
        return BackBuffer.Get();
    }

    virtual FRHIRenderTargetView* GetRenderTargetView() const override final
    {
        return BackBuffer ? BackBuffer->GetRenderTargetView() : nullptr;
    }

    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final
    {
        return BackBuffer ? BackBuffer->GetUnorderedAccessView() : nullptr;
    }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final
    {
        return BackBuffer ? BackBuffer->GetShaderResourceView() : nullptr;
    }

    virtual uint32 GetNumResources() const override final
    {
        return kNumBackBuffers;
    }

    virtual bool IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const override final
    {
        // NullRHI is permissive: any well-defined format / color-space combination is "supported".
        return Format != EFormat::Unknown && ColorSpace != EColorSpace::Unknown;
    }

    virtual bool QueryDisplayHDRInfo(FRHIDisplayHDRInfo& /*OutInfo*/) const override final
    {
        return false;
    }

    bool SetHDRMetadata(const FRHIHDRMetadata& Metadata)
    {
        Desc.HDRMetadata = Metadata;
        return true;
    }

    uint32 GetCurrentBackBufferIndex() const
    {
        return BackBufferIndex;
    }

    void AcquireNextBackBuffer()
    {
        BackBufferIndex = (BackBufferIndex + 1u) % kNumBackBuffers;
    }

    bool Present(bool /*bVerticalSync*/)
    {
        // The index only moves in AcquireNextBackBuffer
        return true;
    }

    bool Resize(uint32 InWidth, uint32 InHeight, EFormat NewFormat = EFormat::Unknown, EColorSpace /*NewColorSpace*/ = EColorSpace::Unknown)
    {
        Desc.Width  = uint16(InWidth);
        Desc.Height = uint16(InHeight);

        if (NewFormat != EFormat::Unknown)
        {
            Desc.ColorFormat = NewFormat;
        }

        AllocateBackBuffers();
        BackBufferIndex = 0;
        return true;
    }

private:
    void AllocateBackBuffers()
    {
        if (BackBuffer)
        {
            BackBuffer->UpdateSwapChainTexture(Desc.ColorFormat, Desc.Width, Desc.Height);
            return;
        }

        ETextureUsageFlags BackBufferUsageFlags = ETextureUsageFlags::Presentable;

        if (Desc.IsRenderTarget())
        {
            BackBufferUsageFlags |= ETextureUsageFlags::RenderTarget;
        }

        if (Desc.IsUnorderedAccess())
        {
            BackBufferUsageFlags |= ETextureUsageFlags::UnorderedAccessTexture;
        }

        if (Desc.IsShaderResource())
        {
            BackBufferUsageFlags |= ETextureUsageFlags::ShaderResourceTexture;
        }

        const FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(Desc.ColorFormat,
            Desc.Width, Desc.Height, 1, 1, BackBufferUsageFlags);

        BackBuffer = new FNullTextureRHI(BackBufferDesc);
    }

    TSharedRef<FNullTextureRHI> BackBuffer;
    uint32                      BackBufferIndex;
};

struct FNullQueryRHI : public FRHIQuery
{
    FNullQueryRHI(EQueryType InQueryType)
        : FRHIQuery(InQueryType)
    {
    }
};

class FNullFenceRHI : public FRHIFence
{
public:
    FNullFenceRHI()
        : FRHIFence()
        , DebugName()
    {
    }

    // FRHIFence Interface
    virtual void* GetRHINativeFence() const override final
    {
        return nullptr;
    }

    virtual bool IsSignaled() const override final
    {
        return true;
    }

    virtual bool Wait(uint64 /*TimeoutNs*/) const override final
    {
        return true;
    }

    virtual void SetDebugName(const String& InName) override final
    {
        DebugName = InName;
    }

    virtual void GetDebugName(String& OutDebugName) const override final
    {
        OutDebugName = DebugName;
    }

private:
    String DebugName;
};

class FNullInputLayoutRHI : public FRHIInputLayout
{
public:
    FNullInputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements)
        : FRHIInputLayout()
        , InputElements(InInputElements)
    {
    }

    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }

    virtual const FRHIInputElementDesc* GetInputElementDesc(uint32 Index) const override final
    {
        return (Index < static_cast<uint32>(InputElements.Size())) ? &InputElements[Index] : nullptr;
    }

    virtual uint32 GetNumInputElementDescs() const override final
    {
        return InputElements.Size();
    }

private:
    TArray<FRHIInputElementDesc> InputElements;
};

class FNullDepthStencilStateRHI : public FRHIDepthStencilState
{
public:
    FNullDepthStencilStateRHI(const FRHIDepthStencilStateDesc& InDesc)
        : FRHIDepthStencilState(InDesc)
    {
    }

    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }
};

class FNullRasterizerStateRHI : public FRHIRasterizerState
{
public:
    FNullRasterizerStateRHI(const FRHIRasterizerStateDesc& InDesc)
        : FRHIRasterizerState(InDesc)
    {
    }

    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }
};

struct FNullBlendStateRHI : public FRHIBlendState
{
public:
    FNullBlendStateRHI(const FRHIBlendStateDesc& InDesc)
        : FRHIBlendState(InDesc)
    {
    }

    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }
};

struct FNullGraphicsPipelineStateRHI : public FRHIGraphicsPipelineState
{
    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }

    virtual void SetDebugName(const String& InDebugName) override final
    {
        DebugName = InDebugName;
    }

    virtual void GetDebugName(String& OutDebugName) const override final
    {
        OutDebugName = DebugName;
    }

private:
    String DebugName;
};

struct FNullComputePipelineStateRHI : public FRHIComputePipelineState
{
    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }

    virtual void SetDebugName(const String& InDebugName) override final
    {
        DebugName = InDebugName;
    }

    virtual void GetDebugName(String& OutDebugName) const override final
    {
        OutDebugName = DebugName;
    }

private:
    String DebugName;
};

struct FNullMeshletPipelineStateRHI : public FRHIMeshletPipelineState
{
    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }

    virtual void SetDebugName(const String& InDebugName) override final
    {
        DebugName = InDebugName;
    }

    virtual void GetDebugName(String& OutDebugName) const override final
    {
        OutDebugName = DebugName;
    }

private:
    String DebugName;
};

struct FNullRayTracingPipelineStateRHI : public FRHIRayTracingPipelineState
{
    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }

    virtual void GetExportName(ERayTracingShaderRecordKind, uint32, String& OutExportName) const override final { OutExportName = String(); }
    virtual uint32 GetNumExportNames(ERayTracingShaderRecordKind) const override final { return 0; }

    virtual void SetDebugName(const String& InDebugName) override final
    {
        DebugName = InDebugName;
    }

    virtual void GetDebugName(String& OutDebugName) const override final
    {
        OutDebugName = DebugName;
    }

private:
    String DebugName;
};

class FNullShaderBindingTableRHI : public FRHIShaderBindingTable
{
public:
    explicit FNullShaderBindingTableRHI(const FRHIShaderBindingTableDesc& InDesc)
        : FRHIShaderBindingTable(InDesc)
        , BaseAddress(0)
    {
        const uint64 RawAddress = reinterpret_cast<uint64>(this);
        BaseAddress = (RawAddress + RecordStride - 1) & ~(uint64(RecordStride) - 1);
    }

    virtual void* GetRHINativeResource() const override final
    {
        return nullptr;
    }

    virtual FRHIShaderBindingTableAddressInfo GetAddressInfo() const override final
    {
        FRHIShaderBindingTableAddressInfo AddressInfo = {};
        uint64 CurrentAddress = BaseAddress;

        const uint32 NumRayGen = Math::Max<uint32>(GetDesc().NumRayGenerationShaders, 1u);
        AddressInfo.RayGeneration = { CurrentAddress, RecordStride, RecordStride };
        CurrentAddress += uint64(NumRayGen) * RecordStride;

        if (GetDesc().NumMissShaders > 0)
        {
            AddressInfo.Miss = { CurrentAddress, uint64(GetDesc().NumMissShaders) * RecordStride, RecordStride };
            CurrentAddress += AddressInfo.Miss.SizeInBytes;
        }

        if (GetDesc().NumHitGroupRecords > 0)
        {
            AddressInfo.HitGroup = { CurrentAddress, uint64(GetDesc().NumHitGroupRecords) * RecordStride, RecordStride };
            CurrentAddress += AddressInfo.HitGroup.SizeInBytes;
        }

        if (GetDesc().NumCallableShaders > 0)
        {
            AddressInfo.Callable = { CurrentAddress, uint64(GetDesc().NumCallableShaders) * RecordStride, RecordStride };
        }

        return AddressInfo;
    }

private:
    static constexpr uint32 RecordStride = 64;
    uint64 BaseAddress;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
