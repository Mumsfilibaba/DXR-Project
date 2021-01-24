#pragma once
#include "RenderLayer/RenderingCore.h"
#include "RenderLayer/Format.h"
#include "RenderLayer/ResourceHelpers.h"

#include "Utilities/StringUtilities.h"

enum ETextureFactoryFlags : UInt32
{
	TextureFactoryFlag_None			= 0,
	TextureFactoryFlag_GenerateMips = FLAG(1),
};

class TextureFactory
{
public:
	static Bool Init();
	static void Release();

	// TODO: Supports R8G8B8A8 and R32G32B32A32 for now, support more formats? Such as Float16?
	static Texture2D* LoadFromFile(
		const std::string& Filepath, 
		UInt32 CreateFlags, 
		EFormat Format);

	static Texture2D* LoadFromMemory(
		const Byte* Pixels, 
		UInt32 Width, 
		UInt32 Height, 
		UInt32 CreateFlags, 
		EFormat Format);

	static SampledTexture2D LoadSampledTextureFromFile(
		const std::string& Filepath, 
		UInt32 CreateFlags, 
		EFormat Format);

	static SampledTexture2D LoadSampledTextureFromMemory(
		const Byte* Pixels, 
		UInt32 Width, 
		UInt32 Height, 
		UInt32 CreateFlags, 
		EFormat Format);

	static TextureCube* CreateTextureCubeFromPanorma(
		const SampledTexture2D& PanoramaSource,
		UInt32 CubeMapSize,
		UInt32 CreateFlags,
		EFormat Format);
};