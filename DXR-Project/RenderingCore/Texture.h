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
};

/*
* Texture
*/

class Texture : public Resource
{
public:
	inline Texture(EFormat InFormat, Uint32 InUsage, const ClearValue& InOptimizedClearValue)
		: Format(InFormat)
		, Usage(InUsage)
		, OptimizedClearValue(InOptimizedClearValue)
	{
	}

	~Texture() = default;

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
	virtual Uint32 GetWidth() const
	{
		return 0;
	}

	virtual Uint32 GetHeight() const
	{
		return 1;
	}

	virtual Uint32 GetDepth() const
	{
		return 1;
	}

	virtual Uint32 GetArrayCount() const
	{
		return 1;
	}

	virtual Uint32 GetMipLevels() const
	{
		return 1;
	}

	virtual Uint32 GetSampleCount() const
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

	FORCEINLINE Uint32 GetUsage() const
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
	Uint32	Usage;
	ClearValue OptimizedClearValue;
};

/*
* Texture1D
*/

class Texture1D : public Texture
{
public:
	inline Texture1D(EFormat InFormat, Uint32 InUsage, Uint32 InWidth, Uint32 InMipLevels, const ClearValue& InOptimizedClearValue)
		: Texture(InFormat, InUsage, InOptimizedClearValue)
		, Width(InWidth)
		, MipLevels(InMipLevels)
	{
	}

	~Texture1D() = default;

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
	virtual Uint32 GetWidth() const override
	{
		return Width;
	}

	virtual Uint32 GetMipLevels() const override
	{
		return MipLevels;
	}

protected:
	Uint32 Width;
	Uint32 MipLevels;
};

/*
* Texture1DArray
*/

class Texture1DArray : public Texture
{
public:
	inline Texture1DArray(EFormat InFormat, Uint32 InUsage, Uint32 InWidth, Uint32 InMipLevels, Uint32 InArrayCount, const ClearValue& InOptimizedClearValue)
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
	virtual Uint32 GetWidth() const override
	{
		return Width;
	}

	virtual Uint32 GetMipLevels() const override
	{
		return MipLevels;
	}

	virtual Uint32 GetArrayCount() const override
	{
		return ArrayCount;
	}

protected:
	Uint32 Width;
	Uint32 MipLevels;
	Uint32 ArrayCount;
};

/*
* Texture2D
*/

