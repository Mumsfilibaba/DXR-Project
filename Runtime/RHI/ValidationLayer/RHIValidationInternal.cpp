#include "Core/Containers/Set.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Platform/PlatformStackTrace.h"
#include "Core/Threading/Atomic.h"
#include "Core/Threading/ScopedLock.h"
#include "RHI/RHIValidation.h"
#include "RHI/ValidationLayer/RHIValidationInternal.h"

static TAutoConsoleVariable<bool> CVarEnableValidationDebugBreak(
    "RHI.EnableValidationDebugBreak",
    "Enables debug-breaks when detecting errors in the custom RHI-validation layer",
    true);

static TAutoConsoleVariable<bool> CVarEnableResourceStateValidation(
    "RHI.EnableResourceStateValidation",
    "Tracks the state of every resource so the validation layer can detect a missing barrier",
    true);

static TAutoConsoleVariable<bool> CVarEnableValidationCallstack(
    "RHI.EnableValidationCallstack",
    "Prints a callstack the first time each site in the custom RHI-validation layer reports an error",
    true);

static constexpr int32 ValidationCallStackDepth = 24;

static AtomicInt32 GValidationErrorCount(0);

bool RHIValidationInternal::ShouldBreakOnValidationError()
{
    return CVarEnableValidationDebugBreak.GetValue();
}

bool RHIValidationInternal::ShouldValidateResourceStates()
{
    return CVarEnableResourceStateValidation.GetValue();
}

void RHIValidationInternal::IncrementErrorCount()
{
    GValidationErrorCount.Increment();
}

static TSet<uint64>& GetReportedCallStackSites()
{
    static TSet<uint64> ReportedCallStackSites;
    return ReportedCallStackSites;
}

static FCriticalSection& GetCallStackCriticalSection()
{
    static FCriticalSection CallStackCriticalSection;
    return CallStackCriticalSection;
}

void RHIValidationInternal::LogCallStack(const CHAR* Filename, int32 Line)
{
    if (!CVarEnableValidationCallstack.GetValue())
    {
        return;
    }

    {
        // The __FILE__ literal of a site always has the same address, so the pointer identifies the file without hashing it
        const uint64 FileKey = static_cast<uint64>(reinterpret_cast<uintptr_t>(Filename));
        const uint64 SiteKey = (FileKey * 1099511628211ull) ^ static_cast<uint64>(Line);

        TScopedLock Lock(GetCallStackCriticalSection());
        if (GetReportedCallStackSites().Contains(SiteKey))
        {
            return;
        }

        GetReportedCallStackSites().Add(SiteKey);
    }

    const TArray<FStackTraceEntry> Stack = FPlatformStackTrace::GetStack(ValidationCallStackDepth, 0);

    int32 FirstFrame = 0;
    while (FirstFrame < Stack.Size() && CString::Strstr(Stack[FirstFrame].FunctionName, "RHIValidation") != nullptr)
    {
        FirstFrame++;
    }

    if (FirstFrame >= Stack.Size())
    {
        FirstFrame = 0;
    }

    String Record;
    for (int32 Index = FirstFrame; Index < Stack.Size(); Index++)
    {
        const FStackTraceEntry& Entry = Stack[Index];
        if (Entry.Filename[0])
        {
            Record.AppendPrintf("\n    [%2d] %s (%s:%u)", Index - FirstFrame, Entry.FunctionName, Entry.Filename, Entry.Line);
        }
        else
        {
            Record.AppendPrintf("\n    [%2d] %s [%s]", Index - FirstFrame, Entry.FunctionName, Entry.ModuleName);
        }
    }

    if (!Record.IsEmpty())
    {
        LOG_ERROR("[RHI VALIDATION ERROR] Callstack:%s", *Record);
    }

    // The break that follows can stop the process before the file device, which writes asynchronously, has the stack
    FOutputDeviceLogger::Get()->Flush();
}

int32 RHIValidation::GetErrorCount()
{
    return GValidationErrorCount.Load();
}

void RHIValidation::ResetErrorCount()
{
    GValidationErrorCount.Store(0);
}

ERHIType RHIValidationInternal::SafeGetRHIType(FRHIDevice* RealRHI)
{
    return RealRHI ? RealRHI->GetRHIType() : ERHIType::Unknown;
}

