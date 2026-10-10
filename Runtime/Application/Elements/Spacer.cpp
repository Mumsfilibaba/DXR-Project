#include "Application/Elements/Spacer.h"

TSharedPtr<FSpacer> FSpacer::Create(const FIntVector2& InSize)
{
    TSharedPtr<FSpacer> NewSpacer = MakeSharedPtr<FSpacer>();
    NewSpacer->SetSize(InSize);
    return NewSpacer;
}

TSharedPtr<FSpacer> FSpacer::CreateHorizontal(int32 InWidth)
{
    return Create(FIntVector2(InWidth, 0));
}

TSharedPtr<FSpacer> FSpacer::CreateVertical(int32 InHeight)
{
    return Create(FIntVector2(0, InHeight));
}

FSpacer::FSpacer()
    : FVisualElement()
    , Size()
{
}

FSpacer::~FSpacer() = default;

FIntVector2 FSpacer::ComputeDesiredSize() const
{
    return Size;
}

void FSpacer::SetSize(const FIntVector2& InSize)
{
    if (Size != InSize)
    {
        Size = InSize;
        InvalidateDesiredSize();
    }
}
