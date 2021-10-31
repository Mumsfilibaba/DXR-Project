#pragma once
#include "Component.h"

#include "Core/Containers/SharedPtr.h"

#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

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

    TSharedPtr<CMaterial> Material;
    TSharedPtr<CMesh>     Mesh;
};
