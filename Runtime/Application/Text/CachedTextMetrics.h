#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"

struct IFontFace;

class APPLICATION_API FCachedTextMetrics
{
public:
    FCachedTextMetrics();
    ~FCachedTextMetrics();

    /**
     * @brief Measures a string.
     *
     * @param Font The face to measure in, which may be null.
     * @param Text The string to measure.
     * @return The width in pixels, zero without a face.
     */
    NODISCARD int32 GetWidth(const IFontFace* Font, const StringView& Text);

    /**
     * @brief Elides a string to a width.
     *
     * @param Font     The face to measure in, which may be null.
     * @param Text     The string to elide.
     * @param MaxWidth The widest the result may measure, in pixels.
     * @return The elided string, which stays valid until the next call.
     */
    NODISCARD const String& GetElided(const IFontFace* Font, const StringView& Text, int32 MaxWidth);

private:
    void Refresh(const IFontFace* Font, const StringView& Text);

    const IFontFace* CachedFont;
    uint64           CachedLayoutRevision;
    String           CachedText;
    int32            CachedWidth;
    int32            CachedMaxWidth;
    String           CachedElided;
};
