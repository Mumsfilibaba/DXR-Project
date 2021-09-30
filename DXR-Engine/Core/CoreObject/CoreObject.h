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
		CLASS_DESCRIPTION( TCoreObject );										  \
        static CClassType ClassInfo( Super::GetStaticClass(), ClassDescription ); \
        return &ClassInfo;                                                        \
    }                                                                             \
                                                                                  \
private:

#define CORE_OBJECT_INIT()                 \
    this->SetClass(This::GetStaticClass())

class CCoreObject
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

template<typename T>
inline bool IsSubClassOf( CCoreObject* Object )
{
    Assert( Object != nullptr );
    Assert( Object->GetClass() != nullptr );
    return Object->GetClass()->IsSubClassOf<T>();
}

template<typename T>
inline T* Cast( CCoreObject* Object )
{
    return IsSubClassOf<T>( Object ) ? static_cast<T*>(Object) : nullptr;
}
