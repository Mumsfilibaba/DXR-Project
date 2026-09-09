#include "Application/Elements/TexturePreview.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Menus/ToolTipService.h"
#include "Core/Math/Math.h"

constexpr float TEXTURE_PREVIEW_TOOL_TIP_DELAY = 0.25f;

TSharedPtr<FTexturePreview> FTexturePreview::Create(const FDesc& Desc)
{
    TSharedPtr<FTexturePreview> NewTexturePreview = MakeSharedPtr<FTexturePreview>();
    NewTexturePreview->Initialize(Desc);
    return NewTexturePreview;
}

FTexturePreview::FTexturePreview()
    : FVisualElement()
    , Brush()
    , Font(nullptr)
    , EmptyText("None")
    , PreviewSize(48)
    , ZoomSize(256)
{
}

FTexturePreview::~FTexturePreview() = default;

void FTexturePreview::Initialize(const FDesc& Desc)
{
    Brush       = Desc.Brush;
    Font        = Desc.Font;
    EmptyText   = Desc.EmptyText;
    PreviewSize = Math::Max(1, Desc.PreviewSize);
    ZoomSize    = Math::Max(0, Desc.ZoomSize);
}

IntVector2 FTexturePreview::ComputeDesiredSize() const
{
    if (Brush.IsValid())
    {
        return IntVector2(PreviewSize, PreviewSize);
    }

    const int32 Height = Font ? Font->GetTextBandHeight() : FUIStyle::GetDefault().Metrics.RowHeight;
    const int32 Width  = (Font && !EmptyText.IsEmpty()) ? Font->MeasureWidth(StringView(EmptyText.Data(), EmptyText.Length())) : 0;

    return IntVector2(Width, Height);
}

int32 FTexturePreview::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    if (!Brush.IsValid())
    {
        if (Font && !EmptyText.IsEmpty())
        {
            const FRectangle& Bounds = AllottedGeometry.Bounds;

            FRectangle TextBounds;
            TextBounds.Position.X = Bounds.Position.X;
            TextBounds.Position.Y = Bounds.Position.Y + Font->GetTextBandOffset(Bounds.Height);
            TextBounds.Width      = Bounds.Width;
            TextBounds.Height     = Font->GetTextBandHeight();

            OutCommandList.AddText(LayerId, TextBounds, EmptyText, Font.Get(), Style.Colors.TextDisabled);
        }

        return LayerId;
    }

    const FRectangle   ImageBounds  = GetImageRectangle(AllottedGeometry.Bounds);
    const FCornerRadii CornerRadius = FCornerRadii(Style.Metrics.CornerRadius);

    OutCommandList.AddImage(LayerId, ImageBounds, Brush, FFloatColor::White);
    OutCommandList.AddBoxOutline(LayerId + 1, ImageBounds, Style.Colors.InputFieldBorder, Style.Metrics.BorderThickness, CornerRadius);

    return LayerId + 1;
}

FEventResponse FTexturePreview::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    UNREFERENCED_VARIABLE(CursorEvent);

    if (!Brush.IsValid() || ZoomSize <= 0)
    {
        return FEventResponse::Unhandled();
    }

    FDesc ZoomDesc;
    ZoomDesc.Brush       = Brush;
    ZoomDesc.PreviewSize = ZoomSize;
    ZoomDesc.ZoomSize    = 0;

    FToolTipService::Get().RequestToolTip(AsSharedPtr(), Create(ZoomDesc), EToolTipPlacement::FollowCursor, TEXTURE_PREVIEW_TOOL_TIP_DELAY);
    return FEventResponse::Unhandled();
}

FEventResponse FTexturePreview::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    FToolTipService::Get().CancelToolTip(AsSharedPtr());
    return FVisualElement::OnMouseLeft(CursorEvent);
}

void FTexturePreview::SetBrush(const FUIBrush& InBrush)
{
    Brush = InBrush;
}

FRectangle FTexturePreview::GetImageRectangle(const FRectangle& Bounds) const
{
    const int32 Extent = Math::Min(PreviewSize, Math::Min(Bounds.Width, Bounds.Height));
    return FRectangle(Bounds.Position, Math::Max(0, Extent), Math::Max(0, Extent));
}
