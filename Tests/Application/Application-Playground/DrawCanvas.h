#pragma once
#include <Core/Delegates/Delegate.h>
#include <Core/Math/IntVector2.h>
#include <Application/Elements/VisualElement.h>

/** @brief Draws into the rectangle the canvas was arranged into, and returns the highest layer used. */
DECLARE_RETURN_DELEGATE(FOnCanvasDraw, int32, const FDrawGeometry&, FDrawCommandList&, int32);

class FDrawCanvas final : public FVisualElement
{
public:
    struct FDesc
    {
        FDesc()
            : DesiredSize()
            , OnCanvasDraw()
        {
        }

        IntVector2    DesiredSize;
        FOnCanvasDraw OnCanvasDraw;
    };

public:
    static TSharedPtr<FDrawCanvas> Create(const FDesc& Desc);

public:
    FDrawCanvas();
    virtual ~FDrawCanvas();

    /**
     * @brief Initializes the canvas with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

private:
    IntVector2    DesiredSize;
    FOnCanvasDraw OnCanvasDraw;
};
