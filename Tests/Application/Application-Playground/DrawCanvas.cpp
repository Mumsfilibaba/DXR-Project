#include <Application/Draw/DrawCommandList.h>

#include "DrawCanvas.h"

TSharedPtr<FDrawCanvas> FDrawCanvas::Create(const FDesc& Desc)
{
    TSharedPtr<FDrawCanvas> NewCanvas = MakeSharedPtr<FDrawCanvas>();
    NewCanvas->Initialize(Desc);
    return NewCanvas;
}

FDrawCanvas::FDrawCanvas()
    : FVisualElement()
    , DesiredSize()
    , OnCanvasDraw()
{
}

FDrawCanvas::~FDrawCanvas()
{
}

void FDrawCanvas::Initialize(const FDesc& Desc)
{
    DesiredSize  = Desc.DesiredSize;
    OnCanvasDraw = Desc.OnCanvasDraw;
}

IntVector2 FDrawCanvas::ComputeDesiredSize() const
{
    return DesiredSize;
}

int32 FDrawCanvas::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (!OnCanvasDraw.IsBound() || AllottedGeometry.Bounds.IsEmpty())
    {
        return LayerId;
    }

    return OnCanvasDraw.Execute(AllottedGeometry, OutCommandList, LayerId);
}
