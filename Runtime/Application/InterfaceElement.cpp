#include "Widget.h"

FInterfaceElement::FInterfaceElement(const TWeakPtr<FInterfaceElement>& InParentElement)
    : ParentElement(InParentElement)
    , ChildElements()
{ }

void FInterfaceElement::OnDraw()
{
    for (TSharedPtr<FInterfaceElement>& Element : ChildElements)
    {
        if (Element->IsActive())
        {
            Element->OnDraw();
        }
    }
}

void FInterfaceElement::AddChildElement(const TSharedPtr<FInterfaceElement>& InChildElement) 
{
    ChildElements.Push(InChildElement);
}

void FInterfaceElement::RemoveChildElement(const TSharedPtr<FInterfaceElement>& InChildElement)
{
    ChildElements.Remove(InChildElement);
}