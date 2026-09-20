#pragma once
#include "Core/Math/Color.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/CompoundElement.h"

class APPLICATION_API FBorder final : public FCompoundElement
{
public:
    struct FDesc
    {
        FDesc()
            : BackgroundColor(0.0f, 0.0f, 0.0f, 0.0f)
            , BorderColor(0.0f, 0.0f, 0.0f, 0.0f)
            , Padding()
            , CornerRadius(0.0f)
            , BorderThickness(0.0f)
            , MinWidth(0)
            , MinHeight(0)
            , Cursor(ECursor::None)
            , bHasCursor(false)
            , bDrawBorderOverContent(false)
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
        FFloatColor                BorderColor;
        FMargin                    Padding;
        FCornerRadii               CornerRadius;
        float                      BorderThickness;
        int32                      MinWidth;
        int32                      MinHeight;
        ECursor                    Cursor;
        bool                       bHasCursor : 1;

        /** @brief Draws the stroke after the child, so a child that fills the border cannot paint over it. */
        bool                       bDrawBorderOverContent : 1;

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
    virtual void SetOuterCornerRadius(float InCornerRadius) override;

    /**
     * @brief Sets the fill drawn behind the child.
     *
     * @param InBackgroundColor The fill color. A zero alpha draws nothing.
     */
    void SetBackgroundColor(const FFloatColor& InBackgroundColor);

    /** @return The fill color drawn behind the child, whose zero alpha means nothing is drawn. */
    NODISCARD FORCEINLINE const FFloatColor& GetBackgroundColor() const
    {
        return BackgroundColor;
    }

    /**
     * @brief Sets the stroke drawn inward from the edge, which needs a thickness to show.
     *
     * @param InBorderColor The stroke color. A zero alpha draws nothing.
     */
    void SetBorderColor(const FFloatColor& InBorderColor);

    /** @return The stroke color drawn inward from the edge, whose zero alpha means nothing is drawn. */
    NODISCARD FORCEINLINE const FFloatColor& GetBorderColor() const
    {
        return BorderColor;
    }

    /**
     * @brief Sets how far the fill is rounded at each corner.
     *
     * @param InCornerRadius The radius in pixels, clamped when drawn to half the shorter side.
     */
    void SetCornerRadius(const FCornerRadii& InCornerRadius);

    /** @return The corner radii of the fill in pixels, clamped when drawn to half the shorter side. */
    NODISCARD FORCEINLINE const FCornerRadii& GetCornerRadius() const
    {
        return CornerRadius;
    }

    /**
     * @brief Sets how wide the stroke is.
     *
     * @param InBorderThickness The width in pixels. Zero draws no stroke.
     */
    void SetBorderThickness(float InBorderThickness);

    /** @return The width of the stroke in pixels, where zero draws no stroke. */
    NODISCARD FORCEINLINE float GetBorderThickness() const
    {
        return BorderThickness;
    }

    /**
     * @brief Sets the least width the border is measured at, which is how a border is held to a fixed size
     * inside a box that would otherwise size it to its content.
     *
     * @param InMinWidth The minimum width in pixels. Zero leaves the border as wide as its content.
     */
    void SetMinWidth(int32 InMinWidth);

    /** @return The least width in pixels the border measures at, zero leaving it as wide as its content. */
    NODISCARD FORCEINLINE int32 GetMinWidth() const
    {
        return MinWidth;
    }

    /**
     * @brief Sets the least height the border is measured at.
     *
     * @param InMinHeight The minimum height in pixels. Zero leaves the border as tall as its content.
     */
    void SetMinHeight(int32 InMinHeight);

    /** @return The least height in pixels the border measures at, zero leaving it as tall as its content. */
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

    /**
     * @brief Sets whether the stroke is drawn after the child rather than before it.
     *
     * @param bInDrawBorderOverContent True to draw the stroke last, which is what a view that fills the
     * border edge to edge needs so its own fills cannot bury the stroke.
     */
    void SetDrawBorderOverContent(bool bInDrawBorderOverContent);

    /** @return True when the stroke is drawn after the child. */
    NODISCARD FORCEINLINE bool DrawsBorderOverContent() const
    {
        return bDrawBorderOverContent;
    }

private:
    FFloatColor  BackgroundColor;
    FFloatColor  BorderColor;
    FCornerRadii CornerRadius;
    float        BorderThickness;
    int32        MinWidth;
    int32        MinHeight;
    ECursor      Cursor;
    bool         bHasCursor;
    bool         bDrawBorderOverContent;
};
