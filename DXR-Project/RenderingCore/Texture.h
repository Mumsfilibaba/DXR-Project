#pragma once
#include "Resource.h"

class Texture1D;
class Texture1DArray;
class Texture2D;
class Texture2DArray;
class Texture3D;
class TextureCube;
class TextureCubeArray;

/*
* ETextureUsage
*/

enum ETextureUsage
{
	TextureUsage_None		= 0,
	TextureUsage_RTV		= FLAG(1), // RenderTargetView
	TextureUsage_DSV		= FLAG(2), // DepthStencilView
	TextureUsage_UAV		= FLAG(3), // UnorderedAccessView
	TextureUsage_SRV		= FLAG(4), // ShaderResourceView
	TextureUsage_Default 	= FLAG(5), // Default memory
	TextureUsage_Dynamic 	= FLAG(6), // CPU memory

	// Defines for often used combinations
	TextureUsage_RWTexture		= TextureUsage_UAV | TextureUsage_SRV, 
	TextureUsage_RenderTarget	= TextureUsage_RTV | TextureUsage_SRV,
	TextureUsage_ShadowMap		= TextureUsage_DSV | TextureUsage_SRV,
};

/*
* Texture
*/

class Texture : public Resource
{
public:
	inline Texture(EFormat InFormat, UInt32 InUsage, const ClearValue& InOptimizedClearValue)
		: Format(InFormat)
		, Usage(InUsage)
		, OptimizedClearValue(InOptimizedClearValue)
	{
	}

	// Casting functions
	virtual Texture* AsTexture() override
	{
		return this;
	}

	virtual const Texture* AsTexture() const override
	{
		return this;
	}

	virtual Texture1D* AsTexture1D()
	{
		return nullptr;
	}

	virtual const Texture1D* AsTexture1D() const
	{
		return nullptr;
	}

	virtual Texture1DArray* AsTexture1DArray()
	{
		return nullptr;
	}

	virtual const Texture1DArray* AsTexture1DArray() const
	{
		return nullptr;
	}

	virtual Texture2D* AsTexture2D()
	{
		return nullptr;
	}

	virtual const Texture2D* AsTexture2D() const
	{
		return nullptr;
	}

	virtual Texture2DArray* AsTexture2DArray()
	{
		return nullptr;
	}

	virtual const Texture2DArray* AsTexture2DArray() const
	{
		return nullptr;
	}

	virtual Texture3D* AsTexture3D()
	{
		return nullptr;
	}

	virtual const Texture3D* AsTexture3D() const
	{
		return nullptr;
	}

	virtual TextureCube* AsTextureCube()
	{
		return nullptr;
	}

	virtual const TextureCube* AsTextureCube() const
	{
		return nullptr;
	}

	virtual TextureCubeArray* AsTextureCubeArray()
	{
		return nullptr;
	}

	virtual const TextureCubeArray* AsTextureCubeArray() const
	{
		return nullptr;
	}

	// Info
	virtual UInt32 GetWidth() const
	{
		return 0;
	}

	virtual UInt32 GetHeight() const
	{
		return 1;
	}

	virtual UInt32 GetDepth() const
	{
		return 1;
	}

	virtual UInt32 GetArrayCount() const
	{
		return 1;
	}

	virtual UInt32 GetMipLevels() const
	{
		return 1;
	}

	virtual UInt32 GetSampleCount() const
	{
		return 1;
	}

	virtual bool IsMultiSampled() const
	{
		return false;
	}

	FORCEINLINE EFormat GetFormat() const
	{
		return Format;
	}

	FORCEINLINE UInt32 GetUsage() const
	{
		return Usage;
	}

	FORCEINLINE bool HasShaderResourceUsage() const
	{
		return Usage & TextureUsage_SRV;
	}

	FORCEINLINE bool HasUnorderedAccessUsage() const
	{
		return Usage & TextureUsage_UAV;
	}

	FORCEINLINE bool HasRenderTargetUsage() const
	{
		return Usage & TextureUsage_RTV;
	}

	FORCEINLINE bool HasDepthStencilUsage() const
	{
		return Usage & TextureUsage_DSV;
	}

protected:
	EFormat Format;
	UInt32	Usage;
	ClearValue OptimizedClearValue;
};

