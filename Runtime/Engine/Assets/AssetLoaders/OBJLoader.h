#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"

#include "Engine/Assets/SceneData.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FOBJLoader

class ENGINE_API FOBJLoader
{
public:
    static bool LoadFile(const FString& Filename, FSceneData& OutScene, bool ReverseHandedness = false);
};