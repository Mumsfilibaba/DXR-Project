#include "RHIBootTests.h"

#include "TestCommon/TestMacros.h"

#include <Core/Memory/Memory.h>
#include <Core/Misc/ConsoleManager.h>
#include <Core/Misc/Paths.h>
#include <RHI/RHI.h>
#include <RHI/RHICommandList.h>
#include <RHI/RHIIndirect.h>
#include <RHI/RHIPipelineState.h>
#include <RHI/RHIQuery.h>
#include <RHI/RHIResources.h>
#include <RHI/RHISamplerState.h>
#include <RHI/RHITexture.h>
#include <RHI/ShaderCompiler.h>

#if PLATFORM_MACOS
#include <MetalRHI/MetalBindlessDescriptors.h>
#include <MetalRHI/MetalCapabilities.h>
#include <MetalRHI/MetalCommandContext.h>
#include <MetalRHI/MetalDeviceDebug.h>
#include <MetalRHI/MetalPipelineState.h>
#include <MetalRHI/MetalQueue.h>
#include <MetalRHI/MetalRHI.h>
#include <MetalRHI/MetalTexture.h>
#include <RHI/MSLShaderBindings.h>
#endif

static void SetConsoleVariable(const CHAR* VariableName, bool bValue)
{
    if (IConsoleVariable* Variable = FConsoleManager::Get().FindConsoleVariable(VariableName))
    {
        Variable->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
    }
    else
    {
        LOG_WARNING("Console variable '%s' was not found", VariableName);
    }
}

static void SetConsoleVariable(const CHAR* VariableName, const CHAR* Value)
{
    if (IConsoleVariable* Variable = FConsoleManager::Get().FindConsoleVariable(VariableName))
    {
        Variable->SetString(Value, EConsoleVariableFlags::SetByCode);
    }
    else
    {
        LOG_WARNING("Console variable '%s' was not found", VariableName);
    }
}

class FBootTextureData final : public IRHITextureData
{
public:
    FBootTextureData(const void* InData, int64 InRowPitch, int64 InSlicePitch)
        : Data(const_cast<void*>(InData))
        , RowPitch(InRowPitch)
        , SlicePitch(InSlicePitch)
    {
    }

    virtual int64 GetMipRowPitch(uint32 MipLevel = 0)   const override final { return MipLevel == 0 ? RowPitch : 0; }
    virtual int64 GetMipSlicePitch(uint32 MipLevel = 0) const override final { return MipLevel == 0 ? SlicePitch : 0; }
    virtual void* GetMipData(uint32 MipLevel = 0)       const override final { return MipLevel == 0 ? Data : nullptr; }

private:
    void* Data;
    int64 RowPitch;
    int64 SlicePitch;
};

static bool ProbeBuffers(bool bCanMap)
{
    TEST_BEGIN();

    const uint32 InitialData[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

    TEST_SECTION("Buffers");

    FRHIBufferRef VertexBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateVertexBuffer(sizeof(uint32) * 4, 2));
    TEST_EXPECT(VertexBuffer != nullptr);

    FRHIBufferRef VertexBufferWithData = RHI::CreateBuffer(FRHIBufferDesc::CreateVertexBuffer(sizeof(uint32) * 4, 2), ERHIResourceState::Common, InitialData);
    TEST_EXPECT(VertexBufferWithData != nullptr);

    FRHIBufferRef IndexBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateIndexBuffer(EIndexFormat::uint32, 8), ERHIResourceState::Common, InitialData);
    TEST_EXPECT(IndexBuffer != nullptr);

    FRHIBufferRef ConstantBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateConstantBuffer(sizeof(InitialData)), ERHIResourceState::Common, InitialData);
    TEST_EXPECT(ConstantBuffer != nullptr);

    FRHIBufferRef StructuredBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateStructuredBuffer(sizeof(uint32), 8), ERHIResourceState::Common, InitialData);
    TEST_EXPECT(StructuredBuffer != nullptr);

    const FRHIBufferDesc RawUAVDesc(EBufferFlags::Default | EBufferFlags::RWBuffer, sizeof(uint32), sizeof(InitialData));
    FRHIBufferRef RawUAVBuffer = RHI::CreateBuffer(RawUAVDesc);
    TEST_EXPECT(RawUAVBuffer != nullptr);

    TEST_SECTION("Buffer views");

    if (StructuredBuffer)
    {
        FRHIShaderResourceViewRef BufferSRV = RHI::CreateShaderResourceView(StructuredBuffer.Get(), FRHIShaderResourceViewDesc::CreateBuffer(0, 8));
        TEST_EXPECT(BufferSRV != nullptr);
    }

    if (RawUAVBuffer)
    {
        FRHIUnorderedAccessViewRef BufferUAV = RHI::CreateUnorderedAccessView(RawUAVBuffer.Get(), FRHIUnorderedAccessViewDesc::CreateBuffer(0, 8, EBufferViewType::ByteAddress));
        TEST_EXPECT(BufferUAV != nullptr);
    }

    TEST_SECTION("Dynamic map round-trip");

    const FRHIBufferDesc DynamicDesc(EBufferFlags::Dynamic | EBufferFlags::VertexBuffer, sizeof(uint32), sizeof(InitialData));
    FRHIBufferRef DynamicBuffer = RHI::CreateBuffer(DynamicDesc);
    TEST_EXPECT(DynamicBuffer != nullptr);

    if (DynamicBuffer && bCanMap)
    {
        uint32* Mapped = static_cast<uint32*>(DynamicBuffer->Map());
        TEST_EXPECT(Mapped != nullptr);
        if (Mapped)
        {
            for (uint32 Index = 0; Index < 8; ++Index)
            {
                Mapped[Index] = InitialData[Index] + 100;
            }

            DynamicBuffer->Unmap();

            const uint32* ReadBack = static_cast<const uint32*>(DynamicBuffer->Map());
            TEST_EXPECT(ReadBack != nullptr);
            if (ReadBack)
            {
                TEST_EXPECT_EQ(ReadBack[0], 100u);
                TEST_EXPECT_EQ(ReadBack[7], 107u);
                DynamicBuffer->Unmap();
            }
        }
    }

    TEST_SECTION("Readback buffer");

    FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(sizeof(InitialData)));
    TEST_EXPECT(ReadbackBuffer != nullptr);

    if (ReadbackBuffer && bCanMap)
    {
        TEST_EXPECT(ReadbackBuffer->Map() != nullptr);
        ReadbackBuffer->Unmap();
    }

    TEST_END();
}

static bool ProbeTextures()
{
    TEST_BEGIN();

    const ETextureUsageFlags SRVUsage = ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest;

    TEST_SECTION("Texture dimensions");

    FRHITextureRef Texture1D = RHI::CreateTexture(FRHITextureDesc::CreateTexture1D(EFormat::R8G8B8A8_Unorm, 4, 1, SRVUsage));
    TEST_EXPECT(Texture1D != nullptr);

    FRHITextureRef Texture1DArray = RHI::CreateTexture(FRHITextureDesc::CreateTexture1DArray(EFormat::R8G8B8A8_Unorm, 4, 2, 1, SRVUsage));
    TEST_EXPECT(Texture1DArray != nullptr);

    FRHITextureRef Texture2D = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1, SRVUsage));
    TEST_EXPECT(Texture2D != nullptr);

    FRHITextureRef Texture2DArray = RHI::CreateTexture(FRHITextureDesc::CreateTexture2DArray(EFormat::R8G8B8A8_Unorm, 4, 4, 2, 1, 1, SRVUsage));
    TEST_EXPECT(Texture2DArray != nullptr);

    FRHITextureRef TextureCube = RHI::CreateTexture(FRHITextureDesc::CreateTextureCube(EFormat::R8G8B8A8_Unorm, 4, 1, 1, SRVUsage));
    TEST_EXPECT(TextureCube != nullptr);

    FRHITextureRef TextureCubeArray = RHI::CreateTexture(FRHITextureDesc::CreateTextureCubeArray(EFormat::R8G8B8A8_Unorm, 4, 2, 1, 1, SRVUsage));
    TEST_EXPECT(TextureCubeArray != nullptr);

    FRHITextureRef Texture3D = RHI::CreateTexture(FRHITextureDesc::CreateTexture3D(EFormat::R8G8B8A8_Unorm, 4, 4, 4, 1, 1, SRVUsage));
    TEST_EXPECT(Texture3D != nullptr);

    TEST_SECTION("Default views");

    TEST_EXPECT(Texture2D && Texture2D->GetShaderResourceView() != nullptr);

    FRHITextureRef RenderTarget = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
        EFormat::R8G8B8A8_Unorm, 8, 8, 1, 1, ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture));
    TEST_EXPECT(RenderTarget != nullptr);
    TEST_EXPECT(RenderTarget && RenderTarget->GetRenderTargetView() != nullptr);

    FRHITextureRef DepthTarget = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
        EFormat::D32_Float, 8, 8, 1, 1, ETextureUsageFlags::DepthStencil));
    TEST_EXPECT(DepthTarget != nullptr);
    TEST_EXPECT(DepthTarget && DepthTarget->GetDepthStencilView() != nullptr);

    FRHITextureRef UAVTexture = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
        EFormat::R32_Uint, 8, 8, 1, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture));
    TEST_EXPECT(UAVTexture != nullptr);
    TEST_EXPECT(UAVTexture && UAVTexture->GetUnorderedAccessView() != nullptr);

    TEST_SECTION("Explicit views");

    if (Texture2DArray)
    {
        FRHIShaderResourceViewRef SliceSRV = RHI::CreateShaderResourceView(
            Texture2DArray.Get(), FRHIShaderResourceViewDesc::CreateTexture2DArray(EFormat::R8G8B8A8_Unorm, 0, 1, 1, 1));
        TEST_EXPECT(SliceSRV != nullptr);
    }

    if (UAVTexture)
    {
        FRHIUnorderedAccessViewRef MipUAV = RHI::CreateUnorderedAccessView(
            UAVTexture.Get(), FRHIUnorderedAccessViewDesc::CreateTexture2D(EFormat::R32_Uint, 0));
        TEST_EXPECT(MipUAV != nullptr);
    }

    TEST_SECTION("Upload with initial data");

    const uint32 Pixels[16] =
    {
        0xffffffff, 0xff000000, 0xffffffff, 0xff000000,
        0xff000000, 0xffffffff, 0xff000000, 0xffffffff,
        0xffffffff, 0xff000000, 0xffffffff, 0xff000000,
        0xff000000, 0xffffffff, 0xff000000, 0xffffffff,
    };

    const FBootTextureData Texture2DData(Pixels, 4 * sizeof(uint32), sizeof(Pixels));
    FRHITextureRef UploadedTexture2D = RHI::CreateTexture(
        FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1, SRVUsage), ERHIResourceState::Common, &Texture2DData);
    TEST_EXPECT(UploadedTexture2D != nullptr);

    const FBootTextureData Texture3DData(Pixels, 4 * sizeof(uint32), 4 * 4 * sizeof(uint32));
    FRHITextureRef UploadedTexture3D = RHI::CreateTexture(
        FRHITextureDesc::CreateTexture3D(EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1, 1, SRVUsage), ERHIResourceState::Common, &Texture3DData);
    TEST_EXPECT(UploadedTexture3D != nullptr);

    TEST_END();
}

