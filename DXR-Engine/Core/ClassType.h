#pragma once
#include "Core.h"

// ClassType stores info about a class, for now inheritance
class ClassType
{
public:
    ClassType(
        const char* InName, 
        const ClassType* InSuperClass,
        uint32 SizeInBytes);

    ~ClassType() = default;

    bool IsSubClassOf(const ClassType* Class) const;

    template<typename T>
    FORCEINLINE bool IsSubClassOf() const
    {
        return IsSubClassOf(T::GetStaticClass());
    }

    const char* GetName() const { return Name; }
    const ClassType* GetSuperClass() const { return SuperClass; }

    uint32 GetSizeInBytes() const { return SizeInBytes; }

private:
    const char*      Name;
    const ClassType* SuperClass;
    const uint32     SizeInBytes;
};