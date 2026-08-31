#include "Engine/EngineUI/EditorUI/Panels/EditorViewportImage.h"
#include "Application/Draw/DrawCommandList.h"

TSharedPtr<FEditorViewportImage> FEditorViewportImage::Create()
{
    return MakeSharedPtr<FEditorViewportImage>();
}

FEditorViewportImage::FEditorViewportImage()
    : FVisualElement()
    , Brush()
{
}

FEditorViewportImage::~FEditorViewportImage()
{
}

IntVector2 FEditorViewportImage::ComputeDesiredSize() const
{
    // The scene renders at whatever the dock leaf allots, so the image never asks for a size of its own.
    return IntVector2(0, 0);
}

int32 FEditorViewportImage::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (!Brush.IsValid())
    {
        OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));
        return LayerId;
    }

    OutCommandList.AddImage(LayerId, AllottedGeometry.Bounds, Brush, FFloatColor(1.0f, 1.0f, 1.0f, 1.0f));
    return LayerId;
}

void FEditorViewportImage::SetBrush(const FUIBrush& InBrush)
{
    Brush = InBrush;
}
