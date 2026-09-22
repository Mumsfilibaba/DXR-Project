#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/String.h"
#include "Application/Text/FontAtlas.h"
#include "Application/Text/IFontFace.h"

class APPLICATION_API FTrueTypeFontFace final : public IFontFace
{
public:

    /**
     * @brief Loads a TrueType file and rasterizes it at one size.
     *
     * @param Filename    The path to the font file.
     * @param PixelHeight The height to rasterize at, in pixels.
     * @return The face, or null when the file could not be read or packed.
     */
    static TSharedPtr<FTrueTypeFontFace> CreateFromFile(const String& Filename, int32 PixelHeight);

public:
    FTrueTypeFontFace();
    virtual ~FTrueTypeFontFace();

    // IFontFace Interface
    virtual const FFontAtlas* GetAtlas() const override final;
    virtual int32 GetLineHeight() const override final;
    virtual int32 GetAscent() const override final;
    virtual int32 GetDescent() const override final;
    virtual int32 GetCapHeight() const override final;
    virtual const FShapedRun& ShapeText(const StringView& Text) const override final;

private:
    static constexpr int32 ShapedRunCacheWays     = 4;
    static constexpr int32 ShapedRunCacheSetCount = 512;
    static constexpr int32 ShapedRunCacheSize     = ShapedRunCacheWays * ShapedRunCacheSetCount;

    struct FCachedRun
    {
        FCachedRun()
            : Text()
            , Run()
            , Revision(0)
        {
        }

        String     Text;
        FShapedRun Run;
        uint64     Revision;
    };

    void ShapeRun(const StringView& Text, FShapedRun& OutRun) const;

    FFontAtlas                                           Atlas;
    mutable TStaticArray<FCachedRun, ShapedRunCacheSize> ShapedRuns;
    mutable TStaticArray<uint8, ShapedRunCacheSetCount>  ShapedRunReplacementWays;
};
