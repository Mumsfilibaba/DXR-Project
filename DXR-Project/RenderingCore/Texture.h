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
* ETextureFlags
*/

typedef Uint32 TextureFlags;
enum ETextureFlag : TextureFlags
{
	TEXTURE_FLAG_NONE					= 0,
	TEXTURE_FLAG_RENDER_TARGET			= FLAG(1),
	TEXTURE_FLAG_DEPTH_STENCIL_TARGET	= FLAG(2),
	TEXTURE_FLAG_UNORDERED_ACCESS		= FLAG(3),
	TEXTURE_FLAG_SHADER_RESOURCE		= FLAG(4),
};

/*
* SubresourceIndex
*/

struct SubresourceIndex
{
	inline explicit SubresourceIndex(Int32 InMipSlice, Int32 InArraySlice, Int32 InPlaneSlice, Int32 InMipLevels, Int32 InArraySize)
		: MipSlice(InMipSlice)
		, MipLevels(InMipLevels)
		, ArraySlice(InArraySlice)
		, ArraySize(InArraySize)
		, PlaneSlice(InPlaneSlice)
	{
	}

	inline SubresourceIndex(const SubresourceIndex& Other)
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
	Texture()	= default;
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

	// Resource views
	void SetRenderTargetView(RenderTargetView* InRenderTargetView, const SubresourceIndex& InSubresourceIndex)
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		if (SubresourceIndex < RenderTargetViews.Size())
		{
			RenderTargetViews.Resize(SubresourceIndex + 1);
		}

		RenderTargetViews[SubresourceIndex] = InRenderTargetView;
	}

	void SetDepthStencilView(DepthStencilView* InDepthStencilView, const SubresourceIndex& InSubresourceIndex)
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		if (SubresourceIndex < DepthStencilViews.Size())
		{
			DepthStencilViews.Resize(SubresourceIndex + 1);
		}

		DepthStencilViews[SubresourceIndex] = InDepthStencilView;
	}

	void SetShaderResourceView(ShaderResourceView* InShaderResourceView, const SubresourceIndex& InSubresourceIndex)
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		if (SubresourceIndex < ShaderResourceViews.Size())
		{
			ShaderResourceViews.Resize(SubresourceIndex + 1);
		}

		ShaderResourceViews[SubresourceIndex] = InShaderResourceView;
	}

	void SetUnorderedAccessView(UnorderedAccessView* InUnorderedAccessView, const SubresourceIndex& InSubresourceIndex)
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		if (SubresourceIndex < UnorderedAccessViews.Size())
		{
			UnorderedAccessViews.Resize(SubresourceIndex + 1);
		}

		UnorderedAccessViews[SubresourceIndex] = InUnorderedAccessView;
	}

	FORCEINLINE RenderTargetView* GetRenderTargetView(const SubresourceIndex& InSubresourceIndex) const
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		return RenderTargetViews[SubresourceIndex];
	}

	FORCEINLINE DepthStencilView* GetDepthStencilView(const SubresourceIndex& InSubresourceIndex) const
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		return DepthStencilViews[SubresourceIndex];
	}

	FORCEINLINE ShaderResourceView* GetShaderResourceView(const SubresourceIndex& InSubresourceIndex) const
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		return ShaderResourceViews[SubresourceIndex];
	}

	FORCEINLINE UnorderedAccessView* GetUnorderedAccessView(const SubresourceIndex& InSubresourceIndex) const
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		return UnorderedAccessViews[SubresourceIndex];
	}

protected:
	TArray<RenderTargetView*>		RenderTargetViews;
	TArray<DepthStencilView*>		DepthStencilViews;
	TArray<ShaderResourceView*>		ShaderResourceViews;
	TArray<UnorderedAccessView*>	UnorderedAccessViews;
};

/*
* Texture1DInitializer
*/

struct Texture1DInitializer
{
	inline Texture1DInitializer(EFormat InFormat, TextureFlags InFlags, Uint32 InWidth, Uint32 InMipLevels, const ClearValue& InOptimizedClearValue)
		: Format(InFormat)
		, Flags(InFlags)
		, Width(InWidth)
		, MipLevels(InMipLevels)
		, OptimizedClearValue(InOptimizedClearValue)
	{
	}

	FORCEINLINE bool HasRenderTargetAccess() const
	{
		return (Flags & TEXTURE_FLAG_RENDER_TARGET);
	}

	FORCEINLINE bool HasDepthStencilAccess() const
	{
		return (Flags & TEXTURE_FLAG_DEPTH_STENCIL_TARGET);
	}

	FORCEINLINE bool HasUnorderedAccess() const
	{
		return (Flags & TEXTURE_FLAG_UNORDERED_ACCESS);
	}

