#pragma once
#include "Core/Time/Timespan.h"

#include "Engine/CoreObject/CoreObject.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

class FActor;

class ENGINE_API FComponent
    : public FCoreObject
{
    CORE_OBJECT(FComponent, FCoreObject);

public:
    FComponent(FActor* InActorOwner);
    FComponent(FActor* InActorOwner, bool bInIsStartable, bool bInIsTickable);
    virtual ~FComponent() = default;

    /**
     * @brief - Start component, called in the beginning of the run, perform initialization here
     */
    virtual void Start() { }

    /**
     * @brief           - Tick component, should be called once every frame 
     * @param DeltaTime - Time since the last call to tick
     */
    virtual void Tick(FTimespan DeltaTime) { }

    /**
     * @brief  - Retrieve the actor that the component belongs to
     * @return - Returns a pointer to the actor that the component belongs to
     */
    FORCEINLINE FActor* GetActor() const
    {
        return ActorOwner;
    }

    /**
     * @brief  - Check if Start should be called on the component
     * @return - Returns true if the component's Start-method should be called 
     */
    FORCEINLINE bool IsStartable() const
    {
        return bIsStartable;
    }

    /**
     * @brief  - Check if Tick should be called on the component
     * @return - Returns true if the component's Tick-method should be called
     */
    FORCEINLINE bool IsTickable() const
    {
        return bIsTickable;
    }

private:
    FActor* ActorOwner = nullptr;

    bool bIsStartable : 1;
    bool bIsTickable  : 1;
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif