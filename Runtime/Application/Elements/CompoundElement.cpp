#include "Application/Elements/CompoundElement.h"
#include "Application/ElementPath.h"
#include "Application/Draw/DrawCommandList.h"

FCompoundElement::FCompoundElement()
    : FVisualElement()
    , Content(nullptr)
    , Padding()
{
}

FCompoundElement::~FCompoundElement() = default;

IntVector2 FCompoundElement::ComputeDesiredSize() const
{
    IntVector2 DesiredSize(Padding.GetTotalHorizontal(), Padding.GetTotalVertical());
    if (Content)
    {
        const IntVector2 ContentSize = Content->GetCachedDesiredSize();
        DesiredSize.X += ContentSize.X;
        DesiredSize.Y += ContentSize.Y;
    }

    return DesiredSize;
}

void FCompoundElement::OnArrange(const FRectangle& AllottedBounds)
{
    if (Content)
    {
        Content->Arrange(AllottedBounds.Deflate(Padding));
    }
}

EChildVisit FCompoundElement::VisitChildren(FChildVisitor& Visitor, EChildOrder /*Order*/) const
{
    return VisitChild(Visitor, Content);
}

int32 FCompoundElement::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (!Content)
    {
        return LayerId;
    }

    const FDrawGeometry ContentGeometry(Content->GetContentRectangle(), AllottedGeometry.Scale);
    return Content->Draw(ContentGeometry, OutCommandList, LayerId + 1);
}
void FCompoundElement::SetContent(const TSharedPtr<FVisualElement>& InContent)
{
    Content = InContent;
    if (Content)
    {
        Content->SetParentElement(AsWeakPtr());
    }

    InvalidateDesiredSize();
}

void FCompoundElement::SetPadding(const FMargin& InPadding)
{
    Padding = InPadding;
    InvalidateDesiredSize();
}
