#pragma once
#include "Engine/EngineModule.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ClassDescription

struct FClassDescription
{
    /** Name of the class */
    const char* Name = nullptr;
    /** Size of the class in bytes */
    uint32 SizeInBytes = 0;
    /** Alignment of the class in bytes */
    uint32 Alignment = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ClassType - Stores info about a class, for now inheritance

class ENGINE_API FClassType
{
public:

    FClassType(const FClassType* InSuperClass, const FClassDescription& ClassDescription);
    ~FClassType() = default;

    bool IsSubClassOf(const FClassType* Class) const;

    template<typename T>
    FORCEINLINE bool IsSubClassOf() const
    {
        return IsSubClassOf(T::GetStaticClass());
    }

    FORCEINLINE const char* GetName() const
    {
        return Name;
    }

    FORCEINLINE const FClassType* GetSuperClass() const
    {
        return SuperClass;
    }

    FORCEINLINE uint32 GetSizeInBytes() const
    {
        return SizeInBytes;
    }

private:
    const char*       Name;
    const FClassType* SuperClass;
    
    uint32 SizeInBytes;
    uint32 Alignment;
};
