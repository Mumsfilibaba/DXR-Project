#include "Application/Elements/SearchBox.h"
#include "Application/Elements/EditableText.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/ElementPath.h"
#include "Application/Input/Keys.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

/** @brief The gap between an icon square and the line of text, in pixels. */
constexpr int32 SEARCH_BOX_ICON_SPACING = 4;

/** @brief How far the drawn cross is inset inside the clear button square, in pixels. */
constexpr int32 SEARCH_BOX_CROSS_INSET = 4;

/** @brief The width of the strokes the drawn cross is made of, in pixels. */
constexpr float SEARCH_BOX_CROSS_THICKNESS = 1.0f;

TSharedPtr<FSearchBox> FSearchBox::Create(const FDesc& Desc)
{
    TSharedPtr<FSearchBox> NewSearchBox = MakeSharedPtr<FSearchBox>();
    NewSearchBox->Initialize(Desc);
    return NewSearchBox;
}

FSearchBox::FSearchBox()
    : FVisualElement()
    , Editor(nullptr)
    , SearchIcon()
    , ClearIcon()
    , Padding(6, 3, 6, 3)
    , IconSize(14)
    , bIsHovered(false)
    , OnTextChangedDelegate()
{
}

FSearchBox::~FSearchBox() = default;

void FSearchBox::Initialize(const FDesc& Desc)
{
    SearchIcon            = Desc.SearchIcon;
    ClearIcon             = Desc.ClearIcon;
    Padding               = Desc.Padding;
    IconSize              = Math::Max(0, Desc.IconSize);
    OnTextChangedDelegate = Desc.OnTextChanged;

    FEditableText::FDesc EditorDesc;
    EditorDesc.HintText        = Desc.HintText;
    EditorDesc.Font            = Desc.Font;
    EditorDesc.ForegroundColor = FUIStyle::GetDefault().Colors.Text;
    EditorDesc.HintColor       = FUIStyle::GetDefault().Colors.TextDisabled;
    EditorDesc.Padding         = FMargin();

    Editor = FEditableText::Create(EditorDesc);
    Editor->SetParentElement(AsWeakPtr());

    Editor->SetActivationPolicy(EElementActivationPolicy::DoNotAutoFocusOnWindowActivate);
    Editor->GetOnTextChanged().BindLambda([this](const String& InText)
    {
        HandleTextChanged(InText);
    });
}

IntVector2 FSearchBox::ComputeDesiredSize() const
{
    IntVector2 DesiredSize(Padding.GetTotalHorizontal(), Padding.GetTotalVertical());

    if (Editor)
    {
        const IntVector2 EditorSize = Editor->GetCachedDesiredSize();
        DesiredSize.X += EditorSize.X;
        DesiredSize.Y += Math::Max(EditorSize.Y, IconSize);
    }
    else
    {
        DesiredSize.Y += IconSize;
    }

    if (SearchIcon.IsValid())
    {
        DesiredSize.X += IconSize + SEARCH_BOX_ICON_SPACING;
    }

    DesiredSize.X += IconSize + SEARCH_BOX_ICON_SPACING;
    return DesiredSize;
}

void FSearchBox::OnArrange(const FRectangle& AllottedBounds)
{
    if (Editor)
    {
        Editor->Tick(GetEditorRectangle(AllottedBounds));
    }
}

void FSearchBox::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    if (Editor)
    {
        OutChildren.Add(Editor);
    }
}

void FSearchBox::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    const FRectangle ClearBounds = GetClearButtonRectangle(GetContentRectangle());
    if (!ClearBounds.IsEmpty() && ClearBounds.EncapsulatesPoint(ClientPosition))
    {
        return;
    }

    if (Editor)
    {
        Editor->FindChildrenContainingPoint(ClientPosition, OutChildElements);
    }
}

