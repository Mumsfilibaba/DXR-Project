#pragma once
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

/** @brief Called when a press and its release both landed on the button. */
DECLARE_DELEGATE(FOnClicked);

class APPLICATION_API FButton final : public FInteractiveElement
{
public:
    struct FDesc
    {
        FDesc()
            : Text()
            , Font(nullptr)
            , Padding(FUIStyle::GetDefault().Metrics.ButtonPadding)
            , CornerRadius(FUIStyle::GetDefault().Metrics.ButtonCornerRadius)
            , HorizontalContentAlignment(EHorizontalAlignment::Center)
            , VerticalContentAlignment(EVerticalAlignment::Center)
            , MinHeight(FUIStyle::GetDefault().Metrics.ButtonHeight)
            , bHasBorder(false)
            , bIsGhost(false)
            , OnClicked()
            , Content(nullptr)
        {
        }

        /**
         * @brief Sets the label, which Initialize wraps in a text block when no content was given.
         *
         * @param InText The text to show.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetText(const String& InText)
        {
            Text = InText;
            return *this;
        }

        /**
         * @brief Sets the face the label is measured and drawn with.
         *
         * @param InFont The face to use.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetFont(const TSharedPtr<IFontFace>& InFont)
        {
            Font = InFont;
            return *this;
        }

        /**
         * @brief Sets the delegate called on a completed click.
         *
         * @param InOnClicked The delegate to call.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetOnClicked(const FOnClicked& InOnClicked)
        {
            OnClicked = InOnClicked;
            return *this;
        }

        String                     Text;
        TSharedPtr<IFontFace>      Font;
        FMargin                    Padding;
        FCornerRadii               CornerRadius;
        EHorizontalAlignment       HorizontalContentAlignment;
        EVerticalAlignment         VerticalContentAlignment;
        int32                      MinHeight;
        bool                       bHasBorder : 1;

        /**
         * @brief True paints no fill at rest, so the frame only appears once the cursor arrives, which is
         * what a button sitting on a tool bar rather than on a panel wants.
         */
        bool bIsGhost : 1;

        FOnClicked                 OnClicked;
        TSharedPtr<FVisualElement> Content;
    };

public:
    static TSharedPtr<FButton> Create(const FDesc& Desc);

public:
    FButton();
    virtual ~FButton();

    /**
     * @brief Initializes the button with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Sets the delegate called on a completed click, replacing whatever the desc carried.
     *
     * @param InOnClicked The delegate to call.
     */
    void SetOnClicked(const FOnClicked& InOnClicked);

    /**
     * @brief Replaces the label, which does nothing when the button was given content of its own.
     *
     * @param InText The text to show.
     */
    void SetText(const String& InText);

    /** @return The label the button draws, which is empty for a button carrying content of its own. */
    NODISCARD const String& GetText() const;

    /**
     * @brief Holds a ghost button lit while something it owns is showing, which is what keeps a drop-down's
     * button from vanishing once the cursor has moved off it and onto the menu. An ordinary button draws
     * from its interaction state alone and ignores this.
     *
     * @param bInIsHighlighted True to draw the button as though the cursor were on it.
     */
    void SetHighlighted(bool bInIsHighlighted);

    /** @return True while the button is held lit, whether or not it is a ghost that would act on it. */
    NODISCARD FORCEINLINE bool IsHighlighted() const
    {
        return bIsHighlighted;
    }

    /** @return The corner radii of the fill in pixels, clamped when drawn to half the shorter side. */
    NODISCARD FORCEINLINE const FCornerRadii& GetCornerRadius() const
    {
        return CornerRadius;
    }

protected:

    // FInteractiveElement Interface
    virtual void OnClicked() override;

private:
    TSharedPtr<class FTextBlock> Label;
    FCornerRadii                 CornerRadius;
    EHorizontalAlignment         HorizontalContentAlignment;
    EVerticalAlignment           VerticalContentAlignment;
    int32                        MinHeight;
    bool                         bHasBorder;
    bool                         bIsGhost;
    bool                         bIsHighlighted;
    FOnClicked                   OnClickedDelegate;
};