/*
* Texture1D
*/

class Texture1D : public Texture
{
public:
	inline Texture1D(EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InMipLevels, const ClearValue& InOptimizedClearValue)
		: Texture(InFormat, InUsage, InOptimizedClearValue)
		, Width(InWidth)
		, MipLevels(InMipLevels)
	{
	}

	// Casting functions
	virtual Texture1D* AsTexture1D() override
	{
		return this;
	}

	virtual const Texture1D* AsTexture1D() const override
	{
		return this;
	}

	// Info
	virtual UInt32 GetWidth() const override
	{
		return Width;
	}

	virtual UInt32 GetMipLevels() const override
	{
		return MipLevels;
	}

protected:
	UInt32 Width;
	UInt32 MipLevels;
};

/*
* Texture1DArray
*/

class Texture1DArray : public Texture
{
public:
	inline Texture1DArray(EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InMipLevels, UInt32 InArrayCount, const ClearValue& InOptimizedClearValue)
		: Texture(InFormat, InUsage, InOptimizedClearValue)
		, Width(InWidth)
		, MipLevels(InMipLevels)
		, ArrayCount(InArrayCount)
	{
	}

	~Texture1DArray() = default;

	// Casting functions
	virtual Texture1DArray* AsTexture1DArray() override
	{
		return this;
	}

	virtual const Texture1DArray* AsTexture1DArray() const override
	{
		return this;
	}

	// Info
	virtual UInt32 GetWidth() const override
	{
		return Width;
	}

	virtual UInt32 GetMipLevels() const override
	{
		return MipLevels;
	}

	virtual UInt32 GetArrayCount() const override
	{
		return ArrayCount;
	}

protected:
	UInt32 Width;
	UInt32 MipLevels;
	UInt32 ArrayCount;
};

/*
* Texture2D
*/