static bool ProbeCreateAndDestroy()
{
    TEST_BEGIN();

    TEST_SECTION("Destroy immediately after create");

    const uint32 InitialData[4] = { 1, 2, 3, 4 };

    for (uint32 Iteration = 0; Iteration < 8; ++Iteration)
    {
        FRHIBufferRef Buffer = RHI::CreateBuffer(FRHIBufferDesc::CreateVertexBuffer(sizeof(uint32), 4), ERHIResourceState::Common, InitialData);
        TEST_EXPECT(Buffer != nullptr);

        FRHITextureRef Texture = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
            EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1, ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest));
        TEST_EXPECT(Texture != nullptr);
    }

    if (FRHICommandListExecutor::IsInitialized())
    {
        FRHICommandListExecutor::Get().WaitForGPU();
    }

    TEST_END();
}

static bool ProbePipelineObjects()
{
    TEST_BEGIN();

    TEST_SECTION("Fixed-function state");

    FRHIDepthStencilStateRef DepthStencilState = RHI::CreateDepthStencilState(FRHIDepthStencilStateDesc());
    TEST_EXPECT(DepthStencilState != nullptr);
    TEST_EXPECT(DepthStencilState && DepthStencilState->GetRHINativeState() == nullptr);

    FRHIRasterizerStateRef RasterizerState = RHI::CreateRasterizerState(FRHIRasterizerStateDesc());
    TEST_EXPECT(RasterizerState != nullptr);
    TEST_EXPECT(RasterizerState && RasterizerState->GetRHINativeState() == nullptr);

    FRHIBlendStateRef BlendState = RHI::CreateBlendState(FRHIBlendStateDesc());
    TEST_EXPECT(BlendState != nullptr);
    TEST_EXPECT(BlendState && BlendState->GetRHINativeState() == nullptr);

    TEST_SECTION("Input layout");

    TArray<FRHIInputElementDesc> InputElements;
    FRHIInputElementDesc& Position = InputElements.Emplace();
    Position.Semantic     = "POSITION";
    Position.Format       = EFormat::R32G32B32_Float;
    Position.VertexStride = static_cast<uint16>(sizeof(float) * 3);
    Position.InputSlot    = 0;
    Position.ByteOffset   = 0;

    FRHIInputLayoutRef InputLayout = RHI::CreateInputLayout(InputElements);
    TEST_EXPECT(InputLayout != nullptr);
    TEST_EXPECT(InputLayout && InputLayout->GetNumInputElementDescs() == 1u);
    TEST_EXPECT(InputLayout && InputLayout->GetInputElementDesc(0) != nullptr);

    TEST_END();
}

static bool ProbeShaders()
{
    TEST_BEGIN();

    TEST_SECTION("Compile a shader with colliding HLSL registers");

    if (!FShaderCompiler::Initialize(Paths::GetAssetDir()))
    {
        TEST_EXPECT(false);
        TEST_END();
    }

    TArray<uint8> ByteCode;
    const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    const bool bCompiled = FShaderCompiler::Get().CompileFromFile("Shaders/Shadows/CascadeMatrixGen.hlsl", CompileInfo, ByteCode);
    TEST_EXPECT(bCompiled);

    if (bCompiled)
    {
        TEST_SECTION("The backend accepts the compiled shader");

        FRHIComputeShaderRef ComputeShader = RHI::CreateComputeShader(ByteCode);
        TEST_EXPECT(ComputeShader != nullptr);

        if (ComputeShader)
        {
            TEST_SECTION("A pipeline resolves the shader's bindings");

            FRHIComputePipelineStateDesc PipelineDesc;
            PipelineDesc.Shader = ComputeShader.Get();

            FRHIComputePipelineStateRef PipelineState = RHI::CreateComputePipelineState(PipelineDesc);
            TEST_EXPECT(PipelineState != nullptr);
        }
    }

#if PLATFORM_MACOS
    if (bCompiled && RHI::Device->GetRHIType() == ERHIType::Metal)
    {
        TEST_SECTION("The compute blob carries a non-zero threadgroup size");
        FMSLShaderHeader Header;
        Memory::Memcpy(&Header, ByteCode.Data(), sizeof(FMSLShaderHeader));
        TEST_EXPECT(Header.ThreadGroupSizeX != 0);
    }
#endif

#if PLATFORM_MACOS
    if (RHI::Device->GetRHIType() == ERHIType::Metal)
    {
        TEST_SECTION("A compute PSO materialises a static sampler on s0");

        const String SamplerSource(
            "SamplerState LinearSampler : register(s0);\n"
            "Texture2D<float4> SourceTex : register(t0);\n"
            "RWTexture2D<float4> DestTex : register(u0);\n"
            "[numthreads(1, 1, 1)]\n"
            "void Main(uint3 DispatchThreadID : SV_DispatchThreadID)\n"
            "{\n"
            "    DestTex[DispatchThreadID.xy] = SourceTex.SampleLevel(LinearSampler, float2(0.5, 0.5), 0);\n"
            "}\n");

        TArray<uint8> SamplerByteCode;
        const FShaderCompileInfo SamplerCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        const bool bSamplerCompiled = FShaderCompiler::Get().CompileFromSource(SamplerSource, SamplerCompileInfo, SamplerByteCode);
        TEST_EXPECT(bSamplerCompiled);

        if (bSamplerCompiled)
        {
            FRHIComputeShaderRef SamplerComputeShader = RHI::CreateComputeShader(SamplerByteCode);
            TEST_EXPECT(SamplerComputeShader != nullptr);

            if (SamplerComputeShader)
            {
                FRHIStaticSamplerInfo StaticSampler;
                StaticSampler.ShaderRegister   = 0;
                StaticSampler.ShaderVisibility = EShaderStage::Compute;
                StaticSampler.Filter           = ESamplerFilter::MinMagMipLinear;

                FRHIComputePipelineStateDesc SamplerPipelineDesc;
                SamplerPipelineDesc.Shader         = SamplerComputeShader.Get();
                SamplerPipelineDesc.StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>(&StaticSampler, 1);

                FRHIComputePipelineStateRef SamplerPipelineState = RHI::CreateComputePipelineState(SamplerPipelineDesc);
                TEST_EXPECT(SamplerPipelineState != nullptr);

                if (SamplerPipelineState)
                {
                    const FMetalComputePipelineStateRHI* MetalPipeline = static_cast<const FMetalComputePipelineStateRHI*>(SamplerPipelineState.Get());
                    TEST_EXPECT(MetalPipeline->GetBindings().GetSlot(EShaderVisibility::Compute, EMSLBindingType::Sampler, 0) != FMetalPipelineBindingLayout::InvalidSlot);
                    TEST_EXPECT(MetalPipeline->HasStaticSampler(EShaderVisibility::Compute, 0));
                }
            }
        }
    }
#endif

    FShaderCompiler::Destroy();
    TEST_END();
}

static bool ProbeTimestamps();
static bool ProbeIndirectCommands();
#if PLATFORM_MACOS
static bool ProbeDispatchMesh();
#endif

static bool ProbeCommandRecording()
{
    TEST_BEGIN();

    if (RHI::Device->GetRHIType() == ERHIType::Metal && RHI::bSupportsTimestampQueries)
    {
        TEST_EXPECT(ProbeTimestamps());
    }

    TEST_SECTION("WriteFence becomes signaled after submit");
    {
        FRHIFenceRef Fence = RHI::CreateFence();
        TEST_EXPECT(Fence != nullptr);

        if (Fence)
        {
            const uint32 Dummy = 0;
            FRHIBufferRef Scratch = RHI::CreateBuffer(FRHIBufferDesc::CreateVertexBuffer(sizeof(uint32), 1), ERHIResourceState::Common, &Dummy);
            TEST_EXPECT(Scratch != nullptr);

            FRHICommandList CommandList;
            if (Scratch)
            {
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                    Scratch.Get(), ERHIResourceState::Common, ERHIResourceState::VertexBuffer));
            }
            CommandList.WriteFence(Fence.Get());
            FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
            FRHICommandListExecutor::Get().WaitForCommands();

            TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));
            TEST_EXPECT(Fence->IsSignaled());
        }
    }

    TEST_SECTION("ClearDepthStencilView records without crashing");
    {
        FRHITextureRef DepthTarget = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
            EFormat::D32_Float, 8, 8, 1, 1, ETextureUsageFlags::DepthStencil));
        TEST_EXPECT(DepthTarget != nullptr);
        TEST_EXPECT(DepthTarget && DepthTarget->GetDepthStencilView() != nullptr);

        if (DepthTarget && DepthTarget->GetDepthStencilView())
        {
            FRHICommandList CommandList;
            CommandList.ClearDepthStencilView(DepthTarget->GetDepthStencilView(), 1.0f, 0);
            FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
        }
    }

    TEST_SECTION("Buffer UAV clear copies the pattern to a readback");
    {
        const uint32 ClearValues[4] = { 7u, 7u, 7u, 7u };
        const uint32 NumUInt4       = 2;
        const uint64 ByteSize       = NumUInt4 * sizeof(uint32) * 4;

        const FRHIBufferDesc UAVDesc(EBufferFlags::Default | EBufferFlags::RWBuffer | EBufferFlags::CopySource, sizeof(uint32), ByteSize);
        FRHIBufferRef UAVBuffer = RHI::CreateBuffer(UAVDesc);
        TEST_EXPECT(UAVBuffer != nullptr);

        FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(ByteSize));
        TEST_EXPECT(ReadbackBuffer != nullptr);

        if (UAVBuffer && ReadbackBuffer)
        {
            FRHIUnorderedAccessViewRef BufferUAV = RHI::CreateUnorderedAccessView(
                UAVBuffer.Get(), FRHIUnorderedAccessViewDesc::CreateTypedBuffer(0, NumUInt4, EFormat::R32G32B32A32_Uint));
            TEST_EXPECT(BufferUAV != nullptr);

            if (BufferUAV)
            {
                FRHIFenceRef Fence = RHI::CreateFence();
                FRHICommandList CommandList;
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                    UAVBuffer.Get(), ERHIResourceState::Common, ERHIResourceState::UnorderedAccess));
                CommandList.ClearUnorderedAccessViewUint(BufferUAV.Get(), ClearValues);
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                    UAVBuffer.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
                CommandList.CopyBuffer(ReadbackBuffer.Get(), UAVBuffer.Get(), FRHIBufferCopyDesc(0, 0, ByteSize));
                CommandList.WriteFence(Fence.Get());
                FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
                FRHICommandListExecutor::Get().WaitForCommands();

                TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));

                const uint32* Mapped = static_cast<const uint32*>(ReadbackBuffer->Map());
                TEST_EXPECT(Mapped != nullptr);
                if (Mapped)
                {
                    TEST_EXPECT_EQ(Mapped[0], 7u);
                    TEST_EXPECT_EQ(Mapped[7], 7u);
                    ReadbackBuffer->Unmap();
                }
            }
        }
    }

