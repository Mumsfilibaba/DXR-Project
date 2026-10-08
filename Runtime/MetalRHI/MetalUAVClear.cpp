#include "MetalRHI/MetalUAVClear.h"
#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalEncoderManager.h"
#include "MetalRHI/MetalPipelineState.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalViews.h"
#include "MetalRHI/Generated/ClearBufferUAV_Float.h"
#include "MetalRHI/Generated/ClearBufferUAV_Raw.h"
#include "MetalRHI/Generated/ClearBufferUAV_Sint.h"
#include "MetalRHI/Generated/ClearBufferUAV_Uint.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture1D_Float.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture1D_Sint.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture1D_Uint.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture1DArray_Float.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture1DArray_Sint.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture1DArray_Uint.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture2D_Float.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture2D_Sint.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture2D_Uint.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture2DArray_Float.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture2DArray_Sint.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture2DArray_Uint.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture3D_Float.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture3D_Sint.h"
#include "MetalRHI/Generated/ClearTextureUAV_Texture3D_Uint.h"
#include "Core/Containers/Array.h"
#include "Core/Math/Math.h"
#include "RHI/RHI.h"
#include "ShaderCore/ShaderCode.h"

struct FMetalUAVClearKernel
{
    const uint8* Code;
    uint32       Size;
};

static constexpr uint32 BufferClearThreadCount   = 64;
static constexpr uint32 TextureClearThreadCountX = 8;
static constexpr uint32 TextureClearThreadCountY = 8;

static const FMetalUAVClearKernel GMetalUAVClearKernels[] =
{
    { GMetalClearBufferUAV_Float,                   sizeof(GMetalClearBufferUAV_Float) },
    { GMetalClearBufferUAV_Sint,                    sizeof(GMetalClearBufferUAV_Sint) },
    { GMetalClearBufferUAV_Uint,                    sizeof(GMetalClearBufferUAV_Uint) },
    { GMetalClearTextureUAV_Texture1D_Float,        sizeof(GMetalClearTextureUAV_Texture1D_Float) },
    { GMetalClearTextureUAV_Texture1D_Sint,         sizeof(GMetalClearTextureUAV_Texture1D_Sint) },
    { GMetalClearTextureUAV_Texture1D_Uint,         sizeof(GMetalClearTextureUAV_Texture1D_Uint) },
    { GMetalClearTextureUAV_Texture1DArray_Float,   sizeof(GMetalClearTextureUAV_Texture1DArray_Float) },
    { GMetalClearTextureUAV_Texture1DArray_Sint,    sizeof(GMetalClearTextureUAV_Texture1DArray_Sint) },
    { GMetalClearTextureUAV_Texture1DArray_Uint,    sizeof(GMetalClearTextureUAV_Texture1DArray_Uint) },
    { GMetalClearTextureUAV_Texture2D_Float,        sizeof(GMetalClearTextureUAV_Texture2D_Float) },
    { GMetalClearTextureUAV_Texture2D_Sint,         sizeof(GMetalClearTextureUAV_Texture2D_Sint) },
    { GMetalClearTextureUAV_Texture2D_Uint,         sizeof(GMetalClearTextureUAV_Texture2D_Uint) },
    { GMetalClearTextureUAV_Texture2DArray_Float,   sizeof(GMetalClearTextureUAV_Texture2DArray_Float) },
    { GMetalClearTextureUAV_Texture2DArray_Sint,    sizeof(GMetalClearTextureUAV_Texture2DArray_Sint) },
    { GMetalClearTextureUAV_Texture2DArray_Uint,    sizeof(GMetalClearTextureUAV_Texture2DArray_Uint) },
    { GMetalClearTextureUAV_Texture3D_Float,        sizeof(GMetalClearTextureUAV_Texture3D_Float) },
    { GMetalClearTextureUAV_Texture3D_Sint,         sizeof(GMetalClearTextureUAV_Texture3D_Sint) },
    { GMetalClearTextureUAV_Texture3D_Uint,         sizeof(GMetalClearTextureUAV_Texture3D_Uint) },
};

static constexpr EMSLTextureDimension GMetalUAVClearDimensions[] =
{
    EMSLTextureDimension::TextureBuffer,
    EMSLTextureDimension::Texture1D,
    EMSLTextureDimension::Texture1DArray,
    EMSLTextureDimension::Texture2D,
    EMSLTextureDimension::Texture2DArray,
    EMSLTextureDimension::Texture3D,
};

