#pragma once
#include "RHI/RHIResources.h"

#include "NullRHIViews.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHITexture2D

class CNullRHITexture2D : public CRHITexture2D
{
public:

    CNullRHITexture2D(const CRHITexture2DInitializer& Initializer)
        : CRHITexture2D(Initializer)
        , RenderTargetView(dbg_new CNullRHIRenderTargetView())
        , DepthStencilView(dbg_new CNullRHIDepthStencilView())
        , UnorderedAccessView(dbg_new CNullRHIUnorderedAccessView())
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual CRHIRenderTargetView* GetRenderTargetView() const override { return RenderTargetView.Get(); }

    virtual CRHIDepthStencilView* GetDepthStencilView() const override { return DepthStencilView.Get(); }

    virtual CRHIUnorderedAccessView* GetUnorderedAccessView() const override { return UnorderedAccessView.Get(); }

private:
    TSharedRef<CNullRHIRenderTargetView>    RenderTargetView;
    TSharedRef<CNullRHIDepthStencilView>    DepthStencilView;
    TSharedRef<CNullRHIUnorderedAccessView> UnorderedAccessView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHITexture2DArray

class CNullRHITexture2DArray : public CRHITexture2DArray
{
public:

    CNullRHITexture2DArray(const CRHITexture2DArrayInitializer& Initializer)
        : CRHITexture2DArray(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHITextureCube

class CNullRHITextureCube : public CRHITextureCube
{
public:

    CNullRHITextureCube(const CRHITextureCubeInitializer& Initializer)
        : CRHITextureCube(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHITextureCubeArray

class CNullRHITextureCubeArray : public CRHITextureCubeArray
{
public:
    
    CNullRHITextureCubeArray(const CRHITextureCubeArrayInitializer& Initializer)
        : CRHITextureCubeArray(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHITexture3D

class CNullRHITexture3D : public CRHITexture3D
{
public:
    
    CNullRHITexture3D(const CRHITexture3DInitializer& Initializer)
        : CRHITexture3D(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TNullRHITexture

template<typename BaseTextureType>
class TNullRHITexture : public BaseTextureType
{
public:

    template<typename BaseTextureInitializer>
    TNullRHITexture(const BaseTextureInitializer& Initializer)
        : BaseTextureType(Initializer)
        , ShaderResourceView(dbg_new CNullRHIShaderResourceView())
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual void* GetRHIBaseResource() const { return nullptr; }

    virtual void* GetRHIBaseTexture() { return reinterpret_cast<void*>(this); }

    virtual class CRHIShaderResourceView* GetDefaultShaderResourceView() const override final { return ShaderResourceView.Get(); }

    virtual CRHIDescriptorHandle GetDefaultBindlessSRVHandle() const { return CRHIDescriptorHandle(); }

private:
    TSharedRef<CNullRHIShaderResourceView> ShaderResourceView;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