#if PLATFORM_MACOS
    if (RHI::Device->GetRHIType() == ERHIType::Metal)
    {
        if (!FShaderCompiler::Initialize(Paths::GetAssetDir()))
        {
            TEST_EXPECT(false);
        }
        else
        {
            TEST_SECTION("A static sampler dispatch samples into a UAV");
            {
                const uint8 Magenta[4] = { 255, 0, 255, 255 };
                uint8 SourcePixels[4 * 4 * 4];
                for (int32 Index = 0; Index < 16; ++Index)
                {
                    SourcePixels[Index * 4 + 0] = Magenta[0];
                    SourcePixels[Index * 4 + 1] = Magenta[1];
                    SourcePixels[Index * 4 + 2] = Magenta[2];
                    SourcePixels[Index * 4 + 3] = Magenta[3];
                }

                FBootTextureData SourceData(SourcePixels, 4 * 4, 4 * 4 * 4);
                FRHITextureRef SourceTexture = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
                    EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1, ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest),
                    ERHIResourceState::Common, &SourceData);
                TEST_EXPECT(SourceTexture != nullptr);

                FRHITextureRef DestTexture = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
                    EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1,
                    ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopySource));
                TEST_EXPECT(DestTexture != nullptr);

                const String SamplerSource(
                    "SamplerState LinearSampler : register(s0);\n"
                    "Texture2D<float4> SourceTex : register(t0);\n"
                    "RWTexture2D<float4> DestTex : register(u0);\n"
                    "[numthreads(1, 1, 1)]\n"
                    "void Main(uint3 DispatchThreadID : SV_DispatchThreadID)\n"
                    "{\n"
                    "    DestTex[DispatchThreadID.xy] = SourceTex.SampleLevel(LinearSampler, float2(0.5, 0.5), 0);\n"
                    "}\n");

                TArray<uint8> SamplerByteCode;
                const FShaderCompileInfo SamplerCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
                const bool bSamplerCompiled = FShaderCompiler::Get().CompileFromSource(SamplerSource, SamplerCompileInfo, SamplerByteCode);
                TEST_EXPECT(bSamplerCompiled);

                if (bSamplerCompiled && SourceTexture && DestTexture)
                {
                    FRHIComputeShaderRef SamplerComputeShader = RHI::CreateComputeShader(SamplerByteCode);
                    TEST_EXPECT(SamplerComputeShader != nullptr);

                    FRHIStaticSamplerInfo StaticSampler;
                    StaticSampler.ShaderRegister   = 0;
                    StaticSampler.ShaderVisibility = EShaderStage::Compute;
                    StaticSampler.Filter           = ESamplerFilter::MinMagMipLinear;

                    FRHIComputePipelineStateDesc SamplerPipelineDesc;
                    SamplerPipelineDesc.Shader         = SamplerComputeShader.Get();
                    SamplerPipelineDesc.StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>(&StaticSampler, 1);

                    FRHIComputePipelineStateRef SamplerPipelineState = RHI::CreateComputePipelineState(SamplerPipelineDesc);
                    TEST_EXPECT(SamplerPipelineState != nullptr);

                    FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(256));
                    TEST_EXPECT(ReadbackBuffer != nullptr);

                    if (SamplerComputeShader && SamplerPipelineState && ReadbackBuffer
                        && SourceTexture->GetShaderResourceView() && DestTexture->GetUnorderedAccessView())
                    {
                        FRHIFenceRef Fence = RHI::CreateFence();
                        FRHICommandList CommandList;
                        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
                            SourceTexture.Get(), ERHIResourceState::Common, ERHIResourceState::ShaderResource));
                        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
                            DestTexture.Get(), ERHIResourceState::Common, ERHIResourceState::UnorderedAccess));
                        CommandList.SetComputePipelineState(SamplerPipelineState.Get());
                        CommandList.SetShaderResourceView(SamplerComputeShader.Get(), SourceTexture->GetShaderResourceView(), 0);
                        CommandList.SetUnorderedAccessView(SamplerComputeShader.Get(), DestTexture->GetUnorderedAccessView(), 0);
                        CommandList.Dispatch(1, 1, 1);
                        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
                            DestTexture.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
                        CommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), 0, DestTexture.Get(), FTextureRegion2D(1, 1), 0);
                        CommandList.WriteFence(Fence.Get());
                        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
                        FRHICommandListExecutor::Get().WaitForCommands();

                        TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));

                        const uint8* Mapped = static_cast<const uint8*>(ReadbackBuffer->Map());
                        TEST_EXPECT(Mapped != nullptr);
                        if (Mapped)
                        {
                            TEST_EXPECT_EQ(Mapped[0], static_cast<uint8>(255));
                            TEST_EXPECT_EQ(Mapped[1], static_cast<uint8>(0));
                            TEST_EXPECT_EQ(Mapped[2], static_cast<uint8>(255));
                            ReadbackBuffer->Unmap();
                        }
                    }
                }
            }

            TEST_SECTION("An offscreen Draw(3) writes a red pixel");
            {
                const String VertexSource(
                    "float4 Main(uint VertexID : SV_VertexID) : SV_Position\n"
                    "{\n"
                    "    float2 Positions[3];\n"
                    "    Positions[0] = float2(-1.0, -1.0);\n"
                    "    Positions[1] = float2( 3.0, -1.0);\n"
                    "    Positions[2] = float2(-1.0,  3.0);\n"
                    "    return float4(Positions[VertexID], 0.0, 1.0);\n"
                    "}\n");

                const String PixelSource(
                    "float4 Main() : SV_Target\n"
                    "{\n"
                    "    return float4(1.0, 0.0, 0.0, 1.0);\n"
                    "}\n");

                TArray<uint8> VertexByteCode;
                TArray<uint8> PixelByteCode;
                const FShaderCompileInfo VertexCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Vertex);
                const FShaderCompileInfo PixelCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Pixel);
                const bool bVertexCompiled = FShaderCompiler::Get().CompileFromSource(VertexSource, VertexCompileInfo, VertexByteCode);
                const bool bPixelCompiled  = FShaderCompiler::Get().CompileFromSource(PixelSource, PixelCompileInfo, PixelByteCode);
                TEST_EXPECT(bVertexCompiled);
                TEST_EXPECT(bPixelCompiled);

                if (bVertexCompiled && bPixelCompiled)
                {
                    FRHIVertexShaderRef VertexShader = RHI::CreateVertexShader(VertexByteCode);
                    FRHIPixelShaderRef  PixelShader  = RHI::CreatePixelShader(PixelByteCode);
                    TEST_EXPECT(VertexShader != nullptr);
                    TEST_EXPECT(PixelShader != nullptr);

                    FRHIRasterizerStateDesc RasterizerDesc;
                    RasterizerDesc.CullMode = ECullMode::None;

                    FRHIDepthStencilStateDesc DepthStencilDesc;
                    DepthStencilDesc.bDepthEnable      = false;
                    DepthStencilDesc.bDepthWriteEnable = false;

                    FRHIDepthStencilStateRef DepthStencilState = RHI::CreateDepthStencilState(DepthStencilDesc);
                    FRHIRasterizerStateRef   RasterizerState   = RHI::CreateRasterizerState(RasterizerDesc);
                    FRHIBlendStateRef        BlendState        = RHI::CreateBlendState(FRHIBlendStateDesc());

                    FRHIGraphicsPipelineStateDesc GraphicsDesc;
                    GraphicsDesc.VertexShader                         = VertexShader.Get();
                    GraphicsDesc.PixelShader                          = PixelShader.Get();
                    GraphicsDesc.DepthStencilState                    = DepthStencilState.Get();
                    GraphicsDesc.RasterizerState                      = RasterizerState.Get();
                    GraphicsDesc.BlendState                           = BlendState.Get();
                    GraphicsDesc.PrimitiveTopology                    = EPrimitiveTopology::TriangleList;
                    GraphicsDesc.RasterizerOutputFormats.NumRenderTargets = 1;
                    GraphicsDesc.RasterizerOutputFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;

                    FRHIGraphicsPipelineStateRef GraphicsPipeline = RHI::CreateGraphicsPipelineState(GraphicsDesc);
                    TEST_EXPECT(GraphicsPipeline != nullptr);

                    if (RHI::bSupportsViewInstancing)
                    {
                        TEST_SECTION("A graphics PSO with view instancing creates on this device");
                        GraphicsDesc.ViewInstancingState.bEnableViewInstancing = 1;
                        GraphicsDesc.ViewInstancingState.NumArraySlices        = 2;
                        FRHIGraphicsPipelineStateRef InstancedPipeline = RHI::CreateGraphicsPipelineState(GraphicsDesc);
                        TEST_EXPECT(InstancedPipeline != nullptr);
                        if (InstancedPipeline)
                        {
                            const FMetalGraphicsPipelineStateRHI* MetalPipeline = static_cast<const FMetalGraphicsPipelineStateRHI*>(InstancedPipeline.Get());
                            TEST_EXPECT(MetalPipeline->GetViewInstancingState().bEnableViewInstancing);
                        }
                    }

                    FRHITextureRef RenderTarget = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
                        EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1,
                        ETextureUsageFlags::RenderTarget | ETextureUsageFlags::CopySource));
                    TEST_EXPECT(RenderTarget != nullptr);
                    TEST_EXPECT(RenderTarget && RenderTarget->GetRenderTargetView() != nullptr);

                    FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(256));
                    TEST_EXPECT(ReadbackBuffer != nullptr);

                    if (GraphicsPipeline && RenderTarget && RenderTarget->GetRenderTargetView() && ReadbackBuffer)
                    {
                        FRHIBeginRenderPassDesc::FRenderTargetAttachments Attachments;
                        Attachments[0] = FRHIRenderTargetAttachment(
                            RenderTarget->GetRenderTargetView(),
                            EAttachmentLoadAction::Clear,
                            EAttachmentStoreAction::Store);

                        FRHIFenceRef Fence = RHI::CreateFence();
                        FRHICommandList CommandList;
                        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
                            RenderTarget.Get(), ERHIResourceState::Common, ERHIResourceState::RenderTarget));
                        CommandList.BeginRenderPass(FRHIBeginRenderPassDesc(Attachments, 1));
                        CommandList.SetGraphicsPipelineState(GraphicsPipeline.Get());
                        CommandList.SetViewport(FViewportRegion(4.0f, 4.0f, 0.0f, 0.0f, 0.0f, 1.0f));
                        CommandList.Draw(3, 0);
                        CommandList.EndRenderPass();
                        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
                            RenderTarget.Get(), ERHIResourceState::RenderTarget, ERHIResourceState::CopySource));
                        CommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), 0, RenderTarget.Get(), FTextureRegion2D(1, 1), 0);
                        CommandList.WriteFence(Fence.Get());
                        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
                        FRHICommandListExecutor::Get().WaitForCommands();

                        TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));

                        const uint8* Mapped = static_cast<const uint8*>(ReadbackBuffer->Map());
                        TEST_EXPECT(Mapped != nullptr);
                        if (Mapped)
                        {
                            TEST_EXPECT_EQ(Mapped[0], static_cast<uint8>(255));
                            TEST_EXPECT_EQ(Mapped[1], static_cast<uint8>(0));
                            TEST_EXPECT_EQ(Mapped[2], static_cast<uint8>(0));
                            ReadbackBuffer->Unmap();
                        }
                    }
                }
            }

            TEST_SECTION("Indirect draw, indexed draw and dispatch");
            TEST_EXPECT(ProbeIndirectCommands());

            TEST_SECTION("Mesh dispatch");
            TEST_EXPECT(ProbeDispatchMesh());

            FShaderCompiler::Destroy();
        }
    }
