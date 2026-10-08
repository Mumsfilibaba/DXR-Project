#pragma once
#include "Core/Containers/String.h"
#include "Core/Math/Math.h"
#include "Core/Math/IntVector2.h"
#include "Core/Templates/NumericLimits.h"

template<typename T>
struct TNumericRange
{
    /** @return The value clamped into the range and snapped to the nearest step, which stays inside the range. */
    NODISCARD T Sanitize(T InValue) const
    {
        T Result = Math::Clamp(InValue, Min, Max);
        if (Step > T(0))
        {
            const double Steps = Math::Round(static_cast<double>(Result - Min) / static_cast<double>(Step));
            Result             = Math::Clamp(static_cast<T>(Min + static_cast<T>(Steps * static_cast<double>(Step))), Min, Max);
        }

        return Result;
    }

    /** @return Where a value lies between the ends, from zero to one, and zero for a range with no width. */
    NODISCARD float GetFraction(T InValue) const
    {
        const double Width = static_cast<double>(Max) - static_cast<double>(Min);
        return Width > 0.0 ? Math::Saturate(static_cast<float>((static_cast<double>(InValue) - static_cast<double>(Min)) / Width)) : 0.0f;
    }

    /** @return The value that far between the ends, before any snapping. */
    NODISCARD T FromFraction(float Fraction) const
    {
        const double Width = static_cast<double>(Max) - static_cast<double>(Min);
        return static_cast<T>(static_cast<double>(Min) + (static_cast<double>(Math::Saturate(Fraction)) * Width));
    }

    /** @return True when both ends are finite and apart, so the value has somewhere to be shown against. */
    NODISCARD bool IsBounded() const
    {
        return Min > TNumericLimits<T>::Lowest() && Max < TNumericLimits<T>::Max() && Max > Min;
    }

    /** @return The value as text with the range's precision. */
    NODISCARD String Format(T InValue) const
    {
        return String::Printf("%.*f", Precision, static_cast<double>(InValue));
    }

    T     Min       = T(0);
    T     Max       = T(1);
    T     Step      = T(0);
    int32 Precision = 2;
};

template<typename T>
class TValueScrubber
{
public:
    TValueScrubber()
        : StartValue(T(0))
        , StartPosition()
        , bIsPressed(false)
        , bHasScrubbed(false)
    {
    }

    NODISCARD FORCEINLINE bool IsPressed()   const { return bIsPressed; }
    NODISCARD FORCEINLINE bool HasScrubbed() const { return bHasScrubbed; }

    NODISCARD FORCEINLINE T GetStartValue() const { return StartValue; }

    /** @brief Starts tracking a press at a position, over a value. */
    void Begin(const IntVector2& Position, T Value)
    {
        StartValue    = Value;
        StartPosition = Position;
        bIsPressed    = true;
        bHasScrubbed  = false;
    }

    /**
     * @brief Follows the cursor during a press.
     *
     * @param Position  Where the cursor is.
     * @param Threshold How far it has to travel along X before the press becomes a scrub.
     * @param OutTravel How far it has travelled along X since the press, written every call.
     * @return True once the press is a scrub, which it stays until the next Begin.
     */
    bool Update(const IntVector2& Position, int32 Threshold, int32& OutTravel)
    {
        OutTravel = Position.X - StartPosition.X;
        if (!bHasScrubbed && Math::Abs(OutTravel) < Threshold)
        {
            return false;
        }

        bHasScrubbed = true;
        return true;
    }

    /**
     * @brief Ends the press.
     *
     * @return True when it had become a scrub, false when it was a click.
     */
    bool End()
    {
        bIsPressed = false;
        return bHasScrubbed;
    }

private:
    T          StartValue;
    IntVector2 StartPosition;
    bool       bIsPressed;
    bool       bHasScrubbed;
};
