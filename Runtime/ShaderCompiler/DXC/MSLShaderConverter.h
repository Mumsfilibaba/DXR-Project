#pragma once
#include "ShaderCompiler/ShaderCompiler.h"
#include "ShaderCore/ShaderReflection.h"

struct FMSLShaderConverter
{
    /**
     * @brief Converts SPIR-V to MSL and reflects it in the same SPIRV-Cross context, because the MSL slots are only known
     * once the source has been emitted. Fills Bindings, MSLSlots, MSLInfo, ShaderConstantsSize, EntryPoint and,
     * with bDebugInfo, BindingNames. Vertex inputs must already be in OutReflection.
     */
    static bool Convert(const TArray<uint32>& Spirv, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, TArray<uint8>& OutMSLSource, String& OutErrors);
};
