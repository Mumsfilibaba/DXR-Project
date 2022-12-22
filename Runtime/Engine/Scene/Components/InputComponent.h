#pragma once 
#include "Component.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/String.h"

DECLARE_DELEGATE(FInputActionDelegate);

enum class EActionState
{
    Pressed  = 1,
    Released = 2
};

struct ENGINE_API FActionInputBinding
{
    FActionInputBinding(const FStringView& InName)
        : Name(InName)
    { }

    FString              Name;
    FInputActionDelegate ActionDelegate;
};

class ENGINE_API FInputComponent
    : public FComponent
{
    FOBJECT_BODY(FInputComponent, FComponent);

public:
    FInputComponent(FActor* InActorOwner);
    ~FInputComponent() = default;

    FActionInputBinding& BindAction(const FStringView& InName, EActionState ActionState, FActor* Actor);
    FActionInputBinding& AddActionBinding(const FStringView& InName, EActionState ActionState, const FInputActionDelegate& Delegate);

private:
    TArray<FActionInputBinding> ActionBindings;
};