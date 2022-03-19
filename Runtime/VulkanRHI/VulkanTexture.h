#pragma once
#include "VulkanResourceView.h"
#include "VulkanDeviceObject.h"

#include "RHI/RHIResources.h"

#include "Core/Containers/SharedRef.h"

class CVulkanViewport;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<CVulkanViewport>         CVulkanViewportRef;
typedef TSharedRef<class CVulkanTexture2D>  CVulkanTexture2DRef;
typedef TSharedRef<class CVulkanBackBuffer> CVulkanBackBufferRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTexture

class CVulkanTexture : public CVulkanDeviceObject
{
public:
    CVulkanTexture(CVulkanDevice* InDevice);
    virtual ~CVulkanTexture();

    FORCEINLINE VkImage GetVkImage() const
    {
        return Image;
    }

protected:

    bool CreateImage(VkImageType ImageType, VkImageCreateFlags Flags, VkImageUsageFlags Usage, VkFormat Format, VkExtent2D Extent, uint32 DepthOrArraySize, uint32 NumMipLevels, uint32 NumSamples);

    VkImage        Image;
    bool           bIsImageOwner;

    VkDeviceMemory DeviceMemory;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTexture2D

class CVulkanTexture2D : public CRHITexture2D, public CVulkanTexture
{
public:

    /** Create a new Texture2D */
    static CVulkanTexture2DRef CreateTexture2D(CVulkanDevice* InDevice, ERHIFormat InFormat, uint32 InSizeX, uint32 InSizeY, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags, const SClearValue& InOptimalClearValue);

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture2D Interface

    virtual CRHIRenderTargetView*    GetRenderTargetView()    const override final { return RenderTargetView.Get();    }
    virtual CRHIDepthStencilView*    GetDepthStencilView()    const override final { return DepthStencilView.Get();    }
    virtual CRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }
    virtual CRHIShaderResourceView*  GetShaderResourceView()  const override final { return ShaderResourceView.Get();  }
    
    virtual void SetName(const String& InName) override final
    {
        CRHIResource::SetName(InName);
    }

    virtual bool IsValid() const override final
    {
        return true;
    }

protected:
    CVulkanTexture2D(CVulkanDevice* InDevice, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags, const SClearValue& InOptimalClearValue);
    virtual ~CVulkanTexture2D() = default;

    bool Initialize();

    CVulkanRenderTargetViewRef    RenderTargetView;
    CVulkanDepthStencilViewRef    DepthStencilView;
    CVulkanUnorderedAccessViewRef UnorderedAccessView;
    CVulkanShaderResourceViewRef  ShaderResourceView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanBackBuffer

class CVulkanBackBuffer : public CVulkanTexture2D
{
public:

    /** Create a new BackBuffer interface for a certain viewport */
    static CVulkanBackBufferRef CreateBackBuffer(CVulkanDevice* InDevice, CVulkanViewport* InViewport, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumSamples);

    void AquireNextImage();
    
    FORCEINLINE CVulkanViewport* GetViewport() const
    {
        return Viewport.Get();
    }

private:
    CVulkanBackBuffer(CVulkanDevice* InDevice, CVulkanViewport* InViewport, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumSamples);
    ~CVulkanBackBuffer() = default;

    CVulkanViewportRef Viewport;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTexture2DArray

class CVulkanTexture2DArray : public CRHITexture2DArray, public CVulkanTexture
{
public:
    CVulkanTexture2DArray(CVulkanDevice* InDevice, ERHIFormat InFormat, uint32 InSizeX, uint32 InSizeY, uint32 InSizeZ, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags, const SClearValue& InOptimalClearValue)
        : CRHITexture2DArray(InFormat, InSizeX, InSizeY, InNumMips, InNumSamples, InSizeZ, InFlags, InOptimalClearValue)
        , CVulkanTexture(InDevice)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTextureCube

class CVulkanTextureCube : public CRHITextureCube, public CVulkanTexture
{
public:
    CVulkanTextureCube(CVulkanDevice* InDevice, ERHIFormat InFormat, uint32 InSize, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimalClearValue)
        : CRHITextureCube(InFormat, InSize, InNumMips, InFlags, InOptimalClearValue)
        , CVulkanTexture(InDevice)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTextureCubeArray

class CVulkanTextureCubeArray : public CRHITextureCubeArray, public CVulkanTexture
{
public:
    CVulkanTextureCubeArray(CVulkanDevice* InDevice, ERHIFormat InFormat, uint32 InSizeX, uint32 InSizeZ, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimalClearValue)
        : CRHITextureCubeArray(InFormat, InSizeX, InNumMips, InSizeZ, InFlags, InOptimalClearValue)
        , CVulkanTexture(InDevice)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTexture3D

class CVulkanTexture3D : public CRHITexture3D, public CVulkanTexture
{
public:
    CVulkanTexture3D(CVulkanDevice* InDevice, ERHIFormat InFormat, uint32 InSizeX, uint32 InSizeY, uint32 InSizeZ, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimalClearValue)
        : CRHITexture3D(InFormat, InSizeX, InSizeY, InSizeZ, InNumMips, InFlags, InOptimalClearValue)
        , CVulkanTexture(InDevice)
    { }
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
    { }

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// VulkanTextureCast

inline CVulkanTexture* CastTexture(CRHITexture* Texture)
{
    if (Texture)
    {
        if (CRHITexture2D* Texture2D = Texture->AsTexture2D())
        {
            return reinterpret_cast<CVulkanTexture2D*>(Texture2D);
        }
        else if (CRHITexture2DArray* Texture2DArray = Texture->AsTexture2DArray())
        {
            return reinterpret_cast<CVulkanTexture2DArray*>(Texture2DArray);
        }
        else if (CRHITextureCube* TextureCube = Texture->AsTextureCube())
        {
            return reinterpret_cast<CVulkanTextureCube*>(TextureCube);
        }
        else if (CRHITextureCubeArray* TextureCubeArray = Texture->AsTextureCubeArray())
        {
            return reinterpret_cast<CVulkanTextureCubeArray*>(TextureCubeArray);
        }
        else if (CRHITexture3D* Texture3D = Texture->AsTexture3D())
        {
            return reinterpret_cast<CVulkanTexture3D*>(Texture3D);
        }
    }
    
    return nullptr;
}
