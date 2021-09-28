#pragma once
#include "Scene/Actor.h"

class MeshComponent : public CComponent
{
    CORE_OBJECT( MeshComponent, CComponent );

	// Needs a default constructor, should not be used
	MeshComponent()
	: CComponent( nullptr )
	, Material( nullptr )
	, Mesh( nullptr )
	{
		CORE_OBJECT_INIT();
	}

public:
		
    MeshComponent( CActor* InOwningActor )
        : CComponent( InOwningActor )
        , Material( nullptr )
        , Mesh( nullptr )
    {
        CORE_OBJECT_INIT();
    }

    TSharedPtr<class CMaterial> Material;
    TSharedPtr<class Mesh>      Mesh;
};
