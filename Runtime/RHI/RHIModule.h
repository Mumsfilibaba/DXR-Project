#pragma once
#include "RHICore.h"

#include "Core/Modules/ModuleInterface.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIInstanceType

enum class ERHIInstanceType : uint32
{
    Unknown = 0,
    Null    = 1,
    D3D12   = 2,
	Metal   = 3,
};

inline const CHAR* ToString(ERHIInstanceType RenderLayerApi)
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

class RHI_API FRHIModule 
    : public FDefaultModule
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
    virtual class FRHIInterface* CreateInterface() = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Global Variable

extern RHI_API class FRHIInterface* GRHIInterface;
