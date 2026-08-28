#pragma once
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Text/IFontFace.h"

enum class ECheckBoxState : uint8
{
    Unchecked,
    Checked,
    Undetermined,
};

/** @brief Called with the state the box moved to. */
DECLARE_DELEGATE(FOnCheckStateChanged, ECheckBoxState /*NewState*/);

class APPLICATION_API FCheckBox final : public FInteractiveElement
{
public:
    struct FDesc
    {
        FDesc()
            : Text()
            , Font(nullptr)
            , InitialState(ECheckBoxState::Unchecked)
            , BoxSize(16)
            , LabelSpacing(8)
            , bIsTriState(false)
            , OnStateChanged()
            , Label(nullptr)
        {
        }

        /**
         * @brief Sets the label placed beside the box.
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

        /** @brief The label placed beside the box, ignored when Label is set. */
        String Text;

        /** @brief The face the label is measured and drawn with. */
        TSharedPtr<IFontFace> Font;

        /** @brief The state the box starts in. */
        ECheckBoxState InitialState;

        /** @brief The side of the square the check mark is drawn in, in pixels. */
        int32 BoxSize;

        /** @brief The gap between the box and its label, in pixels. */
        int32 LabelSpacing;

        /** @brief True to let a click walk through Undetermined, which a mixed selection needs. */
        bool bIsTriState : 1;

        /** @brief Fired with the state the box moved to. */
        FOnCheckStateChanged OnStateChanged;

        /** @brief An element to place beside the box in place of Text, which it takes precedence over. */
        TSharedPtr<FVisualElement> Label;
    };

public:
    static TSharedPtr<FCheckBox> Create(const FDesc& Desc);

public:
    FCheckBox();
    virtual ~FCheckBox();

    /**
     * @brief Initializes the check box with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Sets the state without firing the delegate, which is how a host pushes a model value in.
     *
     * @param InState The state to move to.
     */
    void SetCheckState(ECheckBoxState InState);

    /** @return The state the box is in, which is Undetermined only on a tri-state box. */
    NODISCARD FORCEINLINE ECheckBoxState GetCheckState() const
    {
        return CheckState;
    }

    /** @return True only for Checked, so Undetermined reads as unchecked. */
    NODISCARD FORCEINLINE bool IsChecked() const
    {
        return CheckState == ECheckBoxState::Checked;
    }

    /**
     * @brief Gets where the box itself is drawn, which is what a hit test against the mark needs.
     *
     * @return The square the box occupies, left-aligned and centered in the row, excluding the label.
     */
    NODISCARD FRectangle GetBoxBounds() const;

protected:

    // FInteractiveElement Interface
    virtual void OnClicked() override;

private:
    NODISCARD ECheckBoxState GetNextState() const;

    TSharedPtr<class FTextBlock> LabelText;
    ECheckBoxState               CheckState;
    int32                        BoxSize;
    int32                        LabelSpacing;
    bool                         bIsTriState;
    FOnCheckStateChanged         OnStateChangedDelegate;
};
