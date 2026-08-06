#include "Core/Math/Float.h"
#include "VulkanRHI/VulkanBufferClear.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanResourceViews.h"

FVulkanBufferClearRegion VulkanClearBufferUAV::ResolveRegion(FVulkanUnorderedAccessViewRHI* View)
{
    FVulkanBufferClearRegion Region = {};

    VkDeviceSize ViewOffset = 0;
    if (View->GetType() == FVulkanResourceView::EType::StructuredBufferView)
    {
        const FVulkanResourceView::FStructuredBufferView& Info = View->GetStructuredBufferInfo();
        Region.Buffer = Info.Buffer;
        Region.Offset = Info.Offset;
        Region.Size   = Info.Range;
        ViewOffset    = Info.ViewOffset;
    }
    else
    {
        const FVulkanResourceView::FTypedBufferView& Info = View->GetTypedBufferInfo();
        Region.Buffer = Info.Buffer;
        Region.Offset = Info.Offset;
        Region.Size   = Info.Range;
        ViewOffset    = Info.ViewOffset;
    }

    if (Region.Size == 0 || Region.Size == VK_WHOLE_SIZE)
    {
        FVulkanBufferRHI* OwnerBuffer = static_cast<FVulkanBufferRHI*>(View->GetOwnerResource());
        if (!OwnerBuffer)
        {
            VULKAN_ERROR("Cannot clear a buffer UAV whose owning resource has been released");
            return Region;
        }

        const VkDeviceSize OwnerSize = OwnerBuffer->GetBindRange();
        if (ViewOffset >= OwnerSize)
        {
            VULKAN_ERROR("Cannot clear a buffer UAV starting at %llu, which is past the end of its %llu byte resource", ViewOffset, OwnerSize);
            return Region;
        }

        Region.Size = OwnerSize - ViewOffset;
    }

    if ((Region.Offset % 4) != 0 || (Region.Size % 4) != 0)
    {
        VULKAN_WARNING("Cannot clear buffer UAV range [%llu, %llu): vkCmdFillBuffer requires a 4-byte aligned offset and size",
            Region.Offset, Region.Offset + Region.Size);
        return Region;
    }

    Region.bIsValid = true;
    return Region;
}

static FORCEINLINE uint32 VulkanPackUnormComponent(uint32 Value, bool bIsFloat, uint32 NumBits)
{
    const uint32 MaxValue = (1u << NumBits) - 1u;
    if (!bIsFloat)
    {
        return Value & MaxValue;
    }

    const float Normalized = Math::Clamp(BitCast<float>(Value), 0.0f, 1.0f);
    return static_cast<uint32>(Math::RoundToInt(Normalized * static_cast<float>(MaxValue))) & MaxValue;
}

static FORCEINLINE uint32 VulkanPackSnormComponent(uint32 Value, bool bIsFloat, uint32 NumBits)
{
    const uint32 Mask = (1u << NumBits) - 1u;
    if (!bIsFloat)
    {
        return Value & Mask;
    }

    const float MaxValue   = static_cast<float>((1u << (NumBits - 1u)) - 1u);
    const float Normalized = Math::Clamp(BitCast<float>(Value), -1.0f, 1.0f);

    return static_cast<uint32>(Math::RoundToInt(Normalized * MaxValue)) & Mask;
}

static FORCEINLINE uint32 VulkanPackIntegerComponent(uint32 Value, bool bIsFloat, uint32 NumBits)
{
    const uint32 Mask = (NumBits >= 32u) ? ~0u : ((1u << NumBits) - 1u);
    if (!bIsFloat)
    {
        return Value & Mask;
    }

    return static_cast<uint32>(Math::RoundToInt(BitCast<float>(Value))) & Mask;
}

static FORCEINLINE uint32 VulkanPackHalfComponent(uint32 Value, bool bIsFloat)
{
    return bIsFloat ? static_cast<uint32>(FFloat16(BitCast<float>(Value)).GetBits()) : (Value & 0xFFFFu);
}

static FORCEINLINE uint32 VulkanPackSmallFloatComponent(uint32 Value, bool bIsFloat, uint32 MantissaBits)
{
    if (!bIsFloat)
    {
        return Value & ((1u << (MantissaBits + 5u)) - 1u);
    }

    const FFloat16 Half(Math::Max(BitCast<float>(Value), 0.0f));
    return (static_cast<uint32>(Half.Exponent) << MantissaBits) | (static_cast<uint32>(Half.Mantissa) >> (10u - MantissaBits));
}

