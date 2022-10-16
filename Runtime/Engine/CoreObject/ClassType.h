#pragma once
#include "Engine/EngineModule.h"

struct FClassDescription
{
    /** @brief - Name of the class */
    const CHAR* Name = nullptr;
    /** @brief - Size of the class in bytes */
    uint32 SizeInBytes = 0;
    /** @brief - Alignment of the class in bytes */
    uint32 Alignment = 0;
};

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

    FORCEINLINE const CHAR* GetName() const
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
    const CHAR*       Name;
    const FClassType* SuperClass;
    
    uint32 SizeInBytes;
    uint32 Alignment;
};
