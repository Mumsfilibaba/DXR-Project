#pragma once
#include "ShaderCore/ShaderReflection.h"

struct FShaderReflectionUtils
{
    /** @brief Checks the limits of FShaderResourceBinding and appends it, with its name when bKeepName */
    static bool AddBinding(const CHAR* Name, EShaderResourceType Type, EShaderResourceDimension Dimension, EShaderBindingSpace Space, uint32 Register, uint32 Count, bool bKeepName, FShaderReflection& OutReflection, String& OutErrors);

    /** @brief Checks that the shader constants fit the container and the RHIs, and stores their size */
    static bool SetShaderConstantsSize(const CHAR* Name, uint32 SizeInBytes, FShaderReflection& OutReflection, String& OutErrors);

    /** @brief Sorts by Location and fails when two inputs share a semantic hash and index */
    static bool SortAndValidateVertexInputs(TArray<FShaderVertexInput>& InOutVertexInputs, String& OutErrors);
};
