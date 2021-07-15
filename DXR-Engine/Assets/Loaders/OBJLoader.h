#pragma once
#include "Core.h"

#include "Assets/SceneData.h"

class COBJLoader
{
public:
    static bool LoadFile( const String& Filename, SSceneData& OutScene, bool ReverseHandedness = false );
};