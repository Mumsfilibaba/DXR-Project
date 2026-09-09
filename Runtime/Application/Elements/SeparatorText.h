#pragma once
#include "Core/Containers/String.h"
#include "Core/Math/Color.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Layout/LayoutTypes.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

class APPLICATION_API FSeparatorText final : public FVisualElement
{
public:
    struct FDesc
    {
        /** @brief The caption the rule is broken around. */
        String Text;

        /** @brief The face the caption is measured and drawn with. */
        TSharedPtr<IFontFace> Font = nullptr;

        /** @brief The tint of that caption. */
        FFloatColor TextColor = FUIStyle::GetDefault().Colors.Text;

        /** @brief The tint of the rule either side of it. */
        FFloatColor RuleColor = FFloatColor(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f);

        /** @brief How thick that rule is, in pixels. */
        float RuleThickness = 4.0f;

        /** @brief The space between the element's edges and the rule. */
        FMargin Padding;

        /** @brief Where the caption sits across the width, as a fraction from left to right. */
        float TextAlignment = 0.1f;

        /** @brief How far the rule stops short of the caption on either side, in pixels. */
        int32 TextGap = 8;
    };

public:
    static TSharedPtr<FSeparatorText> Create(const FDesc& Desc);

public:
    FSeparatorText();
    virtual ~FSeparatorText();

    /**
     * @brief Initializes the labelled rule with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Replaces the caption, which is what renaming the thing the group describes needs.
     *
     * @param InText The new caption.
     */
    void SetText(const String& InText);

    /** @return The caption the rule is broken around. */
    NODISCARD FORCEINLINE const String& GetText() const
    {
        return Text;
    }

private:
    NODISCARD int32 GetTextWidth() const;

    String                Text;
    TSharedPtr<IFontFace> Font;
    FFloatColor           TextColor;
    FFloatColor           RuleColor;
    FMargin               Padding;
    float                 RuleThickness;
    float                 TextAlignment;
    int32                 TextGap;
};