static_assert(ARRAY_COUNT(GMetalUAVClearDimensions) == static_cast<int32>(EMetalUAVClearTarget::Count), "Every clear target needs a texture dimension");

static constexpr EShaderResourceDimension GMetalUAVClearResourceDimensions[] =
{
    EShaderResourceDimension::Buffer,
    EShaderResourceDimension::Texture1D,
    EShaderResourceDimension::Texture1DArray,
    EShaderResourceDimension::Texture2D,
    EShaderResourceDimension::Texture2DArray,
    EShaderResourceDimension::Texture3D,
};

static_assert(ARRAY_COUNT(GMetalUAVClearResourceDimensions) == static_cast<int32>(EMetalUAVClearTarget::Count), "Every clear target needs a resource dimension");

// The generated kernels are compiled at build time, so the reflection the shader compiler would produce is written out by hand
static bool WriteClearShaderCode(const FMetalUAVClearKernel& Kernel, const FShaderResourceBinding& OutputBinding, const FMSLBindingSlot& OutputSlot, bool bBuffer, TArray<uint8>& OutShaderCode)
{
    FShaderReflection Reflection;
    Reflection.Info.ShaderConstantsSize = static_cast<uint8>((bBuffer ? 5u : 4u) * sizeof(uint32));
    Reflection.EntryPoint               = "Main";

    Reflection.MSLInfo.ThreadGroupSize[0] = static_cast<uint16>(bBuffer ? BufferClearThreadCount : TextureClearThreadCountX);
    Reflection.MSLInfo.ThreadGroupSize[1] = static_cast<uint16>(bBuffer ? 1 : TextureClearThreadCountY);
    Reflection.MSLInfo.ThreadGroupSize[2] = 1;

    FShaderResourceBinding ConstantsBinding;
    ConstantsBinding.Type  = EShaderResourceType::ConstantBuffer;
    ConstantsBinding.Space = EShaderBindingSpace::ShaderConstants;

    Reflection.Bindings.Add(ConstantsBinding);
    Reflection.MSLSlots.Add(FMSLBindingSlot());
    Reflection.Bindings.Add(OutputBinding);
    Reflection.MSLSlots.Add(OutputSlot);

    String Error;
    if (!FShaderCodeWriter::Write(EShaderOutputLanguage::MSL, EShaderStage::Compute, EShaderCodeFlags::None, Reflection, TArrayView<const uint8>(Kernel.Code, static_cast<int32>(Kernel.Size)), OutShaderCode, &Error))
    {
        METAL_ERROR("Failed to write a UAV clear shader container: %s", *Error);
        return false;
    }

    return true;
}

static bool BuildClearShaderCode(EMetalUAVClearTarget Target, EMSLTextureComponent Component, TArray<uint8>& OutShaderCode)
{
    const bool bBuffer = Target == EMetalUAVClearTarget::Buffer;

    FShaderResourceBinding OutputBinding;
    OutputBinding.Type      = bBuffer ? EShaderResourceType::RWTypedBuffer : EShaderResourceType::RWTexture;
    OutputBinding.Dimension = GMetalUAVClearResourceDimensions[static_cast<int32>(Target)];

    FMSLBindingSlot OutputSlot;
    OutputSlot.NullTextureType = MakeMSLNullTextureType(GMetalUAVClearDimensions[static_cast<int32>(Target)], Component);

    const FMetalUAVClearKernel& Kernel = GMetalUAVClearKernels[static_cast<int32>(Target) * 3 + static_cast<int32>(Component)];
    return WriteClearShaderCode(Kernel, OutputBinding, OutputSlot, bBuffer, OutShaderCode);
}

static bool BuildRawClearShaderCode(TArray<uint8>& OutShaderCode)
{
    FShaderResourceBinding OutputBinding;
    OutputBinding.Type      = EShaderResourceType::RWByteAddressBuffer;
    OutputBinding.Dimension = EShaderResourceDimension::Buffer;

    // spirv-cross places the constants at buffer 0 and the RWByteAddressBuffer at buffer 1
    FMSLBindingSlot OutputSlot;
    OutputSlot.Slot = 1;

    const FMetalUAVClearKernel Kernel = { GMetalClearBufferUAV_Raw, sizeof(GMetalClearBufferUAV_Raw) };
    return WriteClearShaderCode(Kernel, OutputBinding, OutputSlot, true, OutShaderCode);
}

