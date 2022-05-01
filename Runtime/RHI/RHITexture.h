#pragma once
#include "RHIResourceBase.h"

#include "Core/Templates/EnumUtilities.h"
#include "Core/Containers/SharedRef.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHITexture>        RHITextureRef;

typedef TSharedRef<class CRHITexture2D>      RHITexture2DRef;
typedef TSharedRef<class CRHITexture2DArray> RHITexture2DArrayRef;
typedef TSharedRef<class CRHITextureCube>    RHITextureCubeRef;
typedef TSharedRef<class CRHITexture3D>      RHITexture3DRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ETextureUsageFlags

enum class ETextureUsageFlags
{
    None         = 0,
    AllowRTV     = FLAG(1), // RenderTargetView
    AllowDSV     = FLAG(2), // DepthStencilView
    AllowUAV     = FLAG(3), // UnorderedAccessView
    AllowSRV     = FLAG(4), // ShaderResourceView
    NoDefaultRTV = FLAG(5), // Do not create default RenderTargetView
    NoDefaultDSV = FLAG(6), // Do not create default DepthStencilView
    NoDefaultUAV = FLAG(7), // Do not create default UnorderedAccessView
    NoDefaultSRV = FLAG(8), // Do not create default ShaderResourceView
    
    RWTexture    = AllowUAV | AllowSRV,
    RenderTarget = AllowRTV | AllowSRV,
    ShadowMap    = AllowDSV | AllowSRV,
};

ENUM_CLASS_OPERATORS(ETextureUsageFlags);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture

class CRHITexture : public CRHIResource
{
protected:

    CRHITexture(EFormat InFormat, uint32 InNumMips, ETextureUsageFlags InFlags, const SClearValue& InOptimalClearValue)
        : CRHIResource()
        , Format(InFormat)
        , NumMips(uint8(InNumMips))
        , UsageFlags(InFlags)
        , OptimalClearValue(InOptimalClearValue)
    { }

public:

    /** @return: Returns a pointer to a CRHITexture2D if the interface is implemented */
    virtual class CRHITexture2D* GetTexture2D() { return nullptr; }

    /** @return: Returns a pointer to a CRHITexture2DArray if the interface is implemented */
    virtual class CRHITexture2DArray* GetTexture2DArray() { return nullptr; }
    
    /** @return: Returns a pointer to a CRHITextureCube if the interface is implemented */
    virtual class CRHITextureCube* GetTextureCube() { return nullptr; }

    /** @return: Returns a pointer to a CRHITextureCubeArray if the interface is implemented */
    virtual class CRHITextureCubeArray* GetTextureCubeArray() { return nullptr; }

    /** @return: Returns a pointer to a CRHITexture3D if the interface is implemented */
    virtual class CRHITexture3D* GetTexture3D() { return nullptr; }

    /** @return: Returns the RHI-backend Resource interface */
    virtual void* GetRHIBaseResource() const { return nullptr; }

    /** @return: Returns the RHI-backend BaseTexture interface */
    virtual void* GetRHIBaseTexture() { return nullptr; }

    /** @return: Returns the default SRV of the full resource if texture is created with the flag AllowSRV */
    virtual class CRHIShaderResourceView* GetDefaultShaderResourceView() const { return nullptr; }

    /** @return: Returns a Bindless descriptor-handle to the default ShaderResourceView if the RHI supports it */
    virtual CRHIDescriptorHandle GetDefaultBindlessHandle() const { return CRHIDescriptorHandle(); }

    /** @return: Returns a IntVector3 with Width, Height, and Depth */
    virtual CIntVector3 GetExtent() const { return CIntVector3(1, 1, 1); }

    /** @return: Returns the texture Width */
    virtual uint32 GetWidth() const { return 1; }

    /** @return: Returns the texture Height */
    virtual uint32 GetHeight() const { return 1; }

    /** @return: Returns the texture Depth */
    virtual uint32 GetDepth() const { return 1; }

    /** @return: Returns the texture ArraySize */
    virtual uint32 GetArraySize() const { return 1; }

    /** @return: Returns the number of Samples of the texture */
    virtual uint32 GetNumSamples() const { return 1; }

    /** @brief: Set the debug-name of the Texture */
    virtual void SetName(const String& InName) { }

    /** @return: Returns the name of the Texture */
    virtual String GetName() const { return ""; }

    /** @return: Returns true if the number of samples is more than one */
    bool IsMultiSampled() const { return (GetNumSamples() > 1); }

    /** @return: Returns the Usage-Flags of the texture */
    ETextureUsageFlags GetFlags() const { return UsageFlags; }

    /** @return: Returns the texture Format */
    EFormat GetFormat() const { return Format; }

    /** @return: Returns the number of MipLevels of the texture */
    uint32 GetNumMips() const { return NumMips; }

    /** @return: Returns the clear-value */
    const SClearValue& GetClearValue() const { return OptimalClearValue; }

protected:
    EFormat            Format;
    uint8              NumMips;
    ETextureUsageFlags UsageFlags;
    SClearValue        OptimalClearValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2D

class CRHITexture2D : public CRHITexture
{
protected:

