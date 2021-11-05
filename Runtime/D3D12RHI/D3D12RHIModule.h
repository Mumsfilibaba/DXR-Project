#pragma once
#include "Core.h"

#include "RHI/RHIModule.h"

#if PLATFORM_WINDOWS

#if D3D12RHI_API_EXPORT
#define D3D12RHI_API __declspec(dllexport)
#else
#define D3D12RHI_API
#endif

#else
#define D3D12RHI_API
#endif

class CD3D12RHIModule final : public CRHIModule
{
public:

    CD3D12RHIModule() = default;
    ~CD3D12RHIModule() = default;

    /* Creates the core RHI object */
    virtual class CRHICore* CreateCore() override final;

    /* Creates the RHI shader compiler */
    virtual class IRHIShaderCompiler* CreateCompiler() override final;

    /* Retrieve the module's name */
    virtual const char* GetName() const override final;

};