#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Elements/CompoundElement.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Menus/Menu.h"
#include "Application/Menus/MenuAnchor.h"
#include "Application/Text/IFontFace.h"

/** @brief Called when a different option is chosen from the drop-down. */
DECLARE_DELEGATE(FOnComboSelectionChanged, int32 /*SelectedIndex*/);

class FComboBox;

class APPLICATION_API FComboBoxButton final : public FInteractiveElement
{
public:
    static TSharedPtr<FComboBoxButton> Create(const TSharedPtr<IFontFace>& InFont);

public:
    FComboBoxButton();
    virtual ~FComboBoxButton();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Tells the button which anchor to toggle, which the combo box does as it builds it.
     *
     * @param InAnchor The anchor holding the button.
     */
    void SetAnchor(FMenuAnchor* InAnchor);

    /**
     * @brief Sets the text shown while the drop-down is closed.
     *
     * @param InText The selected option, or the placeholder when nothing is selected.
     */
    void SetText(const String& InText);

    /**
     * @brief Sets the width the widest option needs, so the button does not resize as the selection changes.
     *
     * @param InTextWidth The width in pixels.
     */
    void SetReservedTextWidth(int32 InTextWidth);

protected:

    // FInteractiveElement Interface
    virtual void OnClicked() override;

private:
    String                Label;
    TSharedPtr<IFontFace> Font;
    int32                 ReservedTextWidth;

    FMenuAnchor* Anchor;
};

class APPLICATION_API FComboBox final : public FCompoundElement
{
public:
    struct FDesc
    {
        FDesc()
            : Options()
            , SelectedIndex(-1)
            , Font(nullptr)
            , PlaceholderText()
            , OnSelectionChanged()
        {
        }

        /**
         * @brief Sets the options the drop-down offers.
         *
         * @param InOptions The options, in the order they are listed.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetOptions(const TArray<String>& InOptions)
        {
            Options = InOptions;
            return *this;
        }

        /**
         * @brief Sets the face the button and the options are drawn with.
         *
         * @param InFont The face to use.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetFont(const TSharedPtr<IFontFace>& InFont)
        {
            Font = InFont;
            return *this;
        }

        /** @brief The options the drop-down offers, in the order they are listed. */
        TArray<String> Options;

        /** @brief The option selected at the start, or -1 for none. */
        int32 SelectedIndex;

        /** @brief The face the button and the options are drawn with. */
        TSharedPtr<IFontFace> Font;

        /** @brief Shown while nothing is selected. */
        String PlaceholderText;

        /** @brief Fired when a different option is chosen from the drop-down. */
        FOnComboSelectionChanged OnSelectionChanged;
    };

public:
    static TSharedPtr<FComboBox> Create(const FDesc& Desc);

public:
    FComboBox();
    virtual ~FComboBox();

    /**
     * @brief Initializes the combo box with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    /**
     * @brief Chooses an option and fires the delegate, which an out of range index clears the selection with.
     *
     * @param Index The option to select.
     */
    void SetSelectedIndex(int32 Index);

    /** @return The index of the chosen option, or -1 when nothing is selected. */
    NODISCARD FORCEINLINE int32 GetSelectedIndex() const
    {
        return SelectedIndex;
    }

    /** @return The closed button's text: the selected option, or the placeholder when none is selected. */
    NODISCARD const String& GetSelectedText() const;

    /**
     * @brief Replaces the options and clears the selection.
     *
     * @param InOptions The options, in the order they are listed.
     */
    void SetOptions(const TArray<String>& InOptions);

    /** @return The options the drop-down offers, in the order they are listed. */
    NODISCARD FORCEINLINE const TArray<String>& GetOptions() const
    {
        return Options;
    }

    /** @brief Opens the drop-down, as a click on the button does. */
    void OpenMenu();

    /** @brief Closes the drop-down. */
    void CloseMenu();

    /** @return True while the drop-down is open and its options are showing. */
    NODISCARD bool IsMenuOpen() const;

private:
    NODISCARD TSharedPtr<FVisualElement> BuildMenu();

    void RefreshButtonText();

    TArray<String>              Options;
    int32                       SelectedIndex;
    String                      PlaceholderText;
    TSharedPtr<IFontFace>       Font;
    TSharedPtr<FComboBoxButton> Button;
    TSharedPtr<FMenuAnchor>     Anchor;
    FOnComboSelectionChanged    OnSelectionChangedDelegate;
};
