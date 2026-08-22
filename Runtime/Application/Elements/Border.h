#pragma once
#include "Core/Math/Color.h"
#include "Application/Elements/CompoundElement.h"

class APPLICATION_API FBorder final : public FCompoundElement
{
public:
    struct FDesc
    {
        FDesc()
            : BackgroundColor(0.0f, 0.0f, 0.0f, 0.0f)
            , Padding()
            , CornerRadius(0.0f)
            , MinHeight(0)
            , Cursor(ECursor::None)
            , bHasCursor(false)
            , Content(nullptr)
        {
        }

        /**
         * @brief Gives the border an opinion on the cursor shape, which its padding ring carries too.
         *
         * @param InCursor The shape to show over the border.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetCursor(ECursor InCursor)
        {
            Cursor     = InCursor;
            bHasCursor = true;
            return *this;
        }

        FFloatColor                BackgroundColor;
        FMargin                    Padding;
        float                      CornerRadius;
        int32                      MinHeight;
        ECursor                    Cursor;
        bool                       bHasCursor;
        TSharedPtr<FVisualElement> Content;
    };

public:
    static TSharedPtr<FBorder> Create(const FDesc& Desc);

public:
    FBorder();
    virtual ~FBorder();

    /**
     * @brief Initializes the border with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual bool GetCursor(ECursor& OutCursor) const override;

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

    /**
     * @brief Sets how far the fill is rounded at each corner.
     *
     * @param InCornerRadius The radius in pixels, clamped when drawn to half the shorter side.
     */
    void SetCornerRadius(float InCornerRadius);

    /** @brief How far the fill is rounded at each corner, in pixels. */
    NODISCARD FORCEINLINE float GetCornerRadius() const
    {
        return CornerRadius;
    }

    /**
     * @brief Sets the least height the border is measured at.
     *
     * @param InMinHeight The minimum height in pixels. Zero leaves the border as tall as its content.
     */
    void SetMinHeight(int32 InMinHeight);

    /** @brief The least height the border is measured at, in pixels. */
    NODISCARD FORCEINLINE int32 GetMinHeight() const
    {
        return MinHeight;
    }

    /**
     * @brief Sets the shape the cursor takes over the border, its padding included.
     *
     * @param InCursor The shape to show.
     */
    void SetCursor(ECursor InCursor);

    /** @brief Drops the opinion again, leaving the shape to whatever else is under the cursor. */
    void ClearCursor();

private:
    FFloatColor BackgroundColor;
    float       CornerRadius;
    int32       MinHeight;
    ECursor     Cursor;
    bool        bHasCursor;
};
