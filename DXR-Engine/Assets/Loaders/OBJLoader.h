#pragma once
#include "Core.h"

#include "Core/Containers/String.h"
#include "Assets/SceneData.h"

class COBJLoader
{
public:
    static bool LoadFile( const CString& Filename, SSceneData& OutScene, bool ReverseHandedness = false );
};