static FRHIComputePipelineStateRef CreateClearPipeline(const TArray<uint8>& ShaderCode, FRHIComputeShaderRef& OutShader)
{
    OutShader = RHI::CreateComputeShader(ShaderCode);

    if (!OutShader)
    {
        METAL_ERROR("Failed to create a UAV clear compute shader");
        return nullptr;
    }

    FRHIComputePipelineStateDesc PipelineDesc;
    PipelineDesc.Shader = OutShader.Get();

    FRHIComputePipelineStateRef Pipeline = RHI::CreateComputePipelineState(PipelineDesc);

    if (!Pipeline)
    {
        METAL_ERROR("Failed to create a UAV clear pipeline");
    }

    return Pipeline;
}

void FMetalUAVClearPipelines::Release()
{
    for (int32 Index = 0; Index < NumPipelines; ++Index)
    {
        Pipelines[Index] = nullptr;
        Shaders[Index]   = nullptr;
    }

    RawPipeline = nullptr;
    RawShader   = nullptr;
}

FMetalComputePipelineStateRHI* FMetalUAVClearPipelines::GetOrCreate(EMetalUAVClearTarget Target, EMSLTextureComponent Component)
{
    if (Component == EMSLTextureComponent::Depth)
    {
        Component = EMSLTextureComponent::Float;
    }

    const int32 Index = static_cast<int32>(Target) * NumComponents + static_cast<int32>(Component);

    if (!Pipelines[Index])
    {
        TArray<uint8> ShaderCode;
        if (BuildClearShaderCode(Target, Component, ShaderCode))
        {
            Pipelines[Index] = CreateClearPipeline(ShaderCode, Shaders[Index]);
        }
    }

    return static_cast<FMetalComputePipelineStateRHI*>(Pipelines[Index].Get());
}

FMetalComputePipelineStateRHI* FMetalUAVClearPipelines::GetOrCreateRaw()
{
    if (!RawPipeline)
    {
        TArray<uint8> ShaderCode;
        if (BuildRawClearShaderCode(ShaderCode))
        {
            RawPipeline = CreateClearPipeline(ShaderCode, RawShader);
        }
    }

    return static_cast<FMetalComputePipelineStateRHI*>(RawPipeline.Get());
}

static void ClearRawBuffer(FMetalCommandContext& Context, FMetalUnorderedAccessViewRHI* View, uint32 Value)
{
    id<MTLBuffer> Buffer = View->GetMTLBuffer();

    if (!Buffer)
    {
        METAL_ERROR("A buffer UAV without a buffer cannot be cleared");
        return;
    }

    const NSRange Range = NSMakeRange(static_cast<NSUInteger>(View->GetBufferOffset()), static_cast<NSUInteger>(View->GetBufferSize()));
    if (Range.length == 0)
    {
        return;
    }

    FMetalEncoderManager& Encoders = Context.GetEncoders();
    Encoders.UpdateResidency(View->GetResidencyEntry());

    const uint8 Byte = static_cast<uint8>(Value & 0xFFu);
    if (Value == static_cast<uint32>(Byte) * 0x01010101u)
    {
        [Encoders.RequireBlitEncoder() fillBuffer:Buffer range:Range value:Byte];
        return;
    }

    FMetalComputePipelineStateRHI* Pipeline = FMetalDeviceRHI::Get()->GetUAVClearPipelines().GetOrCreateRaw();
    if (!Pipeline)
    {
        return;
    }

    const FMetalStageBindPlan& Plan = Pipeline->GetBindings().Stages[EShaderVisibility::Compute];
    CHECK(Plan.NumShaderConstants <= MAX_SHADER_CONSTANTS);

    const uint32 NumWords                        = static_cast<uint32>(Range.length / sizeof(uint32));
    uint32       Constants[MAX_SHADER_CONSTANTS] = { Value, Value, Value, Value, NumWords };

    id<MTLComputeCommandEncoder> Encoder = Encoders.RequireComputeEncoder();
    FMetalEncoderBindingCache&   Cache   = Encoders.GetBindingCache();

    [Encoder setComputePipelineState:Pipeline->GetMTLPipelineState()];

    Cache.SetBuffer<EShaderVisibility::Compute>(Encoder, Buffer, Range.location, Plan.UnorderedAccessSlots[0]);
    Cache.Commit<EShaderVisibility::Compute>(Encoder);
    Cache.SetBytes<EShaderVisibility::Compute>(Encoder, Constants, sizeof(uint32) * Plan.NumShaderConstants, Plan.ShaderConstantsSlot);

    [Encoder dispatchThreadgroups:MTLSizeMake(Math::DivideByMultiple(NumWords, BufferClearThreadCount), 1, 1) threadsPerThreadgroup:MTLSizeMake(BufferClearThreadCount, 1, 1)];

    Context.GetContextState().InvalidateComputeEncoderState();
}

