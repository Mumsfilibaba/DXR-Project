#include "CoreApplication/PlatformInterface/AnalogDeadzones.h"
#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"

static TAutoConsoleVariable<float> CVarLeftThumbDeadzone(
    "Input.LeftThumbDeadzone",
    "Normalized left-stick deadzone (XInput-equivalent default)",
    AnalogInput::DefaultLeftThumbDeadzone,
    0.0f,
    1.0f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarRightThumbDeadzone(
    "Input.RightThumbDeadzone",
    "Normalized right-stick deadzone (XInput-equivalent default)",
    AnalogInput::DefaultRightThumbDeadzone,
    0.0f,
    1.0f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarTriggerDeadzone(
    "Input.TriggerDeadzone",
    "Normalized trigger deadzone (XInput-equivalent default)",
    AnalogInput::DefaultTriggerDeadzone,
    0.0f,
    1.0f,
    EConsoleVariableFlags::Default);

float AnalogInput::GetDeadzone(EAnalogSourceName::Type Source)
{
    switch (Source)
    {
    case EAnalogSourceName::LeftThumbX:
    case EAnalogSourceName::LeftThumbY:
        return CVarLeftThumbDeadzone.GetValue();

    case EAnalogSourceName::RightThumbX:
    case EAnalogSourceName::RightThumbY:
        return CVarRightThumbDeadzone.GetValue();

    case EAnalogSourceName::LeftTrigger:
    case EAnalogSourceName::RightTrigger:
        return CVarTriggerDeadzone.GetValue();

    default:
        return 0.0f;
    }
}

float AnalogInput::ApplyDeadzone(float Value, float Deadzone)
{
    return Math::Abs(Value) > Deadzone ? Value : 0.0f;
}