static bool VulkanPackNarrowTypedElement(EFormat Format, const uint32 Values[4], bool bIsFloat, uint32& OutElement)
{
    switch (Format)
    {
        case EFormat::R10G10B10A2_Typeless:
        case EFormat::R10G10B10A2_Unorm:
        {
            OutElement = VulkanPackUnormComponent(Values[0], bIsFloat, 10)
                | (VulkanPackUnormComponent(Values[1], bIsFloat, 10) << 10)
                | (VulkanPackUnormComponent(Values[2], bIsFloat, 10) << 20)
                | (VulkanPackUnormComponent(Values[3], bIsFloat, 2)  << 30);
            return true;
        }

        case EFormat::R10G10B10A2_Uint:
        {
            OutElement = VulkanPackIntegerComponent(Values[0], bIsFloat, 10)
                | (VulkanPackIntegerComponent(Values[1], bIsFloat, 10) << 10)
                | (VulkanPackIntegerComponent(Values[2], bIsFloat, 10) << 20)
                | (VulkanPackIntegerComponent(Values[3], bIsFloat, 2)  << 30);
            return true;
        }

        case EFormat::R11G11B10_Float:
        {
            OutElement = VulkanPackSmallFloatComponent(Values[0], bIsFloat, 6)
                | (VulkanPackSmallFloatComponent(Values[1], bIsFloat, 6) << 11)
                | (VulkanPackSmallFloatComponent(Values[2], bIsFloat, 5) << 22);
            return true;
        }

        case EFormat::R8G8B8A8_Typeless:
        case EFormat::R8G8B8A8_Unorm:
        case EFormat::R8G8B8A8_Unorm_SRGB:
        {
            OutElement = VulkanPackUnormComponent(Values[0], bIsFloat, 8)
                | (VulkanPackUnormComponent(Values[1], bIsFloat, 8) << 8)
                | (VulkanPackUnormComponent(Values[2], bIsFloat, 8) << 16)
                | (VulkanPackUnormComponent(Values[3], bIsFloat, 8) << 24);
            return true;
        }

        case EFormat::R8G8B8A8_Snorm:
        {
            OutElement = VulkanPackSnormComponent(Values[0], bIsFloat, 8)
                | (VulkanPackSnormComponent(Values[1], bIsFloat, 8) << 8)
                | (VulkanPackSnormComponent(Values[2], bIsFloat, 8) << 16)
                | (VulkanPackSnormComponent(Values[3], bIsFloat, 8) << 24);
            return true;
        }

        case EFormat::R8G8B8A8_Uint:
        case EFormat::R8G8B8A8_Sint:
        {
            OutElement = VulkanPackIntegerComponent(Values[0], bIsFloat, 8)
                | (VulkanPackIntegerComponent(Values[1], bIsFloat, 8) << 8)
                | (VulkanPackIntegerComponent(Values[2], bIsFloat, 8) << 16)
                | (VulkanPackIntegerComponent(Values[3], bIsFloat, 8) << 24);
            return true;
        }

        case EFormat::R16G16_Typeless:
        case EFormat::R16G16_Float:
        {
            OutElement = VulkanPackHalfComponent(Values[0], bIsFloat) | (VulkanPackHalfComponent(Values[1], bIsFloat) << 16);
            return true;
        }

        case EFormat::R16G16_Unorm:
        {
            OutElement = VulkanPackUnormComponent(Values[0], bIsFloat, 16) | (VulkanPackUnormComponent(Values[1], bIsFloat, 16) << 16);
            return true;
        }

        case EFormat::R16G16_Snorm:
        {
            OutElement = VulkanPackSnormComponent(Values[0], bIsFloat, 16) | (VulkanPackSnormComponent(Values[1], bIsFloat, 16) << 16);
            return true;
        }

        case EFormat::R16G16_Uint:
        case EFormat::R16G16_Sint:
        {
            OutElement = VulkanPackIntegerComponent(Values[0], bIsFloat, 16) | (VulkanPackIntegerComponent(Values[1], bIsFloat, 16) << 16);
            return true;
        }

        case EFormat::R32_Typeless:
        case EFormat::D32_Float:
        case EFormat::R32_Float:
        {
            OutElement = Values[0];
            return true;
        }

        case EFormat::R32_Uint:
        case EFormat::R32_Sint:
        {
            OutElement = VulkanPackIntegerComponent(Values[0], bIsFloat, 32);
            return true;
        }

        case EFormat::R24G8_Typeless:
        case EFormat::D24_Unorm_S8_Uint:
        {
            OutElement = VulkanPackUnormComponent(Values[0], bIsFloat, 24) | (VulkanPackIntegerComponent(Values[1], bIsFloat, 8) << 24);
            return true;
        }

        case EFormat::R24_Unorm_X8_Typeless:
        {
            OutElement = VulkanPackUnormComponent(Values[0], bIsFloat, 24);
            return true;
        }

        case EFormat::X24_Typeless_G8_Uint:
        {
            OutElement = VulkanPackIntegerComponent(Values[1], bIsFloat, 8) << 24;
            return true;
        }

        case EFormat::R8G8_Typeless:
        case EFormat::R8G8_Unorm:
        {
            OutElement = VulkanPackUnormComponent(Values[0], bIsFloat, 8) | (VulkanPackUnormComponent(Values[1], bIsFloat, 8) << 8);
            return true;
        }

        case EFormat::R8G8_Snorm:
        {
            OutElement = VulkanPackSnormComponent(Values[0], bIsFloat, 8) | (VulkanPackSnormComponent(Values[1], bIsFloat, 8) << 8);
            return true;
        }

        case EFormat::R8G8_Uint:
        case EFormat::R8G8_Sint:
        {
            OutElement = VulkanPackIntegerComponent(Values[0], bIsFloat, 8) | (VulkanPackIntegerComponent(Values[1], bIsFloat, 8) << 8);
            return true;
        }

        case EFormat::R16_Typeless:
        case EFormat::R16_Float:
        {
            OutElement = VulkanPackHalfComponent(Values[0], bIsFloat);
            return true;
        }

        case EFormat::D16_Unorm:
        case EFormat::R16_Unorm:
        {
            OutElement = VulkanPackUnormComponent(Values[0], bIsFloat, 16);
            return true;
        }

        case EFormat::R16_Snorm:
        {
            OutElement = VulkanPackSnormComponent(Values[0], bIsFloat, 16);
            return true;
        }

        case EFormat::R16_Uint:
        case EFormat::R16_Sint:
        {
            OutElement = VulkanPackIntegerComponent(Values[0], bIsFloat, 16);
            return true;
        }

        case EFormat::R8_Typeless:
        case EFormat::R8_Unorm:
        {
            OutElement = VulkanPackUnormComponent(Values[0], bIsFloat, 8);
            return true;
        }

        case EFormat::R8_Snorm:
        {
            OutElement = VulkanPackSnormComponent(Values[0], bIsFloat, 8);
            return true;
        }

        case EFormat::R8_Uint:
        case EFormat::R8_Sint:
        {
            OutElement = VulkanPackIntegerComponent(Values[0], bIsFloat, 8);
            return true;
        }

        default:
        {
            return false;
        }
    }
}

