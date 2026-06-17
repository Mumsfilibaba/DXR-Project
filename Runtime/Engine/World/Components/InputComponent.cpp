#include "Engine/World/Components/InputComponent.h"

FOBJECT_IMPLEMENT_CLASS(FInputComponent);

FInputComponent::FInputComponent(const FObjectInitializer& ObjectInitializer)
    : FActorComponent(ObjectInitializer)
    , ActionBindings()
{
    bIsTickable  = false;
    bIsStartable = false;
}

int32 FInputComponent::BindAxis(const StringView& InName, const FInputAxisDelegate& Delegate)
{
    const int32 Identifier = AxisBindings.Size();
    AxisBindings.Emplace(InName, Delegate);
    return Identifier;
}

int32 FInputComponent::BindAction(const StringView& InName, EActionState ActionState, const FInputActionDelegate& Delegate)
{
    const int32 Identifier = ActionBindings.Size();
    ActionBindings.Emplace(InName, ActionState, Delegate);
    return Identifier;
}
