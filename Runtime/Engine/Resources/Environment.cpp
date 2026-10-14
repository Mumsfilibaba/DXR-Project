#include "Engine/Resources/Environment.h"

FEnvironment::FEnvironment(const FRHITextureRef& InSkyCubeMap, const FRHITextureRef& InSpecularCubeMap, const FRHITextureRef& InDiffuseCubeMap)
    : FAsset()
    , SkyCubeMap(InSkyCubeMap)
    , SpecularCubeMap(InSpecularCubeMap)
    , DiffuseCubeMap(InDiffuseCubeMap)
{
}

FEnvironment::~FEnvironment() = default;
