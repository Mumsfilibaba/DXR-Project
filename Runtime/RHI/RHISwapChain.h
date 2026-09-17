#pragma once
#include "RHI/RHIResource.h"

class FRHIRenderTargetView;
class FRHIUnorderedAccessView;
class FRHIShaderResourceView;

enum class ESwapChainUsageFlags : uint8
{
    None            = 0,
    RenderTarget    = FLAG(1),
    UnorderedAccess = FLAG(2),
    ShaderResource  = FLAG(3),
};

ENUM_CLASS_OPERATORS(ESwapChainUsageFlags);

NODISCARD constexpr const CHAR* ToString(ESwapChainUsageFlags Usage)
{
    switch (Usage)
    {
        case ESwapChainUsageFlags::None:            return "None";
        case ESwapChainUsageFlags::RenderTarget:    return "RenderTarget";
        case ESwapChainUsageFlags::UnorderedAccess: return "UnorderedAccess";
        case ESwapChainUsageFlags::ShaderResource:  return "ShaderResource";

        default: return "Unknown";
    }
}

enum class ESwapChainFlags : uint8
{
    None        = 0,
    Transparent = FLAG(1),
};

ENUM_CLASS_OPERATORS(ESwapChainFlags);

NODISCARD constexpr const CHAR* ToString(ESwapChainFlags Flags)
{
    switch (Flags)
    {
        case ESwapChainFlags::None:        return "None";
        case ESwapChainFlags::Transparent: return "Transparent";

        default: return "Unknown";
    }
}

struct FRHISwapChainDesc
{
    constexpr FRHISwapChainDesc() noexcept = default;

    constexpr FRHISwapChainDesc(void* InWindowHandle, EFormat InColorFormat, uint16 InWidth, uint16 InHeight) noexcept
        : WindowHandle(InWindowHandle)
        , ColorFormat(InColorFormat)
        , Width(InWidth)
        , Height(InHeight)
    {
    }

    constexpr bool operator==(const FRHISwapChainDesc& Other) const noexcept = default;

    NODISCARD constexpr bool IsRenderTarget()    const { return IsEnumFlagSet(Usage, ESwapChainUsageFlags::RenderTarget); }
    NODISCARD constexpr bool IsUnorderedAccess() const { return IsEnumFlagSet(Usage, ESwapChainUsageFlags::UnorderedAccess); }
    NODISCARD constexpr bool IsShaderResource()  const { return IsEnumFlagSet(Usage, ESwapChainUsageFlags::ShaderResource); }
    NODISCARD constexpr bool IsTransparent()     const { return IsEnumFlagSet(Flags, ESwapChainFlags::Transparent); }

    void*                WindowHandle = nullptr;
    EFormat              ColorFormat  = EFormat::Unknown;
    uint16               Width        = 0;
    uint16               Height       = 0;
    bool                 bFramePacing = false;
    ESwapChainUsageFlags Usage        = ESwapChainUsageFlags::RenderTarget;
    ESwapChainFlags      Flags        = ESwapChainFlags::None;
    EColorSpace          ColorSpace   = EColorSpace::Unknown;
    FRHIHDRMetadata      HDRMetadata  = {};
};

class FRHISwapChain : public FRHIResource
{
protected:
    explicit FRHISwapChain(const FRHISwapChainDesc& InSwapChainDesc)
        : FRHIResource(ERHIResourceType::SwapChain)
        , Desc(InSwapChainDesc)
    {
    }

    virtual ~FRHISwapChain() = default;

public:

    /** @return D3D12: IDXGISwapChain3*. Vulkan: VkSwapchainKHR. Metal: CAMetalLayer*. Null: nullptr. */
    virtual void* GetRHINativeHandle() const = 0;

    /** @return D3D12: ID3D12Resource*. Vulkan: VkImage. Metal: id<MTLTexture>. Null: nullptr. */
    virtual void* GetRHINativeResourceFromIndex(uint32 Index) const = 0;

    /** @return D3D12: D3D12_CPU_DESCRIPTOR_HANDLE. Vulkan: VkImageView. Metal: nullptr. Null: nullptr. */
    virtual void* GetRHINativeRenderTargetViewFromIndex(uint32 Index) const = 0;

    /** @return D3D12: D3D12_CPU_DESCRIPTOR_HANDLE. Vulkan: VkImageView. Metal: nullptr. Null: nullptr. */
    virtual void* GetRHINativeUnorderedAccessViewFromIndex(uint32 Index) const = 0;

    /** @return D3D12: D3D12_CPU_DESCRIPTOR_HANDLE. Vulkan: VkImageView. Metal: nullptr. Null: nullptr. */
    virtual void* GetRHINativeShaderResourceViewFromIndex(uint32 Index) const = 0;

    /** @return The one texture that stands for every back-buffer image, re-pointed at the acquired image by AcquireNextBackBuffer */
    virtual FRHITexture* GetBackBuffer() const = 0;

    /** @return The back buffer's render-target view, or nullptr without ESwapChainUsageFlags::RenderTarget */
    virtual FRHIRenderTargetView* GetRenderTargetView() const = 0;

    /** @return The back buffer's unordered-access view, or nullptr without ESwapChainUsageFlags::UnorderedAccess */
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const = 0;

    /** @return The back buffer's shader-resource view, or nullptr without ESwapChainUsageFlags::ShaderResource */
    virtual FRHIShaderResourceView* GetShaderResourceView() const = 0;

    /** @return The number of back-buffer images the swap chain holds, bounding the index the getters above take */
    virtual uint32 GetNumResources() const = 0;

    virtual bool IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const = 0;
    virtual bool QueryDisplayHDRInfo(FRHIDisplayHDRInfo& OutInfo) const = 0;

    NODISCARD const FRHISwapChainDesc& GetDesc() const
    {
        return Desc;
    }

    NODISCARD const FRHIHDRMetadata& GetHDRMetadata() const
    {
        return Desc.HDRMetadata;
    }

protected:
    FRHISwapChainDesc Desc;
};
