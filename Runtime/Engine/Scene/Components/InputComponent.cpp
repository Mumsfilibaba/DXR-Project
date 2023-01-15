#include "InputComponent.h"

FInputComponent::FInputComponent(FActor* InActorOwner)
    : FComponent(InActorOwner)
{ }

FActionInputBinding FInputComponent::BindAction(const FStringView& InName, EActionState ActionState, FActor* Actor)
{
    return FActionInputBinding();
}

FActionInputBinding FInputComponent::AddActionBinding(const FStringView& InName, EActionState ActionState, const FInputActionDelegate& Delegate)
{
    return FActionInputBinding();
}
