#pragma once
#include <Core/Containers/String.h>
#include <Core/Delegates/Delegate.h>
#include <Application/Elements/InteractiveElement.h>
#include <Application/Text/IFontFace.h>

/** @brief Called with the index the entry was created with. */
DECLARE_DELEGATE(FOnNavButtonClicked, int32);

class FNavButton final : public FInteractiveElement
{
public:
    struct FDesc
    {
        FDesc()
            : Label()
            , Font(nullptr)
            , Index(0)
        {
        }

        String                Label;
        TSharedPtr<IFontFace> Font;
        int32                 Index;
    };

public:
    static TSharedPtr<FNavButton> Create(const FDesc& Desc);

public:
    FNavButton();
    virtual ~FNavButton();

    /**
     * @brief Initializes the entry with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Marks the entry as the one whose scene is showing, which draws it in the accent color.
     *
     * @param bInIsSelected True when this entry owns the content area.
     */
    void SetSelected(bool bInIsSelected);

    /** @return True while this entry's scene is showing, which is what draws it in the accent color. */
    NODISCARD FORCEINLINE bool IsSelected() const
    {
        return bIsSelected;
    }

    /**
     * @brief Sets the delegate called when the entry is clicked.
     *
     * @param InOnClicked The delegate to call, with the entry's index.
     */
    void SetOnClicked(const FOnNavButtonClicked& InOnClicked);

protected:

    // FInteractiveElement Interface
    virtual void OnClicked() override;

private:
    TSharedPtr<class FTextBlock> Label;
    FOnNavButtonClicked          OnClickedDelegate;
    int32                        Index;
    bool                         bIsSelected;
};