#endif

    TEST_END();
}

static bool ProbeIndirectCommands()
{
    TEST_BEGIN();

    if (!RHI::bSupportsDrawIndirect && !RHI::bSupportsDispatchIndirect)
    {
        TEST_END();
        return true;
    }

    const String VertexSource(
        "float4 Main(uint VertexID : SV_VertexID) : SV_Position\n"
        "{\n"
        "    float2 Positions[3];\n"
        "    Positions[0] = float2(-1.0, -1.0);\n"
        "    Positions[1] = float2( 3.0, -1.0);\n"
        "    Positions[2] = float2(-1.0,  3.0);\n"
        "    return float4(Positions[VertexID], 0.0, 1.0);\n"
        "}\n");

    const String PixelSource(
        "float4 Main() : SV_Target\n"
        "{\n"
        "    return float4(1.0, 0.0, 0.0, 1.0);\n"
        "}\n");

    const String ComputeSource(
        "RWBuffer<uint> OutBuf : register(u0);\n"
        "[numthreads(1, 1, 1)]\n"
        "void Main(uint3 DispatchThreadID : SV_DispatchThreadID)\n"
        "{\n"
        "    OutBuf[0] = 42;\n"
        "}\n");

    TArray<uint8> VertexByteCode;
    TArray<uint8> PixelByteCode;
    TArray<uint8> ComputeByteCode;
    const FShaderCompileInfo VertexCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Vertex);
    const FShaderCompileInfo PixelCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Pixel);
    const FShaderCompileInfo ComputeCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    const bool bVertexCompiled  = FShaderCompiler::Get().CompileFromSource(VertexSource, VertexCompileInfo, VertexByteCode);
    const bool bPixelCompiled   = FShaderCompiler::Get().CompileFromSource(PixelSource, PixelCompileInfo, PixelByteCode);
    const bool bComputeCompiled = FShaderCompiler::Get().CompileFromSource(ComputeSource, ComputeCompileInfo, ComputeByteCode);
    TEST_EXPECT(bVertexCompiled);
    TEST_EXPECT(bPixelCompiled);
    TEST_EXPECT(bComputeCompiled);

    FRHIGraphicsPipelineStateRef GraphicsPipeline;
    FRHIVertexShaderRef          VertexShader;
    FRHIPixelShaderRef           PixelShader;
    FRHIDepthStencilStateRef     DepthStencilState;
    FRHIRasterizerStateRef       RasterizerState;
    FRHIBlendStateRef            BlendState;
    if (bVertexCompiled && bPixelCompiled)
    {
        VertexShader = RHI::CreateVertexShader(VertexByteCode);
        PixelShader  = RHI::CreatePixelShader(PixelByteCode);

        FRHIRasterizerStateDesc RasterizerDesc;
        RasterizerDesc.CullMode = ECullMode::None;

        FRHIDepthStencilStateDesc DepthStencilDesc;
        DepthStencilDesc.bDepthEnable      = false;
        DepthStencilDesc.bDepthWriteEnable = false;

        DepthStencilState = RHI::CreateDepthStencilState(DepthStencilDesc);
        RasterizerState   = RHI::CreateRasterizerState(RasterizerDesc);
        BlendState        = RHI::CreateBlendState(FRHIBlendStateDesc());

        FRHIGraphicsPipelineStateDesc GraphicsDesc;
        GraphicsDesc.VertexShader                                   = VertexShader.Get();
        GraphicsDesc.PixelShader                                    = PixelShader.Get();
        GraphicsDesc.DepthStencilState                              = DepthStencilState.Get();
        GraphicsDesc.RasterizerState                                = RasterizerState.Get();
        GraphicsDesc.BlendState                                     = BlendState.Get();
        GraphicsDesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        GraphicsDesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        GraphicsDesc.RasterizerOutputFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
        GraphicsPipeline = RHI::CreateGraphicsPipelineState(GraphicsDesc);
        TEST_EXPECT(GraphicsPipeline != nullptr);
    }

    auto ExpectRedPixel = [](FRHIBuffer* ReadbackBuffer) -> bool
    {
        const uint8* Mapped = static_cast<const uint8*>(ReadbackBuffer->Map());
        const bool   bRed   = Mapped && Mapped[0] == 255 && Mapped[1] == 0 && Mapped[2] == 0;
        if (Mapped)
        {
            ReadbackBuffer->Unmap();
        }

        return bRed;
    };

    if (RHI::bSupportsDrawIndirect && GraphicsPipeline)
    {
        TEST_SECTION("DrawIndirect fills a render target from GPU arguments");
        {
            FRHIDrawIndirectParameters Args;
            Args.VertexCountPerInstance = 3;
            Args.InstanceCount          = 1;
            Args.StartVertexLocation    = 0;
            Args.StartInstanceLocation  = 0;

            FRHIBufferRef ArgumentBuffer = RHI::CreateBuffer(FRHIBufferDesc(
                EBufferFlags::Default | EBufferFlags::IndirectArguments | EBufferFlags::CopyDest,
                sizeof(FRHIDrawIndirectParameters),
                sizeof(FRHIDrawIndirectParameters)));
            TEST_EXPECT(ArgumentBuffer != nullptr);

            FRHITextureRef RenderTarget = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
                EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1,
                ETextureUsageFlags::RenderTarget | ETextureUsageFlags::CopySource));
            FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(256));
            TEST_EXPECT(RenderTarget != nullptr);
            TEST_EXPECT(ReadbackBuffer != nullptr);

            if (ArgumentBuffer && RenderTarget && RenderTarget->GetRenderTargetView() && ReadbackBuffer)
            {
                FRHIBeginRenderPassDesc::FRenderTargetAttachments Attachments;
                Attachments[0] = FRHIRenderTargetAttachment(
                    RenderTarget->GetRenderTargetView(),
                    EAttachmentLoadAction::Clear,
                    EAttachmentStoreAction::Store);

                FRHIFenceRef Fence = RHI::CreateFence();
                FRHICommandList CommandList;
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                    ArgumentBuffer.Get(), ERHIResourceState::Common, ERHIResourceState::CopyDest));
                CommandList.UpdateBuffer(ArgumentBuffer.Get(), FBufferRegion(0, sizeof(Args)), &Args);
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                    ArgumentBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::IndirectArgument));
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
                    RenderTarget.Get(), ERHIResourceState::Common, ERHIResourceState::RenderTarget));
                CommandList.BeginRenderPass(FRHIBeginRenderPassDesc(Attachments, 1));
                CommandList.SetGraphicsPipelineState(GraphicsPipeline.Get());
                CommandList.SetViewport(FViewportRegion(4.0f, 4.0f, 0.0f, 0.0f, 0.0f, 1.0f));
                CommandList.DrawIndirect(ArgumentBuffer.Get(), 0, 1);
                CommandList.EndRenderPass();
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
                    RenderTarget.Get(), ERHIResourceState::RenderTarget, ERHIResourceState::CopySource));
                CommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), 0, RenderTarget.Get(), FTextureRegion2D(1, 1), 0);
                CommandList.WriteFence(Fence.Get());
                FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
                FRHICommandListExecutor::Get().WaitForCommands();
                TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));
                TEST_EXPECT(ExpectRedPixel(ReadbackBuffer.Get()));
            }
        }

        TEST_SECTION("DrawIndexedIndirect fills a render target from GPU arguments");
        {
            FRHIDrawIndexedIndirectParameters Args;
            Args.IndexCountPerInstance = 3;
            Args.InstanceCount         = 1;
            Args.StartIndexLocation    = 0;
            Args.BaseVertexLocation    = 0;
            Args.StartInstanceLocation = 0;

            const uint16 Indices[3] = { 0, 1, 2 };
            FRHIBufferRef IndexBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateIndexBuffer(
                EIndexFormat::uint16, 3, EBufferFlags::Default | EBufferFlags::CopyDest));
            TEST_EXPECT(IndexBuffer != nullptr);

            FRHIBufferRef ArgumentBuffer = RHI::CreateBuffer(FRHIBufferDesc(
                EBufferFlags::Default | EBufferFlags::IndirectArguments | EBufferFlags::CopyDest,
                sizeof(FRHIDrawIndexedIndirectParameters),
                sizeof(FRHIDrawIndexedIndirectParameters)));
            TEST_EXPECT(ArgumentBuffer != nullptr);

            FRHITextureRef RenderTarget = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
                EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1,
                ETextureUsageFlags::RenderTarget | ETextureUsageFlags::CopySource));
            FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(256));
            TEST_EXPECT(RenderTarget != nullptr);
            TEST_EXPECT(ReadbackBuffer != nullptr);

            if (IndexBuffer && ArgumentBuffer && RenderTarget && RenderTarget->GetRenderTargetView() && ReadbackBuffer)
            {
                FRHIBeginRenderPassDesc::FRenderTargetAttachments Attachments;
                Attachments[0] = FRHIRenderTargetAttachment(
                    RenderTarget->GetRenderTargetView(),
                    EAttachmentLoadAction::Clear,
                    EAttachmentStoreAction::Store);

                FRHIFenceRef Fence = RHI::CreateFence();
                FRHICommandList CommandList;
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                    IndexBuffer.Get(), ERHIResourceState::Common, ERHIResourceState::CopyDest));
                CommandList.UpdateBuffer(IndexBuffer.Get(), FBufferRegion(0, sizeof(Indices)), Indices);
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                    IndexBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::IndexBuffer));
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                    ArgumentBuffer.Get(), ERHIResourceState::Common, ERHIResourceState::CopyDest));
                CommandList.UpdateBuffer(ArgumentBuffer.Get(), FBufferRegion(0, sizeof(Args)), &Args);
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                    ArgumentBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::IndirectArgument));
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
                    RenderTarget.Get(), ERHIResourceState::Common, ERHIResourceState::RenderTarget));
                CommandList.BeginRenderPass(FRHIBeginRenderPassDesc(Attachments, 1));
                CommandList.SetGraphicsPipelineState(GraphicsPipeline.Get());
                CommandList.SetIndexBuffer(IndexBuffer.Get(), EIndexFormat::uint16);
                CommandList.SetViewport(FViewportRegion(4.0f, 4.0f, 0.0f, 0.0f, 0.0f, 1.0f));
                CommandList.DrawIndexedIndirect(ArgumentBuffer.Get(), 0, 1);
                CommandList.EndRenderPass();
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
                    RenderTarget.Get(), ERHIResourceState::RenderTarget, ERHIResourceState::CopySource));
                CommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), 0, RenderTarget.Get(), FTextureRegion2D(1, 1), 0);
                CommandList.WriteFence(Fence.Get());
                FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
                FRHICommandListExecutor::Get().WaitForCommands();
                TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));
                TEST_EXPECT(ExpectRedPixel(ReadbackBuffer.Get()));
            }
        }

        TEST_SECTION("DrawIndirect CommandCount 2 encodes two packed records");
        {
            FRHIDrawIndirectParameters Args[2];
            Args[0].VertexCountPerInstance = 3;
            Args[0].InstanceCount          = 1;
            Args[1].VertexCountPerInstance = 3;
            Args[1].InstanceCount          = 1;

            FRHIBufferRef ArgumentBuffer = RHI::CreateBuffer(FRHIBufferDesc(
                EBufferFlags::Default | EBufferFlags::IndirectArguments | EBufferFlags::CopyDest,
                sizeof(FRHIDrawIndirectParameters),
                sizeof(Args)));
            TEST_EXPECT(ArgumentBuffer != nullptr);

            FRHITextureRef RenderTarget = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
                EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1,
                ETextureUsageFlags::RenderTarget | ETextureUsageFlags::CopySource));
            FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(256));
            TEST_EXPECT(RenderTarget != nullptr);
            TEST_EXPECT(ReadbackBuffer != nullptr);

            if (ArgumentBuffer && RenderTarget && RenderTarget->GetRenderTargetView() && ReadbackBuffer)
            {
                FRHIBeginRenderPassDesc::FRenderTargetAttachments Attachments;
                Attachments[0] = FRHIRenderTargetAttachment(
                    RenderTarget->GetRenderTargetView(),
                    EAttachmentLoadAction::Clear,
                    EAttachmentStoreAction::Store);

                FRHIFenceRef Fence = RHI::CreateFence();
                FRHICommandList CommandList;
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                    ArgumentBuffer.Get(), ERHIResourceState::Common, ERHIResourceState::CopyDest));
                CommandList.UpdateBuffer(ArgumentBuffer.Get(), FBufferRegion(0, sizeof(Args)), Args);
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                    ArgumentBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::IndirectArgument));
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
                    RenderTarget.Get(), ERHIResourceState::Common, ERHIResourceState::RenderTarget));
                CommandList.BeginRenderPass(FRHIBeginRenderPassDesc(Attachments, 1));
                CommandList.SetGraphicsPipelineState(GraphicsPipeline.Get());
                CommandList.SetViewport(FViewportRegion(4.0f, 4.0f, 0.0f, 0.0f, 0.0f, 1.0f));
                CommandList.DrawIndirect(ArgumentBuffer.Get(), 0, 2);
                CommandList.EndRenderPass();
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
                    RenderTarget.Get(), ERHIResourceState::RenderTarget, ERHIResourceState::CopySource));
                CommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), 0, RenderTarget.Get(), FTextureRegion2D(1, 1), 0);
                CommandList.WriteFence(Fence.Get());
                FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
                FRHICommandListExecutor::Get().WaitForCommands();
                TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));
                TEST_EXPECT(ExpectRedPixel(ReadbackBuffer.Get()));
            }
        }
    }

    if (RHI::bSupportsDispatchIndirect && bComputeCompiled)
    {
        TEST_SECTION("DispatchIndirect runs a compute grid from GPU arguments");
        {
            FRHIComputeShaderRef ComputeShader = RHI::CreateComputeShader(ComputeByteCode);
            TEST_EXPECT(ComputeShader != nullptr);

            FRHIComputePipelineStateDesc PipelineDesc;
            PipelineDesc.Shader = ComputeShader.Get();
            FRHIComputePipelineStateRef PipelineState = RHI::CreateComputePipelineState(PipelineDesc);
            TEST_EXPECT(PipelineState != nullptr);

            const uint32 Zero = 0;
            FRHIBufferRef UAVBuffer = RHI::CreateBuffer(
                FRHIBufferDesc(EBufferFlags::Default | EBufferFlags::RWBuffer | EBufferFlags::CopySource, sizeof(uint32), sizeof(uint32)),
                ERHIResourceState::Common,
                &Zero);
            TEST_EXPECT(UAVBuffer != nullptr);

            FRHIDispatchIndirectParameters Args;
            Args.ThreadGroupCountX = 1;
            Args.ThreadGroupCountY = 1;
            Args.ThreadGroupCountZ = 1;

            FRHIBufferRef ArgumentBuffer = RHI::CreateBuffer(FRHIBufferDesc(
                EBufferFlags::Default | EBufferFlags::IndirectArguments | EBufferFlags::CopyDest,
                sizeof(FRHIDispatchIndirectParameters),
                sizeof(FRHIDispatchIndirectParameters)));
            TEST_EXPECT(ArgumentBuffer != nullptr);

            FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(sizeof(uint32)));
            TEST_EXPECT(ReadbackBuffer != nullptr);

            if (ComputeShader && PipelineState && UAVBuffer && ArgumentBuffer && ReadbackBuffer)
            {
                FRHIUnorderedAccessViewRef BufferUAV = RHI::CreateUnorderedAccessView(
                    UAVBuffer.Get(), FRHIUnorderedAccessViewDesc::CreateTypedBuffer(0, 1, EFormat::R32_Uint));
                TEST_EXPECT(BufferUAV != nullptr);

                if (BufferUAV)
                {
                    FRHIFenceRef Fence = RHI::CreateFence();
                    FRHICommandList CommandList;
                    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                        ArgumentBuffer.Get(), ERHIResourceState::Common, ERHIResourceState::CopyDest));
                    CommandList.UpdateBuffer(ArgumentBuffer.Get(), FBufferRegion(0, sizeof(Args)), &Args);
                    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                        ArgumentBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::IndirectArgument));
                    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                        UAVBuffer.Get(), ERHIResourceState::Common, ERHIResourceState::UnorderedAccess));
                    CommandList.SetComputePipelineState(PipelineState.Get());
                    CommandList.SetUnorderedAccessView(ComputeShader.Get(), BufferUAV.Get(), 0);
                    CommandList.DispatchIndirect(ArgumentBuffer.Get(), 0);
                    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                        UAVBuffer.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
                    CommandList.CopyBuffer(ReadbackBuffer.Get(), UAVBuffer.Get(), FRHIBufferCopyDesc(0, 0, sizeof(uint32)));
                    CommandList.WriteFence(Fence.Get());
                    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
                    FRHICommandListExecutor::Get().WaitForCommands();
                    TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));

                    const uint32* Mapped = static_cast<const uint32*>(ReadbackBuffer->Map());
                    TEST_EXPECT(Mapped != nullptr);
                    if (Mapped)
                    {
                        TEST_EXPECT_EQ(*Mapped, 42u);
                        ReadbackBuffer->Unmap();
                    }
                }
            }
        }
    }

    TEST_END();
}

