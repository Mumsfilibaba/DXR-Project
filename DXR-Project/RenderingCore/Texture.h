#pragma once
#include "Resource.h"

class DepthStencilView;
class RenderTargetView;
class ShaderResourceView;
class UnorderedAccessView;
class Texture1D;
class Texture2D;
class Texture3D;
class TextureCube;
class Texture2DArray;

/*
* ETextureUsage
*/

enum ETextureUsage
{
	TextureUsage_None	= 0,
	TextureUsage_RTV	= FLAG(1), // RenderTargetView
	TextureUsage_DSV	= FLAG(2), // DepthStencilView
	TextureUsage_UAV	= FLAG(3), // UnorderedAccessView
	TextureUsage_SRV	= FLAG(4), // ShaderResourceView
};

/*
* TextureRange
*/

struct TextureRange
{
	inline explicit TextureRange(Int32 InMipSlice, Int32 InArraySlice, Int32 InPlaneSlice, Int32 InMipLevels, Int32 InArraySize)
		: MipSlice(InMipSlice)
		, MipLevels(InMipLevels)
		, ArraySlice(InArraySlice)
		, ArraySize(InArraySize)
		, PlaneSlice(InPlaneSlice)
	{
	}

	inline TextureRange(const TextureRange& Other)
		: MipSlice(Other.MipSlice)
		, MipLevels(Other.MipLevels)
		, ArraySlice(Other.ArraySlice)
		, ArraySize(Other.ArraySize)
		, PlaneSlice(Other.PlaneSlice)
	{
	}

	FORCEINLINE Int32 GetSubresourceIndex() const
	{
		return MipSlice + (ArraySlice * MipLevels) + (PlaneSlice * MipLevels * ArraySize);
	}

	const Int32 MipSlice;
	const Int32 MipLevels;
	const Int32 ArraySlice;
	const Int32 ArraySize;
	const Int32 PlaneSlice;
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

	~Texture()	= default;

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

	virtual Uint32 GetMipLevels() const
	{
		return MipLevels;
	}

protected:
	Uint32 Width;
	Uint32 MipLevels;
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
	virtual Texture2D* AsTexture2D()
	{
		return nullptr;
	}

	virtual const Texture2D* AsTexture2D() const
	{
		return nullptr;
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

	virtual Uint32 GetMipLevels() const
	{
		return MipLevels;
	}

	virtual Uint32 GetSampleCount() const
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

	~Texture2DArray()	= default;

	// Casting functions
	virtual Texture2DArray* AsTexture2DArray()
	{
		return this;
	}

	virtual const Texture2DArray* AsTexture2DArray() const
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

	virtual Uint32 GetMipLevels() const
	{
		return MipLevels;
	}

	virtual Uint32 GetMipLevels() const
	{
		return ArrayCount;
	}

	virtual Uint32 GetSampleCount() const
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

	// Size
	virtual Uint32 GetWidth() const override
	{
		return Size;
	}

	virtual Uint32 GetHeight() const override
	{
		return Size;
	}

	virtual Uint32 GetMipLevels() const
	{
		return MipLevels;
	}

	virtual Uint32 GetSampleCount() const
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
	virtual Uint32 GetWidth() const
	{
		return Width;
	}

	virtual Uint32 GetHeight() const
	{
		return Height;
	}

	virtual Uint32 GetDepth() const
	{
		return Depth;
	}

	virtual Uint32 GetMipLevels() const
	{
		return MipLevels;
	}

protected:
	Uint32 Width;
	Uint32 Height;
	Uint32 Depth;
	Uint32 MipLevels;
};