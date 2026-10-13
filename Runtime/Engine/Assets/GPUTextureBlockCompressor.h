#pragma once
#include "AssetCore/TextureAssetFactory.h"
#include "Engine/EngineModule.h"

/** @brief Compresses imported textures on the GPU with FTextureCompressor, and reads the blocks back */
struct ENGINE_API FGPUTextureBlockCompressor final : public ITextureBlockCompressor
{
    /** @brief Waits for the GPU, so call it from the main thread */
    virtual bool Compress(ETextureFormat SourceFormat, const TArray<FTextureMip>& Mips, ETextureFormat Format, TArray<FTextureMip>& OutMips, FString& OutError) override;
};
