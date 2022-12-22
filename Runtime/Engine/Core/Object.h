#pragma once
#include "Class.h"

#define DECLARE_CLASS_INFO(FObjectType) \
    static FClassInfo ClassInfo =       \
    {                                   \
        #FObjectType,                   \
        sizeof(FObjectType),            \
        alignof(FObjectType),           \
    }

#define FOBJECT_BODY(FObjectType, TSuperClass)                       \
private:                                                             \
    typedef FObjectType This;                                        \
    typedef TSuperClass Super;                                       \
                                                                     \
public:                                                              \
    static FClass* GetStaticClass()                                  \
    {                                                                \
        DECLARE_CLASS_INFO(FObjectType);                             \
        static FClass ClassInfo(Super::GetStaticClass(), ClassInfo); \
        return &ClassInfo;                                           \
    }                                                                \
                                                                     \
private:

#define FOBJECT_INIT() \
    this->SetClass(This::GetStaticClass())

class ENGINE_API FObject
{
public:
    virtual ~FObject() = default;

    static const FClass* GetStaticClass()
    {
        DECLARE_CLASS_INFO(FObject);
        static FClass StaticClass(nullptr, ClassInfo);
        return &StaticClass;
    }

    FORCEINLINE const FClass* GetClass() const
    {
        return Class;
    }

    FORCEINLINE void SetClass(const FClass* InClass)
    {
        Class = InClass;
    }

private:
    const FClass* Class = nullptr;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers

inline bool IsSubClassOf(FObject* Object, FClass* Class)
{
    CHECK(Object != nullptr);
    CHECK(Object->GetClass() != nullptr);
    return Object->GetClass()->IsSubClassOf(Class);
}

template<typename T>
inline bool IsSubClassOf(FObject* Object)
{
    return IsSubClassOf(Object, T::GetStaticClass());
}

template<typename T>
inline T* Cast(FObject* Object)
{
    return IsSubClassOf<T>(Object) ? static_cast<T*>(Object) : nullptr;
}
