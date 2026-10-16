#pragma once
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"

class FDisplayDPI
{
public:
    static constexpr uint32 Standard = 96;

    NODISCARD static constexpr FDisplayDPI FromScale(float Scale) noexcept
    {
        return FDisplayDPI(Scale > 0.0f 
            ? static_cast<uint32>(Scale * static_cast<float>(Standard) + 0.5f) 
            : 0u);
    }

public:
    constexpr FDisplayDPI() noexcept = default;

    constexpr explicit FDisplayDPI(uint32 InDotsPerInch) noexcept
        : DotsPerInch(InDotsPerInch)
    {
    }

    NODISCARD constexpr uint32 GetDotsPerInch() const noexcept
    {
        return (DotsPerInch > 0) ? DotsPerInch : Standard;
    }

    NODISCARD constexpr float GetScale() const noexcept
    {
        return static_cast<float>(GetDotsPerInch()) / static_cast<float>(Standard);
    }

    constexpr bool operator==(const FDisplayDPI& Other) const noexcept = default;

private:
    uint32 DotsPerInch = 0;
};
