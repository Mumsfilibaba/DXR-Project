#pragma once 
#include "Component.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/String.h"

DECLARE_DELEGATE(FInputActionDelegate);
DECLARE_DELEGATE(FInputAxisDelegate, float);

template<typename ClassType>
using ActionFunction = typename TMemberFunctionType<false, ClassType, void(void)>::Type;

template<typename ClassType>
using AxisFunction = typename TMemberFunctionType<false, ClassType, void(float)>::Type;

enum class EActionState
{
    Unknown = 0,
    Pressed,
    Repeat,
    Released,
    DoubleClick,
};

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

struct FAxisInputBinding
{
    FAxisInputBinding() = default;

    FAxisInputBinding(const FStringView& InName, const FInputAxisDelegate& InActionDelegate)
        : Name(InName)
        , ActionDelegate(InActionDelegate)
    {
    }

    FString            Name;
    FInputAxisDelegate ActionDelegate;
};

class ENGINE_API FInputComponent : public FComponent
{
public:
    FOBJECT_DECLARE_CLASS(FInputComponent, FComponent);

    FInputComponent(const FObjectInitializer& ObjectInitializer);
    ~FInputComponent() = default;

    int32 BindAction(const FStringView& InName, EActionState ActionState, const FInputActionDelegate& Delegate);

    int32 BindAxis(const FStringView& InName, const FInputAxisDelegate& Delegate);

    template<typename ClassType>
    FORCEINLINE int32 BindAction(const FStringView& InName, EActionState ActionState, ClassType* Actor, ActionFunction<ClassType> ActorFunction)
    {
        const FInputActionDelegate NewDelegate = FInputActionDelegate::CreateRaw(Actor, ActorFunction);
        return BindAction(InName, ActionState, NewDelegate);
    }

    template<typename ClassType>
    FORCEINLINE int32 BindAxis(const FStringView& InName, ClassType* Actor, AxisFunction<ClassType> ActorFunction)
    {
        const FInputAxisDelegate NewDelegate = FInputAxisDelegate::CreateRaw(Actor, ActorFunction);
        return BindAxis(InName, NewDelegate);
    }

    TArray<FAxisInputBinding>   AxisBindings;
    TArray<FActionInputBinding> ActionBindings;
};
