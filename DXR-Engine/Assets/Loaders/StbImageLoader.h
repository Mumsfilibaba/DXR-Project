#pragma once
#include "Assets/SceneData.h"

class CStbImageLoader
{
public:
    static TSharedPtr<SImage2D> LoadFile( const String& Filename );
};