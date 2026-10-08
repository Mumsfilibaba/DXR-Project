#pragma once
#include "Core/CoreTypes.h"

struct APPLICATION_API FUIFrameClock
{
    /** @return The current UI time, in seconds from an arbitrary origin. */
    NODISCARD static double Now();

    /** @brief Pins the UI time to this instant until EndFrame. */
    static void BeginFrame();

    /** @brief Lets the UI time follow the wall clock again. */
    static void EndFrame();
};

struct APPLICATION_API FUIAnimation
{
    /**
     * @brief Starts moving from one value to another.
     *
     * @param InDurationSeconds How long the move takes. Zero or less settles on the target at once.
     * @param InStartValue      The value the move starts at.
     * @param InTargetValue     The value the move ends at.
     */
    void Start(float InDurationSeconds, float InStartValue, float InTargetValue);

    /**
     * @brief Settles on a value at once, ending any move in progress.
     *
     * @param InValue The value to hold.
     */
    void Settle(float InValue);

    /** @return How far the move has run, from zero at the start to one once it is done. */
    NODISCARD float GetProgress() const;

    /** @return The value at the current UI time, which is the target once the move is done. */
    NODISCARD float Evaluate() const;

    /** @return The value at the current UI time eased out, so the move starts quickly and slows down as it settles. */
    NODISCARD float EvaluateEaseOut() const;

    /**
     * @brief Evaluates the move eased out past its target, so it overshoots a little before settling back on it.
     *
     * @param Overshoot How far past the target the move swings, where 1.70158 is the classic ease-out-back and zero is a plain cubic ease-out.
     * @return The value at the current UI time, which can lie beyond the target while the move swings back.
     */
    NODISCARD float EvaluateEaseOutBack(float Overshoot = 1.70158f) const;

    /** @return True while the move has not reached its target. */
    NODISCARD bool IsRunning() const;

    /** @return The value the move ends at. */
    NODISCARD FORCEINLINE float GetTarget() const
    {
        return TargetValue;
    }

    double StartSeconds    = 0.0;
    float  DurationSeconds = 0.0f;
    float  StartValue      = 0.0f;
    float  TargetValue     = 0.0f;
};
