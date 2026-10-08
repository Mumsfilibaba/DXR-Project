#pragma once
#include "Core/Core.h"

struct ShaderBindings
{
    static constexpr uint32 GlobalSpace          = 0;
    static constexpr uint32 ShaderConstantsSpace = 1;
    static constexpr uint32 RayTracingLocalSpace = 2;

    /** In 32-bit values */
    static constexpr uint32 MaxShaderConstants = 32;

    /** The cbuffer name FFXCShaderTranslator gives the shader constants in DXBC */
    static constexpr const CHAR* ShaderConstantsBufferName = "Constants_CB";

    /** Must match the -fvk-bind-*-heap arguments in DXCShaderCompiler.cpp. Shared by the Vulkan and the MSL conversion. */
    static constexpr uint32 SpirvHeapMarkerSet       = 31;
    static constexpr uint32 SpirvHeapResourceBinding = 0;
    static constexpr uint32 SpirvHeapSamplerBinding  = 1;
    static constexpr uint32 SpirvHeapCounterBinding  = 16;
};
