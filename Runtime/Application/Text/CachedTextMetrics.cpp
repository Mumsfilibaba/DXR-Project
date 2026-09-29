#include "Application/Text/CachedTextMetrics.h"
#include "Application/Text/FontAtlas.h"
#include "Application/Text/IFontFace.h"
#include "Core/Memory/Memory.h"

FCachedTextMetrics::FCachedTextMetrics()
    : CachedFont(nullptr)
    , CachedLayoutRevision(0)
    , CachedText()
    , CachedWidth(-1)
    , CachedMaxWidth(-1)
    , CachedElided()
{
}

FCachedTextMetrics::~FCachedTextMetrics() = default;

void FCachedTextMetrics::Refresh(const IFontFace* Font, const StringView& Text)
{
    const FFontAtlas* Atlas          = Font ? Font->GetAtlas() : nullptr;
    const uint64      LayoutRevision = Atlas ? Atlas->GetLayoutRevision() : 0;

    const bool bSameText = CachedText.Length() == Text.Length()
        && (Text.Length() == 0 || Memory::Memcmp(CachedText.Data(), Text.Data(), static_cast<uint64>(Text.Length())) == 0);

    if (Font == CachedFont && LayoutRevision == CachedLayoutRevision && bSameText)
    {
        return;
    }

    CachedFont           = Font;
    CachedLayoutRevision = LayoutRevision;
    CachedWidth          = -1;
    CachedMaxWidth       = -1;

    if (!bSameText)
    {
        CachedText = String(Text.Data(), Text.Length());
    }
}

int32 FCachedTextMetrics::GetWidth(const IFontFace* Font, const StringView& Text)
{
    Refresh(Font, Text);

    if (CachedWidth < 0)
    {
        CachedWidth = Font ? Font->MeasureWidth(Text) : 0;
    }

    return CachedWidth;
}

const String& FCachedTextMetrics::GetElided(const IFontFace* Font, const StringView& Text, int32 MaxWidth)
{
    Refresh(Font, Text);

    if (CachedMaxWidth != MaxWidth)
    {
        CachedElided   = Font ? Font->ElideText(Text, MaxWidth) : String(Text.Data(), Text.Length());
        CachedMaxWidth = MaxWidth;
    }

    return CachedElided;
}
