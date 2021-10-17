#pragma once
#include "ClassType.h"

#define CLASS_DESCRIPTION( TCoreObject )        \
    static SClassDescription ClassDescription = \
    {                                           \
        #TCoreObject,                           \
        sizeof(TCoreObject),                    \
        alignof(TCoreObject),                   \
    }

#define CORE_OBJECT( TCoreObject, TSuperClass )                                   \
private:                                                                          \
    typedef TCoreObject This;                                                     \
    typedef TSuperClass Super;                                                    \
                                                                                  \
public:                                                                           \
    static CClassType* GetStaticClass()                                           \
    {                                                                             \
        CLASS_DESCRIPTION( TCoreObject );                                         \
        static CClassType ClassInfo( Super::GetStaticClass(), ClassDescription ); \
        return &ClassInfo;                                                        \
    }                                                                             \
                                                                                  \
private:

#define CORE_OBJECT_INIT()                 \
    this->SetClass(This::GetStaticClass())

class CORE_API CCoreObject
{
public:

    virtual ~CCoreObject() = default;

    FORCEINLINE const CClassType* GetClass() const
    {
        return Class;
    }

    static const CClassType* GetStaticClass()
    {
        CLASS_DESCRIPTION( CCoreObject );
        static CClassType ClassInfo( nullptr, ClassDescription );
        return &ClassInfo;
    }

protected:

    CCoreObject() = default;

    FORCEINLINE void SetClass( const CClassType* InClass )
    {
        Class = InClass;
    }

private:

    /* Object representing the class */
    const CClassType* Class = nullptr;
};

inline bool IsSubClassOf( CCoreObject* CoreObject, CClassType* ClassType )
{
    Assert( CoreObject != nullptr );
    Assert( CoreObject->GetClass() != nullptr );
    return CoreObject->GetClass()->IsSubClassOf( ClassType );
}

template<typename T>
inline bool IsSubClassOf( CCoreObject* CoreObject )
{
    return IsSubClassOf( CoreObject, T::GetStaticClass() );
}

template<typename T>
inline T* Cast( CCoreObject* Object )
{
    return IsSubClassOf<T>( Object ) ? static_cast<T*>(Object) : nullptr;
}
