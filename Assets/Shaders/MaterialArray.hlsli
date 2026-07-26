#ifndef MATERIAL_ARRAY_HLSLI
#define MATERIAL_ARRAY_HLSLI

#include "Structs.hlsli"

#ifndef MATERIAL_ARRAY_REGISTER
    #define MATERIAL_ARRAY_REGISTER t4
#endif

StructuredBuffer<FMaterial> Materials : register(MATERIAL_ARRAY_REGISTER);
#endif
