#pragma once
#include "Core/Containers/String.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

class APPLICATION_API FTexturePreview final : public FVisualElement
{
public:
    struct FDesc
    {
        /** @brief The image to show, an invalid brush drawing the empty text instead. */
        FUIBrush Brush;

        /** @brief The face the empty text is measured and drawn with. */
        TSharedPtr<IFontFace> Font = nullptr;

        /** @brief Drawn dimmed in place of the image while the brush holds no texture. */
        String EmptyText = "None";

        /** @brief The edge of the square the image is drawn in, in pixels. */
        int32 PreviewSize = 48;

        /** @brief The edge of the square the hover tip shows the image at, in pixels, where zero shows no tip. */
        int32 ZoomSize = 256;
    };

public:
    static TSharedPtr<FTexturePreview> Create(const FDesc& Desc);

public:
    FTexturePreview();
    virtual ~FTexturePreview();

    /**
     * @brief Initializes the preview with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Replaces the image the preview shows, which is what a host does when a slot is refilled.
     *
     * @param InBrush The image to show, an invalid brush going back to the empty text.
     */
    void SetBrush(const FUIBrush& InBrush);

    /** @return The image the preview shows, which is invalid while the slot is empty. */
    NODISCARD FORCEINLINE const FUIBrush& GetBrush() const
    {
        return Brush;
    }

private:
    NODISCARD FRectangle GetImageRectangle(const FRectangle& Bounds) const;

    FUIBrush              Brush;
    TSharedPtr<IFontFace> Font;
    String                EmptyText;
    int32                 PreviewSize;
    int32                 ZoomSize;
};
