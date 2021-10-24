#pragma once
#include "CoreRHI/RHIResources.h"

#include "NullRHIViews.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

class CNullRHITexture2D : public CRHITexture2D
{
public:
    CNullRHITexture2D( EFormat InFormat, uint32 InSizeX, uint32 InSizeY, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags, const SClearValue& InOptimalClearValue )
        : CRHITexture2D( InFormat, InSizeX, InSizeY, InNumMips, InNumSamples, InFlags, InOptimalClearValue )
        , RenderTargetView( DBG_NEW CNullRHIRenderTargetView() )
        , DepthStencilView( DBG_NEW CNullRHIDepthStencilView() )
        , UnorderedAccessView( DBG_NEW CNullRHIUnorderedAccessView() )
    {
    }

    virtual CRHIRenderTargetView* GetRenderTargetView() const override
    {
        return RenderTargetView.Get();
    }

    virtual CRHIDepthStencilView* GetDepthStencilView() const override
    {
        return DepthStencilView.Get();
    }

    virtual CRHIUnorderedAccessView* GetUnorderedAccessView() const override
    {
        return UnorderedAccessView.Get();
    }

private:
    TSharedRef<CNullRHIRenderTargetView> RenderTargetView;
    TSharedRef<CNullRHIDepthStencilView> DepthStencilView;
    TSharedRef<CNullRHIUnorderedAccessView> UnorderedAccessView;
};

class CNullRHITexture2DArray : public CRHITexture2DArray
{
public:
    CNullRHITexture2DArray( EFormat InFormat, uint32 InSizeX, uint32 InSizeY, uint32 InSizeZ, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags, const SClearValue& InOptimalClearValue )
        : CRHITexture2DArray( InFormat, InSizeX, InSizeY, InNumMips, InNumSamples, InSizeZ, InFlags, InOptimalClearValue )
    {
    }
};

class CNullRHITextureCube : public CRHITextureCube
{
public:
    CNullRHITextureCube( EFormat InFormat, uint32 InSize, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimalClearValue )
        : CRHITextureCube( InFormat, InSize, InNumMips, InFlags, InOptimalClearValue )
    {
    }
};

class CNullRHITextureCubeArray : public CRHITextureCubeArray
{
public:
    CNullRHITextureCubeArray( EFormat InFormat, uint32 InSizeX, uint32 InSizeZ, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimalClearValue )
        : CRHITextureCubeArray( InFormat, InSizeX, InNumMips, InSizeZ, InFlags, InOptimalClearValue )
    {
    }
};

class CNullRHITexture3D : public CRHITexture3D
{
public:
    CNullRHITexture3D( EFormat InFormat, uint32 InSizeX, uint32 InSizeY, uint32 InSizeZ, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimalClearValue )
        : CRHITexture3D( InFormat, InSizeX, InSizeY, InSizeZ, InNumMips, InFlags, InOptimalClearValue )
    {
    }
};

template<typename TBaseTexture>
class TNullRHITexture : public TBaseTexture
{
public:
    template<typename... ArgTypes>
    TNullRHITexture( ArgTypes&&... Args )
        : TBaseTexture( Forward<ArgTypes>( Args )... )
        , ShaderResourceView( DBG_NEW CNullRHIShaderResourceView() )
    {
    }

    virtual void SetName( const CString& InName ) override
    {
        CRHIResource::SetName( InName );
    }

    virtual class CRHIShaderResourceView* GetShaderResourceView() const
    {
        return ShaderResourceView.Get();
    }

    virtual bool IsValid() const override
    {
        return true;
    }

private:
    TSharedRef<CNullRHIShaderResourceView> ShaderResourceView;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
