#pragma once
#include "Texture.h"
#include "ResourceViews.h"

/*
* SampledTexture
*/

template<typename TTextureType>
struct SampledTexture
{
	SampledTexture() = default;
	
	SampledTexture(const TSharedRef<TTextureType>& InTexture, const TSharedRef<ShaderResourceView>& InView)
		: Texture(InTexture)
		, View(InView)
	{
	}

	FORCEINLINE void SetName(const std::string& Name)
	{
		Texture->SetName(Name);
		View->SetName(Name + "ShaderResourceView");
	}

	FORCEINLINE Bool operator==(const SampledTexture& Other) const
	{
		return Texture == Other.Texture && View == Other.View;
	}

	FORCEINLINE Bool operator!=(const SampledTexture& Other) const
	{
		return !(*this == Other);
	}

	FORCEINLINE operator Bool() const
	{
		return Texture != nullptr && View != nullptr;
	}

	TSharedRef<TTextureType> Texture;
	TSharedRef<ShaderResourceView> View;
};

/*
* SampledTexture typedefs
*/

using SampledTexture1D			= SampledTexture<Texture1D>;
using SampledTexture1DArray		= SampledTexture<Texture1DArray>;
using SampledTexture2D			= SampledTexture<Texture2D>;
using SampledTexture2DArray		= SampledTexture<Texture2DArray>;
using SampledTexture3D			= SampledTexture<Texture3D>;
using SampledTextureCube		= SampledTexture<TextureCube>;
using SampledTextureCubeArray	= SampledTexture<TextureCubeArray>;