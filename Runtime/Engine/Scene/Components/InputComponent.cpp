#include "InputComponent.h"

FOBJECT_IMPLEMENT_CLASS(FInputComponent);

FInputComponent::FInputComponent(const FObjectInitializer& ObjectInitializer)
    : FComponent(ObjectInitializer)
    , ActionBindings()
{
    bIsTickable  = false;
    bIsStartable = false;
}

int32 FInputComponent::BindAxis(const FStringView& InName, const FInputAxisDelegate& Delegate)
{
    const int32 Identifier = AxisBindings.Size();
    AxisBindings.Emplace(InName, Delegate);
    return Identifier;
}

int32 FInputComponent::BindAction(const FStringView& InName, EActionState ActionState, const FInputActionDelegate& Delegate)
{
    const int32 Identifier = ActionBindings.Size();
    ActionBindings.Emplace(InName, ActionState, Delegate);
    return Identifier;
}