class Texture2D : public Texture
{
public:
	inline Texture2D(EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InHeight, UInt32 InMipLevels, UInt32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Texture(InFormat, InUsage, InOptimizedClearValue)
		, Width(InWidth)
		, Height(InHeight)
		, MipLevels(InMipLevels)
		, SampleCount(InSampleCount)
	{
	}

	// Casting functions
	virtual Texture2D* AsTexture2D() override
	{
		return this;
	}

	virtual const Texture2D* AsTexture2D() const override
	{
		return this;
	}

	// Info
	virtual UInt32 GetWidth() const override
	{
		return Width;
	}

	virtual UInt32 GetHeight() const override
	{
		return Height;
	}

	virtual UInt32 GetMipLevels() const override
	{
		return MipLevels;
	}

	virtual UInt32 GetSampleCount() const override
	{
		return SampleCount;
	}

	virtual bool IsMultiSampled() const override
	{
		return (SampleCount > 1);
	}

protected:
	UInt32 Width;
	UInt32 Height;
	UInt32 MipLevels;
	UInt32 SampleCount;
};

/*
* Texture2DArray
*/

class Texture2DArray : public Texture
{
public:
	inline Texture2DArray(EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InHeight, UInt32 InMipLevels, UInt32 InArrayCount, UInt32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Texture(InFormat, InUsage, InOptimizedClearValue)
		, Width(InWidth)
		, Height(InHeight)
		, MipLevels(InMipLevels)
		, ArrayCount(InArrayCount)
		, SampleCount(InSampleCount)
	{
	}

	// Casting functions
	virtual Texture2DArray* AsTexture2DArray() override
	{
		return this;
	}

	virtual const Texture2DArray* AsTexture2DArray() const override
	{
		return this;
	}

	// Info
	virtual UInt32 GetWidth() const override
	{
		return Width;
	}

	virtual UInt32 GetHeight() const override
	{
		return Height;
	}

	virtual UInt32 GetMipLevels() const override
	{
		return MipLevels;
	}

	virtual UInt32 GetArrayCount() const override
	{
		return ArrayCount;
	}

	virtual UInt32 GetSampleCount() const override
	{
		return SampleCount;
	}

	virtual bool IsMultiSampled() const override
	{
		return (SampleCount > 1);
	}

protected:
	UInt32 Width;
	UInt32 Height;
	UInt32 MipLevels;
	UInt32 ArrayCount;
	UInt32 SampleCount;
};

/*
* TextureCube
*/

class TextureCube : public Texture
{
public:
	inline TextureCube(EFormat InFormat, UInt32 InUsage, UInt32 InSize, UInt32 InMipLevels, UInt32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Texture(InFormat, InUsage, InOptimizedClearValue)
		, Size(InSize)
		, MipLevels(InMipLevels)
		, SampleCount(InSampleCount)
	{
	}

	// Casting functions
	virtual TextureCube* AsTextureCube() override
	{
		return this;
	}

	virtual const TextureCube* AsTextureCube() const override
	{
		return this;
	}

	// Info
	virtual UInt32 GetWidth() const override
	{
		return Size;
	}

	virtual UInt32 GetHeight() const override
	{
		return Size;
	}

	virtual UInt32 GetMipLevels() const override
	{
		return MipLevels;
	}

	virtual UInt32 GetArrayCount() const override
	{
		constexpr UInt32 TEXTURE_CUBE_FACE_COUNT = 6;
		return TEXTURE_CUBE_FACE_COUNT;
	}

	virtual UInt32 GetSampleCount() const override
	{
		return SampleCount;
	}

	virtual bool IsMultiSampled() const override
	{
		return (SampleCount > 1);
	}

protected:
	UInt32 Size;
	UInt32 MipLevels;
	UInt32 SampleCount;
};

/*
* TextureCubeArray
*/

class TextureCubeArray : public Texture
{
public:
	inline TextureCubeArray(EFormat InFormat, UInt32 InUsage, UInt32 InSize, UInt32 InMipLevels, UInt32 InArrayCount, UInt32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Texture(InFormat, InUsage, InOptimizedClearValue)
		, Size(InSize)
		, MipLevels(InMipLevels)
		, ArrayCount(InArrayCount)
		, SampleCount(InSampleCount)
	{
	}

	// Casting functions
	virtual TextureCubeArray* AsTextureCubeArray() override
	{
		return this;
	}

	virtual const TextureCubeArray* AsTextureCubeArray() const override
	{
		return this;
	}

	// Info
	virtual UInt32 GetWidth() const override
	{
		return Size;
	}

	virtual UInt32 GetHeight() const override
	{
		return Size;
	}

	virtual UInt32 GetMipLevels() const override
	{
		return MipLevels;
	}

	virtual UInt32 GetArrayCount() const override
	{
		constexpr UInt32 TEXTURE_CUBE_FACE_COUNT = 6;
		return ArrayCount * TEXTURE_CUBE_FACE_COUNT;
	}

	virtual UInt32 GetSampleCount() const override
	{
		return SampleCount;
	}

	virtual bool IsMultiSampled() const override
	{
		return (SampleCount > 1);
	}

protected:
	UInt32 Size;
	UInt32 MipLevels;
	UInt32 ArrayCount;
	UInt32 SampleCount;
};


/*
* Texture3D
*/

class Texture3D : public Texture
{
public:
	inline Texture3D(EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InHeight, UInt32 InDepth, UInt32 InMipLevels, const ClearValue& InOptimizedClearValue)
		: Texture(InFormat, InUsage, InOptimizedClearValue)
		, Width(InWidth)
		, Height(InHeight)
		, Depth(InDepth)
		, MipLevels(InMipLevels)
	{
	}

	// Casting functions
	virtual Texture3D* AsTexture3D() override
	{
		return this;
	}

	virtual const Texture3D* AsTexture3D() const override
	{
		return this;
	}

	// Info
	virtual UInt32 GetWidth() const override
	{
		return Width;
	}

	virtual UInt32 GetHeight() const override
	{
		return Height;
	}

	virtual UInt32 GetDepth() const override
	{
		return Depth;
	}

	virtual UInt32 GetMipLevels() const override
	{
		return MipLevels;
	}

protected:
	UInt32 Width;
	UInt32 Height;
	UInt32 Depth;
	UInt32 MipLevels;
};