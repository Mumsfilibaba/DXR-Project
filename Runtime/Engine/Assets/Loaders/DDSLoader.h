#pragma once
#include "Core/Containers/String.h"

#include "Engine/Assets/SceneData.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CDDSLoader

class ENGINE_API CDDSLoader
{
public:
    static TSharedPtr<SImage2D> LoadFile(const FString& Filename);
};
