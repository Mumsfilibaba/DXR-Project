#pragma once
#include "Engine/EngineModule.h"

struct FClassInfo
{
    const CHAR* Name = nullptr;

    uint32 SizeOf    = 0;
    uint32 Alignment = 0;
};

class ENGINE_API FClass
{
public:
    FClass(const FClass* InSuperClass, const FClassInfo& ClassInfo);
    ~FClass() = default;

    bool IsSubClassOf(const FClass* Class) const;

    template<typename T>
    FORCEINLINE bool IsSubClassOf() const
    {
        return IsSubClassOf(T::GetStaticClass());
    }

    FORCEINLINE const CHAR* GetName() const
    {
        return ClassInfo.Name;
    }

    FORCEINLINE const FClass* GetSuperClass() const
    {
        return SuperClass;
    }

    FORCEINLINE uint32 GetSizeOf() const
    {
        return ClassInfo.SizeOf;
    }

private:
    const FClass* SuperClass;
    FClassInfo    ClassInfo;
};
