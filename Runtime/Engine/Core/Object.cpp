#include "Object.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FObjects

FObjectInitializer::FObjectInitializer(void* InMemory, FObjectClass* InClass)
    : Memory(InMemory)
    , Class(InClass)
{
}


FOBJECT_IMPLEMENT_CLASS(FObject)

FObject::FObject(const FObjectInitializer& ObjectInitializer)
    : Class(ObjectInitializer.GetClass())
{
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Creation of Objects

FObject* NewObject(FObjectClass* Class)
{
    void* Memory = FMemory::Malloc(Class->GetSize());
    if (!Memory)
    {
        return nullptr;
    }

    FObjectClass::StaticDefaultConstructorType DefaultConstructorFunc = Class->GetDefaultConstructorFunc();
    FObjectInitializer ObjectInitalizer(Memory, Class);
    DefaultConstructorFunc(ObjectInitalizer);
    return reinterpret_cast<FObject*>(Memory);
}
