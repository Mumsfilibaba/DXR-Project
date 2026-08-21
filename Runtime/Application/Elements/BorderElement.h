#pragma once
#include "Core/Math/Color.h"
#include "Application/Elements/CompoundElement.h"

class APPLICATION_API FBorderElement final : public FCompoundElement
{
public:
    struct FInitializer
    {
        FInitializer()
            : BackgroundColor(0.0f, 0.0f, 0.0f, 0.0f)
            , Padding()
            , Content(nullptr)
        {
        }

        FFloatColor                BackgroundColor;
        FMargin                    Padding;
        TSharedPtr<FVisualElement> Content;
    };

public:
    static TSharedPtr<FBorderElement> Create(const FInitializer& Initializer);

public:
    FBorderElement();
    virtual ~FBorderElement();

    /**
     * @brief Initializes the border with the specified parameters.
     *
     * @param Initializer Initialization parameters.
     */
    void Initialize(const FInitializer& Initializer);

    // FVisualElement Interface
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Sets the fill drawn behind the child.
     *
     * @param InBackgroundColor The fill color. A zero alpha draws nothing.
     */
    void SetBackgroundColor(const FFloatColor& InBackgroundColor);

    /** @brief The fill drawn behind the child. */
    NODISCARD FORCEINLINE const FFloatColor& GetBackgroundColor() const
    {
        return BackgroundColor;
    }

private:
    FFloatColor BackgroundColor;
};
