#include "MetalRHI/MetalBufferClear.h"
#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalViews.h"
#include "MetalRHI/Generated/ClearBufferUAV_Float.h"
#include "MetalRHI/Generated/ClearBufferUAV_Sint.h"
#include "MetalRHI/Generated/ClearBufferUAV_Uint.h"
#include "Core/Containers/Array.h"
#include "RHI/MSLShaderBindings.h"
#include "RHI/RHI.h"

static constexpr uint32 ClearThreadCount = 64;

static void BuildClearShaderByteCode(const uint8* Source, int32 SourceSize, TArray<uint8>& OutByteCode)
{
    static constexpr FMSLShaderBinding ClearBindings[] =
    {
        { EMSLBindingType::ShaderConstants,        0, 0, 0 },
        { EMSLBindingType::UnorderedAccessTexture, 0, 0, 0 },
    };

    FMSLShaderHeader Header;
    Header.Magic       = FMSLShaderHeader::ExpectedMagic;
    Header.Version     = FMSLShaderHeader::ExpectedVersion;
    Header.NumBindings = ARRAY_COUNT(ClearBindings);
    Header.SourceSize  = static_cast<uint32>(SourceSize);

    constexpr int32 HeaderSize   = static_cast<int32>(sizeof(FMSLShaderHeader));
    constexpr int32 BindingsSize = static_cast<int32>(sizeof(ClearBindings));

    OutByteCode.Resize(HeaderSize + BindingsSize + SourceSize);

    uint8* ByteCode = OutByteCode.Data();
    Memory::Memcpy(ByteCode, &Header, HeaderSize);
    Memory::Memcpy(ByteCode + HeaderSize, ClearBindings, BindingsSize);
    Memory::Memcpy(ByteCode + HeaderSize + BindingsSize, Source, SourceSize);
}

void FMetalBufferClearPipelines::Release()
{
    for (int32 Index = 0; Index < EMetalBufferClearType::Count; ++Index)
    {
        Pipelines[Index] = nullptr;
        Shaders[Index]   = nullptr;
    }
}

bool FMetalBufferClearPipelines::GetOrCreate(EMetalBufferClearType::Type ClearType, FRHIComputeShader*& OutShader, FRHIComputePipelineState*& OutPipeline)
{
    OutShader   = nullptr;
    OutPipeline = nullptr;

    const int32 Index = ClearType;
    if (Pipelines[Index])
    {
        OutShader   = Shaders[Index].Get();
        OutPipeline = Pipelines[Index].Get();
        return true;
    }

    const uint8* EmbeddedCode = nullptr;
    uint32       EmbeddedSize = 0;
    switch (ClearType)
    {
        case EMetalBufferClearType::Uint:
        {
            EmbeddedCode = GMetalClearBufferUAV_Uint;
            EmbeddedSize = ARRAY_COUNT(GMetalClearBufferUAV_Uint);
            break;
        }

        case EMetalBufferClearType::Sint:
        {
            EmbeddedCode = GMetalClearBufferUAV_Sint;
            EmbeddedSize = ARRAY_COUNT(GMetalClearBufferUAV_Sint);
            break;
        }

        default:
        {
            EmbeddedCode = GMetalClearBufferUAV_Float;
            EmbeddedSize = ARRAY_COUNT(GMetalClearBufferUAV_Float);
            break;
        }
    }

    TArray<uint8> ByteCode;
    BuildClearShaderByteCode(EmbeddedCode, static_cast<int32>(EmbeddedSize), ByteCode);

    FRHIComputeShaderRef Shader = RHI::CreateComputeShader(ByteCode);
    if (!Shader)
    {
        METAL_ERROR("Failed to create the buffer UAV clear compute shader");
        return false;
    }

    FRHIComputePipelineStateDesc PipelineDesc;
    PipelineDesc.Shader = Shader.Get();

    FRHIComputePipelineStateRef Pipeline = RHI::CreateComputePipelineState(PipelineDesc);
    if (!Pipeline)
    {
        METAL_ERROR("Failed to create the buffer UAV clear pipeline");
        return false;
    }

    Shaders[Index]   = Move(Shader);
    Pipelines[Index] = Move(Pipeline);
    OutShader        = Shaders[Index].Get();
    OutPipeline      = Pipelines[Index].Get();
    return true;
}

void MetalClearBufferUAV::Clear(FMetalCommandContext& Context, FMetalUnorderedAccessViewRHI* View, const uint32 Values[4], bool bIsFloat)
{
    CHECK(View != nullptr);
    CHECK(View->GetDesc().IsBufferUAV());

    const uint64 ByteSize    = View->GetBufferSize();
    const uint32 NumElements = static_cast<uint32>(ByteSize / 16ull);
    if (NumElements == 0)
    {
        return;
    }

    const EMetalBufferClearType::Type ClearType = bIsFloat ? EMetalBufferClearType::Float : EMetalBufferClearType::Uint;

    FRHIComputeShader*        Shader   = nullptr;
    FRHIComputePipelineState* Pipeline = nullptr;
    if (!FMetalDeviceRHI::Get()->GetBufferClearPipelines().GetOrCreate(ClearType, Shader, Pipeline))
    {
        return;
    }

    uint32 Constants[5] = { Values[0], Values[1], Values[2], Values[3], NumElements };

    Context.SetComputePipelineState(Pipeline);
    Context.SetUnorderedAccessView(Shader, View, 0);
    Context.SetShaderConstants(Shader, Constants, 5);
    Context.Dispatch((NumElements + ClearThreadCount - 1) / ClearThreadCount, 1, 1);
}
