#include "Application/Elements/TextBlock.h"
#include "Application/Draw/DrawCommandList.h"

TSharedPtr<FTextBlock> FTextBlock::Create(const FDesc& Desc)
{
    TSharedPtr<FTextBlock> NewInstance = MakeSharedPtr<FTextBlock>();
    if (NewInstance)
    {
        NewInstance->Initialize(Desc);
    }

    return NewInstance;
}

FTextBlock::FTextBlock()
    : FVisualElement()
    , Text()
    , Font(nullptr)
    , ColorAndOpacity(FFloatColor::White)
    , Margin()
    , Overflow(ETextOverflow::Overflow)
{
}

FTextBlock::~FTextBlock() = default;

void FTextBlock::Initialize(const FDesc& Desc)
{
    Text            = Desc.Text;
    Font            = Desc.Font;
    ColorAndOpacity = Desc.ColorAndOpacity;
    Margin          = Desc.Margin;
    Overflow        = Desc.Overflow;
}

IntVector2 FTextBlock::ComputeDesiredSize() const
{
    if (!Font)
    {
        return IntVector2(Margin.GetTotalHorizontal(), Margin.GetTotalVertical());
    }

    const int32 TextWidth = Font->MeasureWidth(StringView(Text.Data(), Text.Length()));
    return IntVector2(TextWidth + Margin.GetTotalHorizontal(), Font->GetLineHeight() + Margin.GetTotalVertical());
}

int32 FTextBlock::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (!Text.IsEmpty())
    {
        const FRectangle TextBounds = AllottedGeometry.Bounds.Deflate(Margin);
        if (Overflow == ETextOverflow::Elide && Font)
        {
            const String Elided = Font->ElideText(StringView(Text.Data(), Text.Length()), TextBounds.Width);
            OutCommandList.AddText(LayerId, TextBounds, Elided, Font.Get(), ColorAndOpacity);
        }
        else
        {
            OutCommandList.AddText(LayerId, TextBounds, Text, Font.Get(), ColorAndOpacity);
        }
    }

    return LayerId;
}

void FTextBlock::SetText(const String& InText)
{
    Text = InText;
}

void FTextBlock::SetColorAndOpacity(const FFloatColor& InColorAndOpacity)
{
    ColorAndOpacity = InColorAndOpacity;
}

void FTextBlock::SetFont(const TSharedPtr<IFontFace>& InFont)
{
    Font = InFont;
}

void FTextBlock::SetMargin(const FMargin& InMargin)
{
    Margin = InMargin;
}
