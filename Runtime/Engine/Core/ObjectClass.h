#pragma once
#include "Engine/EngineModule.h"

class FObjectClass;
class FObjectInitializer;

// This information is stored globally via the FOBJECT_IMPLEMENT_CLASS macro.
// It is used to ensure that the class is only implemented once and to prevent
// that a function is not implemented multiple times
struct FGlobalObjectClassInfo
{
    FGlobalObjectClassInfo()
        : ClassSingleton(nullptr)
    {
    }
    
    FObjectClass* ClassSingleton;
};


// This class contains information about a class, such as size, alignment and name of the class.
// For now it is mostly used to quickly see if a class is subclassed instead of using dynamic cast.
class ENGINE_API FObjectClass
{
public:
    typedef void (*StaticDefaultConstructorType)(const FObjectInitializer&);
    typedef FObjectClass* (*StaticClassFunctionType)();

    static void GlobalRegisterClass(
        FGlobalObjectClassInfo& OutClassInfo,
        const CHAR* ClassName,
        uint64 Size,
        uint64 Alignment,
        FObjectClass::StaticClassFunctionType SuperStaticClassFunc,
        FObjectClass::StaticClassFunctionType StaticClassFunc,
        FObjectClass::StaticDefaultConstructorType DefaultConstructorFunc);
    
    FObjectClass(
        const CHAR* InName,
        uint64 InSize,
        uint64 InAlignment,
        StaticClassFunctionType InStaticClassFunc,
        StaticDefaultConstructorType InDefaultConstructorFunc);
    ~FObjectClass();

    bool IsSubClassOf(const FObjectClass* Class) const;

    template<typename T>
    bool IsSubClassOf() const
    {
        return IsSubClassOf(T::GetStaticClass());
    }

    const CHAR* GetName() const
    {
        return Name;
    }

    void SetSuperClass(FObjectClass* NewSuperClass)
    {
        SuperClass = NewSuperClass;
    }
    
    FObjectClass* GetSuperClass() const
    {
        return SuperClass;
    }

    StaticClassFunctionType GetStaticClassFunc() const { return StaticClassFunc; }
    StaticDefaultConstructorType GetDefaultConstructorFunc() const { return DefaultConstructorFunc; }

    uint64 GetSize()      const { return Size; }
    uint64 GetAlignment() const { return Alignment; }

private:
    FObjectClass*     SuperClass;
    const CHAR* Name;
    uint64      Size;
    uint64      Alignment;

    StaticClassFunctionType      StaticClassFunc;
    StaticDefaultConstructorType DefaultConstructorFunc;
};