void MetalUAVClear::Clear(FMetalCommandContext& Context, FMetalUnorderedAccessViewRHI* View, const uint32 Values[4])
{
    CHECK(View != nullptr);

    id<MTLTexture> Texture = View->GetMTLTexture();

    if (!Texture)
    {
        ClearRawBuffer(Context, View, Values[0]);
        return;
    }

    Context.GetEncoders().FlushPendingClears(Texture);

    EMetalUAVClearTarget Target;
    switch (Texture.textureType)
    {
        case MTLTextureTypeTextureBuffer: Target = EMetalUAVClearTarget::Buffer;         break;
        case MTLTextureType1D:            Target = EMetalUAVClearTarget::Texture1D;      break;
        case MTLTextureType1DArray:       Target = EMetalUAVClearTarget::Texture1DArray; break;
        case MTLTextureType2D:            Target = EMetalUAVClearTarget::Texture2D;      break;
        case MTLTextureType2DArray:       Target = EMetalUAVClearTarget::Texture2DArray; break;
        case MTLTextureType3D:            Target = EMetalUAVClearTarget::Texture3D;      break;

        default:
            METAL_ERROR("A UAV of texture type %u cannot be cleared", static_cast<uint32>(Texture.textureType));
            return;
    }

    FMetalComputePipelineStateRHI* Pipeline = FMetalDeviceRHI::Get()->GetUAVClearPipelines().GetOrCreate(Target, MetalRHI::GetTextureComponent(Texture.pixelFormat));
    if (!Pipeline)
    {
        return;
    }

    const FMetalStageBindPlan& Plan = Pipeline->GetBindings().Stages[EShaderVisibility::Compute];
    CHECK(Plan.NumShaderConstants <= MAX_SHADER_CONSTANTS);

    uint32  Constants[MAX_SHADER_CONSTANTS] = { Values[0], Values[1], Values[2], Values[3] };
    MTLSize Threadgroups;
    MTLSize ThreadsPerThreadgroup;

    if (Target == EMetalUAVClearTarget::Buffer)
    {
        const uint32 NumElements = static_cast<uint32>(Texture.width);
        Constants[4]          = NumElements;
        Threadgroups          = MTLSizeMake(Math::DivideByMultiple(NumElements, BufferClearThreadCount), 1, 1);
        ThreadsPerThreadgroup = MTLSizeMake(BufferClearThreadCount, 1, 1);
    }
    else
    {
        const bool       bArray        = Target == EMetalUAVClearTarget::Texture1DArray || Target == EMetalUAVClearTarget::Texture2DArray;
        const NSUInteger SlicesOrDepth = bArray ? Texture.arrayLength : Texture.depth;

        Threadgroups          = MTLSizeMake(Math::DivideByMultiple<NSUInteger>(Texture.width, TextureClearThreadCountX), Math::DivideByMultiple<NSUInteger>(Texture.height, TextureClearThreadCountY), SlicesOrDepth);
        ThreadsPerThreadgroup = MTLSizeMake(TextureClearThreadCountX, TextureClearThreadCountY, 1);
    }

    id<MTLComputeCommandEncoder> Encoder = Context.GetEncoders().RequireComputeEncoder();
    FMetalEncoderBindingCache&   Cache   = Context.GetEncoders().GetBindingCache();

    [Encoder setComputePipelineState:Pipeline->GetMTLPipelineState()];

    Cache.SetTexture<EShaderVisibility::Compute>(Texture, Plan.UnorderedAccessSlots[0]);
    Cache.Commit<EShaderVisibility::Compute>(Encoder);
    Cache.SetBytes<EShaderVisibility::Compute>(Encoder, Constants, sizeof(uint32) * Plan.NumShaderConstants, Plan.ShaderConstantsSlot);

    [Encoder dispatchThreadgroups:Threadgroups threadsPerThreadgroup:ThreadsPerThreadgroup];

    Context.GetContextState().InvalidateComputeEncoderState();
}