static bool VulkanPackWideTypedElement(EFormat Format, const uint32 Values[4], bool bIsFloat, uint32 OutDwords[4])
{
    switch (Format)
    {
        case EFormat::R16G16B16A16_Typeless:
        case EFormat::R16G16B16A16_Float:
        {
            OutDwords[0] = VulkanPackHalfComponent(Values[0], bIsFloat) | (VulkanPackHalfComponent(Values[1], bIsFloat) << 16);
            OutDwords[1] = VulkanPackHalfComponent(Values[2], bIsFloat) | (VulkanPackHalfComponent(Values[3], bIsFloat) << 16);
            return true;
        }

        case EFormat::R16G16B16A16_Unorm:
        {
            OutDwords[0] = VulkanPackUnormComponent(Values[0], bIsFloat, 16) | (VulkanPackUnormComponent(Values[1], bIsFloat, 16) << 16);
            OutDwords[1] = VulkanPackUnormComponent(Values[2], bIsFloat, 16) | (VulkanPackUnormComponent(Values[3], bIsFloat, 16) << 16);
            return true;
        }

        case EFormat::R16G16B16A16_Snorm:
        {
            OutDwords[0] = VulkanPackSnormComponent(Values[0], bIsFloat, 16) | (VulkanPackSnormComponent(Values[1], bIsFloat, 16) << 16);
            OutDwords[1] = VulkanPackSnormComponent(Values[2], bIsFloat, 16) | (VulkanPackSnormComponent(Values[3], bIsFloat, 16) << 16);
            return true;
        }

        case EFormat::R16G16B16A16_Uint:
        case EFormat::R16G16B16A16_Sint:
        {
            OutDwords[0] = VulkanPackIntegerComponent(Values[0], bIsFloat, 16) | (VulkanPackIntegerComponent(Values[1], bIsFloat, 16) << 16);
            OutDwords[1] = VulkanPackIntegerComponent(Values[2], bIsFloat, 16) | (VulkanPackIntegerComponent(Values[3], bIsFloat, 16) << 16);
            return true;
        }

        case EFormat::R32G32_Typeless:
        case EFormat::R32G32_Float:
        case EFormat::R32G32B32_Typeless:
        case EFormat::R32G32B32_Float:
        case EFormat::R32G32B32A32_Typeless:
        case EFormat::R32G32B32A32_Float:
        {
            OutDwords[0] = Values[0];
            OutDwords[1] = Values[1];
            OutDwords[2] = Values[2];
            OutDwords[3] = Values[3];
            return true;
        }

        case EFormat::R32G32_Uint:
        case EFormat::R32G32_Sint:
        case EFormat::R32G32B32_Uint:
        case EFormat::R32G32B32_Sint:
        case EFormat::R32G32B32A32_Uint:
        case EFormat::R32G32B32A32_Sint:
        {
            for (uint32 Index = 0; Index < 4; Index++)
            {
                OutDwords[Index] = VulkanPackIntegerComponent(Values[Index], bIsFloat, 32);
            }

            return true;
        }

        default:
        {
            return false;
        }
    }
}

