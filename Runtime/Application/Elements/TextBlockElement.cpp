#include "Application/Elements/TextBlockElement.h"
#include "Application/Draw/DrawCommandList.h"

TSharedPtr<FTextBlockElement> FTextBlockElement::Create(const FInitializer& Initializer)
{
    TSharedPtr<FTextBlockElement> NewElement = MakeSharedPtr<FTextBlockElement>();
    if (NewElement)
    {
        NewElement->Initialize(Initializer);
    }

    return NewElement;
}

FTextBlockElement::FTextBlockElement()
    : FVisualElement()
    , Text()
    , Font(nullptr)
    , ColorAndOpacity(FFloatColor::White)
    , Margin()
{
}

FTextBlockElement::~FTextBlockElement() = default;

void FTextBlockElement::Initialize(const FInitializer& Initializer)
{
    Text            = Initializer.Text;
    Font            = Initializer.Font;
    ColorAndOpacity = Initializer.ColorAndOpacity;
    Margin          = Initializer.Margin;
}

IntVector2 FTextBlockElement::ComputeDesiredSize() const
{
    if (!Font)
    {
        return IntVector2(Margin.GetTotalHorizontal(), Margin.GetTotalVertical());
    }

    const int32 TextWidth = Font->MeasureWidth(StringView(Text.Data(), Text.Length()));
    return IntVector2(TextWidth + Margin.GetTotalHorizontal(), Font->GetLineHeight() + Margin.GetTotalVertical());
}

int32 FTextBlockElement::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (!Text.IsEmpty())
    {
        OutCommandList.AddText(LayerId, AllottedGeometry.Bounds.Deflate(Margin), Text, Font.Get(), ColorAndOpacity);
    }

    return LayerId;
}

void FTextBlockElement::SetText(const String& InText)
{
    Text = InText;
}

void FTextBlockElement::SetColorAndOpacity(const FFloatColor& InColorAndOpacity)
{
    ColorAndOpacity = InColorAndOpacity;
}

void FTextBlockElement::SetFont(const TSharedPtr<IFontFace>& InFont)
{
    Font = InFont;
}

void FTextBlockElement::SetMargin(const FMargin& InMargin)
{
    Margin = InMargin;
}
