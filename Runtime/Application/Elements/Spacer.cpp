#include "Application/Elements/Spacer.h"

TSharedPtr<FSpacer> FSpacer::Create(const IntVector2& InSize)
{
    TSharedPtr<FSpacer> NewSpacer = MakeSharedPtr<FSpacer>();
    NewSpacer->SetSize(InSize);
    return NewSpacer;
}

TSharedPtr<FSpacer> FSpacer::CreateHorizontal(int32 InWidth)
{
    return Create(IntVector2(InWidth, 0));
}

TSharedPtr<FSpacer> FSpacer::CreateVertical(int32 InHeight)
{
    return Create(IntVector2(0, InHeight));
}

FSpacer::FSpacer()
    : FVisualElement()
    , Size()
{
}

FSpacer::~FSpacer() = default;

IntVector2 FSpacer::ComputeDesiredSize() const
{
    return Size;
}

void FSpacer::SetSize(const IntVector2& InSize)
{
    Size = InSize;
}
