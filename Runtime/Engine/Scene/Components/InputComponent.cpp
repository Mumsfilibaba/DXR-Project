#include "InputComponent.h"

FInputComponent::FInputComponent(FActor* InActorOwner)
    : FComponent(InActorOwner)
    , ActionBindings()
{
}

int32 FInputComponent::BindAction(const FStringView& InName, EActionState ActionState, const FInputActionDelegate& Delegate)
{
    const int32 Identifier = ActionBindings.Size();
    ActionBindings.Emplace(InName, ActionState, Delegate);
    return Identifier;
}

int32 FInputComponent::BindAxis(const FStringView& InName, const FInputAxisDelegate& Delegate)
{
    const int32 Identifier = AxisBindings.Size();
    AxisBindings.Emplace(InName, Delegate);
    return Identifier;
}
