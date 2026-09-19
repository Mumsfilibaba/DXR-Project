#include "Application/Elements/MenuHost.h"
#include "Application/ElementPath.h"
#include "Application/Draw/DrawCommandList.h"

TSharedPtr<FMenuHost> FMenuHost::Create()
{
    return MakeSharedPtr<FMenuHost>();
}

FMenuHost::FMenuHost()
    : FVisualElement()
    , Children()
{
}

FMenuHost::~FMenuHost() = default;

void FMenuHost::AddChild(const TSharedPtr<FVisualElement>& InContent, const FRectangle& InClientBounds, bool bInHitTestable)
{
    if (!InContent)
    {
        return;
    }

    FHostedChild& Child = Children.Emplace();
    Child.Content       = InContent;
    Child.ClientBounds  = InClientBounds;
    Child.bHitTestable  = bInHitTestable;

    InContent->SetParentElement(AsWeakPtr());
    InContent->Tick(InClientBounds);
}

void FMenuHost::SetChildBounds(const TSharedPtr<FVisualElement>& InContent, const FRectangle& InClientBounds)
{
    for (FHostedChild& Child : Children)
    {
        if (Child.Content == InContent)
        {
            Child.ClientBounds = InClientBounds;
            Child.Content->Tick(InClientBounds);
            return;
        }
    }
}

void FMenuHost::RemoveChild(const TSharedPtr<FVisualElement>& InContent)
{
    for (int32 Index = 0; Index < Children.Size(); ++Index)
    {
        if (Children[Index].Content == InContent)
        {
            Children.RemoveAt(Index);
            return;
        }
    }
}

bool FMenuHost::IsEmpty() const
{
    return Children.IsEmpty();
}

void FMenuHost::OnArrange(const FRectangle&)
{
    for (const FHostedChild& Child : Children)
    {
        if (Child.Content)
        {
            Child.Content->Tick(Child.ClientBounds);
        }
    }
}

void FMenuHost::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    for (const FHostedChild& Child : Children)
    {
        if (Child.Content)
        {
            OutChildren.Add(Child.Content);
        }
    }
}

int32 FMenuHost::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (Children.IsEmpty())
    {
        return LayerId;
    }

    OutCommandList.PushClip(LayerId, AllottedGeometry.Bounds);

    int32 MaxLayerId = LayerId;
    for (const FHostedChild& Child : Children)
    {
        if (!Child.Content || !Child.Content->IsVisible())
        {
            continue;
        }

        const FDrawGeometry ChildGeometry(Child.Content->GetContentRectangle(), AllottedGeometry.Scale);
        MaxLayerId = Child.Content->OnDraw(ChildGeometry, OutCommandList, MaxLayerId + 1);
    }

    OutCommandList.PopClip(MaxLayerId);

    return MaxLayerId;
}

const FMenuHost::FHostedChild* FMenuHost::FindChildAtPoint(const IntVector2& ClientPosition) const
{
    for (int32 Index = Children.Size() - 1; Index >= 0; --Index)
    {
        const FHostedChild& Child = Children[Index];
        if (!Child.bHitTestable || !Child.Content || !Child.Content->IsVisible())
        {
            continue;
        }

        if (Child.Content->GetContentRectangle().EncapsulatesPoint(ClientPosition))
        {
            return &Child;
        }
    }

    return nullptr;
}

bool FMenuHost::CoversPoint(const IntVector2& ClientPosition) const
{
    return FindChildAtPoint(ClientPosition) != nullptr;
}

void FMenuHost::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    if (const FHostedChild* Child = FindChildAtPoint(ClientPosition))
    {
        OutChildElements.Add(GetVisibility(), AsSharedPtr());
        Child->Content->FindChildrenContainingPoint(ClientPosition, OutChildElements);
    }
}
