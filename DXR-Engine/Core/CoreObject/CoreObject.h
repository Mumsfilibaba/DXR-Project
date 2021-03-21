#pragma once
#include "ClassType.h"

#define CORE_OBJECT(TCoreObject, TSuperClass) \
private: \
    typedef TCoreObject This; \
    typedef TSuperClass Super; \
\
public: \
    static ClassType* GetStaticClass() \
    { \
        static ClassType ClassInfo(#TCoreObject, Super::GetStaticClass(), sizeof(TCoreObject)); \
        return &ClassInfo; \
    }

#define CORE_OBJECT_INIT() \
    this->SetClass(This::GetStaticClass())

class CoreObject
{
public:
    virtual ~CoreObject() = default;

    FORCEINLINE const ClassType* GetClass() const
    {
        return Class;
    }

    static const ClassType* GetStaticClass()
    {
        static ClassType ClassInfo("CoreObject", nullptr, sizeof(CoreObject));
        return &ClassInfo;
    }

protected:
    FORCEINLINE void SetClass(const ClassType* InClass)
    {
        Class = InClass;
    }

private:
    const ClassType* Class = nullptr;
};

template<typename T>
bool IsSubClassOf(CoreObject* Object)
{
    Assert(Object != nullptr);
    Assert(Object->GetClass() != nullptr);
    return Object->GetClass()->IsSubClassOf<T>();
}

template<typename T>
T* Cast(CoreObject* Object)
{
    return IsSubClassOf<T>(Object) ? static_cast<T*>(Object) : nullptr; 
}
