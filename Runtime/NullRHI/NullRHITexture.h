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

class CNullRHITexture2D : public FRHITexture2D
{
public:

    CNullRHITexture2D(const FRHITexture2DInitializer& Initializer)
        : FRHITexture2D(Initializer)
        , UnorderedAccessView(dbg_new CNullRHIUnorderedAccessView(this))
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override { return UnorderedAccessView.Get(); }

private:
    TSharedRef<CNullRHIUnorderedAccessView> UnorderedAccessView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHITexture2DArray

class CNullRHITexture2DArray : public FRHITexture2DArray
{
public:

    CNullRHITexture2DArray(const FRHITexture2DArrayInitializer& Initializer)
        : FRHITexture2DArray(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHITextureCube

class CNullRHITextureCube : public FRHITextureCube
{
public:

    CNullRHITextureCube(const FRHITextureCubeInitializer& Initializer)
        : FRHITextureCube(Initializer)
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

class CNullRHITexture3D : public FRHITexture3D
{
public:
    
    CNullRHITexture3D(const FRHITexture3DInitializer& Initializer)
        : FRHITexture3D(Initializer)
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
        , ShaderResourceView(dbg_new CNullRHIShaderResourceView(this))
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return nullptr; }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(this); }

    virtual class FRHIShaderResourceView* GetShaderResourceView() const override final { return ShaderResourceView.Get(); }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }

private:
    TSharedRef<CNullRHIShaderResourceView> ShaderResourceView;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
