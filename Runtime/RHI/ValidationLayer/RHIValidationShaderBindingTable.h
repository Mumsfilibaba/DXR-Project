#pragma once
#include "RHI/RayTracing/RHIShaderBindingTable.h"

class FRHIValidationCommandContext;

class RHI_API FRHIValidationShaderBindingTable final : public FRHIShaderBindingTable
{
public:
    explicit FRHIValidationShaderBindingTable(FRHIShaderBindingTable* InShaderBindingTable);
    virtual ~FRHIValidationShaderBindingTable() = default;

    virtual void* GetRHINativeResource() const override final;
    virtual FRHIShaderBindingTableAddressInfo GetAddressInfo() const override final;

    NODISCARD FRHIShaderBindingTable* GetRHI() const
    {
        return ShaderBindingTable.Get();
    }

    NODISCARD bool IsBuilt() const
    {
        return bBuilt;
    }

private:
    friend class FRHIValidationCommandContext;

    void MarkDirty()
    {
        bBuilt = false;
    }

    void MarkBuilt()
    {
        bBuilt = true;
    }

    FRHIShaderBindingTableRef ShaderBindingTable;
    bool                      bBuilt;
};
