#pragma once
#include "Core.h"

typedef void(*ConstructorType)(void*);
typedef void(*DestructorType)(void*);

struct SClassDescription
{
	/* The constructor of the class */
	ConstructorType Constructor = nullptr;
	
	/* The destructor of the class */
	DestructorType Destructor = nullptr;
	
	/* Name of the class */
	const char* Name = nullptr;
	
	/* Size of the class in bytes */
	uint32 SizeInBytes = 0;
	
	/* Alignment of the class in bytes */
	uint32 Alignment = 0;
};

// ClassType stores info about a class, for now inheritance
class CClassType
{
public:
	
	CClassType( const CClassType* InSuperClass, const SClassDescription& ClassDescription );
    ~CClassType() = default;

    bool IsSubClassOf( const CClassType* Class ) const;

	class CCoreObject* Construct() const;
	
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
	
	/* The constructor of the class */
	ConstructorType Constructor = nullptr;
	
	/* The destructor of the class */
	DestructorType Destructor = nullptr;
	
	/* Name of the class */
    const char* Name;
	
	/* Class that this class inherits from */
    const CClassType* SuperClass;
	
	/* The size of the class in bytes */
    uint32 SizeInBytes;
	
	/* Alignment of the class in bytes */
	uint32 Alignment;
};

/* Helper to construct a new object */
inline class CCoreObject* NewObject( CClassType* Class )
{
	Assert( Class != nullptr );
	return Class->Construct();
}

/* Helper to construct a new object */
template<typename CoreObjectType>
inline CoreObjectType* NewObject()
{
	return NewObject( CoreObjectType::GetStaticClass() );
}