	FORCEINLINE bool HasShaderResourceAccess() const
	{
		return (Flags & TEXTURE_FLAG_SHADER_RESOURCE);
	}

	EFormat			Format;
	TextureFlags	Flags;
	Uint32			Width;
	Uint32			MipLevels;
	ClearValue		OptimizedClearValue;
};

/*
* Texture1D
*/

class Texture1D : public Texture
{
public:
	Texture1D()		= default;
	~Texture1D()	= default;

	virtual bool Initialize(const Texture1DInitializer& InInitializer) = 0;

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
		return Initializer.Width;
	}

	virtual Uint32 GetMipLevels() const
	{
		return Initializer.MipLevels;
	}

protected:
	Texture1DInitializer Initializer;
};

/*
* Texture2DInitializer
*/

struct Texture2DInitializer
{
	inline Texture2DInitializer(EFormat InFormat, TextureFlags InFlags, Uint32 InWidth, Uint32 InHeight, Uint32 InMipLevels, Uint32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Format(InFormat)
		, Flags(InFlags)
		, Width(InWidth)
		, Height(InHeight)
		, MipLevels(InMipLevels)
		, SampleCount(InSampleCount)
		, OptimizedClearValue(InOptimizedClearValue)
	{
	}

	FORCEINLINE bool HasRenderTargetAccess() const
	{
		return (Flags & TEXTURE_FLAG_RENDER_TARGET);
	}

	FORCEINLINE bool HasDepthStencilAccess() const
	{
		return (Flags & TEXTURE_FLAG_DEPTH_STENCIL_TARGET);
	}

	FORCEINLINE bool HasUnorderedAccess() const
	{
		return (Flags & TEXTURE_FLAG_UNORDERED_ACCESS);
	}

	FORCEINLINE bool HasShaderResourceAccess() const
	{
		return (Flags & TEXTURE_FLAG_SHADER_RESOURCE);
	}

	EFormat			Format;
	TextureFlags	Flags;
	Uint32			Width;
	Uint32			Height;
	Uint32			MipLevels;
	Uint32			SampleCount;
	ClearValue		OptimizedClearValue;
};

/*
* Texture2D
*/

class Texture2D : public Texture
{
public:
	Texture2D() = default;
	~Texture2D() = default;

	virtual bool Initialize(const Texture2DInitializer& InInitializer) = 0;

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
		return Initializer.Width;
	}

	virtual Uint32 GetHeight() const override
	{
		return Initializer.Height;
	}

	virtual Uint32 GetMipLevels() const
	{
		return Initializer.MipLevels;
	}

	virtual Uint32 GetSampleCount() const
	{
		return Initializer.SampleCount;
	}

	virtual bool IsMultiSampled() const override
	{
		return (Initializer.SampleCount > 1);
	}

protected:
	Texture2DInitializer Initializer;
};

/*
* Texture2DArrayInitializer
*/

struct Texture2DArrayInitializer
{
	inline Texture2DArrayInitializer(EFormat InFormat, TextureFlags InFlags, Uint32 InWidth, Uint32 InHeight, Uint32 InMipLevels, Uint32 InArrayCount, Uint32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Format(InFormat)
		, Flags(InFlags)
		, Width(InWidth)
		, Height(InHeight)
		, MipLevels(InMipLevels)
		, ArrayCount(InArrayCount)
		, SampleCount(InSampleCount)
		, OptimizedClearValue(InOptimizedClearValue)
	{
	}

	FORCEINLINE bool HasRenderTargetAccess() const
	{
		return (Flags & TEXTURE_FLAG_RENDER_TARGET);
	}

	FORCEINLINE bool HasDepthStencilAccess() const
	{
		return (Flags & TEXTURE_FLAG_DEPTH_STENCIL_TARGET);
	}

	FORCEINLINE bool HasUnorderedAccess() const
	{
		return (Flags & TEXTURE_FLAG_UNORDERED_ACCESS);
	}

	FORCEINLINE bool HasShaderResourceAccess() const
	{
		return (Flags & TEXTURE_FLAG_SHADER_RESOURCE);
	}

	EFormat			Format;
	TextureFlags	Flags;
	Uint32			Width;
	Uint32			Height;
	Uint32			MipLevels;
	Uint32			ArrayCount;
	Uint32			SampleCount;
	ClearValue		OptimizedClearValue;
};

/*
* Texture2DArray
*/

class Texture2DArray : public Texture
{
public:
	Texture2DArray()		= default;
	~Texture2DArray()	= default;

	virtual bool Initialize(const Texture2DArrayInitializer& InInitializer) = 0;

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
		return Initializer.Width;
	}

