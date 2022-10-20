#pragma once
#include "RHI/RHIResources.h"

#include "NullRHIViews.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

class FNullRHITexture
    : public FRHITexture
{
public:
    explicit FNullRHITexture(const FRHITextureDesc& InDesc)
        : FRHITexture(InDesc)
        , ShaderResourceView(dbg_new FNullRHIShaderResourceView(this))
        , UnorderedAccessView(dbg_new FNullRHIUnorderedAccessView(this))
    { }

    virtual void* GetRHIBaseTexture()        override final { return reinterpret_cast<void*>(this); }
    virtual void* GetRHIBaseResource() const override final { return nullptr; }

    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final { return nullptr; }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return nullptr; }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final { return FRHIDescriptorHandle(); }

private:
    TSharedRef<FNullRHIShaderResourceView>  ShaderResourceView;
    TSharedRef<FNullRHIUnorderedAccessView> UnorderedAccessView;
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
