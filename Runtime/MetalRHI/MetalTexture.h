#pragma once
#include "MetalViews.h"

#include "RHI/RHIResources.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTexture2D

class CMetalTexture2D : public CRHITexture2D
{
public:

    CMetalTexture2D(const CRHITexture2DInitializer& Initializer)
        : CRHITexture2D(Initializer)
        , UnorderedAccessView(dbg_new CMetalUnorderedAccessView(this))
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual CRHIUnorderedAccessView* GetUnorderedAccessView() const override { return UnorderedAccessView.Get(); }

private:
    TSharedRef<CMetalUnorderedAccessView> UnorderedAccessView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTexture2DArray

class CMetalTexture2DArray : public CRHITexture2DArray
{
public:

    CMetalTexture2DArray(const CRHITexture2DArrayInitializer& Initializer)
        : CRHITexture2DArray(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTextureCube

class CMetalTextureCube : public CRHITextureCube
{
public:

    CMetalTextureCube(const CRHITextureCubeInitializer& Initializer)
        : CRHITextureCube(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTextureCubeArray

class CMetalTextureCubeArray : public CRHITextureCubeArray
{
public:
    
    CMetalTextureCubeArray(const CRHITextureCubeArrayInitializer& Initializer)
        : CRHITextureCubeArray(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTexture3D

class CMetalTexture3D : public CRHITexture3D
{
public:
    
    CMetalTexture3D(const CRHITexture3DInitializer& Initializer)
        : CRHITexture3D(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TMetalTexture

template<typename BaseTextureType>
class TMetalTexture : public BaseTextureType
{
public:

    template<typename BaseTextureInitializer>
    TMetalTexture(const BaseTextureInitializer& Initializer)
        : BaseTextureType(Initializer)
        , ShaderResourceView(dbg_new CMetalShaderResourceView(this))
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return nullptr; }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(this); }

    virtual class CRHIShaderResourceView* GetShaderResourceView() const override final { return ShaderResourceView.Get(); }

    virtual CRHIDescriptorHandle GetBindlessSRVHandle() const override final { return CRHIDescriptorHandle(); }

private:
    TSharedRef<CMetalShaderResourceView> ShaderResourceView;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
