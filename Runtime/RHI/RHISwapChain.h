#pragma once
#include "RHI/RHIResource.h"

class FRHIRenderTargetView;
class FRHIUnorderedAccessView;

enum class ESwapChainUsageFlags : uint8
{
    None            = 0,
    RenderTarget    = FLAG(1),
    UnorderedAccess = FLAG(2),
};

ENUM_CLASS_OPERATORS(ESwapChainUsageFlags);

NODISCARD constexpr const CHAR* ToString(ESwapChainUsageFlags Usage)
{
    switch (Usage)
    {
        case ESwapChainUsageFlags::None:            return "None";
        case ESwapChainUsageFlags::RenderTarget:    return "RenderTarget";
        case ESwapChainUsageFlags::UnorderedAccess: return "UnorderedAccess";

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

    void*                WindowHandle = nullptr;
    EFormat              ColorFormat  = EFormat::Unknown;
    uint16               Width        = 0;
    uint16               Height       = 0;
    bool                 bFramePacing = false;
    ESwapChainUsageFlags Usage        = ESwapChainUsageFlags::RenderTarget;
    EColorSpace          ColorSpace   = EColorSpace::Unknown;
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
    virtual void* GetRHINativeHandle()                                             const = 0;
    virtual void* GetRHINativeBackBufferResourceFromIndex(uint32 Index)            const = 0;
    virtual void* GetRHINativeBackBufferRenderTargetViewFromIndex(uint32 Index)    const = 0;
    virtual void* GetRHINativeBackBufferUnorderedAccessViewFromIndex(uint32 Index) const = 0;

    virtual FRHITexture* GetBackBuffer()                              const = 0;
    virtual FRHITexture* GetBackBufferResourceFromIndex(uint32 Index) const = 0;
    virtual uint32       GetNumBackBufferResources()                  const = 0;

    virtual FRHIRenderTargetView*    GetBackBufferRenderTargetView()    const = 0;
    virtual FRHIUnorderedAccessView* GetBackBufferUnorderedAccessView() const = 0;
    
    virtual bool IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const = 0;
    
    NODISCARD EColorSpace GetColorSpace() const
    {
        return Desc.ColorSpace;
    }
    
    NODISCARD EFormat GetColorFormat() const
    {
        return Desc.ColorFormat;
    }
    
    NODISCARD uint32 GetWidth() const
    {
        return Desc.Width;
    }
    
    NODISCARD uint32 GetHeight() const
    {
        return Desc.Height;
    }
    
    NODISCARD ESwapChainUsageFlags GetUsage() const
    {
        return Desc.Usage;
    }

    NODISCARD const FRHISwapChainDesc& GetDesc() const
    {
        return Desc;
    }

protected:
    FRHISwapChainDesc Desc;
};
