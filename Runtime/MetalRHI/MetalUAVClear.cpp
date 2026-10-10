#include "MetalRHI/MetalUAVClear.h"
#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalEncoderManager.h"
#include "MetalRHI/MetalPipelineState.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalViews.h"
#include "MetalRHI/Generated/ClearBufferUAV_Float.h"
#include "MetalRHI/Generated/ClearBufferUAV_Untyped.h"
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

static_assert(ARRAY_COUNT(GMetalUAVClearKernels) == static_cast<int32>(EMetalUAVClearTarget::Count) * 3, "Every clear target needs a kernel per component");

static FRHIComputePipelineStateRef CreateClearPipeline(const FMetalUAVClearKernel& Kernel, FRHIComputeShaderRef& OutShader)
{
    TArray<uint8>   ShaderCode;
    FShaderCodeView CodeView;
    String          ReadError;
    if (!FShaderCodeReader::ReadEmbedded(TArrayView<const uint8>(Kernel.Code, static_cast<int32>(Kernel.Size)), ShaderCode, CodeView, &ReadError))
    {
        METAL_ERROR("A UAV clear shader header is invalid, building MetalRHI regenerates it: %s", *ReadError);
        return nullptr;
    }

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

    UntypedPipeline = nullptr;
    UntypedShader   = nullptr;
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
        Pipelines[Index] = CreateClearPipeline(GMetalUAVClearKernels[Index], Shaders[Index]);
    }

    return static_cast<FMetalComputePipelineStateRHI*>(Pipelines[Index].Get());
}

FMetalComputePipelineStateRHI* FMetalUAVClearPipelines::GetOrCreateUntyped()
{
    if (!UntypedPipeline)
    {
        UntypedPipeline = CreateClearPipeline({ GMetalClearBufferUAV_Untyped, sizeof(GMetalClearBufferUAV_Untyped) }, UntypedShader);
    }

    return static_cast<FMetalComputePipelineStateRHI*>(UntypedPipeline.Get());
}

static void ClearUntypedBuffer(FMetalCommandContext& Context, FMetalUnorderedAccessViewRHI* View, uint32 Value)
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

    FMetalComputePipelineStateRHI* Pipeline = FMetalDeviceRHI::Get()->GetUAVClearPipelines().GetOrCreateUntyped();
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
        ClearUntypedBuffer(Context, View, Values[0]);
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
