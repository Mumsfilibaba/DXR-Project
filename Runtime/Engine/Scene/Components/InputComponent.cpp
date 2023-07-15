#include "InputComponent.h"

FInputComponent::FInputComponent(FActor* InActorOwner)
    : FComponent(InActorOwner)
    , ActionBindings()
{
}

FActionBindingIdentifier FInputComponent::BindAction(const FStringView& InName, EActionState ActionState, const FInputActionDelegate& Delegate)
{
    FActionBindingIdentifier Identifier = ActionBindings.Size();
    ActionBindings.Emplace(InName, ActionState, Delegate);
    return Identifier;
}
