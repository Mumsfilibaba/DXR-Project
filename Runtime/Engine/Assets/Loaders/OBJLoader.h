#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"

#include "Engine/Assets/SceneData.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// COBJLoader

class ENGINE_API COBJLoader
{
public:
    static bool LoadFile(const CString& Filename, SSceneData& OutScene, bool ReverseHandedness = false);
};