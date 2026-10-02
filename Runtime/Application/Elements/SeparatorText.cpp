#include "Application/Elements/SeparatorText.h"
#include "Application/Draw/DrawCommandList.h"
#include "Core/Math/Math.h"

TSharedPtr<FSeparatorText> FSeparatorText::Create(const FDesc& Desc)
{
    TSharedPtr<FSeparatorText> NewSeparatorText = MakeSharedPtr<FSeparatorText>();
    NewSeparatorText->Initialize(Desc);
    return NewSeparatorText;
}

FSeparatorText::FSeparatorText()
    : FVisualElement()
    , Text()
    , Font(nullptr)
    , TextColor(FFloatColor::White)
    , RuleColor(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f)
    , Padding()
    , RuleThickness(4.0f)
    , TextAlignment(0.1f)
    , TextGap(8)
{
}

FSeparatorText::~FSeparatorText() = default;

void FSeparatorText::Initialize(const FDesc& Desc)
{
    Text          = Desc.Text;
    Font          = Desc.Font;
    TextColor     = Desc.TextColor;
    RuleColor     = Desc.RuleColor;
    Padding       = Desc.Padding;
    RuleThickness = Math::Max(Desc.RuleThickness, 1.0f);
    TextAlignment = Math::Clamp(Desc.TextAlignment, 0.0f, 1.0f);
    TextGap       = Math::Max(Desc.TextGap, 0);
}

IntVector2 FSeparatorText::ComputeDesiredSize() const
{
    const int32 BandHeight = Font ? Font->GetTextBandHeight() : Math::CeilToInt(RuleThickness);
    return IntVector2(GetTextWidth() + (TextGap * 2) + Padding.GetTotalHorizontal(), BandHeight + Padding.GetTotalVertical());
}

int32 FSeparatorText::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FRectangle Available = AllottedGeometry.Bounds.Deflate(Padding);
    if (Available.IsEmpty())
    {
        return LayerId;
    }

    FRectangle Rule;
    Rule.Height     = Math::Min(Math::CeilToInt(RuleThickness), Available.Height);
    Rule.Position.Y = Available.Position.Y + ((Available.Height - Rule.Height) / 2);

    const int32 TextWidth = GetTextWidth();
    if (TextWidth <= 0)
    {
        Rule.Position.X = Available.Position.X;
        Rule.Width      = Available.Width;

        OutCommandList.AddBox(LayerId, Rule, RuleColor);
        return LayerId;
    }

    const int32 TextLeft  = Available.Position.X + Math::RoundToInt(static_cast<float>(Math::Max(0, Available.Width - TextWidth)) * TextAlignment);
    const int32 TextRight = TextLeft + TextWidth;

    Rule.Position.X = Available.Position.X;
    Rule.Width      = Math::Max(0, (TextLeft - TextGap) - Available.Position.X);

    if (!Rule.IsEmpty())
    {
        OutCommandList.AddBox(LayerId, Rule, RuleColor);
    }

    Rule.Position.X = TextRight + TextGap;
    Rule.Width      = Math::Max(0, Available.GetRight() - Rule.Position.X);

    if (!Rule.IsEmpty())
    {
        OutCommandList.AddBox(LayerId, Rule, RuleColor);
    }

    const FRectangle TextBounds(IntVector2(TextLeft, Available.Position.Y), TextWidth, Available.Height);
    OutCommandList.AddText(LayerId, TextBounds, Text, Font.Get(), TextColor);

    return LayerId;
}

void FSeparatorText::SetText(const String& InText)
{
    if (Text != InText)
    {
        Text = InText;
        InvalidateDesiredSize();
    }
}

int32 FSeparatorText::GetTextWidth() const
{
    return Font ? Font->MeasureWidth(StringView(Text.Data(), Text.Length())) : 0;
}
