#pragma once
#include "VulkanResourceView.h"

#include "RHI/RHIResources.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTexture2D

class CVulkanTexture2D : public CRHITexture2D
{
public:
    CVulkanTexture2D(EFormat InFormat, uint32 InSizeX, uint32 InSizeY, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags, const SClearValue& InOptimalClearValue)
        : CRHITexture2D(InFormat, InSizeX, InSizeY, InNumMips, InNumSamples, InFlags, InOptimalClearValue)
        , RenderTargetView(dbg_new CVulkanRenderTargetView())
        , DepthStencilView(dbg_new CVulkanDepthStencilView())
        , UnorderedAccessView(dbg_new CVulkanUnorderedAccessView())
    {
    }

    virtual CRHIRenderTargetView*    GetRenderTargetView()    const override { return RenderTargetView.Get(); }
    virtual CRHIDepthStencilView*    GetDepthStencilView()    const override { return DepthStencilView.Get(); }
    virtual CRHIUnorderedAccessView* GetUnorderedAccessView() const override { return UnorderedAccessView.Get(); }

private:
    TSharedRef<CVulkanRenderTargetView>    RenderTargetView;
    TSharedRef<CVulkanDepthStencilView>    DepthStencilView;
    TSharedRef<CVulkanUnorderedAccessView> UnorderedAccessView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTexture2DArray

class CVulkanTexture2DArray : public CRHITexture2DArray
{
public:
    CVulkanTexture2DArray(EFormat InFormat, uint32 InSizeX, uint32 InSizeY, uint32 InSizeZ, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags, const SClearValue& InOptimalClearValue)
        : CRHITexture2DArray(InFormat, InSizeX, InSizeY, InNumMips, InNumSamples, InSizeZ, InFlags, InOptimalClearValue)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTextureCube

class CVulkanTextureCube : public CRHITextureCube
{
public:
    CVulkanTextureCube(EFormat InFormat, uint32 InSize, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimalClearValue)
        : CRHITextureCube(InFormat, InSize, InNumMips, InFlags, InOptimalClearValue)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTextureCubeArray

class CVulkanTextureCubeArray : public CRHITextureCubeArray
{
public:
    CVulkanTextureCubeArray(EFormat InFormat, uint32 InSizeX, uint32 InSizeZ, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimalClearValue)
        : CRHITextureCubeArray(InFormat, InSizeX, InNumMips, InSizeZ, InFlags, InOptimalClearValue)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTexture3D

class CVulkanTexture3D : public CRHITexture3D
{
public:
    CVulkanTexture3D(EFormat InFormat, uint32 InSizeX, uint32 InSizeY, uint32 InSizeZ, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimalClearValue)
        : CRHITexture3D(InFormat, InSizeX, InSizeY, InSizeZ, InNumMips, InFlags, InOptimalClearValue)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TVulkanTexture

template<typename BaseTextureType>
class TVulkanTexture : public BaseTextureType
{
public:
    template<typename... ArgTypes>
    TVulkanTexture(ArgTypes&&... Args)
        : BaseTextureType(Forward<ArgTypes>(Args)...)
        , ShaderResourceView(dbg_new CVulkanShaderResourceView())
    {
    }

    virtual void SetName(const String& InName) override final
    {
        CRHIResource::SetName(InName);
    }

    virtual class CRHIShaderResourceView* GetShaderResourceView() const override final
    {
        return ShaderResourceView.Get();
    }

    virtual bool IsValid() const override final
    {
        return true;
    }

private:
    TSharedRef<CVulkanShaderResourceView> ShaderResourceView;
};
