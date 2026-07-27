#include "RHI/ValidationLayer/RHIValidationShaderBindingTable.h"

static const FRHIShaderBindingTableDesc& GetShaderBindingTableDesc(FRHIShaderBindingTable* ShaderBindingTable)
{
    CHECK(ShaderBindingTable != nullptr);
    return ShaderBindingTable->GetDesc();
}

FRHIValidationShaderBindingTable::FRHIValidationShaderBindingTable(FRHIShaderBindingTable* InShaderBindingTable)
    : FRHIShaderBindingTable(GetShaderBindingTableDesc(InShaderBindingTable))
    , ShaderBindingTable(InShaderBindingTable)
    , bBuilt(false)
{
}

void* FRHIValidationShaderBindingTable::GetRHINativeResource() const
{
    return ShaderBindingTable->GetRHINativeResource();
}

FRHIShaderBindingTableAddressInfo FRHIValidationShaderBindingTable::GetAddressInfo() const
{
    return ShaderBindingTable->GetAddressInfo();
}
