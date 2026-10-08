#pragma once
#include "ShaderCompiler/ShaderCompiler.h"
#include "ShaderCore/ShaderReflection.h"

struct FSpirvShaderReflector
{
    /** @brief Spirv must already have been through FSpirvTransforms, the recorded word offsets refer to it */
    static bool Reflect(const TArray<uint32>& Spirv, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, String& OutErrors);

    /**
     * @brief Needs the SPIR-V straight from DXC (compiled with -fspv-reflect), before FSpirvTransforms strips the
     * HlslSemanticGOOGLE decorations. Reads the stage inputs that have a Location, splits "TEXCOORD1" into the name
     * and index 1, and sorts the inputs by Location.
     */
    static bool ReflectVertexInputs(const TArray<uint32>& UntransformedSpirv, FShaderReflection& OutReflection, String& OutErrors);
};
