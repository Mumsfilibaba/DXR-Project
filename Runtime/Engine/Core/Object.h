#pragma once
#include "ObjectClass.h"
#include "Core/Containers/Map.h"

class FObjectInitializer;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FObject Macro

#define FOBJECT_DECLARE_DEFAULT_CONSTRUCTOR(FObjectType) \
    static void StaticDefaultConstructor(const FObjectInitializer& ObjectInitializer)

#define FOBJECT_IMPLEMENT_DEFAULT_CONSTRUCTOR(FObjectType) \
    void FObjectType::StaticDefaultConstructor(const FObjectInitializer& ObjectInitializer) \
    { \
        new(ObjectInitializer.GetMemory())FObjectType(ObjectInitializer); \
    }

// This macro is used to declare important information to retrieve global information about the class.
// It should be used in a public segment of the class declaration.
#define FOBJECT_DECLARE_CLASS(FObjectType, FSuperClassType) \
    private: \
        /* Globals used to retrieve information about this class */ \
        static FGlobalObjectClassInfo GlobalClassInfo; \
        static FObjectClass* GetStaticClassPrivate(); \
        /* Declare a static function that can create an object of this class with the help of a ObjectInitializer */ \
        FOBJECT_DECLARE_DEFAULT_CONSTRUCTOR(FObjectType); \
    public: \
        /* Typedefs for This and Super class helper types */ \
        typedef FObjectType     This; \
        typedef FSuperClassType Super; \
        /* Retrieve a static version of the FObjectClass object for this type */ \
        static FObjectClass* StaticClass() \
        { \
            return GetStaticClassPrivate(); \
        } \

// This macro implements/declares variables and function needed to declare information about
// the class instance for this class.
#define FOBJECT_IMPLEMENT_CLASS(FObjectType) \
    /* Implement the default constructor */ \
    FOBJECT_IMPLEMENT_DEFAULT_CONSTRUCTOR(FObjectType) \
    /* Info that contains global information about this class */ \
    FGlobalObjectClassInfo FObjectType::GlobalClassInfo; \
    /* Private version of retrieving the FObjectClass instance for this class */ \
    FObjectClass* FObjectType::GetStaticClassPrivate() \
    { \
        if (!GlobalClassInfo.ClassSingleton) \
        { \
            FObjectClass::GlobalRegisterClass( \
                GlobalClassInfo, \
                #FObjectType, \
                sizeof(FObjectType), \
                alignof(FObjectType), \
                &Super::StaticClass, \
                &FObjectType::StaticClass, \
                &FObjectType::StaticDefaultConstructor); \
        } \
     \
        return GlobalClassInfo.ClassSingleton; \
    } \


/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FObjects

class ENGINE_API FObjectInitializer
{
public:
    FObjectInitializer(void* InMemory, FObjectClass* InClass);
    ~FObjectInitializer()
    {
        Memory = nullptr;
        Class  = nullptr;
    }

    void* GetMemory() const
    {
        return Memory;
    }
    
    FObjectClass* GetClass() const
    {
        return Class;
    }
    
private:
    void*         Memory;
    FObjectClass* Class;
};


class ENGINE_API FObject
{
public:
    FOBJECT_DECLARE_CLASS(FObject, FObject);
    
    FObject(const FObjectInitializer& ObjectInitializer);
    virtual ~FObject() = default;

    FObjectClass* GetClass() const
    {
        return Class;
    }

private:
    FObjectClass* Class = nullptr;
};


/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Casting of Objects

inline bool IsSubClassOf(FObject* Object, FObjectClass* Class)
{
    CHECK(Object != nullptr);
    CHECK(Object->GetClass() != nullptr);
    return Object->GetClass()->IsSubClassOf(Class);
}

template<typename T>
inline bool IsSubClassOf(FObject* Object)
{
    return IsSubClassOf(Object, T::StaticClass());
}

template<typename T>
inline T* Cast(FObject* Object)
{
    return IsSubClassOf<T>(Object) ? static_cast<T*>(Object) : nullptr;
}


/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Creation of Objects

ENGINE_API FObject* NewObject(FObjectClass* Class);

template<typename T>
T* NewObject()
{
    // TODO: We might want to allocate via a allocator
    void* Memory = FMemory::Malloc(sizeof(T));
    if (!Memory)
    {
        return nullptr;
    }

    FObjectClass* Class = T::StaticClass();
    FObjectInitializer ObjectInitalizer(Memory, Class);
    FObjectClass::StaticDefaultConstructorType DefaultConstructorFunc = Class->GetDefaultConstructorFunc();
    DefaultConstructorFunc(ObjectInitalizer);
    return reinterpret_cast<T*>(Memory);
}
