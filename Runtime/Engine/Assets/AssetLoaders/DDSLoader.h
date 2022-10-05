#pragma once
#include "Core/Containers/String.h"

#include "Engine/Assets/SceneData.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDDSLoader

class ENGINE_API FDDSLoader
{
public:
    static void* LoadFile(const FString& Filename);
};
