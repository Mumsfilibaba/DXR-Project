#pragma once
#if PLATFORM_WINDOWS
#include <Unknwn.h>
#include <dxc/dxcapi.h>
#include <d3d12shader.h>

#include "ShaderCompiler/ShaderCompiler.h"
#include "ShaderCore/ShaderReflection.h"

class FDXILShaderReflector
{
public:

    /** @brief ReflectionBlob is DXC_OUT_REFLECTION, or the shader object itself when DXC does not return one */
    static bool Reflect(IDxcUtils* Utils, IDxcBlob* ShaderObject, IDxcBlob* ReflectionBlob, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, String& OutErrors);

private:
    static bool ReflectShader(ID3D12ShaderReflection* Reflection, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, String& OutErrors);
    static bool ReflectLibrary(ID3D12LibraryReflection* Reflection, IDxcUtils* Utils, IDxcBlob* ShaderObject, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, String& OutErrors);

    template<typename ReflectionType>
    static bool ReflectBinding(ReflectionType* Reflection, const D3D12_SHADER_INPUT_BIND_DESC& BindDesc, bool bIsLibrary, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, String& OutErrors);

    static bool HasPart(IDxcUtils* Utils, IDxcBlob* ShaderObject, uint32 FourCC);
    static bool ReadFeatureInfo(IDxcUtils* Utils, IDxcBlob* ShaderObject, uint64& OutFlags);
    static String DemangleFunctionName(const String& MangledName);
};

#endif