	virtual Uint32 GetHeight() const override
	{
		return Initializer.Height;
	}

	virtual Uint32 GetMipLevels() const
	{
		return Initializer.MipLevels;
	}

	virtual Uint32 GetMipLevels() const
	{
		return Initializer.ArrayCount;
	}

	virtual Uint32 GetSampleCount() const
	{
		return Initializer.SampleCount;
	}

	virtual bool IsMultiSampled() const override
	{
		return (Initializer.SampleCount > 1);
	}

protected:
	Texture2DArrayInitializer Initializer;
};

/*
* TextureCubeInitializer
*/

struct TextureCubeInitializer
{
	inline TextureCubeInitializer(EFormat InFormat, TextureFlags InFlags, Uint32 InSize, Uint32 InMipLevels, Uint32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Format(InFormat)
		, Flags(InFlags)
		, Size(InSize)
		, MipLevels(InMipLevels)
		, SampleCount(InSampleCount)
		, OptimizedClearValue(InOptimizedClearValue)
	{
	}

	FORCEINLINE bool HasRenderTargetAccess() const
	{
		return (Flags & TEXTURE_FLAG_RENDER_TARGET);
	}

	FORCEINLINE bool HasDepthStencilAccess() const
	{
		return (Flags & TEXTURE_FLAG_DEPTH_STENCIL_TARGET);
	}

	FORCEINLINE bool HasUnorderedAccess() const
	{
		return (Flags & TEXTURE_FLAG_UNORDERED_ACCESS);
	}

	FORCEINLINE bool HasShaderResourceAccess() const
	{
		return (Flags & TEXTURE_FLAG_SHADER_RESOURCE);
	}

	EFormat			Format;
	TextureFlags	Flags;
	Uint32			Size;
	Uint32			MipLevels;
	Uint32			SampleCount;
	ClearValue		OptimizedClearValue;
};

/*
* TextureCube
*/

class TextureCube : public Texture
{
public:
	TextureCube() = default;
	~TextureCube() = default;

	virtual bool Initialize(const TextureCubeInitializer& InInitializer) = 0;

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
		return Initializer.Size;
	}

	virtual Uint32 GetHeight() const override
	{
		return Initializer.Size;
	}

	virtual Uint32 GetMipLevels() const
	{
		return Initializer.MipLevels;
	}

	virtual Uint32 GetSampleCount() const
	{
		return Initializer.SampleCount;
	}

	virtual bool IsMultiSampled() const override
	{
		return (Initializer.SampleCount > 1);
	}

protected:
	TextureCubeInitializer Initializer;
};

/*
* Texture3DInitializer
*/

struct Texture3DInitializer
{
	inline Texture3DInitializer(EFormat InFormat, TextureFlags InFlags, Uint32 InWidth, Uint32 InHeight, Uint32 InDepth, Uint32 InMipLevels, const ClearValue& InOptimizedClearValue)
		: Format(InFormat)
		, Flags(InFlags)
		, Width(InWidth)
		, Height(InHeight)
		, Depth(InDepth)
		, MipLevels(InMipLevels)
		, OptimizedClearValue(InOptimizedClearValue)
	{
	}

	FORCEINLINE bool HasRenderTargetAccess() const
	{
		return (Flags & TEXTURE_FLAG_RENDER_TARGET);
	}

	FORCEINLINE bool HasDepthStencilAccess() const
	{
		return (Flags & TEXTURE_FLAG_DEPTH_STENCIL_TARGET);
	}

	FORCEINLINE bool HasUnorderedAccess() const
	{
		return (Flags & TEXTURE_FLAG_UNORDERED_ACCESS);
	}

	FORCEINLINE bool HasShaderResourceAccess() const
	{
		return (Flags & TEXTURE_FLAG_SHADER_RESOURCE);
	}

	EFormat			Format;
	TextureFlags	Flags;
	Uint32			Width;
	Uint32			Height;
	Uint32			Depth;
	Uint32			MipLevels;
	ClearValue		OptimizedClearValue;
};

/*
* Texture3D
*/

class Texture3D : public Texture
{
public:
	Texture3D()		= default;
	~Texture3D()	= default;

	virtual bool Initialize(const Texture3DInitializer& InInitializer) = 0;

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
		return Initializer.Width;
	}

	virtual Uint32 GetHeight() const
	{
		return Initializer.Height;
	}

	virtual Uint32 GetDepth() const
	{
		return Initializer.Depth;
	}

	virtual Uint32 GetMipLevels() const
	{
		return Initializer.MipLevels;
	}

protected:
	Texture3DInitializer Initializer;
};