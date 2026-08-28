#pragma once
#include "Application/Elements/VisualElement.h"

class APPLICATION_API FSpacer final : public FVisualElement
{
public:

    /**
     * @brief Creates a spacer of a given size.
     *
     * @param InSize The space the spacer asks for.
     * @return The new spacer.
     */
    static TSharedPtr<FSpacer> Create(const IntVector2& InSize);

    /**
     * @brief Creates a spacer that is only as wide as it is asked to be.
     *
     * @param InWidth The width the spacer asks for.
     * @return The new spacer, which asks for no height.
     */
    static TSharedPtr<FSpacer> CreateHorizontal(int32 InWidth);

    /**
     * @brief Creates a spacer that is only as tall as it is asked to be.
     *
     * @param InHeight The height the spacer asks for.
     * @return The new spacer, which asks for no width.
     */
    static TSharedPtr<FSpacer> CreateVertical(int32 InHeight);

public:
    FSpacer();
    virtual ~FSpacer();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;

    /**
     * @brief Sets the space the spacer asks for.
     *
     * @param InSize The new size.
     */
    void SetSize(const IntVector2& InSize);

    /** @return The space the spacer asks for, in pixels on each axis. */
    NODISCARD FORCEINLINE const IntVector2& GetSize() const
    {
        return Size;
    }

private:
    IntVector2 Size;
};
