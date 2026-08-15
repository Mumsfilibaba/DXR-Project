#pragma once
#include "CoreApplication/PlatformInterface/InputCodes.h"

struct AnalogInput
{
    // XInput-normalized defaults (7849/32767, 8689/32767, 30/255)
    static constexpr float DefaultLeftThumbDeadzone  = 7849.0f / 32767.0f;
    static constexpr float DefaultRightThumbDeadzone = 8689.0f / 32767.0f;
    static constexpr float DefaultTriggerDeadzone    = 30.0f / 255.0f;

    static COREAPPLICATION_API float GetDeadzone(EAnalogSourceName::Type Source);
    static COREAPPLICATION_API float ApplyDeadzone(float Value, float Deadzone);
};