String RHIValidationInternal::GetResourceIdentity(const FRHIResource* Resource)
{
    if (!Resource)
    {
        return String("<null>");
    }

    // Only textures and buffers carry a debug name, and the shared base does not declare the accessor
    String DebugName;
    switch (Resource->GetResourceType())
    {
        case ERHIResourceType::Texture: static_cast<const FRHITexture*>(Resource)->GetDebugName(DebugName); break;
        case ERHIResourceType::Buffer:  static_cast<const FRHIBuffer*>(Resource)->GetDebugName(DebugName);  break;
        default: break;
    }

    const CHAR* ResourceType = ToString(Resource->GetResourceType());
    if (DebugName.IsEmpty())
    {
        return String::Printf("%s <unnamed> (%p)", ResourceType, reinterpret_cast<const void*>(Resource));
    }

    return String::Printf("%s '%s'", ResourceType, *DebugName);
}

bool RHIValidationInternal::IsBufferValidAsCopyDestination(const FRHIBufferDesc& BufferDesc)
{
    return BufferDesc.IsCopyDest() || BufferDesc.IsReadBack();
}

bool RHIValidationInternal::IsBufferValidAsCopySource(const FRHIBufferDesc& BufferDesc)
{
    return BufferDesc.IsCopySource();
}

bool RHIValidationInternal::IsDepthStencilFormat(EFormat Format)
{
    return Format == EFormat::D16_Unorm || Format == EFormat::D24_Unorm_S8_Uint || Format == EFormat::D32_Float;
}

bool RHIValidationInternal::ValidateBufferRange(const CHAR* Caller, const FRHIBufferDesc& BufferDesc, uint64 Offset, uint64 Size)
{
    if (!RHIValidationHelpers::IsRangeValid(BufferDesc.Size, Offset, Size))
    {
        RHI_VALIDATION_ERROR("%s: non-empty range [Offset=%llu, Size=%llu] exceeds buffer size %llu.", Caller, Offset, Size, BufferDesc.Size);
        return false;
    }

    return true;
}

bool RHIValidationInternal::ValidateIndirectCountBuffer(const CHAR* Operation, FRHIBuffer* CountBuffer, uint64 CountBufferOffset)
{
    if (!CountBuffer || !CountBuffer->GetDesc().IsIndirectArguments())
    {
        RHI_VALIDATION_ERROR("%s requires a non-null IndirectArguments count buffer.", Operation);
        return false;
    }

    if ((CountBufferOffset % RHIValidationHelpers::IndirectArgumentOffsetAlignment) != 0 ||
        !RHIValidationHelpers::IsIndirectCommandRangeValid(CountBuffer->GetDesc().Size, CountBufferOffset, sizeof(uint32), 1))
    {
        RHI_VALIDATION_ERROR("%s: %s count-buffer range is misaligned or outside the buffer.", *GetResourceIdentity(CountBuffer), Operation);
        return false;
    }

    return true;
}

bool RHIValidationInternal::ValidateBufferView(const CHAR* Caller, const FRHIBufferDesc& BufferDesc, EBufferViewType ViewType, uint32 FirstElement, uint32 NumElements, EFormat Format)
{
    if (ViewType == EBufferViewType::Unknown)
    {
        RHI_VALIDATION_ERROR("%s: buffer view type cannot be Unknown.", Caller);
        return false;
    }

    uint32 ElementSize = 0;
    switch (ViewType)
    {
        case EBufferViewType::Structured:
        {
            if (BufferDesc.Stride == 0)
            {
                RHI_VALIDATION_ERROR("%s: structured buffer views require a non-zero buffer stride.", Caller);
                return false;
            }

            ElementSize = BufferDesc.Stride;
            break;
        }

        case EBufferViewType::ByteAddress:
        {
            if (Format != EFormat::Unknown)
            {
                RHI_VALIDATION_ERROR("%s: byte-address buffer views must use EFormat::Unknown.", Caller);
                return false;
            }

            ElementSize = sizeof(uint32);
            break;
        }

        case EBufferViewType::Typed:
        {
            if (Format == EFormat::Unknown || IsTypelessFormat(Format))
            {
                RHI_VALIDATION_ERROR("%s: typed buffer views require a concrete typed format.", Caller);
                return false;
            }

            ElementSize = GetByteStrideFromFormat(Format);
            if (ElementSize == 0)
            {
                RHI_VALIDATION_ERROR("%s: format '%s' is not valid for a typed buffer view.", Caller, ToString(Format));
                return false;
            }

            break;
        }

        default:
            return false;
    }

    const uint64 ByteOffset = uint64(FirstElement) * uint64(ElementSize);
    const uint64 ByteSize   = uint64(NumElements) * uint64(ElementSize);

    return ValidateBufferRange(Caller, BufferDesc, ByteOffset, ByteSize);
}

