#pragma once
#include "Core.h"

#include "SceneData.h"

class COBJLoader
{
public:
    static bool LoadFile( const String& Filename, SSceneData& OutScene, bool ReverseHandedness = false );
};