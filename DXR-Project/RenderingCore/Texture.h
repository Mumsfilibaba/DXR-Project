#pragma once
#include "Resource.h"

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

class DepthStencilView;
class RenderTargetView;
class Texture2D;
class TextureCube;

/*
* Texture
*/

class Texture : public Resource
{
public:
	Texture() = default;
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

	virtual Texture2D* AsTexture2D()
	{
		return nullptr;
	}

	virtual const Texture2D* AsTexture2D() const
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

	// Size
	virtual Uint32 GetWidth() const
	{
		return 0;
	}

	virtual Uint32 GetHeight() const
	{
		return 0;
	}

	virtual Uint32 GetDepth() const
	{
		return 1;
	}

	// Other
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

	RenderTargetView* GetRenderTargetView(const SubresourceIndex& InSubresourceIndex) const
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		return RenderTargetViews[SubresourceIndex];
	}

	DepthStencilView* GetDepthStencilView(const SubresourceIndex& InSubresourceIndex) const
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		return DepthStencilViews[SubresourceIndex];
	}

protected:
	TArray<RenderTargetView*> RenderTargetViews;
	TArray<DepthStencilView*> DepthStencilViews;
};

/*
* TextureInitializer
*/

struct TextureInitializer
{
	inline TextureInitializer(EFormat InFormat, TextureFlags InFlags, const ClearValue& InOptimizedClearValue)
		: Format(InFormat)
		, Flags(InFlags)
		, OptimizedClearValue(InOptimizedClearValue)
	{
	}

	inline bool HasRenderTargetAccess() const
	{
		return (Flags & TEXTURE_FLAG_RENDER_TARGET);
	}

	inline bool HasDepthStencilAccess() const
	{
		return (Flags & TEXTURE_FLAG_DEPTH_STENCIL_TARGET);
	}

	inline bool HasUnorderedAccess() const
	{
		return (Flags & TEXTURE_FLAG_UNORDERED_ACCESS);
	}

	inline bool HasShaderResourceAccess() const
	{
		return (Flags & TEXTURE_FLAG_SHADER_RESOURCE);
	}
	
	EFormat			Format;
	TextureFlags	Flags;
	ClearValue		OptimizedClearValue;
};

/*
* Texture2DInitializer
*/

struct Texture2DInitializer : public TextureInitializer
{
	inline Texture2DInitializer()
		: TextureInitializer(EFormat::FORMAT_UNKNOWN, 0, ClearValue())
		, Width(0)
		, Height(0)
		, MipLevelCount(0)
		, SampleCount(0)
	{
	}

	inline Texture2DInitializer(EFormat InFormat, TextureFlags InFlags, Uint32 InWidth, Uint32 InHeight)
		: TextureInitializer(InFormat, InFlags, ClearValue())
		, Width(InWidth)
		, Height(InHeight)
		, MipLevelCount(1)
		, SampleCount(1)
	{
	}

	inline Texture2DInitializer(EFormat InFormat, TextureFlags InFlags, Uint32 InWidth, Uint32 InHeight, Uint32 InMipLevelCount, Uint32 InSampleCount, const ClearValue& InClearValue)
		: TextureInitializer(InFormat, InFlags, InClearValue)
		, Width(InWidth)
		, Height(InHeight)
		, MipLevelCount(InMipLevelCount)
		, SampleCount(InSampleCount)
	{
	}

	Uint32 Width;
	Uint32 Height;
	Uint32 MipLevelCount;
	Uint32 SampleCount;
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

	virtual Uint32 GetWidth() const override
	{
		return Initializer.Width;
	}

	virtual Uint32 GetHeight() const override
	{
		return Initializer.Height;
	}

	virtual bool IsMultiSampled() const override
	{
		return (Initializer.SampleCount > 1);
	}

protected:
	Texture2DInitializer Initializer;
};

/*
* TextureCubeInitializer
*/

struct TextureCubeInitializer : public TextureInitializer
{
	inline TextureCubeInitializer()
		: TextureInitializer(EFormat::FORMAT_UNKNOWN, 0, ClearValue())
		, Size(0)
		, MipLevelCount(0)
		, SampleCount(0)
	{
	}

	inline TextureCubeInitializer(EFormat InFormat, TextureFlags InFlags, Uint32 InSize)
		: TextureInitializer(InFormat, InFlags, ClearValue())
		, MipLevelCount(1)
		, SampleCount(1)
	{
	}

	inline TextureCubeInitializer(EFormat InFormat, TextureFlags InFlags, Uint32 InSize, Uint32 InMipLevelCount, Uint32 InSampleCount, const ClearValue& InClearValue)
		: TextureInitializer(InFormat, InFlags, ClearValue())
		, Size(InSize)
		, MipLevelCount(InMipLevelCount)
		, SampleCount(InSampleCount)
	{
	}

	Uint32 Size;
	Uint32 MipLevelCount;
	Uint32 SampleCount;
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

	virtual bool IsMultiSampled() const override
	{
		return (Initializer.SampleCount > 1);
	}

protected:
	TextureCubeInitializer Initializer;
};