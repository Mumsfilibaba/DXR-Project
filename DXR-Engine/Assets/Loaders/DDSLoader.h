#pragma once
#include "Assets/SceneData.h"

class CDDSLoader
{
public:
    static TSharedPtr<SImage2D> LoadFile( const String& Filename );
};