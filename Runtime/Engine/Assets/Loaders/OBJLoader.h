#pragma once
#include "Core/CoreModule.h"
#include "Core/Containers/String.h"

#include "Engine/Assets/SceneData.h"

class ENGINE_API COBJLoader
{
public:
    static bool LoadFile( const CString& Filename, SSceneData& OutScene, bool ReverseHandedness = false );
};