#pragma once
#include "Core/Containers/String.h"
#include "Core/Math/Color.h"
#include "Application/Text/IFontFace.h"
#include "Application/Elements/VisualElement.h"

class APPLICATION_API FTextBlock final : public FVisualElement
{
public:
    struct FDesc
    {
        FDesc()
            : Text()
            , Font(nullptr)
            , ColorAndOpacity(FFloatColor::White)
            , Margin()
        {
        }

        String                Text;
        TSharedPtr<IFontFace> Font;
        FFloatColor           ColorAndOpacity;
        FMargin               Margin;
    };

public:
    static TSharedPtr<FTextBlock> Create(const FDesc& Desc);

public:
    FTextBlock();
    virtual ~FTextBlock();

    /**
     * @brief Initializes the text block with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Replaces the text this element draws.
     *
     * @param InText The new text.
     */
    void SetText(const String& InText);

    /** @brief The text this element draws. */
    NODISCARD FORCEINLINE const String& GetText() const
    {
        return Text;
    }

    /**
     * @brief Sets the color the text is drawn in.
     *
     * @param InColorAndOpacity The new color.
     */
    void SetColorAndOpacity(const FFloatColor& InColorAndOpacity);

    /**
     * @brief Sets the face the text is measured and drawn with.
     *
     * @param InFont The new face, which may be null.
     */
    void SetFont(const TSharedPtr<IFontFace>& InFont);

    /**
     * @brief Sets the space reserved around the text.
     *
     * @param InMargin The new margin.
     */
    void SetMargin(const FMargin& InMargin);

private:
    String                Text;
    TSharedPtr<IFontFace> Font;
    FFloatColor           ColorAndOpacity;
    FMargin               Margin;
};