int32 FSearchBox::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&    Style        = FUIStyle::GetDefault();
    const FCornerRadii CornerRadius = FCornerRadii(Style.Metrics.CornerRadius);

    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.Colors.ControlNormal, CornerRadius);
    OutCommandList.AddBoxOutline(LayerId, AllottedGeometry.Bounds, Style.Colors.Border, Style.Metrics.BorderThickness, CornerRadius);

    const FRectangle IconBounds = GetSearchIconRectangle(AllottedGeometry.Bounds);
    if (!IconBounds.IsEmpty())
    {
        OutCommandList.AddImage(LayerId, IconBounds, SearchIcon, Style.Colors.TextDisabled);
    }

    int32 NextLayerId = LayerId;
    if (Editor && Editor->IsVisible())
    {
        const FDrawGeometry EditorGeometry(Editor->GetContentRectangle(), AllottedGeometry.Scale);
        NextLayerId = Math::Max(NextLayerId, Editor->OnDraw(EditorGeometry, OutCommandList, LayerId));
    }

    const FRectangle ClearBounds = GetClearButtonRectangle(AllottedGeometry.Bounds);
    if (!ClearBounds.IsEmpty())
    {
        const FFloatColor& ClearTint = bIsHovered ? Style.Colors.Text : Style.Colors.TextDisabled;

        if (ClearIcon.IsValid())
        {
            OutCommandList.AddImage(NextLayerId, ClearBounds, ClearIcon, ClearTint);
        }
        else
        {
            const FRectangle CrossBounds = ClearBounds.Deflate(FMargin(SEARCH_BOX_CROSS_INSET));
            const float      Left        = static_cast<float>(CrossBounds.Position.X);
            const float      Top         = static_cast<float>(CrossBounds.Position.Y);
            const float      Right       = static_cast<float>(CrossBounds.GetRight());
            const float      Bottom      = static_cast<float>(CrossBounds.GetBottom());

            OutCommandList.AddLine(NextLayerId, Vector2(Left, Top), Vector2(Right, Bottom), ClearTint, SEARCH_BOX_CROSS_THICKNESS);
            OutCommandList.AddLine(NextLayerId, Vector2(Right, Top), Vector2(Left, Bottom), ClearTint, SEARCH_BOX_CROSS_THICKNESS);
        }
    }

    return NextLayerId;
}

FEventResponse FSearchBox::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    const FRectangle ClearBounds = GetClearButtonRectangle(GetContentRectangle());
    if (ClearBounds.IsEmpty() || !ClearBounds.EncapsulatesPoint(CursorEvent.GetClientPosition()))
    {
        return FEventResponse::Unhandled();
    }

    ClearText();
    return FEventResponse::Handled();
}

FEventResponse FSearchBox::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    UNREFERENCED_VARIABLE(CursorEvent);

    bIsHovered = true;
    return FEventResponse::Unhandled();
}

FEventResponse FSearchBox::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    UNREFERENCED_VARIABLE(CursorEvent);

    bIsHovered = false;
    return FEventResponse::Unhandled();
}

void FSearchBox::SetText(const String& InText)
{
    if (Editor)
    {
        Editor->SetText(InText);
    }
}

const String& FSearchBox::GetText() const
{
    CHECK(Editor != nullptr);
    return Editor->GetText();
}

void FSearchBox::ClearText()
{
    if (Editor)
    {
        Editor->ClearText();
    }
}

bool FSearchBox::IsEmpty() const
{
    return !Editor || Editor->GetText().IsEmpty();
}

FRectangle FSearchBox::GetSearchIconRectangle(const FRectangle& Bounds) const
{
    if (!SearchIcon.IsValid())
    {
        return FRectangle();
    }

    const FRectangle Inner = Bounds.Deflate(Padding);
    const int32      Side  = Math::Min(IconSize, Inner.Height);

    return FRectangle(IntVector2(Inner.Position.X, Inner.Position.Y + ((Inner.Height - Side) / 2)), Side, Side);
}

FRectangle FSearchBox::GetClearButtonRectangle(const FRectangle& Bounds) const
{
    if (IsEmpty())
    {
        return FRectangle();
    }

    const FRectangle Inner = Bounds.Deflate(Padding);
    const int32      Side  = Math::Min(IconSize, Inner.Height);

    return FRectangle(IntVector2(Inner.GetRight() - Side, Inner.Position.Y + ((Inner.Height - Side) / 2)), Side, Side);
}

void FSearchBox::HandleTextChanged(const String& InText)
{
    OnTextChangedDelegate.ExecuteIfBound(InText);
}

FRectangle FSearchBox::GetEditorRectangle(const FRectangle& Bounds) const
{
    const FRectangle Inner       = Bounds.Deflate(Padding);
    const FRectangle IconBounds  = GetSearchIconRectangle(Bounds);
    const FRectangle ClearBounds = GetClearButtonRectangle(Bounds);

    int32 Left  = Inner.Position.X;
    int32 Right = Inner.GetRight();

    if (!IconBounds.IsEmpty())
    {
        Left = IconBounds.GetRight() + SEARCH_BOX_ICON_SPACING;
    }

    if (!ClearBounds.IsEmpty())
    {
        Right = ClearBounds.Position.X - SEARCH_BOX_ICON_SPACING;
    }

    return FRectangle(IntVector2(Left, Inner.Position.Y), Math::Max(0, Right - Left), Inner.Height);
}
