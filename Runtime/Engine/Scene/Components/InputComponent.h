#pragma once 
#include "Component.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/String.h"

enum class EActionState
{
    Pressed  = 1,
    Released = 2
};


DECLARE_DELEGATE(FInputActionDelegate);

struct FActionInputBinding
{
    FActionInputBinding() = default;

    FActionInputBinding(const FStringView& InName, EActionState InActionState, const FInputActionDelegate& InActionDelegate)
        : Name(InName)
        , ActionState(InActionState)
        , ActionDelegate(InActionDelegate)
    {
    }

    FString              Name;
    EActionState         ActionState;
    FInputActionDelegate ActionDelegate;
};


typedef int32 FActionBindingIdentifier;

class ENGINE_API FInputComponent : public FComponent
{
    FOBJECT_BODY(FInputComponent, FComponent);

public:
    FInputComponent(FActor* InActorOwner);
    ~FInputComponent() = default;

    template<typename ClassType>
    using ActionFunction = typename TMemberFunctionType<false, ClassType, void(void)>::Type;

    template<typename ClassType>
    FORCEINLINE FActionBindingIdentifier BindAction(const FStringView& InName, EActionState ActionState, ClassType* Actor, ActionFunction<ClassType> ActorFunction)
    {
        const FInputActionDelegate NewDelegate = FInputActionDelegate::CreateRaw(Actor, ActorFunction);
        return BindAction(InName, ActionState, NewDelegate);
    }

    FActionBindingIdentifier BindAction(const FStringView& InName, EActionState ActionState, const FInputActionDelegate& Delegate);

    TArray<FActionInputBinding> ActionBindings;
};