#if PLATFORM_MACOS
static bool ProbeDispatchMesh()
{
    TEST_BEGIN();

    if (!GMetalSupportsMeshShaders)
    {
        TEST_END();
        return true;
    }

    const String MeshSource(
        "struct VertexOut\n"
        "{\n"
        "    float4 Position : SV_Position;\n"
        "};\n"
        "\n"
        "[outputtopology(\"triangle\")]\n"
        "[numthreads(1, 1, 1)]\n"
        "void Main(out vertices VertexOut Verts[3], out indices uint3 Tris[1])\n"
        "{\n"
        "    SetMeshOutputCounts(3, 1);\n"
        "    Verts[0].Position = float4(-1.0, -1.0, 0.0, 1.0);\n"
        "    Verts[1].Position = float4( 3.0, -1.0, 0.0, 1.0);\n"
        "    Verts[2].Position = float4(-1.0,  3.0, 0.0, 1.0);\n"
        "    Tris[0] = uint3(0, 1, 2);\n"
        "}\n");

    const String PixelSource(
        "float4 Main() : SV_Target\n"
        "{\n"
        "    return float4(1.0, 0.0, 0.0, 1.0);\n"
        "}\n");

    TArray<uint8> MeshByteCode;
    TArray<uint8> PixelByteCode;
    const FShaderCompileInfo MeshCompileInfo("Main", EShaderModel::SM_6_5, EShaderStage::Mesh);
    const FShaderCompileInfo PixelCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Pixel);
    const bool bMeshCompiled  = FShaderCompiler::Get().CompileFromSource(MeshSource, MeshCompileInfo, MeshByteCode);
    const bool bPixelCompiled = FShaderCompiler::Get().CompileFromSource(PixelSource, PixelCompileInfo, PixelByteCode);
    TEST_EXPECT(bMeshCompiled);
    TEST_EXPECT(bPixelCompiled);

    FRHIMeshShaderRef            MeshShader;
    FRHIPixelShaderRef           PixelShader;
    FRHIMeshletPipelineStateRef  MeshletPipeline;
    FRHIDepthStencilStateRef     DepthStencilState;
    FRHIRasterizerStateRef       RasterizerState;
    FRHIBlendStateRef            BlendState;
    if (bMeshCompiled && bPixelCompiled)
    {
        MeshShader  = RHI::CreateMeshShader(MeshByteCode);
        PixelShader = RHI::CreatePixelShader(PixelByteCode);
        if (!MeshShader)
        {
            TEST_END();
            return true;
        }

        FRHIRasterizerStateDesc RasterizerDesc;
        RasterizerDesc.CullMode = ECullMode::None;

        FRHIDepthStencilStateDesc DepthStencilDesc;
        DepthStencilDesc.bDepthEnable      = false;
        DepthStencilDesc.bDepthWriteEnable = false;

        DepthStencilState = RHI::CreateDepthStencilState(DepthStencilDesc);
        RasterizerState   = RHI::CreateRasterizerState(RasterizerDesc);
        BlendState        = RHI::CreateBlendState(FRHIBlendStateDesc());

        FRHIMeshletPipelineStateDesc MeshletDesc;
        MeshletDesc.MeshShader                                    = MeshShader.Get();
        MeshletDesc.PixelShader                                   = PixelShader.Get();
        MeshletDesc.DepthStencilState                             = DepthStencilState.Get();
        MeshletDesc.RasterizerState                               = RasterizerState.Get();
        MeshletDesc.BlendState                                    = BlendState.Get();
        MeshletDesc.RasterizerOutputFormats.NumRenderTargets      = 1;
        MeshletDesc.RasterizerOutputFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
        MeshletPipeline = RHI::CreateMeshletPipelineState(MeshletDesc);
        if (!MeshletPipeline)
        {
            TEST_END();
            return true;
        }
    }

    auto ExpectRedPixel = [](FRHIBuffer* ReadbackBuffer) -> bool
    {
        const uint8* Mapped = static_cast<const uint8*>(ReadbackBuffer->Map());
        const bool   bRed   = Mapped && Mapped[0] == 255 && Mapped[1] == 0 && Mapped[2] == 0;
        if (Mapped)
        {
            ReadbackBuffer->Unmap();
        }

        return bRed;
    };

    auto RecordMeshPass = [&](FRHICommandList& CommandList, FRHITexture* RenderTarget, auto&& DispatchFn)
    {
        FRHIBeginRenderPassDesc::FRenderTargetAttachments Attachments;
        Attachments[0] = FRHIRenderTargetAttachment(
            RenderTarget->GetRenderTargetView(),
            EAttachmentLoadAction::Clear,
            EAttachmentStoreAction::Store);

        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
            RenderTarget, ERHIResourceState::Common, ERHIResourceState::RenderTarget));
        CommandList.BeginRenderPass(FRHIBeginRenderPassDesc(Attachments, 1));
        CommandList.SetMeshletPipelineState(MeshletPipeline.Get());
        CommandList.SetViewport(FViewportRegion(4.0f, 4.0f, 0.0f, 0.0f, 0.0f, 1.0f));
        DispatchFn(CommandList);
        CommandList.EndRenderPass();
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
            RenderTarget, ERHIResourceState::RenderTarget, ERHIResourceState::CopySource));
    };

    if (MeshletPipeline)
    {
        TEST_SECTION("DispatchMesh writes a red pixel");
        {
            FRHITextureRef RenderTarget = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
                EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1,
                ETextureUsageFlags::RenderTarget | ETextureUsageFlags::CopySource));
            FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(256));
            TEST_EXPECT(RenderTarget != nullptr);
            TEST_EXPECT(ReadbackBuffer != nullptr);

            if (RenderTarget && RenderTarget->GetRenderTargetView() && ReadbackBuffer)
            {
                FRHIFenceRef Fence = RHI::CreateFence();
                FRHICommandList CommandList;
                RecordMeshPass(CommandList, RenderTarget.Get(), [](FRHICommandList& List)
                {
                    List.DispatchMesh(1, 1, 1);
                });
                CommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), 0, RenderTarget.Get(), FTextureRegion2D(1, 1), 0);
                CommandList.WriteFence(Fence.Get());
                FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
                FRHICommandListExecutor::Get().WaitForCommands();
                TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));
                TEST_EXPECT(ExpectRedPixel(ReadbackBuffer.Get()));
            }
        }

        TEST_SECTION("DispatchMeshIndirect fills a render target from GPU arguments");
        {
            FRHIDispatchMeshIndirectParameters Args;
            Args.ThreadGroupCountX = 1;
            Args.ThreadGroupCountY = 1;
            Args.ThreadGroupCountZ = 1;

            FRHIBufferRef ArgumentBuffer = RHI::CreateBuffer(FRHIBufferDesc(
                EBufferFlags::Default | EBufferFlags::IndirectArguments | EBufferFlags::CopyDest,
                sizeof(FRHIDispatchMeshIndirectParameters),
                sizeof(FRHIDispatchMeshIndirectParameters)));
            TEST_EXPECT(ArgumentBuffer != nullptr);

            FRHITextureRef RenderTarget = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
                EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1,
                ETextureUsageFlags::RenderTarget | ETextureUsageFlags::CopySource));
            FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(256));
            TEST_EXPECT(RenderTarget != nullptr);
            TEST_EXPECT(ReadbackBuffer != nullptr);

            if (ArgumentBuffer && RenderTarget && RenderTarget->GetRenderTargetView() && ReadbackBuffer)
            {
                FRHIFenceRef Fence = RHI::CreateFence();
                FRHICommandList CommandList;
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                    ArgumentBuffer.Get(), ERHIResourceState::Common, ERHIResourceState::CopyDest));
                CommandList.UpdateBuffer(ArgumentBuffer.Get(), FBufferRegion(0, sizeof(Args)), &Args);
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                    ArgumentBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::IndirectArgument));
                RecordMeshPass(CommandList, RenderTarget.Get(), [&](FRHICommandList& List)
                {
                    List.DispatchMeshIndirect(ArgumentBuffer.Get(), 0, 1);
                });
                CommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), 0, RenderTarget.Get(), FTextureRegion2D(1, 1), 0);
                CommandList.WriteFence(Fence.Get());
                FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
                FRHICommandListExecutor::Get().WaitForCommands();
                TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));
                TEST_EXPECT(ExpectRedPixel(ReadbackBuffer.Get()));
            }
        }
    }

    TEST_END();
}
#endif