    CRHITexture2D(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips, uint32 InNumSamples, ETextureUsageFlags InFlags, const SClearValue& InClearValue)
        : CRHITexture(InFormat, InNumMips, InFlags, InClearValue)
        , NumSamples(uint8(InNumSamples))
        , Width(uint16(InWidth))
        , Height(uint16(InHeight))
    { }

public:

    /** @return: Returns the default RTV of the full resource if the texture is created with ETextureUsageFlags::AllowRTV */
    virtual class CRHIRenderTargetView* GetRenderTargetView() const { return nullptr; }

    /** @return: Returns the default DSV of the full resource  if texture is created with ETextureUsageFlags::AllowDSV */
    virtual class CRHIDepthStencilView* GetDepthStencilView() const { return nullptr; }

    /** @return: Returns the default UAV of the full resource if texture is created with ETextureUsageFlags::AllowUAV */
    virtual class CRHIUnorderedAccessView* GetUnorderedAccessView() const { return nullptr; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual CRHITexture2D* GetTexture2D() override { return this; }

    virtual CIntVector3 GetExtent() const override { return CIntVector3(Width, Height, 1); }

    virtual uint32 GetWidth() const override final { return Width; }
    
    virtual uint32 GetHeight() const override final { return Height; }

    virtual uint32 GetNumSamples() const override final { return NumSamples; }

protected:
    uint8  NumSamples;
    uint16 Width;
    uint16 Height;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2DArray

class CRHITexture2DArray : public CRHITexture2D
{
protected:

    CRHITexture2DArray(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips, uint32 InNumSamples, uint32 InArraySize, ETextureUsageFlags InFlags, const SClearValue& InClearValue)
        : CRHITexture2D(InFormat, InWidth, InHeight, InNumMips, InNumSamples, InFlags, InClearValue)
        , ArraySize(uint16(InArraySize))
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual CRHITexture2D* GetTexture2D() override final { return nullptr; }
    
    virtual CRHITexture2DArray* GetTexture2DArray() override final { return this; }

    virtual CIntVector3 GetExtent() const override final { return CIntVector3(GetWidth(), GetDepth(), ArraySize); }

    virtual uint32 GetArraySize() const override final { return ArraySize; }

protected:
    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCube

class CRHITextureCube : public CRHITexture
{
protected:

    CRHITextureCube(EFormat InFormat, uint32 InExtent, uint32 InNumMips, ETextureUsageFlags InFlags, const SClearValue& InClearValue)
        : CRHITexture(InFormat, InNumMips, InFlags, InClearValue)
        , Extent(uint16(InExtent))
        , NumSamples(1)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual CRHITextureCube* GetTextureCube() override { return this; }

    virtual CIntVector3 GetExtent() const override { return CIntVector3(Extent, Extent, 1); }

    virtual uint32 GetWidth()  const override final { return Extent; }
    
    virtual uint32 GetHeight() const override final { return Extent; }

    virtual uint32 GetNumSamples() const override final { return NumSamples; }

protected:
    uint8  NumSamples;
    uint16 Extent;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCubeArray

class CRHITextureCubeArray : public CRHITextureCube
{
protected:

    CRHITextureCubeArray(EFormat InFormat, uint32 InSize, uint32 InNumMips, uint32 InArraySize, ETextureUsageFlags InFlags, const SClearValue& InClearValue)
        : CRHITextureCube(InFormat, InSize, InNumMips, InFlags, InClearValue)
        , ArraySize(uint16(InArraySize))
    { }

public:

    /** @return: Returns the number of Cubes in the array */
    uint32 GetNumCubes() const { return ArraySize; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual CRHITextureCube* GetTextureCube() override final { return nullptr; }
    
    virtual CRHITextureCubeArray* GetTextureCubeArray() override final { return this; }

    virtual CIntVector3 GetExtent() const override final { return CIntVector3(GetWidth(), GetHeight(), ArraySize); }

    virtual uint32 GetArraySize() const override final { return ArraySize; }

protected:
    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CRHITexture3D : public CRHITexture
{
protected:

    CRHITexture3D(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InDepth, uint32 InNumMips, ETextureUsageFlags InFlags, const SClearValue& InClearValue)
        : CRHITexture(InFormat, InNumMips, InFlags, InClearValue)
        , Width(uint16(InWidth))
        , Height(uint16(InHeight))
        , Depth(uint16(InDepth))
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual CRHITexture3D* GetTexture3D() override { return this; }

    virtual CIntVector3 GetExtent() const override final { return CIntVector3(Width, Height, Depth); }

    virtual uint32 GetWidth()  const override final { return Width; }
    
    virtual uint32 GetHeight() const override final { return Height; }

    virtual uint32 GetDepth()  const override final { return Depth; }

protected:
    uint16 Width;
    uint16 Height;
    uint16 Depth;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif