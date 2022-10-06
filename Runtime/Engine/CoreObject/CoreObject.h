#pragma once
#include "ClassType.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Defines for creating CoreObjects

#define CLASS_DESCRIPTION( TCoreObject )        \
    static FClassDescription ClassDescription = \
    {                                           \
        #TCoreObject,                           \
        sizeof(TCoreObject),                    \
        alignof(TCoreObject),                   \
    }

#define CORE_OBJECT( TCoreObject, TSuperClass )                                 \
private:                                                                        \
    typedef TCoreObject This;                                                   \
    typedef TSuperClass Super;                                                  \
                                                                                \
public:                                                                         \
    static FClassType* GetStaticClass()                                         \
    {                                                                           \
        CLASS_DESCRIPTION(TCoreObject);                                         \
        static FClassType ClassInfo(Super::GetStaticClass(), ClassDescription); \
        return &ClassInfo;                                                      \
    }                                                                           \
                                                                                \
private:

#define CORE_OBJECT_INIT()                 \
    this->SetClass(This::GetStaticClass())

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCoreObject

class ENGINE_API FCoreObject
{
public:
    virtual ~FCoreObject() = default;

    FORCEINLINE const FClassType* GetClass() const
    {
        return Class;
    }

    static const FClassType* GetStaticClass()
    {
        CLASS_DESCRIPTION(FCoreObject);
        static FClassType ClassInfo(nullptr, ClassDescription);
        return &ClassInfo;
    }

protected:
    FCoreObject() = default;

    FORCEINLINE void SetClass(const FClassType* InClass)
    {
        Class = InClass;
    }

private:
     /** @brief: Object representing the class */
    const FClassType* Class = nullptr;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers

inline bool IsSubClassOf(FCoreObject* CoreObject, FClassType* ClassType)
{
    CHECK(CoreObject != nullptr);
    CHECK(CoreObject->GetClass() != nullptr);
    return CoreObject->GetClass()->IsSubClassOf(ClassType);
}

template<typename T>
inline bool IsSubClassOf(FCoreObject* CoreObject)
{
    return IsSubClassOf(CoreObject, T::GetStaticClass());
}

template<typename T>
inline T* Cast(FCoreObject* Object)
{
    return IsSubClassOf<T>(Object) ? static_cast<T*>(Object) : nullptr;
}