static bool ProbeTimestamps()
{
    TEST_BEGIN();

    TEST_SECTION("Timestamp queries resolve to increasing nanoseconds");
    {
        FRHIQueryRef BeginQuery = RHI::CreateQuery(EQueryType::Timestamp);
        FRHIQueryRef EndQuery   = RHI::CreateQuery(EQueryType::Timestamp);
        TEST_EXPECT(BeginQuery != nullptr);
        TEST_EXPECT(EndQuery != nullptr);

        const uint32 Dummy = 0;
        FRHIBufferRef Source = RHI::CreateBuffer(
            FRHIBufferDesc::CreateVertexBuffer(sizeof(uint32), 1, EBufferFlags::Default | EBufferFlags::CopySource),
            ERHIResourceState::Common, &Dummy);
        FRHIBufferRef Dest = RHI::CreateBuffer(
            FRHIBufferDesc::CreateVertexBuffer(sizeof(uint32), 1, EBufferFlags::Default | EBufferFlags::CopyDest));
        TEST_EXPECT(Source != nullptr);
        TEST_EXPECT(Dest != nullptr);

        FRHIFenceRef Fence = RHI::CreateFence();
        TEST_EXPECT(Fence != nullptr);

        if (BeginQuery && EndQuery && Source && Dest && Fence)
        {
            FRHICommandList CommandList;
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                Source.Get(), ERHIResourceState::Common, ERHIResourceState::CopySource));
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                Dest.Get(), ERHIResourceState::Common, ERHIResourceState::CopyDest));
            CommandList.QueryTimestamp(BeginQuery.Get());
            CommandList.CopyBuffer(Dest.Get(), Source.Get(), FRHIBufferCopyDesc(0, 0, sizeof(uint32)));
            CommandList.QueryTimestamp(EndQuery.Get());
            CommandList.WriteFence(Fence.Get());
            FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
            FRHICommandListExecutor::Get().WaitForCommands();

            TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));

            uint64 BeginResult = 0;
            uint64 EndResult   = 0;
            TEST_EXPECT(RHI::Device->GetQueryResult(BeginQuery.Get(), BeginResult, EQueryResultMode::Wait));
            TEST_EXPECT(RHI::Device->GetQueryResult(EndQuery.Get(), EndResult, EQueryResultMode::Wait));
            TEST_EXPECT(EndResult > BeginResult);
        }
    }

    TEST_END();
}

#if PLATFORM_MACOS
static bool ProbeCopyQueue()
{
    TEST_BEGIN();

    TEST_SECTION("Command-list copy then GPU readback");
    {
        const uint32 SourceValue = 0xA1B2C3D4u;
        FRHIBufferRef SourceBuffer = RHI::CreateBuffer(
            FRHIBufferDesc(EBufferFlags::Default | EBufferFlags::CopySource | EBufferFlags::CopyDest, sizeof(uint32), sizeof(uint32)));
        TEST_EXPECT(SourceBuffer != nullptr);

        FRHIBufferRef DestBuffer = RHI::CreateBuffer(
            FRHIBufferDesc(EBufferFlags::Default | EBufferFlags::CopyDest | EBufferFlags::CopySource, sizeof(uint32), sizeof(uint32)));
        TEST_EXPECT(DestBuffer != nullptr);

        FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(sizeof(uint32)));
        TEST_EXPECT(ReadbackBuffer != nullptr);

        if (SourceBuffer && DestBuffer && ReadbackBuffer)
        {
            FRHIFenceRef Fence = RHI::CreateFence();
            FRHICommandList CommandList;
            CommandList.UpdateBuffer(SourceBuffer.Get(), FBufferRegion(0, sizeof(uint32)), &SourceValue);
            CommandList.CopyBuffer(DestBuffer.Get(), SourceBuffer.Get(), FRHIBufferCopyDesc(0, 0, sizeof(uint32)));
            CommandList.CopyBuffer(ReadbackBuffer.Get(), DestBuffer.Get(), FRHIBufferCopyDesc(0, 0, sizeof(uint32)));
            CommandList.WriteFence(Fence.Get());
            FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
            FRHICommandListExecutor::Get().WaitForCommands();
            TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));

            const uint32* Mapped = static_cast<const uint32*>(ReadbackBuffer->Map());
            TEST_EXPECT(Mapped != nullptr);
            if (Mapped)
            {
                TEST_EXPECT_EQ(*Mapped, SourceValue);
                ReadbackBuffer->Unmap();
            }
        }
    }

    TEST_SECTION("Deferred delete after Copy submit recycles after Copy completion");
    {
        const uint32 SourceValue = 7u;
        FRHIBufferRef Transient = RHI::CreateBuffer(
            FRHIBufferDesc(EBufferFlags::Default | EBufferFlags::CopySource, sizeof(uint32), sizeof(uint32)),
            ERHIResourceState::Common,
            &SourceValue);
        TEST_EXPECT(Transient != nullptr);

        FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(sizeof(uint32)));
        TEST_EXPECT(ReadbackBuffer != nullptr);

        if (Transient && ReadbackBuffer)
        {
            FRHIFenceRef Fence = RHI::CreateFence();
            FRHICommandList CommandList;
            CommandList.CopyBuffer(ReadbackBuffer.Get(), Transient.Get(), FRHIBufferCopyDesc(0, 0, sizeof(uint32)));
            CommandList.WriteFence(Fence.Get());
            FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
            FRHICommandListExecutor::Get().WaitForCommands();
            TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));

            Transient.Reset();
            FRHICommandListExecutor::Get().WaitForGPU();

            FRHIBufferRef Replacement = RHI::CreateBuffer(
                FRHIBufferDesc(EBufferFlags::Default | EBufferFlags::CopySource, sizeof(uint32), sizeof(uint32)),
                ERHIResourceState::Common,
                &SourceValue);
            TEST_EXPECT(Replacement != nullptr);
        }
    }

    TEST_END();
}