bool VulkanClearBufferUAV::PackPattern(EBufferViewType ViewType, EFormat Format, const uint32 Values[4], bool bIsFloat, uint32& OutPattern)
{
    OutPattern = Values[0];

    if (ViewType != EBufferViewType::Typed)
    {
        return true;
    }

    const uint32 ElementSize = GetByteStrideFromFormat(Format);
    if (ElementSize == 0)
    {
        return false;
    }

    if (ElementSize <= 4)
    {
        uint32 Element = 0;
        if (!VulkanPackNarrowTypedElement(Format, Values, bIsFloat, Element))
        {
            return false;
        }

        switch (ElementSize)
        {
            case 1:  OutPattern = Element * 0x01010101u;      break;
            case 2:  OutPattern = Element | (Element << 16);  break;
            default: OutPattern = Element;                    break;
        }

        return true;
    }

    uint32 Dwords[4] = { 0, 0, 0, 0 };
    if (!VulkanPackWideTypedElement(Format, Values, bIsFloat, Dwords))
    {
        return false;
    }

    const uint32 NumDwords = ElementSize / 4;
    for (uint32 Index = 1; Index < NumDwords; Index++)
    {
        if (Dwords[Index] != Dwords[0])
        {
            return false;
        }
    }

    OutPattern = Dwords[0];
    return true;
}

bool VulkanClearBufferUAV::GetClearType(EFormat Format, EVulkanBufferClearType& OutClearType)
{
    switch (Format)
    {
        case EFormat::R16G16B16A16_Typeless:
        case EFormat::R16G16B16A16_Float:
        case EFormat::R16G16B16A16_Unorm:
        case EFormat::R16G16B16A16_Snorm:
        case EFormat::R32G32_Typeless:
        case EFormat::R32G32_Float:
        case EFormat::R32G32B32_Typeless:
        case EFormat::R32G32B32_Float:
        case EFormat::R32G32B32A32_Typeless:
        case EFormat::R32G32B32A32_Float:
        {
            OutClearType = EVulkanBufferClearType::Float;
            return true;
        }

        case EFormat::R16G16B16A16_Uint:
        case EFormat::R32G32_Uint:
        case EFormat::R32G32B32_Uint:
        case EFormat::R32G32B32A32_Uint:
        {
            OutClearType = EVulkanBufferClearType::Uint;
            return true;
        }

        case EFormat::R16G16B16A16_Sint:
        case EFormat::R32G32_Sint:
        case EFormat::R32G32B32_Sint:
        case EFormat::R32G32B32A32_Sint:
        {
            OutClearType = EVulkanBufferClearType::Sint;
            return true;
        }

        default:
        {
            return false;
        }
    }
}
