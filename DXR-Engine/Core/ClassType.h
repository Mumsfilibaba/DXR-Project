#pragma once
#include "Core.h"

// ClassType stores info about a class, for now inheritance
class ClassType
{
public:
    ClassType(
        const Char* InName, 
        const ClassType* InSuperClass,
        UInt32 SizeInBytes);

    ~ClassType() = default;

    Bool IsSubClassOf(const ClassType* Class) const;

    template<typename T>
    FORCEINLINE Bool IsSubClassOf() const
    {
        return IsSubClassOf(T::GetStaticClass());
    }

    const Char* GetName() const { return Name; }
    const ClassType* GetSuperClass() const { return SuperClass; }

    UInt32 GetSizeInBytes() const { return SizeInBytes; }

private:
    const Char*      Name;
    const ClassType* SuperClass;
    const UInt32     SizeInBytes;
};