static bool ProbeParallelRender()
{
    TEST_BEGIN();
    TEST_SECTION("Parallel render encoder clears a render target");

    FMetalDeviceRHI* MetalDeviceRHI = FMetalDeviceRHI::Get();
    TEST_EXPECT(MetalDeviceRHI != nullptr);
    if (!MetalDeviceRHI)
    {
        TEST_END();
    }

    FMetalQueue* DirectQueue = MetalDeviceRHI->GetMetalDevice()->GetQueue(EMetalQueueType::Direct);
    TEST_EXPECT(DirectQueue != nullptr);
    if (!DirectQueue)
    {
        TEST_END();
    }

    FRHITextureRef RenderTarget = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
        EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1,
        ETextureUsageFlags::RenderTarget | ETextureUsageFlags::CopySource));
    TEST_EXPECT(RenderTarget != nullptr);
    TEST_EXPECT(RenderTarget && RenderTarget->GetRenderTargetView() != nullptr);

    FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(256));
    TEST_EXPECT(ReadbackBuffer != nullptr);

    if (RenderTarget && RenderTarget->GetRenderTargetView() && ReadbackBuffer)
    {
        {
            FMetalScopedCommandContext Context(*DirectQueue);
            FRHIBeginRenderPassDesc::FRenderTargetAttachments Attachments;
            Attachments[0] = FRHIRenderTargetAttachment(
                RenderTarget->GetRenderTargetView(),
                EAttachmentLoadAction::Clear,
                EAttachmentStoreAction::Store,
                FFloatColor(1.0f, 0.0f, 0.0f, 1.0f));

            Context->BeginParallelRenderPass(FRHIBeginRenderPassDesc(Attachments, 1));
            FMetalCommandContext* ChildContext = Context->ObtainParallelChildContext();
            TEST_EXPECT(ChildContext != nullptr);
            if (ChildContext)
            {
                Context->ReleaseParallelChildContext(ChildContext);
            }

            Context->EndParallelRenderPass();
        }

        FRHIFenceRef Fence = RHI::CreateFence();
        FRHICommandList CommandList;
        CommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), 0, RenderTarget.Get(), FTextureRegion2D(1, 1), 0);
        CommandList.WriteFence(Fence.Get());
        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
        FRHICommandListExecutor::Get().WaitForCommands();
        TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));

        const uint8* Mapped = static_cast<const uint8*>(ReadbackBuffer->Map());
        TEST_EXPECT(Mapped != nullptr);
        if (Mapped)
        {
            TEST_EXPECT_EQ(Mapped[0], static_cast<uint8>(255));
            TEST_EXPECT_EQ(Mapped[1], static_cast<uint8>(0));
            TEST_EXPECT_EQ(Mapped[2], static_cast<uint8>(0));
            ReadbackBuffer->Unmap();
        }
    }

    TEST_END();
}

static bool ProbeBindlessDescriptors()
{
    TEST_BEGIN();

    if (!RHI::bSupportsBindless)
    {
        TEST_SECTION("Bindless is unavailable on this Metal device");
        TEST_END();
    }

    if (!FShaderCompiler::Initialize(Paths::GetAssetDir()))
    {
        TEST_EXPECT(false);
        TEST_END();
    }

    auto CompileBindless = [](const CHAR* Source, TArray<uint8>& OutByteCode) -> bool
    {
        const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_6, EShaderStage::Compute);
        return FShaderCompiler::Get().CompileFromSource(Source, CompileInfo, OutByteCode);
    };

    ERHIResourceState OutputState = ERHIResourceState::Common;
    auto DispatchAndReadUint = [&OutputState](FRHIComputePipelineState* Pipeline, FRHIComputeShader* Shader, FRHIBuffer* Params, FRHIBuffer* Output, FRHIUnorderedAccessView* OutputUAV, uint32 Expected) -> bool
    {
        FRHIBufferRef Readback = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(sizeof(uint32)));
        if (!Readback)
        {
            return false;
        }

        FRHIFenceRef Fence = RHI::CreateFence();
        FRHICommandList CommandList;
        if (OutputState != ERHIResourceState::UnorderedAccess)
        {
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Output, OutputState, ERHIResourceState::UnorderedAccess));
        }

        CommandList.SetComputePipelineState(Pipeline);
        CommandList.SetConstantBuffer(Shader, Params, 0);
        CommandList.SetUnorderedAccessView(Shader, OutputUAV, 0);
        CommandList.Dispatch(1, 1, 1);
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Output, ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
        OutputState = ERHIResourceState::CopySource;
        CommandList.CopyBuffer(Readback.Get(), Output, FRHIBufferCopyDesc(0, 0, sizeof(uint32)));
        CommandList.WriteFence(Fence.Get());
        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
        FRHICommandListExecutor::Get().WaitForCommands();
        if (!Fence->Wait(5ull * 1000ull * 1000ull * 1000ull))
        {
            return false;
        }

        const uint32* Mapped = static_cast<const uint32*>(Readback->Map());
        const bool bMatched = Mapped && *Mapped == Expected;
        if (Mapped)
        {
            Readback->Unmap();
        }

        return bMatched;
    };

    const FRHIBufferDesc OutputDesc(EBufferFlags::Default | EBufferFlags::RWBuffer | EBufferFlags::CopySource, sizeof(uint32), sizeof(uint32));
    FRHIBufferRef OutputBuffer = RHI::CreateBuffer(OutputDesc);
    TEST_EXPECT(OutputBuffer != nullptr);
    FRHIUnorderedAccessViewRef OutputUAV = OutputBuffer
        ? RHI::CreateUnorderedAccessView(OutputBuffer.Get(), FRHIUnorderedAccessViewDesc::CreateBuffer(0, 1, EBufferViewType::Structured))
        : nullptr;
    TEST_EXPECT(OutputUAV != nullptr);

    TEST_SECTION("Bindless SRV texture and sampler round-trip");
    {
        static const CHAR Source[] =
            "RWStructuredBuffer<uint> OutBuffer : register(u0);\n"
            "cbuffer Params : register(b0) { uint ResourceIndex; uint SamplerIndex; uint Pad0; uint Pad1; };\n"
            "[numthreads(1,1,1)]\n"
            "void Main()\n"
            "{\n"
            "    Texture2D<float4> Tex = ResourceDescriptorHeap[ResourceIndex];\n"
            "    SamplerState Samp = SamplerDescriptorHeap[SamplerIndex];\n"
            "    float4 Color = Tex.SampleLevel(Samp, float2(0.5, 0.5), 0);\n"
            "    OutBuffer[0] = (uint)(Color.x * 255.0f + 0.5f);\n"
            "}\n";

        TArray<uint8> ByteCode;
        TEST_EXPECT(CompileBindless(Source, ByteCode));
        FRHIComputeShaderRef Shader = RHI::CreateComputeShader(ByteCode);
        TEST_EXPECT(Shader != nullptr);

        FRHIComputePipelineStateDesc PipelineDesc;
        PipelineDesc.Shader = Shader.Get();
        FRHIComputePipelineStateRef Pipeline = RHI::CreateComputePipelineState(PipelineDesc);
        TEST_EXPECT(Pipeline != nullptr);

        const uint8 Red[4] = { 255, 0, 0, 255 };
        FBootTextureData RedData(Red, 4, 4);
        FRHITextureRef Texture = RHI::CreateTexture(
            FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 1, 1, 1, 1, ETextureUsageFlags::ShaderResourceTexture),
            ERHIResourceState::Common,
            &RedData);
        TEST_EXPECT(Texture != nullptr);

        FRHISamplerStateRef Sampler = RHI::CreateSamplerState(FRHISamplerStateDesc());
        TEST_EXPECT(Sampler != nullptr);

        const FRHIDescriptorHandle TextureHandle = Texture ? Texture->GetBindlessSRVHandle() : FRHIDescriptorHandle();
        const FRHIDescriptorHandle SamplerHandle = Sampler ? Sampler->GetBindlessHandle() : FRHIDescriptorHandle();
        TEST_EXPECT(TextureHandle.IsValid());
        TEST_EXPECT(SamplerHandle.IsValid());

        uint32 ParamsData[4] = { TextureHandle.Index, SamplerHandle.Index, 0, 0 };
        FRHIBufferRef Params = RHI::CreateBuffer(FRHIBufferDesc::CreateConstantBuffer(sizeof(ParamsData)), ERHIResourceState::Common, ParamsData);
        TEST_EXPECT(Params != nullptr);

        if (Pipeline && Shader && Params && OutputBuffer && OutputUAV)
        {
            TEST_EXPECT(DispatchAndReadUint(Pipeline.Get(), Shader.Get(), Params.Get(), OutputBuffer.Get(), OutputUAV.Get(), 255u));
        }

        TEST_SECTION("Update-after-submit rewrites the same resource slot");
        const uint8 Green[4] = { 0, 255, 0, 255 };
        FBootTextureData GreenData(Green, 4, 4);
        FRHITextureRef TextureB = RHI::CreateTexture(
            FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 1, 1, 1, 1, ETextureUsageFlags::ShaderResourceTexture),
            ERHIResourceState::Common,
            &GreenData);
        TEST_EXPECT(TextureB != nullptr);

        if (TextureB && TextureHandle.IsValid())
        {
            FMetalDeviceRHI* MetalDeviceRHI = FMetalDeviceRHI::Get();
            FMetalBindlessDescriptorManager* Manager = MetalDeviceRHI ? MetalDeviceRHI->GetMetalDevice()->GetBindlessDescriptorManager() : nullptr;
            TEST_EXPECT(Manager != nullptr);
            FMetalTextureRHI* MetalTextureB = GetMetalTexture(TextureB.Get());
            TEST_EXPECT(MetalTextureB != nullptr && MetalTextureB->GetMTLTexture() != nil);
            if (Manager && MetalTextureB && MetalTextureB->GetMTLTexture())
            {
                Manager->WriteTexture(TextureHandle, MetalTextureB->GetMTLTexture(), false, true, false);
            }

            uint32 UpdateParams[4] = { TextureHandle.Index, SamplerHandle.Index, 0, 0 };
            FRHIBufferRef UpdateBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateConstantBuffer(sizeof(UpdateParams)), ERHIResourceState::Common, UpdateParams);
            if (Pipeline && Shader && UpdateBuffer && OutputBuffer && OutputUAV)
            {
                TEST_EXPECT(DispatchAndReadUint(Pipeline.Get(), Shader.Get(), UpdateBuffer.Get(), OutputBuffer.Get(), OutputUAV.Get(), 0u));
            }
        }
    }

    TEST_SECTION("Bindless UAV structured buffer round-trip");
    {
        static const CHAR Source[] =
            "RWStructuredBuffer<uint> OutBuffer : register(u0);\n"
            "cbuffer Params : register(b0) { uint ResourceIndex; uint Pad0; uint Pad1; uint Pad2; };\n"
            "[numthreads(1,1,1)]\n"
            "void Main()\n"
            "{\n"
            "    RWStructuredBuffer<uint> HeapOut = ResourceDescriptorHeap[ResourceIndex];\n"
            "    HeapOut[0] = 99;\n"
            "    OutBuffer[0] = HeapOut[0];\n"
            "}\n";

        TArray<uint8> ByteCode;
        TEST_EXPECT(CompileBindless(Source, ByteCode));
        FRHIComputeShaderRef Shader = RHI::CreateComputeShader(ByteCode);
        FRHIComputePipelineStateDesc PipelineDesc;
        PipelineDesc.Shader = Shader.Get();
        FRHIComputePipelineStateRef Pipeline = Shader ? RHI::CreateComputePipelineState(PipelineDesc) : nullptr;
        TEST_EXPECT(Pipeline != nullptr);

        const FRHIDescriptorHandle HeapHandle = OutputUAV ? OutputUAV->GetBindlessHandle() : FRHIDescriptorHandle();
        TEST_EXPECT(HeapHandle.IsValid());

        uint32 ParamsData[4] = { HeapHandle.Index, 0, 0, 0 };
        FRHIBufferRef Params = RHI::CreateBuffer(FRHIBufferDesc::CreateConstantBuffer(sizeof(ParamsData)), ERHIResourceState::Common, ParamsData);
        if (Pipeline && Shader && Params && OutputBuffer && OutputUAV)
        {
            TEST_EXPECT(DispatchAndReadUint(Pipeline.Get(), Shader.Get(), Params.Get(), OutputBuffer.Get(), OutputUAV.Get(), 99u));
        }
    }

    TEST_SECTION("Bindless CBV round-trip");
    {
        static const CHAR Source[] =
            "struct FData { uint Value; uint Pad0; uint Pad1; uint Pad2; };\n"
            "RWStructuredBuffer<uint> OutBuffer : register(u0);\n"
            "cbuffer Params : register(b0) { uint ResourceIndex; uint PadA; uint PadB; uint PadC; };\n"
            "[numthreads(1,1,1)]\n"
            "void Main()\n"
            "{\n"
            "    ConstantBuffer<FData> CB = ResourceDescriptorHeap[ResourceIndex];\n"
            "    OutBuffer[0] = CB.Value;\n"
            "}\n";

        TArray<uint8> ByteCode;
        TEST_EXPECT(CompileBindless(Source, ByteCode));
        FRHIComputeShaderRef Shader = RHI::CreateComputeShader(ByteCode);
        FRHIComputePipelineStateDesc PipelineDesc;
        PipelineDesc.Shader = Shader.Get();
        FRHIComputePipelineStateRef Pipeline = Shader ? RHI::CreateComputePipelineState(PipelineDesc) : nullptr;
        TEST_EXPECT(Pipeline != nullptr);

        uint32 ConstantData[4] = { 42u, 0, 0, 0 };
        FRHIBufferRef ConstantBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateConstantBuffer(sizeof(ConstantData)), ERHIResourceState::Common, ConstantData);
        TEST_EXPECT(ConstantBuffer != nullptr);
        const FRHIDescriptorHandle CBVHandle = ConstantBuffer ? ConstantBuffer->GetBindlessHandle() : FRHIDescriptorHandle();
        TEST_EXPECT(CBVHandle.IsValid());

        uint32 ParamsData[4] = { CBVHandle.Index, 0, 0, 0 };
        FRHIBufferRef Params = RHI::CreateBuffer(FRHIBufferDesc::CreateConstantBuffer(sizeof(ParamsData)), ERHIResourceState::Common, ParamsData);
        if (Pipeline && Shader && Params && OutputBuffer && OutputUAV)
        {
            TEST_EXPECT(DispatchAndReadUint(Pipeline.Get(), Shader.Get(), Params.Get(), OutputBuffer.Get(), OutputUAV.Get(), 42u));
        }
    }

    TEST_SECTION("Reuse-after-retirement recycles the bindless index");
    {
        const uint8 Blue[4] = { 0, 0, 255, 255 };
        FBootTextureData BlueData(Blue, 4, 4);
        uint32 FirstIndex = FRHIDescriptorHandle::InvalidHandle;
        {
            FRHITextureRef Texture = RHI::CreateTexture(
                FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 1, 1, 1, 1, ETextureUsageFlags::ShaderResourceTexture),
                ERHIResourceState::Common,
                &BlueData);
            TEST_EXPECT(Texture != nullptr);
            const FRHIDescriptorHandle Handle = Texture ? Texture->GetBindlessSRVHandle() : FRHIDescriptorHandle();
            TEST_EXPECT(Handle.IsValid());
            FirstIndex = Handle.Index;
        }

        if (FRHICommandListExecutor::IsInitialized())
        {
            FRHICommandListExecutor::Get().WaitForGPU();
        }

        FRHITextureRef TextureB = RHI::CreateTexture(
            FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 1, 1, 1, 1, ETextureUsageFlags::ShaderResourceTexture),
            ERHIResourceState::Common,
            &BlueData);
        TEST_EXPECT(TextureB != nullptr);
        const FRHIDescriptorHandle SecondHandle = TextureB ? TextureB->GetBindlessSRVHandle() : FRHIDescriptorHandle();
        TEST_EXPECT(SecondHandle.IsValid());
        TEST_EXPECT_EQ(SecondHandle.Index, FirstIndex);
    }

    TEST_END();
}
#endif

