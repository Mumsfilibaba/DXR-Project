#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Application/Draw/DrawTypes.h"
#include "RHI/RHIResources.h"

class APPLICATION_API FUIAtlas
{
public:

    /**
     * @brief How wide and tall an atlas is, in texels. Fixed rather than fitted to what was packed, because
     * a brush is handed out before the upload and its coordinates would move under it if the size changed.
     */
    static constexpr int32 AtlasSize = 1024;

    /** @brief The space kept between two packed images, so filtering cannot bleed one into the next. */
    static constexpr int32 Padding = 2;

public:
    FUIAtlas();
    ~FUIAtlas();

    FUIAtlas(const FUIAtlas&) = delete;
    FUIAtlas& operator=(const FUIAtlas&) = delete;

    /**
     * @brief Packs an image and builds a brush that samples where it landed. The region is reserved and the
     * pixels are kept here, so the brush is complete before the upload and stays so across later Adds.
     *
     * @param Pixels A tightly packed RGBA8 image of Width by Height texels.
     * @param Width  The image width in texels, which cannot exceed AtlasSize.
     * @param Height The image height in texels.
     * @return A brush covering the packed region, or a brush with no texture when it did not fit.
     */
    NODISCARD FUIBrush Add(const uint8* Pixels, int32 Width, int32 Height);

    /**
     * @brief Uploads everything packed so far into the atlas texture.
     *
     * @return True when the texture exists and holds every image added to it.
     */
    bool Build();

    /** @brief Drops the texture and everything packed into it, leaving the atlas as it was created. */
    void Release();

    /**
     * @brief Names the atlas for a graphics debugger.
     *
     * @param InDebugName The name, applied to the texture as soon as there is one.
     */
    void SetDebugName(const String& InDebugName);

    /** @return The texture every brush from this atlas samples, or null when there is nothing packed yet. */
    NODISCARD FORCEINLINE FRHITexture* GetTexture() const
    {
        return Texture.Get();
    }

    /** @return How tall the packed content is, in texels, which is what a Build uploads. */
    NODISCARD FORCEINLINE int32 GetUsedHeight() const
    {
        return UsedHeight;
    }

    /** @return True when there is a texture for the brushes handed out to sample. */
    NODISCARD FORCEINLINE bool IsValid() const
    {
        return Texture.IsValid();
    }

private:
    bool EnsureTexture();
    bool PackRegion(int32 Width, int32 Height, int32& OutX, int32& OutY);

    TArray<uint8>  Pixels;
    FRHITextureRef Texture;
    String         DebugName;
    int32          ShelfX;
    int32          ShelfY;
    int32          ShelfHeight;
    int32          UsedHeight;
    bool           bIsDirty;
};
