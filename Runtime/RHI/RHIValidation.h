#pragma once

#include "RHI/ValidationLayer/RHIValidationHelpers.h"
#include "RHI/ValidationLayer/RHIValidationShaderBindingTable.h"
#include "RHI/ValidationLayer/RHIValidationDevice.h"
#include "RHI/ValidationLayer/RHIValidationCommandContext.h"

struct RHIValidation
{
    static RHI_API int32 GetErrorCount();
    static RHI_API void  ResetErrorCount();
};