static bool ProbeCapabilityHonesty(ERHIType ExpectedType)
{
    TEST_BEGIN();

    if (ExpectedType == ERHIType::Metal)
    {
        TEST_SECTION("Metal reports ray tracing as unsupported until those subsystems exist");
        TEST_EXPECT(RHI::bSupportsRayTracing == false);
        TEST_EXPECT(RHI::bSupportsInlineRayTracing == false);

        TEST_SECTION("Metal reports timestamp queries when the device has a timestamp counter set");
        TEST_EXPECT(RHI::bSupportsTimestampQueries);

        TEST_SECTION("Metal reports draw and dispatch indirect as supported; mesh-indirect follows the family flag; count stays off");
        TEST_EXPECT(RHI::bSupportsDrawIndirect);
        TEST_EXPECT(RHI::bSupportsDispatchIndirect);
        TEST_EXPECT(RHI::bSupportsDrawIndirectCount == false);
        TEST_EXPECT(RHI::bSupportsDispatchMeshIndirectCount == false);
        TEST_EXPECT(RHI::MaxDrawIndirectCommandCount > 0);
#if PLATFORM_MACOS
        TEST_EXPECT(RHI::bSupportsDispatchMeshIndirect == GMetalSupportsMeshShaders);
        if (GMetalSupportsMeshShaders)
        {
            TEST_EXPECT(RHI::MaxDispatchMeshIndirectCommandCount > 0);
        }
        else
        {
            TEST_EXPECT(RHI::MaxDispatchMeshIndirectCommandCount == 0);
        }
#else
        TEST_EXPECT(RHI::bSupportsDispatchMeshIndirect == false);
#endif

        TEST_SECTION("Metal overwrites Null leftover capability flags");
        TEST_EXPECT(RHI::bSupportsGeometryShaders == false);
        TEST_EXPECT(RHI::SamplePositionsTier == ESamplePositionsTier::NotSupported);
        TEST_EXPECT(RHI::bSupportsVRS == false);
        TEST_EXPECT(RHI::MaxShaderModel == EShaderModel::SM_6_6);
        TEST_EXPECT(RHI::bSupportRenderTargetArrayIndexFromVertexShader);
        TEST_EXPECT(RHI::bSupportsDynamicDepthBias);
    }

    TEST_END();
}

static bool BootRHI(ERHIType ExpectedType)
{
    TEST_BEGIN();

    TEST_SECTION("Initialize");

    SetConsoleVariable("RHI.Type", ToString(ExpectedType));
    SetConsoleVariable("RHI.EnableValidation", true);
    SetConsoleVariable("RHI.EnableValidationDebugBreak", false);
    SetConsoleVariable("TaskGraph.EnableRHIThread", false);
    if (ExpectedType == ERHIType::Metal)
    {
        SetConsoleVariable("RHI.EnableDebugLayer", true);
#if PLATFORM_MACOS
        MetalResetValidationErrors();
#endif
    }

    const bool bInitialized = RHI::Initialize();
    TEST_EXPECT(bInitialized);
    TEST_EXPECT(RHI::Device != nullptr);

    if (bInitialized && RHI::Device)
    {
        TEST_EXPECT_EQ(RHI::Device->GetRHIType(), ExpectedType);
        LOG_INFO("[BOOT] RHI initialized type=%s", ToString(RHI::Device->GetRHIType()));
        RHI::DumpCapabilities();

        const bool bCanMap = (ExpectedType != ERHIType::Null);

        TEST_SECTION("Resources");
        TEST_EXPECT(ProbeBuffers(bCanMap));
        TEST_EXPECT(ProbeTextures());
        TEST_EXPECT(ProbeCreateAndDestroy());
        TEST_EXPECT(ProbePipelineObjects());

        if (ExpectedType != ERHIType::Null)
        {
            TEST_EXPECT(ProbeShaders());
            TEST_EXPECT(ProbeCommandRecording());
#if PLATFORM_MACOS
            if (ExpectedType == ERHIType::Metal)
            {
                TEST_EXPECT(ProbeCopyQueue());
                TEST_EXPECT(ProbeParallelRender());
                TEST_EXPECT(ProbeBindlessDescriptors());
            }
#endif
            TEST_EXPECT(ProbeCapabilityHonesty(ExpectedType));
        }

        if (FRHICommandListExecutor::IsInitialized())
        {
            FRHICommandListExecutor::Get().WaitForGPU();
        }

#if PLATFORM_MACOS
        if (ExpectedType == ERHIType::Metal)
        {
            TEST_EXPECT(!MetalHasValidationErrors());
        }
#endif

        RHI::Release();
        TEST_EXPECT(RHI::Device == nullptr);
    }

    TEST_END();
}

bool RHIBoot_Null_Test()
{
    return BootRHI(ERHIType::Null);
}

#if PLATFORM_MACOS
bool RHIBoot_Metal_Test()
{
    return BootRHI(ERHIType::Metal);
}

bool RHIBoot_Vulkan_Test()
{
    return BootRHI(ERHIType::Vulkan);
}
#elif PLATFORM_WINDOWS
bool RHIBoot_D3D12_Test()
{
    return BootRHI(ERHIType::D3D12);
}

bool RHIBoot_Vulkan_Test()
{
    return BootRHI(ERHIType::Vulkan);
}
#endif