bool RHIValidationInternal::ValidateTextureMip(const CHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 MipLevel, IntVector3& OutExtent)
{
    if (MipLevel >= TextureDesc.NumMipLevels)
    {
        RHI_VALIDATION_ERROR("%s: mip level %u exceeds texture mip count %u.", Caller, MipLevel, TextureDesc.NumMipLevels);
        return false;
    }

    OutExtent.X = Math::Max<int32>(int32(TextureDesc.Extent.X >> MipLevel), 1);
    OutExtent.Y = Math::Max<int32>(int32(TextureDesc.Extent.Y >> MipLevel), 1);
    OutExtent.Z = TextureDesc.Dimension == ETextureDimension::Texture3D ? Math::Max<int32>(int32(TextureDesc.Extent.Z >> MipLevel), 1) : 1;
    return true;
}

bool RHIValidationInternal::ValidateTextureRegion2D(const CHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 MipLevel, const FTextureRegion2D& Region)
{
    IntVector3 MipExtent;
    if (!ValidateTextureMip(Caller, TextureDesc, MipLevel, MipExtent))
    {
        return false;
    }

    if (Region.Width == 0 || Region.Height == 0)
    {
        RHI_VALIDATION_ERROR("%s: region width and height must be greater than zero.", Caller);
        return false;
    }

    if (Region.PositionX > uint32(MipExtent.X) || Region.Width > uint32(MipExtent.X) - Region.PositionX ||
        Region.PositionY > uint32(MipExtent.Y) || Region.Height > uint32(MipExtent.Y) - Region.PositionY)
    {
        RHI_VALIDATION_ERROR("%s: region exceeds mip %u extent (%u x %u).", Caller, MipLevel, uint32(MipExtent.X), uint32(MipExtent.Y));
        return false;
    }

    return true;
}

bool RHIValidationInternal::ValidateTextureRegion3D(const CHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 MipLevel, const FTextureRegion3D& Region)
{
    IntVector3 MipExtent;
    if (!ValidateTextureMip(Caller, TextureDesc, MipLevel, MipExtent))
    {
        return false;
    }

    if (Region.Width == 0 || Region.Height == 0 || Region.Depth == 0)
    {
        RHI_VALIDATION_ERROR("%s: region width, height, and depth must be greater than zero.", Caller);
        return false;
    }

    if (Region.PositionX > uint32(MipExtent.X) || Region.Width > uint32(MipExtent.X) - Region.PositionX ||
        Region.PositionY > uint32(MipExtent.Y) || Region.Height > uint32(MipExtent.Y) - Region.PositionY ||
        Region.PositionZ > uint32(MipExtent.Z) || Region.Depth > uint32(MipExtent.Z) - Region.PositionZ)
    {
        RHI_VALIDATION_ERROR("%s: region exceeds mip %u extent (%u x %u x %u).", Caller, MipLevel, uint32(MipExtent.X), uint32(MipExtent.Y), uint32(MipExtent.Z));
        return false;
    }

    return true;
}

bool RHIValidationInternal::ValidateTextureSlicesAndMips(const CHAR* Caller, const FRHITextureDesc& TextureDesc, uint32 BaseLayer, uint32 LayerCount,
    uint32 FirstMip, uint32 NumMips, EFormat ViewFormat, EViewDimension ViewDimension)
{
    if (ViewFormat == EFormat::Unknown)
    {
        RHI_VALIDATION_ERROR("%s: Format cannot be EFormat::Unknown.", Caller);
        return false;
    }

    if (IsTypelessFormat(ViewFormat))
    {
        RHI_VALIDATION_ERROR("%s: Format cannot be a typeless format.", Caller);
        return false;
    }

    if (!IsViewDimensionCompatible(TextureDesc.Dimension, ViewDimension))
    {
        RHI_VALIDATION_ERROR("%s: ViewDimension '%s' is incompatible with texture dimension '%s'.", Caller, ToString(ViewDimension), ToString(TextureDesc.Dimension));
        return false;
    }

    const uint32 MaxLayers = RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices);
    if (!RHIValidationHelpers::IsSubresourceRangeValid(MaxLayers, BaseLayer, LayerCount))
    {
        RHI_VALIDATION_ERROR("%s: slice range [Base=%u, Count=%u] exceeds texture native layer count (%u).", Caller, BaseLayer, LayerCount, MaxLayers);
        return false;
    }

    if (!RHIValidationHelpers::IsSubresourceRangeValid(TextureDesc.NumMipLevels, FirstMip, NumMips))
    {
        RHI_VALIDATION_ERROR("%s: mip range [First=%u, Count=%u] exceeds texture mip count (%u).", Caller, FirstMip, NumMips, TextureDesc.NumMipLevels);
        return false;
    }

    return true;
}
