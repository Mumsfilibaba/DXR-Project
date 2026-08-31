#pragma once
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Text/IFontFace.h"

/** @brief Called whenever the search text changes, which is every keystroke and every clear. */
DECLARE_DELEGATE(FOnSearchTextChanged, const String& /*SearchText*/);

class APPLICATION_API FSearchBox final : public FVisualElement
{
public:
    struct FDesc
    {
        /** @brief Drawn in place of the text while the field is empty. */
        String HintText = "Search";

        /** @brief The face the text is measured and drawn with. */
        TSharedPtr<IFontFace> Font;

        /** @brief The magnifier drawn at the left, left out of the layout while no texture is set. */
        FUIBrush SearchIcon;

        /** @brief The glyph the clear button draws, replaced by a drawn cross while no texture is set. */
        FUIBrush ClearIcon;

        /** @brief The side of the icon and the clear button squares, in pixels. */
        int32 IconSize = 14;

        /** @brief The space between the field's bounds and its contents. */
        FMargin Padding = FMargin(6, 3, 6, 3);

        /** @brief Fired whenever the search text changes. */
        FOnSearchTextChanged OnTextChanged;
    };

public:
    static TSharedPtr<FSearchBox> Create(const FDesc& Desc);

public:
    FSearchBox();
    virtual ~FSearchBox();

    /**
     * @brief Initializes the search box with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Replaces the search text and fires the change delegate.
     *
     * @param InText The new search text.
     */
    void SetText(const String& InText);

    /** @return The search text being edited, which is empty while the hint shows instead. */
    NODISCARD const String& GetText() const;

    /** @brief Empties the search text and fires the change delegate. */
    void ClearText();

    /** @return True while the field holds no search text, which is when the clear button is hidden. */
    NODISCARD bool IsEmpty() const;

    /** @return The line the text is typed into, which a caller can focus but does not own. */
    NODISCARD FORCEINLINE const TSharedPtr<class FEditableText>& GetEditor() const
    {
        return Editor;
    }

    /** @return The delegate, which fires whenever the search text changes. */
    NODISCARD FORCEINLINE FOnSearchTextChanged& GetOnTextChanged()
    {
        return OnTextChangedDelegate;
    }

    /**
     * @brief The square the magnifier is drawn in.
     *
     * @param Bounds The rectangle the box was arranged into.
     * @return The square, which is empty when the description carried no search icon.
     */
    NODISCARD FRectangle GetSearchIconRectangle(const FRectangle& Bounds) const;

    /**
     * @brief The square a click empties the field in.
     *
     * @param Bounds The rectangle the box was arranged into.
     * @return The square, which is empty while the field holds no text.
     */
    NODISCARD FRectangle GetClearButtonRectangle(const FRectangle& Bounds) const;

private:
    void HandleTextChanged(const String& InText);

    NODISCARD FRectangle GetEditorRectangle(const FRectangle& Bounds) const;

    TSharedPtr<class FEditableText> Editor;
    FUIBrush                       SearchIcon;
    FUIBrush                       ClearIcon;
    FMargin                        Padding;
    int32                          IconSize;
    bool                           bIsHovered;
    FOnSearchTextChanged           OnTextChangedDelegate;
};
