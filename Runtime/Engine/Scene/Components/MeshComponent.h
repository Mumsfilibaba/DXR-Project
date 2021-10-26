#pragma once
#include "Component.h"

#include "Core/Containers/SharedPtr.h"

class ENGINE_API CMeshComponent : public CComponent
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
