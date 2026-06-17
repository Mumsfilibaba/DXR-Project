#pragma once 
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/String.h"
#include "Engine/World/Components/ActorComponent.h"

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

    FActionInputBinding(const StringView& InName, EActionState InActionState, const FInputActionDelegate& InActionDelegate)
        : Name(InName)
        , ActionState(InActionState)
        , ActionDelegate(InActionDelegate)
    {
    }

    String               Name;
    EActionState         ActionState;
    FInputActionDelegate ActionDelegate;
};

struct FAxisInputBinding
{
    FAxisInputBinding() = default;

    FAxisInputBinding(const StringView& InName, const FInputAxisDelegate& InActionDelegate)
        : Name(InName)
        , ActionDelegate(InActionDelegate)
    {
    }

    String             Name;
    FInputAxisDelegate ActionDelegate;
};

class ENGINE_API FInputComponent : public FActorComponent
{
public:
    FOBJECT_DECLARE_CLASS(FInputComponent, FActorComponent);

    FInputComponent(const FObjectInitializer& ObjectInitializer);
    ~FInputComponent() = default;

    int32 BindAxis(const StringView& InName, const FInputAxisDelegate& Delegate);
    int32 BindAction(const StringView& InName, EActionState ActionState, const FInputActionDelegate& Delegate);

    template<typename ClassType>
    FORCEINLINE int32 BindAction(const StringView& InName, EActionState ActionState, ClassType* Actor, ActionFunction<ClassType> ActorFunction)
    {
        const FInputActionDelegate NewDelegate = FInputActionDelegate::CreateRaw(Actor, ActorFunction);
        return BindAction(InName, ActionState, NewDelegate);
    }

    template<typename ClassType>
    FORCEINLINE int32 BindAxis(const StringView& InName, ClassType* Actor, AxisFunction<ClassType> ActorFunction)
    {
        const FInputAxisDelegate NewDelegate = FInputAxisDelegate::CreateRaw(Actor, ActorFunction);
        return BindAxis(InName, NewDelegate);
    }

    TArray<FAxisInputBinding>   AxisBindings;
    TArray<FActionInputBinding> ActionBindings;
};
