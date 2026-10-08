#pragma once
#if PLATFORM_WINDOWS
#include <d3d11shader.h>

#include "ShaderCompiler/ShaderCompiler.h"
#include "ShaderCore/ShaderReflection.h"

typedef HRESULT(WINAPI* PFN_FXC_D3D_REFLECT)(LPCVOID pSrcData, SIZE_T SrcDataSize, REFIID pInterface, void** ppReflector);

struct FDXBCShaderReflector
{
    static bool Reflect(PFN_FXC_D3D_REFLECT D3DReflectFunc, const void* Code, SIZE_T CodeSize, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, String& OutErrors);
};

#endif
