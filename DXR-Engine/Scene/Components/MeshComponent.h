#pragma once
#include "Component.h"

class CORE_API CMeshComponent : public CComponent
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
