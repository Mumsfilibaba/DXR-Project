#include "ObjectClass.h"

void FObjectClass::GlobalRegisterClass(
    FGlobalObjectClassInfo& OutClassInfo,
    const CHAR* ClassName,
    uint64 Size,
    uint64 Alignment,
    FObjectClass::StaticClassFunctionType SuperStaticClassFunc,
    FObjectClass::StaticClassFunctionType StaticClassFunc,
    FObjectClass::StaticDefaultConstructorType DefaultConstructorFunc)
{
    // Class-name cannot be nullptr, this is probably an indication of a serious issue
    CHECK(ClassName != nullptr);
    
    // Class info should always be nullptr when initializing, otherwise something is really wrong
    CHECK(OutClassInfo.ClassSingleton == nullptr);

    // We do this via reference since the SuperStaticClassFunc will check the state of the variable
    FObjectClass* NewClass = new FObjectClass(ClassName, Size, Alignment, StaticClassFunc, DefaultConstructorFunc);
    OutClassInfo.ClassSingleton = NewClass;

    // Setup the superclass
    FObjectClass* SuperClass = SuperStaticClassFunc();
    if (SuperClass == NewClass)
    {
        NewClass->SetSuperClass(nullptr);
    }
    else
    {
        NewClass->SetSuperClass(SuperClass);
    }
}

FObjectClass::FObjectClass(const CHAR* InName, uint64 InSize, uint64 InAlignment, StaticClassFunctionType InStaticClassFunc, StaticDefaultConstructorType InDefaultConstructorFunc)
    : SuperClass(nullptr)
    , Name(InName)
    , Size(InSize)
    , Alignment(InAlignment)
    , StaticClassFunc(InStaticClassFunc)
    , DefaultConstructorFunc(InDefaultConstructorFunc)
{
}

FObjectClass::~FObjectClass()
{
    SuperClass = nullptr;
    Name       = nullptr;
    Size       = 0;
    Alignment  = 0;
}

bool FObjectClass::IsSubClassOf(const FObjectClass* Class) const
{
    for (const FObjectClass* Current = this; Current; Current = Current->GetSuperClass())
    {
        if (Current == Class)
        {
            return true;
        }
    }

    return false;
}
