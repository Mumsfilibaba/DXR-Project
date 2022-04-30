#pragma once
#include "Component.h"

#include "Core/Containers/SharedPtr.h"

#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CMeshComponent

class ENGINE_API CMeshComponent : public CComponent
{
    CORE_OBJECT(CMeshComponent, CComponent);

public:

    CMeshComponent(CActor* InActorOwner)
        : CComponent(InActorOwner, false, false)
        , Material(nullptr)
        , Mesh(nullptr)
    {
        CORE_OBJECT_INIT();
    }

    TSharedPtr<CMaterial> Material;
    TSharedPtr<CMesh>     Mesh;
};
