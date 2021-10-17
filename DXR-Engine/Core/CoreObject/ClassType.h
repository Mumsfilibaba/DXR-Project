#pragma once
#include "Core/CoreAPI.h"

struct SClassDescription
{
    /* Name of the class */
    const char* Name = nullptr;

    /* Size of the class in bytes */
    uint32 SizeInBytes = 0;

    /* Alignment of the class in bytes */
    uint32 Alignment = 0;
};

// ClassType stores info about a class, for now inheritance
class CORE_API CClassType
{
public:

    CClassType( const CClassType* InSuperClass, const SClassDescription& ClassDescription );
    ~CClassType() = default;

    bool IsSubClassOf( const CClassType* Class ) const;

    template<typename T>
    FORCEINLINE bool IsSubClassOf() const
    {
        return IsSubClassOf( T::GetStaticClass() );
    }

    FORCEINLINE const char* GetName() const
    {
        return Name;
    }

    FORCEINLINE const CClassType* GetSuperClass() const
    {
        return SuperClass;
    }

    FORCEINLINE uint32 GetSizeInBytes() const
    {
        return SizeInBytes;
    }

private:

    /* Name of the class */
    const char* Name;

    /* Class that this class inherits from */
    const CClassType* SuperClass;

    /* The size of the class in bytes */
    uint32 SizeInBytes;

    /* Alignment of the class in bytes */
    uint32 Alignment;
};
