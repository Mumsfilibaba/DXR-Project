#pragma once

#include "Core/Misc/OutputDeviceLogger.h"
#include "RHI/RHIDevice.h"
#include "RHI/ValidationLayer/RHIValidationHelpers.h"

namespace RHIValidationInternal
{
    bool ShouldBreakOnValidationError();

    ERHIType SafeGetRHIType(FRHIDevice* RealRHI);
    bool     IsBufferValidAsCopyDestination(const FRHIBufferDesc& BufferDesc);
    bool     IsBufferValidAsCopySource(const FRHIBufferDesc& BufferDesc);
    bool     IsDepthStencilFormat(EFormat Format);
    bool     ValidateBufferRange(const CHAR* Caller, const FRHIBufferDesc& BufferDesc, uint64 Offset, uint64 Size);
    bool     ValidateIndirectCountBuffer(const CHAR* Operation, FRHIBuffer* CountBuffer, uint64 CountBufferOffset);
    bool     ValidateBufferView(const CHAR* Caller, const FRHIBufferDesc& BufferDesc, EBufferViewType ViewType, uint32 FirstElement, uint32 NumElements, EFormat Format);
    bool     ValidateTextureMip(const CHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 MipLevel, IntVector3& OutExtent);
    bool     ValidateTextureRegion2D(const CHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 MipLevel, const FTextureRegion2D& Region);
    bool     ValidateTextureRegion3D(const CHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 MipLevel, const FTextureRegion3D& Region);
    bool     ValidateTextureSlicesAndMips(const CHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 BaseLayer, uint32 LayerCount, uint32 FirstMip, uint32 NumMips, EFormat ViewFormat, EViewDimension ViewDimension);
}

#define RHI_VALIDATION_ERROR(...) \
    do \
    { \
        LOG_ERROR("[RHI VALIDATION ERROR] " __VA_ARGS__); \
        if (RHIValidationInternal::ShouldBreakOnValidationError()) \
        { \
            DEBUG_BREAK(); \
        } \
    } while (false)

#define RHI_VALIDATION_WARNING(...) \
    do \
    { \
        LOG_WARNING("[RHI VALIDATION WARNING] " __VA_ARGS__); \
    } while (false)

namespace RHIValidationInternal
{
    template<typename ParameterType>
    bool ValidateIndirectArguments(const CHAR* Operation, FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
    {
        if (!ArgumentBuffer || !ArgumentBuffer->GetDesc().IsIndirectArguments())
        {
            RHI_VALIDATION_ERROR("%s requires a non-null IndirectArguments buffer.", Operation);
            return false;
        }

        if ((ArgumentBufferOffset % RHIValidationHelpers::IndirectArgumentOffsetAlignment) != 0 ||
            !RHIValidationHelpers::IsIndirectCommandRangeValid(ArgumentBuffer->GetDesc().Size, ArgumentBufferOffset, sizeof(ParameterType), CommandCount))
        {
            RHI_VALIDATION_ERROR("%s argument range is misaligned or outside the buffer.", Operation);
            return false;
        }

        return true;
    }
}