class Texture2D : public Texture
{
public:
	inline Texture2D(EFormat InFormat, Uint32 InUsage, Uint32 InWidth, Uint32 InHeight, Uint32 InMipLevels, Uint32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Texture(InFormat, InUsage, InOptimizedClearValue)
		, Width(InWidth)
		, Height(InHeight)
		, MipLevels(InMipLevels)
		, SampleCount(InSampleCount)
	{
	}

	~Texture2D() = default;

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
	virtual Uint32 GetWidth() const override
	{
		return Width;
	}

	virtual Uint32 GetHeight() const override
	{
		return Height;
	}

	virtual Uint32 GetMipLevels() const override
	{
		return MipLevels;
	}

	virtual Uint32 GetSampleCount() const override
	{
		return SampleCount;
	}

	virtual bool IsMultiSampled() const override
	{
		return (SampleCount > 1);
	}

protected:
	Uint32 Width;
	Uint32 Height;
	Uint32 MipLevels;
	Uint32 SampleCount;
};

/*
* Texture2DArray
*/

class Texture2DArray : public Texture
{
public:
	inline Texture2DArray(EFormat InFormat, Uint32 InUsage, Uint32 InWidth, Uint32 InHeight, Uint32 InMipLevels, Uint32 InArrayCount, Uint32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Texture(InFormat, InUsage, InOptimizedClearValue)
		, Width(InWidth)
		, Height(InHeight)
		, MipLevels(InMipLevels)
		, ArrayCount(InArrayCount)
		, SampleCount(InSampleCount)
	{
	}

	~Texture2DArray() = default;

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
	virtual Uint32 GetWidth() const override
	{
		return Width;
	}

	virtual Uint32 GetHeight() const override
	{
		return Height;
	}

	virtual Uint32 GetMipLevels() const override
	{
		return MipLevels;
	}

	virtual Uint32 GetArrayCount() const override
	{
		return ArrayCount;
	}

	virtual Uint32 GetSampleCount() const override
	{
		return SampleCount;
	}

	virtual bool IsMultiSampled() const override
	{
		return (SampleCount > 1);
	}

protected:
	Uint32 Width;
	Uint32 Height;
	Uint32 MipLevels;
	Uint32 ArrayCount;
	Uint32 SampleCount;
};

/*
* TextureCube
*/

class TextureCube : public Texture
{
public:
	inline TextureCube(EFormat InFormat, Uint32 InUsage, Uint32 InSize, Uint32 InMipLevels, Uint32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Texture(InFormat, InUsage, InOptimizedClearValue)
		, Size(InSize)
		, MipLevels(InMipLevels)
		, SampleCount(InSampleCount)
	{
	}

	~TextureCube() = default;

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
	virtual Uint32 GetWidth() const override
	{
		return Size;
	}

	virtual Uint32 GetHeight() const override
	{
		return Size;
	}

	virtual Uint32 GetMipLevels() const override
	{
		return MipLevels;
	}

	virtual Uint32 GetArrayCount() const override
	{
		constexpr Uint32 TEXTURE_CUBE_FACE_COUNT = 6;
		return TEXTURE_CUBE_FACE_COUNT;
	}

	virtual Uint32 GetSampleCount() const override
	{
		return SampleCount;
	}

	virtual bool IsMultiSampled() const override
	{
		return (SampleCount > 1);
	}

protected:
	Uint32 Size;
	Uint32 MipLevels;
	Uint32 SampleCount;
};

/*
* TextureCubeArray
*/

class TextureCubeArray : public Texture
{
public:
	inline TextureCubeArray(EFormat InFormat, Uint32 InUsage, Uint32 InSize, Uint32 InMipLevels, Uint32 InArrayCount, Uint32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Texture(InFormat, InUsage, InOptimizedClearValue)
		, Size(InSize)
		, MipLevels(InMipLevels)
		, ArrayCount(InArrayCount)
		, SampleCount(InSampleCount)
	{
	}

	~TextureCubeArray() = default;

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
	virtual Uint32 GetWidth() const override
	{
		return Size;
	}

	virtual Uint32 GetHeight() const override
	{
		return Size;
	}

	virtual Uint32 GetMipLevels() const override
	{
		return MipLevels;
	}

	virtual Uint32 GetArrayCount() const override
	{
		constexpr Uint32 TEXTURE_CUBE_FACE_COUNT = 6;
		return ArrayCount * TEXTURE_CUBE_FACE_COUNT;
	}

	virtual Uint32 GetSampleCount() const override
	{
		return SampleCount;
	}

	virtual bool IsMultiSampled() const override
	{
		return (SampleCount > 1);
	}

protected:
	Uint32 Size;
	Uint32 MipLevels;
	Uint32 ArrayCount;
	Uint32 SampleCount;
};


/*
* Texture3D
*/

class Texture3D : public Texture
{
public:
	inline Texture3D(EFormat InFormat, Uint32 InUsage, Uint32 InWidth, Uint32 InHeight, Uint32 InDepth, Uint32 InMipLevels, const ClearValue& InOptimizedClearValue)
		: Texture(InFormat, InUsage, InOptimizedClearValue)
		, Width(InWidth)
		, Height(InHeight)
		, Depth(InDepth)
		, MipLevels(InMipLevels)
	{
	}
	
	~Texture3D() = default;

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
	virtual Uint32 GetWidth() const override
	{
		return Width;
	}

	virtual Uint32 GetHeight() const override
	{
		return Height;
	}

	virtual Uint32 GetDepth() const override
	{
		return Depth;
	}

	virtual Uint32 GetMipLevels() const override
	{
		return MipLevels;
	}

protected:
	Uint32 Width;
	Uint32 Height;
	Uint32 Depth;
	Uint32 MipLevels;
};