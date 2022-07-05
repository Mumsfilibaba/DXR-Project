#pragma once
#include "RHICore.h"

#include "Core/Modules/ModuleManager.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIInstanceType

enum class ERHIInstanceType : uint32
{
    Unknown = 0,
    Null    = 1,
    D3D12   = 2,
	Metal   = 3,
};

inline const char* ToString(ERHIInstanceType RenderLayerApi)
{
    switch (RenderLayerApi)
    {
    case ERHIInstanceType::D3D12: return "D3D12";
	case ERHIInstanceType::Metal: return "Metal";
	case ERHIInstanceType::Null:  return "Null";
    default:                      return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RHI Functions

RHI_API bool RHIInitialize(ERHIInstanceType InRenderApi);
RHI_API void RHIRelease();

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIModule

class RHI_API FRHIModule : public FDefaultEngineModule
{
protected:

    FRHIModule()  = default;
    ~FRHIModule() = default;

public:

    /**
     * @brief: Creates the RHI Instance
     *
     * @return: Returns the newly created RHIInstance
     */
    virtual class FRHICoreInterface* CreateInterface() = 0;

    /**
     * @brief: Creates the RHI shader compiler
     *
     * @return: Returns the shader compiler for this RHI Module
     */
    virtual class IRHIShaderCompiler* CreateCompiler() = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Global variables

extern RHI_API class FRHICoreInterface*  GRHICoreInterface;
extern RHI_API class IRHIShaderCompiler* GShaderCompiler;
