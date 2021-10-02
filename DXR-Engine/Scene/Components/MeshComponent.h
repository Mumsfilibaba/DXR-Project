#pragma once
#include "Scene/Actor.h"

class CMeshComponent : public CComponent
{
    CORE_OBJECT( CMeshComponent, CComponent );

public:

    CMeshComponent( CActor* InOwningActor )
        : CComponent( InOwningActor )
        , Material( nullptr )
        , Mesh( nullptr )
    {
        CORE_OBJECT_INIT();
    }

    TSharedPtr<class CMaterial> Material;
    TSharedPtr<class CMesh>     Mesh;
};
