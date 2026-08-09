#ifndef MATERIAL_ARRAY_HLSLI
#define MATERIAL_ARRAY_HLSLI

#include "Structs.hlsli"

#ifndef MATERIAL_ARRAY_REGISTER
    #error "The material buffer has no register. A pass has to name one, since where it lands depends on what else the pass binds."
#endif

StructuredBuffer<FMaterial> Materials : register(MATERIAL_ARRAY_REGISTER);
#endif
