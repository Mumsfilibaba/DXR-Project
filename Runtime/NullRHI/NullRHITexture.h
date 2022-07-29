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

template<typename T>
class TNullRHITexture;

typedef TNullRHITexture<class FNullRHITexture2DBase>         FNullRHITexture2D;
typedef TNullRHITexture<struct FNullRHITexture2DArrayBase>   FNullRHITexture2DArray;
typedef TNullRHITexture<struct FNullRHITextureCubeBase>      FNullRHITextureCube;
typedef TNullRHITexture<struct FNullRHITextureCubeArrayBase> FNullRHITextureCubeArray;
typedef TNullRHITexture<struct FNullRHITexture3DBase>        FNullRHITexture3D;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHITexture2DBase

class FNullRHITexture2DBase 
    : public FRHITexture2D
{
public:
    explicit FNullRHITexture2DBase(const FRHITexture2DInitializer& Initializer)
        : FRHITexture2D(Initializer)
        , UnorderedAccessView(dbg_new FNullRHIUnorderedAccessView(this))
    { }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override { return UnorderedAccessView.Get(); }

private:
    TSharedRef<FNullRHIUnorderedAccessView> UnorderedAccessView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHITexture2DArrayBase

struct FNullRHITexture2DArrayBase 
    : public FRHITexture2DArray
{
    explicit FNullRHITexture2DArrayBase(const FRHITexture2DArrayInitializer& Initializer)
        : FRHITexture2DArray(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHITextureCubeBase

struct FNullRHITextureCubeBase 
    : public FRHITextureCube
{
    explicit FNullRHITextureCubeBase(const FRHITextureCubeInitializer& Initializer)
        : FRHITextureCube(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHITextureCubeArrayBase

struct FNullRHITextureCubeArrayBase 
    : public FRHITextureCubeArray
{
    explicit FNullRHITextureCubeArrayBase(const FRHITextureCubeArrayInitializer& Initializer)
        : FRHITextureCubeArray(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHITexture3DBase

struct FNullRHITexture3DBase 
    : public FRHITexture3D
{
    explicit FNullRHITexture3DBase(const FRHITexture3DInitializer& Initializer)
        : FRHITexture3D(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TNullRHITexture

template<typename BaseTextureType>
class TNullRHITexture final 
    : public BaseTextureType
{
public:
    template<typename BaseTextureInitializer>
    explicit TNullRHITexture(const BaseTextureInitializer& Initializer)
        : BaseTextureType(Initializer)
        , ShaderResourceView(dbg_new FNullRHIShaderResourceView(this))
    { }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return nullptr; }
    virtual void* GetRHIBaseTexture()  override final       { return reinterpret_cast<void*>(this); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return ShaderResourceView.Get(); }
    virtual FRHIDescriptorHandle    GetBindlessSRVHandle()  const override final { return FRHIDescriptorHandle(); }

private:
    TSharedRef<FNullRHIShaderResourceView> ShaderResourceView;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
