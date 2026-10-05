#include "RHIBootTests.h"

#include "TestCommon/TestMacros.h"

#include <Core/Memory/Memory.h>
#include <Core/Misc/ConsoleManager.h>
#include <Core/Misc/Paths.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Platform/PlatformMisc.h>
#include <RHI/RHI.h>
#include <RHI/RHICommandList.h>
#include <RHI/RHIIndirect.h>
#include <RHI/RHIPipelineState.h>
#include <RHI/RHIQuery.h>
#include <RHI/RHIRayTracing.h>
#include <RHI/RHIResources.h>
#include <RHI/RHISamplerState.h>
#include <RHI/RHITexture.h>
#include <RHI/ShaderCompiler.h>

#if PLATFORM_MACOS
#include <MetalRHI/MetalBinaryArchive.h>
#include <MetalRHI/MetalBindlessDescriptors.h>
#include <MetalRHI/MetalBuffer.h>
#include <MetalRHI/MetalCapabilities.h>
#include <MetalRHI/MetalCommandContext.h>
#include <MetalRHI/MetalDevice.h>
#include <MetalRHI/MetalDeviceDebug.h>
#include <MetalRHI/MetalResidencyManager.h>
#include <MetalRHI/MetalStats.h>
#include <MetalRHI/MetalParallelRenderPass.h>
#include <MetalRHI/MetalPipelineState.h>
#include <MetalRHI/MetalQueue.h>
#include <MetalRHI/MetalRayTracing.h>
#include <MetalRHI/MetalRHI.h>
#include <MetalRHI/MetalShader.h>
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

class FBootMipChainData final : public IRHITextureData
{
public:
    static constexpr uint32 MaxMips = 4;

    FBootMipChainData(const void* const* InMipData, const int64* InRowPitches, const int64* InSlicePitches, uint32 InNumMips)
        : NumMips(InNumMips)
    {
        CHECK(NumMips <= MaxMips);
        for (uint32 MipLevel = 0; MipLevel < NumMips; ++MipLevel)
        {
            Data[MipLevel]         = const_cast<void*>(InMipData[MipLevel]);
            RowPitches[MipLevel]   = InRowPitches[MipLevel];
            SlicePitches[MipLevel] = InSlicePitches[MipLevel];
        }
    }

    virtual int64 GetMipRowPitch(uint32 MipLevel = 0)   const override final { return MipLevel < NumMips ? RowPitches[MipLevel] : 0; }
    virtual int64 GetMipSlicePitch(uint32 MipLevel = 0) const override final { return MipLevel < NumMips ? SlicePitches[MipLevel] : 0; }
    virtual void* GetMipData(uint32 MipLevel = 0)       const override final { return MipLevel < NumMips ? Data[MipLevel] : nullptr; }

private:
    void*  Data[MaxMips]         = {};
    int64  RowPitches[MaxMips]   = {};
    int64  SlicePitches[MaxMips] = {};
    uint32 NumMips;
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
                    TEST_EXPECT((MetalPipeline->GetBindings().Stages[EShaderVisibility::Compute].StaticSamplerMask & 1u) != 0);
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

            TEST_SECTION("Each dispatch reads its own update of a transient constant buffer");
            {
                const String ConstantsSource(
                    "cbuffer Params : register(b0) { uint Index; uint Value; uint2 Padding; };\n"
                    "RWBuffer<uint> Dest : register(u0);\n"
                    "[numthreads(1, 1, 1)]\n"
                    "void Main()\n"
                    "{\n"
                    "    Dest[Index] = Value;\n"
                    "}\n");

                TArray<uint8> ConstantsByteCode;
                const FShaderCompileInfo ConstantsCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
                const bool bConstantsCompiled = FShaderCompiler::Get().CompileFromSource(ConstantsSource, ConstantsCompileInfo, ConstantsByteCode);
                TEST_EXPECT(bConstantsCompiled);

                if (bConstantsCompiled)
                {
                    FRHIComputeShaderRef ConstantsShader = RHI::CreateComputeShader(ConstantsByteCode);
                    TEST_EXPECT(ConstantsShader != nullptr);

                    FRHIComputePipelineStateDesc ConstantsPipelineDesc;
                    ConstantsPipelineDesc.Shader = ConstantsShader.Get();
                    FRHIComputePipelineStateRef ConstantsPipeline = RHI::CreateComputePipelineState(ConstantsPipelineDesc);
                    TEST_EXPECT(ConstantsPipeline != nullptr);

                    constexpr uint32 NumDispatches = 3;
                    const uint32     Zeros[NumDispatches] = { 0, 0, 0 };

                    FRHIBufferRef ConstantBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateConstantBuffer(4 * sizeof(uint32), EBufferFlags::Transient), ERHIResourceState::Common, nullptr);
                    FRHIBufferRef DestBuffer     = RHI::CreateBuffer(
                        FRHIBufferDesc(EBufferFlags::Default | EBufferFlags::RWBuffer | EBufferFlags::CopySource, sizeof(uint32), sizeof(Zeros)),
                        ERHIResourceState::Common,
                        Zeros);
                    FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(sizeof(Zeros)));
                    TEST_EXPECT(ConstantBuffer != nullptr);
                    TEST_EXPECT(DestBuffer != nullptr);
                    TEST_EXPECT(ReadbackBuffer != nullptr);

                    if (ConstantsShader && ConstantsPipeline && ConstantBuffer && DestBuffer && ReadbackBuffer)
                    {
                        FRHIUnorderedAccessViewRef DestUAV = RHI::CreateUnorderedAccessView(
                            DestBuffer.Get(), FRHIUnorderedAccessViewDesc::CreateTypedBuffer(0, NumDispatches, EFormat::R32_Uint));
                        TEST_EXPECT(DestUAV != nullptr);

                        if (DestUAV)
                        {
                            const uint32 Params[NumDispatches][4] = { { 0, 11, 0, 0 }, { 1, 22, 0, 0 }, { 2, 33, 0, 0 } };

                            FRHIFenceRef Fence = RHI::CreateFence();
                            FRHICommandList CommandList;
                            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                                DestBuffer.Get(), ERHIResourceState::Common, ERHIResourceState::UnorderedAccess));
                            CommandList.SetComputePipelineState(ConstantsPipeline.Get());
                            CommandList.SetUnorderedAccessView(ConstantsShader.Get(), DestUAV.Get(), 0);

                            for (uint32 Index = 0; Index < NumDispatches; ++Index)
                            {
                                CommandList.UpdateBuffer(ConstantBuffer.Get(), FBufferRegion(0, sizeof(Params[Index])), Params[Index]);
                                if (Index < 2)
                                {
                                    CommandList.SetConstantBuffer(ConstantsShader.Get(), ConstantBuffer.Get(), 0);
                                }

                                CommandList.Dispatch(1, 1, 1);
                                CommandList.UnorderedAccessBarrier(DestBuffer.Get());
                            }

                            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                                DestBuffer.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
                            CommandList.CopyBuffer(ReadbackBuffer.Get(), DestBuffer.Get(), FRHIBufferCopyDesc(0, 0, sizeof(Zeros)));
                            CommandList.WriteFence(Fence.Get());
                            FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
                            FRHICommandListExecutor::Get().WaitForCommands();

                            TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));

                            const uint32* Mapped = static_cast<const uint32*>(ReadbackBuffer->Map());
                            TEST_EXPECT(Mapped != nullptr);
                            if (Mapped)
                            {
                                TEST_EXPECT_EQ(Mapped[0], 11u);
                                TEST_EXPECT_EQ(Mapped[1], 22u);
                                TEST_EXPECT_EQ(Mapped[2], 33u);
                                ReadbackBuffer->Unmap();
                            }
                        }
                    }
                }
            }

            TEST_SECTION("Views of a transient buffer follow it to each update's storage");
            {
                const String ViewsSource(
                    "StructuredBuffer<uint> Structured : register(t0);\n"
                    "Buffer<uint> Typed : register(t1);\n"
                    "RWBuffer<uint> Dest : register(u0);\n"
                    "[numthreads(1, 1, 1)]\n"
                    "void Main()\n"
                    "{\n"
                    "    const uint Index = Structured[0];\n"
                    "    Dest[Index * 2 + 0] = Structured[1];\n"
                    "    Dest[Index * 2 + 1] = Typed[1];\n"
                    "}\n");

                TArray<uint8> ViewsByteCode;
                const FShaderCompileInfo ViewsCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
                const bool bViewsCompiled = FShaderCompiler::Get().CompileFromSource(ViewsSource, ViewsCompileInfo, ViewsByteCode);
                TEST_EXPECT(bViewsCompiled);

                if (bViewsCompiled)
                {
                    FRHIComputeShaderRef ViewsShader = RHI::CreateComputeShader(ViewsByteCode);
                    TEST_EXPECT(ViewsShader != nullptr);

                    FRHIComputePipelineStateDesc ViewsPipelineDesc;
                    ViewsPipelineDesc.Shader = ViewsShader.Get();
                    FRHIComputePipelineStateRef ViewsPipeline = RHI::CreateComputePipelineState(ViewsPipelineDesc);
                    TEST_EXPECT(ViewsPipeline != nullptr);

                    constexpr uint32 NumUpdates = 2;
                    const uint32     Zeros[NumUpdates * 2] = { 0, 0, 0, 0 };

                    FRHIBufferRef SourceBuffer = RHI::CreateBuffer(
                        FRHIBufferDesc(EBufferFlags::Transient | EBufferFlags::ShaderResourceBuffer, sizeof(uint32), 2 * sizeof(uint32)),
                        ERHIResourceState::Common,
                        nullptr);
                    FRHIBufferRef DestBuffer = RHI::CreateBuffer(
                        FRHIBufferDesc(EBufferFlags::Default | EBufferFlags::RWBuffer | EBufferFlags::CopySource, sizeof(uint32), sizeof(Zeros)),
                        ERHIResourceState::Common,
                        Zeros);
                    FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(sizeof(Zeros)));
                    TEST_EXPECT(SourceBuffer != nullptr);
                    TEST_EXPECT(DestBuffer != nullptr);
                    TEST_EXPECT(ReadbackBuffer != nullptr);

                    if (ViewsShader && ViewsPipeline && SourceBuffer && DestBuffer && ReadbackBuffer)
                    {
                        FRHIShaderResourceViewRef  StructuredSRV = RHI::CreateShaderResourceView(SourceBuffer.Get(), FRHIShaderResourceViewDesc::CreateBuffer(0, 2));
                        FRHIShaderResourceViewRef  TypedSRV      = RHI::CreateShaderResourceView(SourceBuffer.Get(), FRHIShaderResourceViewDesc::CreateTypedBuffer(0, 2, EFormat::R32_Uint));
                        FRHIUnorderedAccessViewRef DestUAV       = RHI::CreateUnorderedAccessView(
                            DestBuffer.Get(), FRHIUnorderedAccessViewDesc::CreateTypedBuffer(0, NumUpdates * 2, EFormat::R32_Uint));
                        TEST_EXPECT(StructuredSRV != nullptr);
                        TEST_EXPECT(TypedSRV != nullptr);
                        TEST_EXPECT(DestUAV != nullptr);

                        if (StructuredSRV && TypedSRV && DestUAV)
                        {
                            const uint32 Contents[NumUpdates][2] = { { 0, 11 }, { 1, 22 } };

                            FRHIFenceRef Fence = RHI::CreateFence();
                            FRHICommandList CommandList;
                            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                                DestBuffer.Get(), ERHIResourceState::Common, ERHIResourceState::UnorderedAccess));
                            CommandList.SetComputePipelineState(ViewsPipeline.Get());
                            CommandList.SetUnorderedAccessView(ViewsShader.Get(), DestUAV.Get(), 0);

                            for (uint32 Index = 0; Index < NumUpdates; ++Index)
                            {
                                CommandList.UpdateBuffer(SourceBuffer.Get(), FBufferRegion(0, sizeof(Contents[Index])), Contents[Index]);
                                if (Index == 0)
                                {
                                    CommandList.SetShaderResourceView(ViewsShader.Get(), StructuredSRV.Get(), 0);
                                    CommandList.SetShaderResourceView(ViewsShader.Get(), TypedSRV.Get(), 1);
                                }

                                CommandList.Dispatch(1, 1, 1);
                                CommandList.UnorderedAccessBarrier(DestBuffer.Get());
                            }

                            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(
                                DestBuffer.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
                            CommandList.CopyBuffer(ReadbackBuffer.Get(), DestBuffer.Get(), FRHIBufferCopyDesc(0, 0, sizeof(Zeros)));
                            CommandList.WriteFence(Fence.Get());
                            FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
                            FRHICommandListExecutor::Get().WaitForCommands();

                            TEST_EXPECT(Fence->Wait(5ull * 1000ull * 1000ull * 1000ull));

                            const uint32* Mapped = static_cast<const uint32*>(ReadbackBuffer->Map());
                            TEST_EXPECT(Mapped != nullptr);
                            if (Mapped)
                            {
                                TEST_EXPECT_EQ(Mapped[0], 11u);
                                TEST_EXPECT_EQ(Mapped[1], 11u);
                                TEST_EXPECT_EQ(Mapped[2], 22u);
                                TEST_EXPECT_EQ(Mapped[3], 22u);
                                ReadbackBuffer->Unmap();
                            }
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
                            TEST_EXPECT(MetalPipeline->GetRenderPipeline().GetViewInstancing().bEnableViewInstancing);
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

    TEST_SECTION("Heap-slice CreateBuffer initial data");
    {
        const uint32 SourceValue = 0x11223344u;
        FRHIBufferRef SourceBuffer = RHI::CreateBuffer(
            FRHIBufferDesc(EBufferFlags::Default | EBufferFlags::CopySource, sizeof(uint32), sizeof(uint32)),
            ERHIResourceState::Common,
            &SourceValue);
        TEST_EXPECT(SourceBuffer != nullptr);

        FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(sizeof(uint32)));
        TEST_EXPECT(ReadbackBuffer != nullptr);

        if (SourceBuffer && ReadbackBuffer)
        {
            FRHIFenceRef Fence = RHI::CreateFence();
            FRHICommandList CommandList;
            CommandList.CopyBuffer(ReadbackBuffer.Get(), SourceBuffer.Get(), FRHIBufferCopyDesc(0, 0, sizeof(uint32)));
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

            FMetalParallelRenderPass ParallelPass(*Context, FRHIBeginRenderPassDesc(Attachments, 1));
            FMetalCommandContext* ChildContext = ParallelPass.ObtainChildContext();
            TEST_EXPECT(ChildContext != nullptr);
            if (ChildContext)
            {
                ParallelPass.ReleaseChildContext(ChildContext);
            }
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
                Manager->WriteTexture(TextureHandle, MetalTextureB->GetMTLTexture(), false, true);
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

    TEST_SECTION("A recycled bindless slot reads zeros");
    {
        static const CHAR Source[] =
            "RWStructuredBuffer<uint> OutBuffer : register(u0);\n"
            "cbuffer Params : register(b0) { uint ResourceIndex; uint Pad0; uint Pad1; uint Pad2; };\n"
            "[numthreads(1,1,1)]\n"
            "void Main()\n"
            "{\n"
            "    Texture2D<float4> Tex = ResourceDescriptorHeap[ResourceIndex];\n"
            "    float4 Color = Tex.Load(int3(0, 0, 0));\n"
            "    OutBuffer[0] = (asuint(Color.x) | asuint(Color.y) | asuint(Color.z) | asuint(Color.w)) ^ 0xA5A5A5A5u;\n"
            "}\n";

        TArray<uint8> ByteCode;
        TEST_EXPECT(CompileBindless(Source, ByteCode));
        FRHIComputeShaderRef Shader = RHI::CreateComputeShader(ByteCode);
        FRHIComputePipelineStateDesc PipelineDesc;
        PipelineDesc.Shader = Shader.Get();
        FRHIComputePipelineStateRef Pipeline = Shader ? RHI::CreateComputePipelineState(PipelineDesc) : nullptr;
        TEST_EXPECT(Pipeline != nullptr);

        const uint8 White[4] = { 255, 255, 255, 255 };
        FBootTextureData WhiteData(White, 4, 4);
        FRHIBufferRef Params;
        {
            FRHITextureRef Texture = RHI::CreateTexture(
                FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 1, 1, 1, 1, ETextureUsageFlags::ShaderResourceTexture),
                ERHIResourceState::Common,
                &WhiteData);
            TEST_EXPECT(Texture != nullptr);

            const FRHIDescriptorHandle Handle = Texture ? Texture->GetBindlessSRVHandle() : FRHIDescriptorHandle();
            TEST_EXPECT(Handle.IsValid());

            uint32 ParamsData[4] = { Handle.Index, 0, 0, 0 };
            Params = RHI::CreateBuffer(FRHIBufferDesc::CreateConstantBuffer(sizeof(ParamsData)), ERHIResourceState::Common, ParamsData);
            TEST_EXPECT(Params != nullptr);

            if (Pipeline && Shader && Params && OutputBuffer && OutputUAV)
            {
                TEST_EXPECT(DispatchAndReadUint(Pipeline.Get(), Shader.Get(), Params.Get(), OutputBuffer.Get(), OutputUAV.Get(), 0x3F800000u ^ 0xA5A5A5A5u));
            }
        }

        if (FRHICommandListExecutor::IsInitialized())
        {
            FRHICommandListExecutor::Get().WaitForGPU();
        }

        if (Pipeline && Shader && Params && OutputBuffer && OutputUAV)
        {
            TEST_EXPECT(DispatchAndReadUint(Pipeline.Get(), Shader.Get(), Params.Get(), OutputBuffer.Get(), OutputUAV.Get(), 0xA5A5A5A5u));
        }
    }

    FShaderCompiler::Destroy();
    TEST_END();
}

static bool ProbeDefaultResourcesAndClears()
{
    TEST_BEGIN();

    if (!FShaderCompiler::Initialize(Paths::GetAssetDir()))
    {
        TEST_EXPECT(false);
        TEST_END();
    }

    auto CreateComputePipeline = [](const CHAR* Source, FRHIComputeShaderRef& OutShader) -> FRHIComputePipelineState*
    {
        TArray<uint8> ByteCode;
        const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        if (!FShaderCompiler::Get().CompileFromSource(Source, CompileInfo, ByteCode))
        {
            return nullptr;
        }

        OutShader = RHI::CreateComputeShader(ByteCode);
        if (!OutShader)
        {
            return nullptr;
        }

        FRHIComputePipelineStateDesc PipelineDesc;
        PipelineDesc.Shader = OutShader.Get();
        return RHI::CreateComputePipelineState(PipelineDesc);
    };

    auto SubmitAndWait = [](FRHICommandList& CommandList) -> bool
    {
        FRHIFenceRef Fence = RHI::CreateFence();
        CommandList.WriteFence(Fence.Get());
        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
        FRHICommandListExecutor::Get().WaitForCommands();
        return Fence->Wait(5ull * 1000ull * 1000ull * 1000ull);
    };

    auto DispatchAndRead = [&SubmitAndWait](FRHIComputePipelineState* Pipeline, FRHIComputeShader* Shader, uint32 NumValues, uint32* OutValues, auto&& Prepare, auto&& Bind) -> bool
    {
        const uint64 ByteSize = static_cast<uint64>(NumValues) * sizeof(uint32);
        FRHIBufferRef Output   = RHI::CreateBuffer(FRHIBufferDesc(EBufferFlags::Default | EBufferFlags::RWBuffer | EBufferFlags::CopySource, sizeof(uint32), ByteSize));
        FRHIBufferRef Readback = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(ByteSize));
        FRHIUnorderedAccessViewRef OutputUAV = Output
            ? RHI::CreateUnorderedAccessView(Output.Get(), FRHIUnorderedAccessViewDesc::CreateBuffer(0, NumValues, EBufferViewType::Structured))
            : nullptr;

        if (!Pipeline || !Shader || !Readback || !OutputUAV)
        {
            return false;
        }

        FRHICommandList CommandList;
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Output.Get(), ERHIResourceState::Common, ERHIResourceState::UnorderedAccess));
        Prepare(CommandList);
        CommandList.SetComputePipelineState(Pipeline);
        Bind(CommandList);
        CommandList.SetUnorderedAccessView(Shader, OutputUAV.Get(), 0);
        CommandList.Dispatch(1, 1, 1);
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Output.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
        CommandList.CopyBuffer(Readback.Get(), Output.Get(), FRHIBufferCopyDesc(0, 0, ByteSize));
        if (!SubmitAndWait(CommandList))
        {
            return false;
        }

        const uint32* Mapped = static_cast<const uint32*>(Readback->Map());
        if (!Mapped)
        {
            return false;
        }

        Memory::Memcpy(OutValues, Mapped, ByteSize);
        Readback->Unmap();
        return true;
    };

    auto NoCommands = [](FRHICommandList&) {};

    TEST_SECTION("Unbound SRV, UAV and constant buffer registers read zeros");
    {
        static const CHAR Source[] =
            "RWStructuredBuffer<uint> OutBuffer : register(u0);\n"
            "RWBuffer<uint> NullRWBuffer : register(u1);\n"
            "Texture2D<float4> NullTexture : register(t0);\n"
            "Buffer<uint> NullTypedBuffer : register(t1);\n"
            "StructuredBuffer<uint> NullStructuredBuffer : register(t2);\n"
            "cbuffer NullConstants : register(b0) { uint4 NullValue; };\n"
            "[numthreads(1, 1, 1)]\n"
            "void Main()\n"
            "{\n"
            "    float4 Color = NullTexture.Load(int3(0, 0, 0));\n"
            "    uint Bits = asuint(Color.x) | asuint(Color.y) | asuint(Color.z) | asuint(Color.w);\n"
            "    Bits |= NullTypedBuffer[0] | NullStructuredBuffer[0] | NullRWBuffer[0];\n"
            "    Bits |= NullValue.x | NullValue.y | NullValue.z | NullValue.w;\n"
            "    OutBuffer[0] = Bits ^ 0xA5A5A5A5u;\n"
            "}\n";

        FRHIComputeShaderRef Shader;
        FRHIComputePipelineStateRef Pipeline = CreateComputePipeline(Source, Shader);
        TEST_EXPECT(Pipeline != nullptr);

        uint32 Value = 0;
        TEST_EXPECT(DispatchAndRead(Pipeline.Get(), Shader.Get(), 1, &Value, NoCommands, NoCommands));
        TEST_EXPECT_EQ(Value, 0xA5A5A5A5u);
    }

    TEST_SECTION("An anisotropic sampler filters linearly between mips");
    {
        static const CHAR Source[] =
            "RWStructuredBuffer<uint> OutBuffer : register(u0);\n"
            "Texture2D<float4> MipTexture : register(t0);\n"
            "SamplerState AnisotropicSampler : register(s0);\n"
            "[numthreads(1, 1, 1)]\n"
            "void Main()\n"
            "{\n"
            "    OutBuffer[0] = (uint)(MipTexture.SampleLevel(AnisotropicSampler, float2(0.5, 0.5), 0.5).x * 255.0 + 0.5);\n"
            "}\n";

        FRHIComputeShaderRef Shader;
        FRHIComputePipelineStateRef Pipeline = CreateComputePipeline(Source, Shader);
        TEST_EXPECT(Pipeline != nullptr);

        const uint8  BlackMip[2 * 2 * 4] = { 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255 };
        const uint8  WhiteMip[4]         = { 255, 255, 255, 255 };
        const void*  MipData[2]          = { BlackMip, WhiteMip };
        const int64  RowPitches[2]       = { 2 * 4, 4 };
        const int64  SlicePitches[2]     = { 2 * 2 * 4, 4 };
        FBootMipChainData MipChainData(MipData, RowPitches, SlicePitches, 2);

        FRHITextureRef MipTexture = RHI::CreateTexture(
            FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 2, 2, 2, 1, ETextureUsageFlags::ShaderResourceTexture),
            ERHIResourceState::Common,
            &MipChainData);
        TEST_EXPECT(MipTexture != nullptr && MipTexture->GetShaderResourceView() != nullptr);

        FRHISamplerStateDesc SamplerDesc = FRHISamplerStateDesc::Create(ESamplerMode::Clamp, ESamplerFilter::Anistrotopic);
        SamplerDesc.MaxAnisotropy = 16;
        FRHISamplerStateRef Sampler = RHI::CreateSamplerState(SamplerDesc);
        TEST_EXPECT(Sampler != nullptr);

        if (MipTexture && MipTexture->GetShaderResourceView() && Sampler)
        {
            auto Prepare = [&MipTexture](FRHICommandList& CommandList)
            {
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(MipTexture.Get(), ERHIResourceState::Common, ERHIResourceState::ShaderResource));
            };

            auto Bind = [&](FRHICommandList& CommandList)
            {
                CommandList.SetShaderResourceView(Shader.Get(), MipTexture->GetShaderResourceView(), 0);
                CommandList.SetSamplerState(Shader.Get(), Sampler.Get(), 0);
            };

            // Halfway between a black and a white mip, a nearest mip filter would give 0 or 255
            uint32 Value = 0;
            TEST_EXPECT(DispatchAndRead(Pipeline.Get(), Shader.Get(), 1, &Value, Prepare, Bind));
            TEST_EXPECT(Value >= 112u && Value <= 143u);
        }
    }

    TEST_SECTION("A UAV clear of a 2D array texture covers every slice");
    {
        static const CHAR Source[] =
            "RWStructuredBuffer<uint> OutBuffer : register(u0);\n"
            "Texture2DArray<uint> ClearedTexture : register(t0);\n"
            "[numthreads(1, 1, 1)]\n"
            "void Main()\n"
            "{\n"
            "    for (uint Slice = 0; Slice < 3; ++Slice)\n"
            "    {\n"
            "        OutBuffer[Slice] = ClearedTexture.Load(int4(3, 3, Slice, 0));\n"
            "    }\n"
            "}\n";

        FRHIComputeShaderRef Shader;
        FRHIComputePipelineStateRef Pipeline = CreateComputePipeline(Source, Shader);
        TEST_EXPECT(Pipeline != nullptr);

        FRHITextureRef ArrayTexture = RHI::CreateTexture(FRHITextureDesc::CreateTexture2DArray(
            EFormat::R32_Uint, 4, 4, 3, 1, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture));
        TEST_EXPECT(ArrayTexture != nullptr);
        TEST_EXPECT(ArrayTexture && ArrayTexture->GetUnorderedAccessView() != nullptr && ArrayTexture->GetShaderResourceView() != nullptr);

        if (ArrayTexture && ArrayTexture->GetUnorderedAccessView() && ArrayTexture->GetShaderResourceView())
        {
            auto Prepare = [&ArrayTexture](FRHICommandList& CommandList)
            {
                const uint32 ClearValues[4] = { 0x5EEDu, 0u, 0u, 0u };
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(ArrayTexture.Get(), ERHIResourceState::Common, ERHIResourceState::UnorderedAccess));
                CommandList.ClearUnorderedAccessViewUint(ArrayTexture->GetUnorderedAccessView(), ClearValues);
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(ArrayTexture.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::ShaderResource));
            };

            auto Bind = [&](FRHICommandList& CommandList)
            {
                CommandList.SetShaderResourceView(Shader.Get(), ArrayTexture->GetShaderResourceView(), 0);
            };

            uint32 Values[3] = {};
            TEST_EXPECT(DispatchAndRead(Pipeline.Get(), Shader.Get(), 3, Values, Prepare, Bind));
            TEST_EXPECT_EQ(Values[0], 0x5EEDu);
            TEST_EXPECT_EQ(Values[1], 0x5EEDu);
            TEST_EXPECT_EQ(Values[2], 0x5EEDu);
        }
    }

    auto ReadRenderTarget = [&SubmitAndWait](FRHITexture* RenderTarget, auto&& Record, uint8* OutPixel) -> bool
    {
        FRHIBufferRef Readback = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(256));
        if (!Readback)
        {
            return false;
        }

        FRHICommandList CommandList;
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(RenderTarget, ERHIResourceState::Common, ERHIResourceState::RenderTarget));
        Record(CommandList);
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(RenderTarget, ERHIResourceState::RenderTarget, ERHIResourceState::CopySource));
        CommandList.CopyTextureRegionToBuffer(Readback.Get(), 0, RenderTarget, FTextureRegion2D(1, 1), 0);
        if (!SubmitAndWait(CommandList))
        {
            return false;
        }

        const uint8* Mapped = static_cast<const uint8*>(Readback->Map());
        if (!Mapped)
        {
            return false;
        }

        Memory::Memcpy(OutPixel, Mapped, 4);
        Readback->Unmap();
        return true;
    };

    const FRHITextureDesc RenderTargetDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1, ETextureUsageFlags::RenderTarget | ETextureUsageFlags::CopySource);

    TEST_SECTION("A render target clear with no pass after it still reaches the texture");
    {
        FRHITextureRef RenderTarget = RHI::CreateTexture(RenderTargetDesc);
        TEST_EXPECT(RenderTarget != nullptr && RenderTarget->GetRenderTargetView() != nullptr);

        if (RenderTarget && RenderTarget->GetRenderTargetView())
        {
            auto Record = [&RenderTarget](FRHICommandList& CommandList)
            {
                CommandList.ClearRenderTargetView(RenderTarget->GetRenderTargetView(), Vector4(0.0f, 0.0f, 1.0f, 1.0f));
                CommandList.ClearRenderTargetView(RenderTarget->GetRenderTargetView(), Vector4(1.0f, 0.0f, 0.0f, 1.0f));
            };

            uint8 Pixel[4] = {};
            TEST_EXPECT(ReadRenderTarget(RenderTarget.Get(), Record, Pixel));
            TEST_EXPECT_EQ(Pixel[0], static_cast<uint8>(255));
            TEST_EXPECT_EQ(Pixel[1], static_cast<uint8>(0));
            TEST_EXPECT_EQ(Pixel[2], static_cast<uint8>(0));
        }
    }

    TEST_SECTION("A render target clear folds into the load action of the next pass");
    {
        FRHITextureRef RenderTarget = RHI::CreateTexture(RenderTargetDesc);
        TEST_EXPECT(RenderTarget != nullptr && RenderTarget->GetRenderTargetView() != nullptr);

        if (RenderTarget && RenderTarget->GetRenderTargetView())
        {
            auto Record = [&RenderTarget](FRHICommandList& CommandList)
            {
                CommandList.ClearRenderTargetView(RenderTarget->GetRenderTargetView(), Vector4(0.0f, 1.0f, 0.0f, 1.0f));

                FRHIBeginRenderPassDesc::FRenderTargetAttachments Attachments;
                Attachments[0] = FRHIRenderTargetAttachment(RenderTarget->GetRenderTargetView(), EAttachmentLoadAction::Load, EAttachmentStoreAction::Store);
                CommandList.BeginRenderPass(FRHIBeginRenderPassDesc(Attachments, 1));
                CommandList.EndRenderPass();
            };

            uint8 Pixel[4] = {};
            TEST_EXPECT(ReadRenderTarget(RenderTarget.Get(), Record, Pixel));
            TEST_EXPECT_EQ(Pixel[0], static_cast<uint8>(0));
            TEST_EXPECT_EQ(Pixel[1], static_cast<uint8>(255));
            TEST_EXPECT_EQ(Pixel[2], static_cast<uint8>(0));
        }
    }

    FShaderCompiler::Destroy();
    TEST_END();
}
#endif

static constexpr uint32 GProbeNumVertices = 6;

static const float GProbeVertices[GProbeNumVertices * 3] =
{
    -1.0f, -1.0f, 0.0f,
     1.0f, -1.0f, 0.0f,
     0.0f,  1.0f, 0.0f,
     2.0f, -1.0f, 0.0f,
     4.0f, -1.0f, 0.0f,
     3.0f,  1.0f, 0.0f,
};

static const uint32 GProbeIndices[GProbeNumVertices] = { 0, 1, 2, 3, 4, 5 };

static constexpr float GProbeRayY         = -0.25f;
static constexpr float GProbeRayZ         = -5.0f;
static constexpr float GProbeBarycentricU = 0.3125f;
static constexpr float GProbeBarycentricV = 0.375f;

static const CHAR GRayQueryProbeSource[] =
    "RWStructuredBuffer<uint4> OutBuffer : register(u0);\n"
    "cbuffer Params : register(b0) { float4 RayOrigin; float4 RayDirection; uint4 SceneIndex; };\n"
    "#if !PROBE_BINDLESS_SCENE\n"
    "RaytracingAccelerationStructure Scene : register(t0);\n"
    "#endif\n"
    "[numthreads(1,1,1)]\n"
    "void Main()\n"
    "{\n"
    "#if PROBE_BINDLESS_SCENE\n"
    "    RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[SceneIndex.x];\n"
    "#endif\n"
    "    RayDesc Ray;\n"
    "    Ray.Origin    = RayOrigin.xyz;\n"
    "    Ray.TMin      = 0.0f;\n"
    "    Ray.Direction = RayDirection.xyz;\n"
    "    Ray.TMax      = 1000.0f;\n"
    "    RayQuery<RAY_FLAG_NONE> Query;\n"
    "    Query.TraceRayInline(Scene, RAY_FLAG_NONE, 0xff, Ray);\n"
    "    Query.Proceed();\n"
    "    uint4 Hit          = uint4(0, 0, 0, 0);\n"
    "    uint4 Barycentrics = uint4(0, 0, 0, 0);\n"
    "    if (Query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)\n"
    "    {\n"
    "        Hit          = uint4(1, Query.CommittedInstanceID(), Query.CommittedPrimitiveIndex(), asuint(Query.CommittedRayT()));\n"
    "        Barycentrics = uint4(asuint(Query.CommittedTriangleBarycentrics()), 0, 0);\n"
    "    }\n"
    "    OutBuffer[0] = Hit;\n"
    "    OutBuffer[1] = Barycentrics;\n"
    "}\n";

struct FRayQueryProbeHit
{
    uint32 bHit;
    uint32 InstanceID;
    uint32 PrimitiveIndex;
    float  HitT;
    float  Barycentrics[2];
    uint32 Padding[2];
};

struct FRayQueryProbe
{
    FRHIComputeShaderRef        Shader;
    FRHIComputePipelineStateRef Pipeline;
    FRHIComputeShaderRef        BindlessShader;
    FRHIComputePipelineStateRef BindlessPipeline;
};

struct FRayTracingProbeGeometry
{
    FRHIBufferRef                        VertexBuffer;
    FRHIBufferRef                        IndexBuffer;
    FRHIGeometryAccelerationStructureRef Geometry;
};

static FRHIComputePipelineStateRef CreateRayQueryPipeline(bool bBindlessScene, FRHIComputeShaderRef& OutShader)
{
    FShaderDefine Defines[] = { FShaderDefine("PROBE_BINDLESS_SCENE", bBindlessScene ? "1" : "0") };

    TArray<uint8> ByteCode;
    const FShaderCompileInfo CompileInfo("Main", bBindlessScene ? EShaderModel::SM_6_6 : EShaderModel::SM_6_5, EShaderStage::Compute, TArrayView<FShaderDefine>(Defines));
    if (!FShaderCompiler::Get().CompileFromSource(GRayQueryProbeSource, CompileInfo, ByteCode))
    {
        return nullptr;
    }

    OutShader = RHI::CreateComputeShader(ByteCode);
    if (!OutShader)
    {
        return nullptr;
    }

    FRHIComputePipelineStateDesc PipelineDesc;
    PipelineDesc.Shader = OutShader.Get();
    return RHI::CreateComputePipelineState(PipelineDesc);
}

static bool CreateRayQueryProbe(FRayQueryProbe& OutProbe)
{
    OutProbe.Pipeline = CreateRayQueryPipeline(false, OutProbe.Shader);
    if (RHI::bSupportsBindless)
    {
        OutProbe.BindlessPipeline = CreateRayQueryPipeline(true, OutProbe.BindlessShader);
    }

    return OutProbe.Pipeline && (!RHI::bSupportsBindless || OutProbe.BindlessPipeline);
}

static bool CreateProbeGeometry(bool bIndexed, EAccelerationStructureBuildFlags Flags, FRayTracingProbeGeometry& OutGeometry)
{
    const EBufferFlags BufferFlags = EBufferFlags::Default | EBufferFlags::ShaderResourceBuffer | EBufferFlags::CopyDest;

    OutGeometry.VertexBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateVertexBuffer(sizeof(float) * 3, GProbeNumVertices, BufferFlags), ERHIResourceState::Common, GProbeVertices);
    if (!OutGeometry.VertexBuffer)
    {
        return false;
    }

    if (bIndexed)
    {
        OutGeometry.IndexBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateIndexBuffer(EIndexFormat::uint32, GProbeNumVertices, BufferFlags), ERHIResourceState::Common, GProbeIndices);
        if (!OutGeometry.IndexBuffer)
        {
            return false;
        }
    }

    const FRHIGeometryAccelerationStructureDesc GeometryDesc(
        OutGeometry.VertexBuffer.Get(),
        GProbeNumVertices,
        OutGeometry.IndexBuffer.Get(),
        bIndexed ? GProbeNumVertices : 0,
        bIndexed ? EIndexFormat::uint32 : EIndexFormat::Unknown,
        Flags);

    OutGeometry.Geometry = RHI::CreateGeometryAccelerationStructure(GeometryDesc);
    return OutGeometry.Geometry != nullptr;
}

static FRHIGeometryAccelerationStructureBuildDesc GetProbeGeometryBuildDesc(const FRayTracingProbeGeometry& Geometry, bool bUpdate)
{
    const bool bIndexed = Geometry.IndexBuffer != nullptr;
    return FRHIGeometryAccelerationStructureBuildDesc(
        Geometry.VertexBuffer.Get(),
        GProbeNumVertices,
        Geometry.IndexBuffer.Get(),
        bIndexed ? GProbeNumVertices : 0,
        bIndexed ? EIndexFormat::uint32 : EIndexFormat::Unknown,
        bUpdate);
}

static FRHIGeometryAccelerationStructureInstance MakeProbeInstance(FRHIGeometryAccelerationStructure* Geometry, uint32 InstanceIndex, float OffsetX)
{
    const Matrix3x4 Transform(
        1.0f, 0.0f, 0.0f, OffsetX,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f);

    return FRHIGeometryAccelerationStructureInstance(Geometry, InstanceIndex, 0, ERayTracingInstanceFlags::None, 0xFF, Transform);
}

static bool IsNearlyEqual(float Lhs, float Rhs)
{
    return Math::Abs(Lhs - Rhs) < 1.0e-3f;
}

static bool TraceProbeRay(FRHICommandList& CommandList, const FRayQueryProbe& Probe, FRHISceneAccelerationStructure* Scene, bool bBindlessScene, float OriginX, FRayQueryProbeHit& OutHit)
{
    FRHIComputePipelineState* Pipeline = bBindlessScene ? Probe.BindlessPipeline.Get() : Probe.Pipeline.Get();
    FRHIComputeShader*        Shader   = bBindlessScene ? Probe.BindlessShader.Get() : Probe.Shader.Get();
    if (!Pipeline || !Shader || !Scene)
    {
        return false;
    }

    const FRHIDescriptorHandle SceneHandle = bBindlessScene ? Scene->GetBindlessHandle() : FRHIDescriptorHandle();
    if (bBindlessScene && !SceneHandle.IsValid())
    {
        return false;
    }

    struct FParams
    {
        float  RayOrigin[4];
        float  RayDirection[4];
        uint32 SceneIndex[4];
    };

    const FParams ParamsData =
    {
        { OriginX, GProbeRayY, GProbeRayZ, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { bBindlessScene ? SceneHandle.Index : 0u, 0u, 0u, 0u },
    };

    FRHIBufferRef Params   = RHI::CreateBuffer(FRHIBufferDesc::CreateConstantBuffer(sizeof(ParamsData)), ERHIResourceState::Common, &ParamsData);
    FRHIBufferRef Output   = RHI::CreateBuffer(FRHIBufferDesc(EBufferFlags::Default | EBufferFlags::RWBuffer | EBufferFlags::CopySource, sizeof(uint32) * 4, sizeof(FRayQueryProbeHit)));
    FRHIBufferRef Readback = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(sizeof(FRayQueryProbeHit)));
    FRHIUnorderedAccessViewRef OutputUAV = Output
        ? RHI::CreateUnorderedAccessView(Output.Get(), FRHIUnorderedAccessViewDesc::CreateBuffer(0, 2, EBufferViewType::Structured))
        : nullptr;

    if (!Params || !Output || !Readback || !OutputUAV)
    {
        return false;
    }

    FRHIFenceRef Fence = RHI::CreateFence();
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Output.Get(), ERHIResourceState::Common, ERHIResourceState::UnorderedAccess));
    CommandList.SetComputePipelineState(Pipeline);
    CommandList.SetConstantBuffer(Shader, Params.Get(), 0);

    if (!bBindlessScene)
    {
        CommandList.SetShaderResourceView(Shader, Scene->GetShaderResourceView(), 0);
    }

    CommandList.SetUnorderedAccessView(Shader, OutputUAV.Get(), 0);
    CommandList.Dispatch(1, 1, 1);
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Output.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
    CommandList.CopyBuffer(Readback.Get(), Output.Get(), FRHIBufferCopyDesc(0, 0, sizeof(FRayQueryProbeHit)));
    CommandList.WriteFence(Fence.Get());
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    FRHICommandListExecutor::Get().WaitForCommands();
    if (!Fence->Wait(5ull * 1000ull * 1000ull * 1000ull))
    {
        return false;
    }

    const FRayQueryProbeHit* Mapped = static_cast<const FRayQueryProbeHit*>(Readback->Map());
    if (!Mapped)
    {
        return false;
    }

    OutHit = *Mapped;
    Readback->Unmap();
    return true;
}

static bool TraceProbeRay(const FRayQueryProbe& Probe, FRHISceneAccelerationStructure* Scene, bool bBindlessScene, float OriginX, FRayQueryProbeHit& OutHit)
{
    FRHICommandList CommandList;
    return TraceProbeRay(CommandList, Probe, Scene, bBindlessScene, OriginX, OutHit);
}

static bool IsExpectedProbeHit(const FRayQueryProbeHit& Hit, uint32 InstanceID, uint32 PrimitiveIndex, float HitT)
{
    const bool bExpected = Hit.bHit == 1 &&
        Hit.InstanceID == InstanceID &&
        Hit.PrimitiveIndex == PrimitiveIndex &&
        IsNearlyEqual(Hit.HitT, HitT) &&
        IsNearlyEqual(Hit.Barycentrics[0], GProbeBarycentricU) &&
        IsNearlyEqual(Hit.Barycentrics[1], GProbeBarycentricV);

    if (!bExpected)
    {
        LOG_ERROR("[BOOT] Expected instance %u primitive %u at t=%.3f, the ray returned hit=%u instance %u primitive %u at t=%.3f barycentrics (%.4f, %.4f)",
            InstanceID, PrimitiveIndex, HitT, Hit.bHit, Hit.InstanceID, Hit.PrimitiveIndex, Hit.HitT, Hit.Barycentrics[0], Hit.Barycentrics[1]);
    }

    return bExpected;
}

static bool ReadCompactedSize(FRHIRayTracingAccelerationStructure* AccelerationStructure, uint64& OutCompactedSize)
{
    FRHIBufferRef Readback = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(sizeof(uint64), sizeof(uint64)), ERHIResourceState::CopyDest, nullptr);
    if (!Readback)
    {
        return false;
    }

    FRHIRayTracingAccelerationStructure* Sources[] = { AccelerationStructure };

    FRHIFenceRef Fence = RHI::CreateFence();
    FRHICommandList CommandList;
    CommandList.WriteAccelerationStructurePostBuildInfo(Readback.Get(), 0, EAccelerationStructurePostBuildInfoType::CompactedSize, Sources, 1);
    CommandList.WriteFence(Fence.Get());
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    FRHICommandListExecutor::Get().WaitForCommands();
    if (!Fence->Wait(5ull * 1000ull * 1000ull * 1000ull))
    {
        return false;
    }

    const uint64* Mapped = static_cast<const uint64*>(Readback->Map());
    if (!Mapped)
    {
        return false;
    }

    OutCompactedSize = *Mapped;
    Readback->Unmap();
    return true;
}

static bool ProbeAccelerationStructures(ERHIType ExpectedType)
{
    TEST_BEGIN();

#if !PLATFORM_MACOS
    UNREFERENCED_VARIABLE(ExpectedType);
#endif

    if (!RHI::bSupportsRayTracing)
    {
        LOG_INFO("[BOOT] Acceleration structure probes skipped, the backend reports no ray tracing");
        TEST_END();
    }

    // Tracing needs RayQuery, a backend with only the pipeline still builds and compacts
    const bool bCanTrace = RHI::bSupportsInlineRayTracing;

    FRayQueryProbe Probe;
    if (bCanTrace)
    {
        const bool bCompilerInitialized = FShaderCompiler::Initialize(Paths::GetAssetDir());
        TEST_EXPECT(bCompilerInitialized);
        TEST_EXPECT(bCompilerInitialized && CreateRayQueryProbe(Probe));
    }

    const bool bTrace         = bCanTrace && Probe.Pipeline;
    const bool bTraceBindless = bTrace && Probe.BindlessPipeline;

    const EAccelerationStructureBuildFlags GeometryFlags = EAccelerationStructureBuildFlags::AllowUpdate | EAccelerationStructureBuildFlags::AllowCompaction;

    TEST_SECTION("Indexed and non-indexed geometry and a scene over both build at creation and again on a command list");
    {
        FRayTracingProbeGeometry Indexed;
        FRayTracingProbeGeometry NonIndexed;
        TEST_EXPECT(CreateProbeGeometry(true, GeometryFlags, Indexed));
        TEST_EXPECT(CreateProbeGeometry(false, GeometryFlags, NonIndexed));

        if (Indexed.Geometry && NonIndexed.Geometry)
        {
            const FRHIGeometryAccelerationStructureInstance Instances[] =
            {
                MakeProbeInstance(Indexed.Geometry.Get(), 7, 0.0f),
                MakeProbeInstance(NonIndexed.Geometry.Get(), 9, 100.0f),
            };

            FRHISceneAccelerationStructureRef Scene = RHI::CreateSceneAccelerationStructure(FRHISceneAccelerationStructureDesc(Instances, EAccelerationStructureBuildFlags::None));
            TEST_EXPECT(Scene != nullptr);
            TEST_EXPECT(!Scene || Scene->GetShaderResourceView() != nullptr);
            TEST_EXPECT(!Scene || !RHI::bSupportsBindless || Scene->GetBindlessHandle().IsValid());

#if PLATFORM_MACOS
            if (ExpectedType == ERHIType::Metal && Scene)
            {
                FRHIRayTracingAccelerationStructure* const AccelerationStructures[] = { Indexed.Geometry.Get(), NonIndexed.Geometry.Get(), Scene.Get() };
                for (FRHIRayTracingAccelerationStructure* AccelerationStructure : AccelerationStructures)
                {
                    FMetalAccelerationStructure* MetalAccelerationStructure = GetMetalAccelerationStructure(AccelerationStructure);
                    TEST_EXPECT(MetalAccelerationStructure->GetMTLAccelerationStructure() != nil);
                    TEST_EXPECT((__bridge void*)MetalAccelerationStructure->GetMTLAccelerationStructure() == AccelerationStructure->GetRHINativeResource());
                    TEST_EXPECT(MetalAccelerationStructure->GetResidencyEntry() != nullptr);
                }
            }
#endif

            if (Scene)
            {
                const FRHISceneAccelerationStructureBuildDesc SceneBuildDesc(Instances, ARRAY_COUNT(Instances), false);

                FRHICommandList CommandList;
                CommandList.BuildGeometryAccelerationStructure(Indexed.Geometry.Get(), GetProbeGeometryBuildDesc(Indexed, false));
                CommandList.BuildGeometryAccelerationStructure(NonIndexed.Geometry.Get(), GetProbeGeometryBuildDesc(NonIndexed, false));
                CommandList.UnorderedAccessBarrier(Indexed.Geometry.Get());
                CommandList.UnorderedAccessBarrier(NonIndexed.Geometry.Get());
                CommandList.BuildSceneAccelerationStructure(Scene.Get(), SceneBuildDesc);
                CommandList.UnorderedAccessBarrier(Scene.Get());

                if (bTrace)
                {
                    FRayQueryProbeHit Hit = {};
                    TEST_EXPECT(TraceProbeRay(CommandList, Probe, Scene.Get(), false, 103.0f, Hit));
                    TEST_EXPECT(IsExpectedProbeHit(Hit, 9, 1, 5.0f));
                }
                else
                {
                    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
                }

                FRHICommandListExecutor::Get().WaitForGPU();
            }
        }
    }

    TEST_SECTION("A scene traced straight after creation, with no wait between, hits");
    if (bTrace)
    {
        FRayTracingProbeGeometry Geometry;
        TEST_EXPECT(CreateProbeGeometry(true, EAccelerationStructureBuildFlags::None, Geometry));

        if (Geometry.Geometry)
        {
            const FRHIGeometryAccelerationStructureInstance Instances[] = { MakeProbeInstance(Geometry.Geometry.Get(), 3, 0.0f) };

            FRHISceneAccelerationStructureRef Scene = RHI::CreateSceneAccelerationStructure(FRHISceneAccelerationStructureDesc(Instances, EAccelerationStructureBuildFlags::None));
            TEST_EXPECT(Scene != nullptr);

            FRayQueryProbeHit Hit = {};
            TEST_EXPECT(Scene && TraceProbeRay(Probe, Scene.Get(), false, 0.0f, Hit));
            TEST_EXPECT(IsExpectedProbeHit(Hit, 3, 0, 5.0f));
        }
    }

    TEST_SECTION("The compacted size reads back nonzero and no larger than the original");
    {
        FRayTracingProbeGeometry Geometry;
        TEST_EXPECT(CreateProbeGeometry(true, EAccelerationStructureBuildFlags::AllowCompaction, Geometry));

        uint64 CompactedSize = 0;
        TEST_EXPECT(Geometry.Geometry && ReadCompactedSize(Geometry.Geometry.Get(), CompactedSize));
        TEST_EXPECT(CompactedSize > 0);

#if PLATFORM_MACOS
        id<MTLAccelerationStructure> Original = nil;
        if (ExpectedType == ERHIType::Metal && Geometry.Geometry)
        {
            Original = GetMetalAccelerationStructure(Geometry.Geometry.Get())->GetMTLAccelerationStructure();
            TEST_EXPECT(Original != nil);
            TEST_EXPECT(Original && CompactedSize <= Original.size);
        }
#endif

        TEST_SECTION("Compacting in place, then rebuilding the scene over the compacted geometry, still hits");
        if (Geometry.Geometry && CompactedSize > 0)
        {
            const FRHIGeometryAccelerationStructureInstance Instances[] = { MakeProbeInstance(Geometry.Geometry.Get(), 5, 0.0f) };

            FRHISceneAccelerationStructureRef Scene = RHI::CreateSceneAccelerationStructure(FRHISceneAccelerationStructureDesc(Instances, EAccelerationStructureBuildFlags::None));
            TEST_EXPECT(Scene != nullptr);

            if (Scene)
            {
                const FRHISceneAccelerationStructureBuildDesc SceneBuildDesc(Instances, ARRAY_COUNT(Instances), false);

                FRHICommandList CommandList;
                CommandList.CompactAccelerationStructure(Geometry.Geometry.Get(), CompactedSize);
                CommandList.UnorderedAccessBarrier(Geometry.Geometry.Get());
                CommandList.BuildSceneAccelerationStructure(Scene.Get(), SceneBuildDesc);
                CommandList.UnorderedAccessBarrier(Scene.Get());

                if (bTrace)
                {
                    FRayQueryProbeHit Hit = {};
                    TEST_EXPECT(TraceProbeRay(CommandList, Probe, Scene.Get(), false, 0.0f, Hit));
                    TEST_EXPECT(IsExpectedProbeHit(Hit, 5, 0, 5.0f));

                    if (bTraceBindless)
                    {
                        Hit = {};
                        TEST_EXPECT(TraceProbeRay(Probe, Scene.Get(), true, 0.0f, Hit));
                        TEST_EXPECT(IsExpectedProbeHit(Hit, 5, 0, 5.0f));
                    }
                }
                else
                {
                    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
                }

                FRHICommandListExecutor::Get().WaitForGPU();

#if PLATFORM_MACOS
                if (ExpectedType == ERHIType::Metal)
                {
                    id<MTLAccelerationStructure> Compacted = GetMetalAccelerationStructure(Geometry.Geometry.Get())->GetMTLAccelerationStructure();
                    TEST_EXPECT(Compacted != nil);
                    TEST_EXPECT(Compacted != Original);
                    TEST_EXPECT(GetMetalAccelerationStructure(Geometry.Geometry.Get())->GetResidencyEntry() != nullptr);
                }
#endif
            }
        }
    }

    TEST_SECTION("Moving the vertices and refitting moves the hit");
    if (bTrace)
    {
        FRayTracingProbeGeometry Geometry;
        TEST_EXPECT(CreateProbeGeometry(true, EAccelerationStructureBuildFlags::AllowUpdate, Geometry));

        if (Geometry.Geometry)
        {
            FRHIGeometryAccelerationStructureInstance Instances[] = { MakeProbeInstance(Geometry.Geometry.Get(), 11, 0.0f) };

            FRHISceneAccelerationStructureRef Scene = RHI::CreateSceneAccelerationStructure(FRHISceneAccelerationStructureDesc(Instances, EAccelerationStructureBuildFlags::AllowUpdate));
            TEST_EXPECT(Scene != nullptr);

            FRayQueryProbeHit Hit = {};
            TEST_EXPECT(Scene && TraceProbeRay(Probe, Scene.Get(), false, 3.0f, Hit));
            TEST_EXPECT(IsExpectedProbeHit(Hit, 11, 1, 5.0f));

            float MovedVertices[GProbeNumVertices * 3];
            for (uint32 Index = 0; Index < ARRAY_COUNT(MovedVertices); ++Index)
            {
                MovedVertices[Index] = (Index % 3 == 2) ? GProbeVertices[Index] + 2.0f : GProbeVertices[Index];
            }

            if (Scene)
            {
                const FRHISceneAccelerationStructureBuildDesc SceneBuildDesc(Instances, ARRAY_COUNT(Instances), true);

                FRHICommandList CommandList;
                CommandList.UpdateBuffer(Geometry.VertexBuffer.Get(), FBufferRegion(0, sizeof(MovedVertices)), MovedVertices);
                CommandList.BuildGeometryAccelerationStructure(Geometry.Geometry.Get(), GetProbeGeometryBuildDesc(Geometry, true));
                CommandList.UnorderedAccessBarrier(Geometry.Geometry.Get());
                CommandList.BuildSceneAccelerationStructure(Scene.Get(), SceneBuildDesc);
                CommandList.UnorderedAccessBarrier(Scene.Get());

                Hit = {};
                TEST_EXPECT(TraceProbeRay(CommandList, Probe, Scene.Get(), false, 3.0f, Hit));
                TEST_EXPECT(IsExpectedProbeHit(Hit, 11, 1, 7.0f));

                Instances[0] = MakeProbeInstance(Geometry.Geometry.Get(), 11, 50.0f);
                const FRHISceneAccelerationStructureBuildDesc MovedSceneBuildDesc(Instances, ARRAY_COUNT(Instances), true);

                FRHICommandList MoveCommandList;
                MoveCommandList.BuildSceneAccelerationStructure(Scene.Get(), MovedSceneBuildDesc);
                MoveCommandList.UnorderedAccessBarrier(Scene.Get());

                Hit = {};
                TEST_EXPECT(TraceProbeRay(MoveCommandList, Probe, Scene.Get(), false, 53.0f, Hit));
                TEST_EXPECT(IsExpectedProbeHit(Hit, 11, 1, 7.0f));
            }
        }
    }

    TEST_SECTION("RayQuery reports a miss, and the instance, primitive and barycentrics of a hit, through the discrete and the bindless scene");
    if (bTrace)
    {
        FRayTracingProbeGeometry Geometry;
        TEST_EXPECT(CreateProbeGeometry(true, EAccelerationStructureBuildFlags::None, Geometry));

        if (Geometry.Geometry)
        {
            const FRHIGeometryAccelerationStructureInstance Instances[] =
            {
                MakeProbeInstance(Geometry.Geometry.Get(), 21, 0.0f),
                MakeProbeInstance(Geometry.Geometry.Get(), 22, 100.0f),
            };

            FRHISceneAccelerationStructureRef Scene = RHI::CreateSceneAccelerationStructure(FRHISceneAccelerationStructureDesc(Instances, EAccelerationStructureBuildFlags::None));
            TEST_EXPECT(Scene != nullptr);

            for (bool bBindlessScene : { false, true })
            {
                if (!Scene || (bBindlessScene && !bTraceBindless))
                {
                    continue;
                }

                FRayQueryProbeHit Hit = {};
                TEST_EXPECT(TraceProbeRay(Probe, Scene.Get(), bBindlessScene, 0.0f, Hit));
                TEST_EXPECT(IsExpectedProbeHit(Hit, 21, 0, 5.0f));

                Hit = {};
                TEST_EXPECT(TraceProbeRay(Probe, Scene.Get(), bBindlessScene, 103.0f, Hit));
                TEST_EXPECT(IsExpectedProbeHit(Hit, 22, 1, 5.0f));

                Hit = {};
                Hit.bHit = 1;
                TEST_EXPECT(TraceProbeRay(Probe, Scene.Get(), bBindlessScene, 50.0f, Hit));
                TEST_EXPECT_EQ(Hit.bHit, 0u);
            }
        }
    }

    TEST_SECTION("Releasing geometry and scenes while their builds are in flight");
    {
        for (uint32 Iteration = 0; Iteration < 8; ++Iteration)
        {
            FRayTracingProbeGeometry Geometry;
            TEST_EXPECT(CreateProbeGeometry(true, EAccelerationStructureBuildFlags::AllowUpdate, Geometry));
            if (!Geometry.Geometry)
            {
                continue;
            }

            const FRHIGeometryAccelerationStructureInstance Instances[] = { MakeProbeInstance(Geometry.Geometry.Get(), Iteration, 0.0f) };

            FRHISceneAccelerationStructureRef Scene = RHI::CreateSceneAccelerationStructure(FRHISceneAccelerationStructureDesc(Instances, EAccelerationStructureBuildFlags::None));
            TEST_EXPECT(Scene != nullptr);

            if (Scene)
            {
                const FRHISceneAccelerationStructureBuildDesc SceneBuildDesc(Instances, ARRAY_COUNT(Instances), false);

                FRHICommandList CommandList;
                CommandList.BuildGeometryAccelerationStructure(Geometry.Geometry.Get(), GetProbeGeometryBuildDesc(Geometry, Iteration % 2 == 1));
                CommandList.UnorderedAccessBarrier(Geometry.Geometry.Get());
                CommandList.BuildSceneAccelerationStructure(Scene.Get(), SceneBuildDesc);
                CommandList.UnorderedAccessBarrier(Scene.Get());
                FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
            }
        }

        FRHICommandListExecutor::Get().WaitForGPU();
    }

    if (bCanTrace)
    {
        FShaderCompiler::Destroy();
    }

    TEST_END();
}

static bool ProbeCapabilityHonesty(ERHIType ExpectedType)
{
    TEST_BEGIN();

    TEST_SECTION("Acceleration structures are reported exactly when pipeline or inline ray tracing is");
    TEST_EXPECT(RHI::bSupportsRayTracing == (RHI::bSupportsRayTracingPipeline || RHI::bSupportsInlineRayTracing));

    if (ExpectedType == ERHIType::Metal)
    {
#if PLATFORM_MACOS
        TEST_SECTION("Metal reports inline ray tracing wherever the device supports ray tracing, and no pipeline until TraceRay is lowered");
        TEST_EXPECT(RHI::bSupportsInlineRayTracing == GMetalFeatures.bRayTracing);
        TEST_EXPECT(RHI::bSupportsRayTracingPipeline == false);
#endif

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
        TEST_EXPECT(RHI::bSupportsVRS == false);
        TEST_EXPECT(RHI::MaxShaderModel == EShaderModel::SM_6_6);
        TEST_EXPECT(RHI::bSupportRenderTargetArrayIndexFromVertexShader);
        TEST_EXPECT(RHI::bSupportsDynamicDepthBias);

        TEST_SECTION("Metal sample counts and sample positions match the device");
        {
            uint32 SampleCounts = 0;
            TEST_EXPECT(RHI::Device->QuerySupportedSampleCounts(EFormat::B8G8R8A8_Unorm, SampleCounts));
            TEST_EXPECT((SampleCounts & RHI_SAMPLE_COUNT_1) != 0);

#if PLATFORM_MACOS
            TEST_EXPECT(RHI::bSupportsProgrammableSamplePositions == GMetalSupportsProgrammableSamplePositions);
            if (GMetalSupportsProgrammableSamplePositions)
            {
                TEST_EXPECT(RHI::SamplePositionsTier == ESamplePositionsTier::Tier1);
                TEST_EXPECT(RHI::MaxSamplePositionGridWidth == 1);
                TEST_EXPECT(RHI::MaxSamplePositionGridHeight == 1);
            }
            else
            {
                TEST_EXPECT(RHI::SamplePositionsTier == ESamplePositionsTier::NotSupported);
            }
#endif
            TEST_EXPECT(RHI::bSupportsGPUTimestampBubblesRemoval == false);

            uint32 CompressedSampleCounts = 0;
            TEST_EXPECT(!RHI::Device->QuerySupportedSampleCounts(EFormat::BC1_UNorm, CompressedSampleCounts));
            TEST_EXPECT(CompressedSampleCounts == 0);
        }

        TEST_SECTION("Metal reports only what the backend implements");
        TEST_EXPECT(RHI::bSupportsViewInstancing == false);
        TEST_EXPECT(RHI::MaxViewInstanceCount == 1);
        TEST_EXPECT(RHI::MaxTexture3DWidth == 2048);
        TEST_EXPECT(RHI::MaxTexture3DHeight == 2048);
        TEST_EXPECT(RHI::MaxTexture3DDepth == 2048);
    }

    TEST_END();
}

#if PLATFORM_MACOS
static constexpr uint32 GMemoryPatternWords = 16;

static uint32 GetMemoryPatternWord(uint32 ResourceIndex, uint32 Word)
{
    return ((ResourceIndex + 1) << 16) | (Word + 1);
}

static void RunMemoryFrames(uint32 NumFrames)
{
    for (uint32 Frame = 0; Frame < NumFrames; ++Frame)
    {
        FRHICommandList CommandList;
        CommandList.BeginFrame();
        CommandList.EndFrame();
        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
        FRHICommandListExecutor::Get().WaitForGPU();
    }
}

static void FinalizePendingMoves()
{
    FRHICommandList CommandList;
    CommandList.BeginFrame();
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    FRHICommandListExecutor::Get().WaitForGPU();
}

static bool WriteMemoryPattern(FRHIBuffer* Buffer, uint32 ResourceIndex)
{
    uint32 Pattern[GMemoryPatternWords];
    for (uint32 Word = 0; Word < GMemoryPatternWords; ++Word)
    {
        Pattern[Word] = GetMemoryPatternWord(ResourceIndex, Word);
    }

    FRHIFenceRef Fence = RHI::CreateFence();
    FRHICommandList CommandList;
    CommandList.UpdateBuffer(Buffer, FBufferRegion(0, sizeof(Pattern)), Pattern);
    CommandList.WriteFence(Fence.Get());
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    FRHICommandListExecutor::Get().WaitForCommands();
    return Fence->Wait(5ull * 1000ull * 1000ull * 1000ull);
}

static bool ReadbackMatchesPattern(FRHIBuffer* Buffer, uint32 ResourceIndex)
{
    constexpr uint64 PatternSize = GMemoryPatternWords * sizeof(uint32);

    FRHIBufferRef Readback = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(PatternSize));
    if (!Readback)
    {
        return false;
    }

    FRHIFenceRef Fence = RHI::CreateFence();
    FRHICommandList CommandList;
    CommandList.CopyBuffer(Readback.Get(), Buffer, FRHIBufferCopyDesc(0, 0, PatternSize));
    CommandList.WriteFence(Fence.Get());
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    FRHICommandListExecutor::Get().WaitForCommands();
    if (!Fence->Wait(5ull * 1000ull * 1000ull * 1000ull))
    {
        return false;
    }

    const uint32* Mapped = static_cast<const uint32*>(Readback->Map());
    if (!Mapped)
    {
        return false;
    }

    bool bMatches = true;
    for (uint32 Word = 0; Word < GMemoryPatternWords; ++Word)
    {
        bMatches = bMatches && Mapped[Word] == GetMemoryPatternWord(ResourceIndex, Word);
    }

    Readback->Unmap();
    return bMatches;
}

static bool ReadTexturePixel(FRHITexture* Texture, uint32 MipLevel, uint8 (&OutPixel)[4])
{
    FRHIBufferRef Readback = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(256));
    if (!Readback)
    {
        return false;
    }

    FRHIFenceRef Fence = RHI::CreateFence();
    FRHICommandList CommandList;
    CommandList.CopyTextureRegionToBuffer(Readback.Get(), 0, Texture, FTextureRegion2D(1, 1), MipLevel);
    CommandList.WriteFence(Fence.Get());
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    FRHICommandListExecutor::Get().WaitForCommands();
    if (!Fence->Wait(5ull * 1000ull * 1000ull * 1000ull))
    {
        return false;
    }

    const uint8* Mapped = static_cast<const uint8*>(Readback->Map());
    if (!Mapped)
    {
        return false;
    }

    Memory::Memcpy(OutPixel, Mapped, sizeof(OutPixel));
    Readback->Unmap();
    return true;
}

static bool DispatchAndReadFirstUint(FRHIComputePipelineState* Pipeline, FRHIComputeShader* Shader, FRHIBuffer* Params, FRHIShaderResourceView* SourceSRV, uint32& OutValue)
{
    FRHIBufferRef Output = RHI::CreateBuffer(FRHIBufferDesc(EBufferFlags::Default | EBufferFlags::RWBuffer | EBufferFlags::CopySource, sizeof(uint32), sizeof(uint32)));
    FRHIBufferRef Readback = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(sizeof(uint32)));
    FRHIUnorderedAccessViewRef OutputUAV = Output
        ? RHI::CreateUnorderedAccessView(Output.Get(), FRHIUnorderedAccessViewDesc::CreateBuffer(0, 1, EBufferViewType::Structured))
        : nullptr;

    if (!Output || !Readback || !OutputUAV)
    {
        return false;
    }

    FRHIFenceRef Fence = RHI::CreateFence();
    FRHICommandList CommandList;
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Output.Get(), ERHIResourceState::Common, ERHIResourceState::UnorderedAccess));
    CommandList.SetComputePipelineState(Pipeline);

    if (Params)
    {
        CommandList.SetConstantBuffer(Shader, Params, 0);
    }

    if (SourceSRV)
    {
        CommandList.SetShaderResourceView(Shader, SourceSRV, 0);
    }

    CommandList.SetUnorderedAccessView(Shader, OutputUAV.Get(), 0);
    CommandList.Dispatch(1, 1, 1);
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Output.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
    CommandList.CopyBuffer(Readback.Get(), Output.Get(), FRHIBufferCopyDesc(0, 0, sizeof(uint32)));
    CommandList.WriteFence(Fence.Get());
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    FRHICommandListExecutor::Get().WaitForCommands();
    if (!Fence->Wait(5ull * 1000ull * 1000ull * 1000ull))
    {
        return false;
    }

    const uint32* Mapped = static_cast<const uint32*>(Readback->Map());
    if (!Mapped)
    {
        return false;
    }

    OutValue = *Mapped;
    Readback->Unmap();
    return true;
}

static FRHIComputePipelineStateRef CreateMemoryProbePipeline(const CHAR* Source, EShaderModel ShaderModel, FRHIComputeShaderRef& OutShader)
{
    TArray<uint8> ByteCode;
    const FShaderCompileInfo CompileInfo("Main", ShaderModel, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromSource(Source, CompileInfo, ByteCode))
    {
        return nullptr;
    }

    OutShader = RHI::CreateComputeShader(ByteCode);
    if (!OutShader)
    {
        return nullptr;
    }

    FRHIComputePipelineStateDesc PipelineDesc;
    PipelineDesc.Shader = OutShader.Get();
    return RHI::CreateComputePipelineState(PipelineDesc);
}

static FRHIBufferRef CreateIndexParams(uint32 Index)
{
    const uint32 ParamsData[4] = { Index, 0, 0, 0 };
    return RHI::CreateBuffer(FRHIBufferDesc::CreateConstantBuffer(sizeof(ParamsData)), ERHIResourceState::Common, ParamsData);
}

struct FMemoryProbeBuffer
{
    FRHIBufferRef             Buffer;
    FRHIShaderResourceViewRef SRV;
    FRHIDescriptorHandle      BindlessHandle;
    FMetalHeap*               Heap   = nullptr;
    uint64                    Offset = 0;
    uint32                    Index  = 0;
};

static TArray<FMemoryProbeBuffer> CreateFragmentedHeapBuffers(uint32 NumBuffers, uint64 BufferSize, bool bWithBindless)
{
    const FRHIBufferDesc Desc(EBufferFlags::Default | EBufferFlags::ShaderResourceBuffer | EBufferFlags::CopySource | EBufferFlags::CopyDest, sizeof(uint32), BufferSize);

    TArray<FMemoryProbeBuffer> Created;
    for (uint32 Index = 0; Index < NumBuffers; ++Index)
    {
        FMemoryProbeBuffer Entry;
        Entry.Buffer = RHI::CreateBuffer(Desc);
        Entry.Index  = Index;

        if (!Entry.Buffer || !WriteMemoryPattern(Entry.Buffer.Get(), Index))
        {
            return TArray<FMemoryProbeBuffer>();
        }

        Entry.Heap = GetMetalBuffer(Entry.Buffer.Get())->GetResourceStorage().GetHeap();
        Created.Add(Move(Entry));
    }

    FMetalHeap* FirstHeap = Created[0].Heap;
    FMetalHeap* LastHeap  = Created.Last().Heap;

    TArray<FMemoryProbeBuffer> Survivors;
    uint32 NumFreedInFirstHeap = 0;
    uint32 NumSeenInLastHeap   = 0;

    for (FMemoryProbeBuffer& Entry : Created)
    {
        bool bFree = false;

        if (Entry.Heap == LastHeap && LastHeap != FirstHeap)
        {
            bFree = (NumSeenInLastHeap++ % 2) == 0;
        }
        else if (Entry.Heap == FirstHeap && NumFreedInFirstHeap < 2)
        {
            bFree = true;
            ++NumFreedInFirstHeap;
        }

        if (!bFree)
        {
            Survivors.Add(Move(Entry));
        }
    }

    Created.Clear();

    for (FMemoryProbeBuffer& Survivor : Survivors)
    {
        const FMetalResourceStorage& Storage = GetMetalBuffer(Survivor.Buffer.Get())->GetResourceStorage();
        Survivor.Heap   = Storage.GetHeap();
        Survivor.Offset = Storage.GetResourceOffset();

        if (bWithBindless)
        {
            Survivor.SRV = RHI::CreateShaderResourceView(Survivor.Buffer.Get(), FRHIShaderResourceViewDesc::CreateBuffer(0, GMemoryPatternWords));
            Survivor.BindlessHandle = Survivor.SRV ? Survivor.SRV->GetBindlessHandle() : FRHIDescriptorHandle();
        }
    }

    return Survivors;
}

static bool HasMoved(const FMemoryProbeBuffer& Entry)
{
    const FMetalResourceStorage& Storage = GetMetalBuffer(Entry.Buffer.Get())->GetResourceStorage();
    return Storage.GetHeap() != Entry.Heap || Storage.GetResourceOffset() != Entry.Offset;
}

static bool ProbeMemoryDefragAndResidency()
{
    TEST_BEGIN();

    FMetalDeviceRHI* MetalDeviceRHI = FMetalDeviceRHI::Get();
    FMetalDevice*    MetalDevice    = MetalDeviceRHI ? MetalDeviceRHI->GetMetalDevice() : nullptr;
    TEST_EXPECT(MetalDevice != nullptr);

    if (!MetalDevice || !FShaderCompiler::Initialize(Paths::GetAssetDir()))
    {
        TEST_EXPECT(false);
        TEST_END();
    }

    constexpr uint64 BufferSize   = 4ull * 1024ull * 1024ull;
    const bool       bBindless    = RHI::bSupportsBindless;

    static const CHAR BindlessBufferSource[] =
        "RWStructuredBuffer<uint> OutBuffer : register(u0);\n"
        "cbuffer Params : register(b0) { uint ResourceIndex; uint Pad0; uint Pad1; uint Pad2; };\n"
        "[numthreads(1,1,1)]\n"
        "void Main()\n"
        "{\n"
        "    StructuredBuffer<uint> Source = ResourceDescriptorHeap[ResourceIndex];\n"
        "    OutBuffer[0] = Source[3];\n"
        "}\n";

    static const CHAR BindlessTextureSource[] =
        "RWStructuredBuffer<uint> OutBuffer : register(u0);\n"
        "cbuffer Params : register(b0) { uint ResourceIndex; uint Pad0; uint Pad1; uint Pad2; };\n"
        "[numthreads(1,1,1)]\n"
        "void Main()\n"
        "{\n"
        "    Texture2D<float4> Source = ResourceDescriptorHeap[ResourceIndex];\n"
        "    OutBuffer[0] = (uint)(Source.Load(int3(0, 0, 0)).x * 255.0f + 0.5f);\n"
        "}\n";

    static const CHAR DirectBufferSource[] =
        "StructuredBuffer<uint> Source : register(t0);\n"
        "RWStructuredBuffer<uint> OutBuffer : register(u0);\n"
        "[numthreads(1,1,1)]\n"
        "void Main()\n"
        "{\n"
        "    OutBuffer[0] = Source[3];\n"
        "}\n";

    FRHIComputeShaderRef        BindlessBufferShader;
    FRHIComputePipelineStateRef BindlessBufferPipeline = bBindless ? CreateMemoryProbePipeline(BindlessBufferSource, EShaderModel::SM_6_6, BindlessBufferShader) : nullptr;
    FRHIComputeShaderRef        BindlessTextureShader;
    FRHIComputePipelineStateRef BindlessTexturePipeline = bBindless ? CreateMemoryProbePipeline(BindlessTextureSource, EShaderModel::SM_6_6, BindlessTextureShader) : nullptr;
    FRHIComputeShaderRef        DirectBufferShader;
    FRHIComputePipelineStateRef DirectBufferPipeline = CreateMemoryProbePipeline(DirectBufferSource, EShaderModel::SM_6_2, DirectBufferShader);
    TEST_EXPECT(!bBindless || (BindlessBufferPipeline != nullptr && BindlessTexturePipeline != nullptr));
    TEST_EXPECT(DirectBufferPipeline != nullptr);

    SetConsoleVariable("MetalRHI.MaxDefragMovesPerFrame", "4");
    SetConsoleVariable("MetalRHI.DefragEligibilityDelay", "0");

    TEST_SECTION("A defrag move keeps buffer contents, views and bindless slots");
    {
        TArray<FMemoryProbeBuffer> Survivors = CreateFragmentedHeapBuffers(24, BufferSize, bBindless);
        TEST_EXPECT(!Survivors.IsEmpty());

        RunMemoryFrames(8);
        FinalizePendingMoves();
        TEST_EXPECT(!MetalDevice->HasPendingDefragMoves());

        uint32 NumMoved = 0;
        for (const FMemoryProbeBuffer& Survivor : Survivors)
        {
            if (HasMoved(Survivor))
            {
                ++NumMoved;
            }

            TEST_EXPECT(ReadbackMatchesPattern(Survivor.Buffer.Get(), Survivor.Index));

            if (bBindless && Survivor.SRV && BindlessBufferPipeline)
            {
                TEST_EXPECT(Survivor.SRV->GetBindlessHandle() == Survivor.BindlessHandle);

                FRHIBufferRef Params = CreateIndexParams(Survivor.BindlessHandle.Index);
                uint32 Value = 0;
                TEST_EXPECT(Params && DispatchAndReadFirstUint(BindlessBufferPipeline.Get(), BindlessBufferShader.Get(), Params.Get(), nullptr, Value));
                TEST_EXPECT_EQ(Value, GetMemoryPatternWord(Survivor.Index, 3));
            }
        }

        LOG_INFO("[BOOT] Defrag moved %u of %d surviving buffers", NumMoved, Survivors.Size());
        TEST_EXPECT(NumMoved > 0);
    }

    TEST_SECTION("A defrag move keeps texture mips, render targets and bindless slots");
    {
        constexpr uint32 NumTextures = 24;
        constexpr uint32 Extent      = 1024;
        constexpr uint32 NumMips     = 3;

        TArray<uint32> MipPixels[NumMips];
        for (uint32 Mip = 0; Mip < NumMips; ++Mip)
        {
            MipPixels[Mip].Resize((Extent >> Mip) * (Extent >> Mip));
        }

        struct FProbeTexture
        {
            FRHITextureRef       Texture;
            FRHIDescriptorHandle BindlessHandle;
            FMetalHeap*          Heap   = nullptr;
            uint64               Offset = 0;
            uint32               Index  = 0;
            uint32               Mips   = 1;
        };

        const auto GetPixel = [](uint32 Index, uint32 Mip) -> uint32
        {
            return 0xFF000000u | ((100u + Mip) << 8) | (10u + Index);
        };

        TArray<FProbeTexture> Created;
        for (uint32 Index = 0; Index < NumTextures; ++Index)
        {
            FProbeTexture Entry;
            Entry.Index = Index;
            Entry.Mips  = (Index % 3 == 0) ? NumMips : 1;

            const void* MipData[NumMips];
            int64       RowPitches[NumMips];
            int64       SlicePitches[NumMips];
            for (uint32 Mip = 0; Mip < Entry.Mips; ++Mip)
            {
                MipPixels[Mip].Fill(GetPixel(Index, Mip));
                MipData[Mip]      = MipPixels[Mip].Data();
                RowPitches[Mip]   = int64(Extent >> Mip) * 4;
                SlicePitches[Mip] = RowPitches[Mip] * int64(Extent >> Mip);
            }

            ETextureUsageFlags Usage = ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopySource;
            if (Index % 5 == 1)
            {
                Usage |= ETextureUsageFlags::RenderTarget;
            }

            FBootMipChainData InitialData(MipData, RowPitches, SlicePitches, Entry.Mips);
            Entry.Texture = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, Extent, Extent, Entry.Mips, 1, Usage), ERHIResourceState::Common, &InitialData);
            TEST_EXPECT(Entry.Texture != nullptr);

            if (Entry.Texture)
            {
                Entry.Heap = GetMetalTexture(Entry.Texture.Get())->GetResourceStorage().GetHeap();
                Created.Add(Move(Entry));
            }
        }

        TArray<FProbeTexture> Survivors;
        if (!Created.IsEmpty())
        {
            FMetalHeap* FirstHeap = Created[0].Heap;
            FMetalHeap* LastHeap  = Created.Last().Heap;
            TEST_EXPECT(FirstHeap != LastHeap);

            uint32 NumFreedInFirstHeap = 0;
            uint32 NumSeenInLastHeap   = 0;
            for (FProbeTexture& Entry : Created)
            {
                bool bFree = false;
                if (Entry.Heap == LastHeap && LastHeap != FirstHeap)
                {
                    bFree = (NumSeenInLastHeap++ % 2) == 0;
                }
                else if (Entry.Heap == FirstHeap && NumFreedInFirstHeap < 3)
                {
                    bFree = true;
                    ++NumFreedInFirstHeap;
                }

                if (!bFree)
                {
                    Survivors.Add(Move(Entry));
                }
            }

            Created.Clear();
        }

        for (FProbeTexture& Survivor : Survivors)
        {
            const FMetalResourceStorage& Storage = GetMetalTexture(Survivor.Texture.Get())->GetResourceStorage();
            Survivor.Heap           = Storage.GetHeap();
            Survivor.Offset         = Storage.GetResourceOffset();
            Survivor.BindlessHandle = bBindless ? Survivor.Texture->GetBindlessSRVHandle() : FRHIDescriptorHandle();
        }

        RunMemoryFrames(9);
        FinalizePendingMoves();
        TEST_EXPECT(!MetalDevice->HasPendingDefragMoves());

        uint32 NumMoved = 0;
        for (const FProbeTexture& Survivor : Survivors)
        {
            const FMetalResourceStorage& Storage = GetMetalTexture(Survivor.Texture.Get())->GetResourceStorage();
            if (Storage.GetHeap() != Survivor.Heap || Storage.GetResourceOffset() != Survivor.Offset)
            {
                ++NumMoved;
            }

            for (uint32 Mip = 0; Mip < Survivor.Mips; ++Mip)
            {
                uint8 Pixel[4] = {};
                TEST_EXPECT(ReadTexturePixel(Survivor.Texture.Get(), Mip, Pixel));
                TEST_EXPECT_EQ(Pixel[0], static_cast<uint8>(10u + Survivor.Index));
                TEST_EXPECT_EQ(Pixel[1], static_cast<uint8>(100u + Mip));
            }

            if (bBindless && BindlessTexturePipeline)
            {
                TEST_EXPECT(Survivor.Texture->GetBindlessSRVHandle() == Survivor.BindlessHandle);

                FRHIBufferRef Params = CreateIndexParams(Survivor.BindlessHandle.Index);
                uint32 Value = 0;
                TEST_EXPECT(Params && DispatchAndReadFirstUint(BindlessTexturePipeline.Get(), BindlessTextureShader.Get(), Params.Get(), nullptr, Value));
                TEST_EXPECT_EQ(Value, 10u + Survivor.Index);
            }
        }

        LOG_INFO("[BOOT] Defrag moved %u of %d surviving textures", NumMoved, Survivors.Size());
        TEST_EXPECT(NumMoved > 0);
    }

    TEST_SECTION("Destroying a resource while its move is pending cancels the move");
    {
        TArray<FMemoryProbeBuffer> Survivors = CreateFragmentedHeapBuffers(24, BufferSize, bBindless);
        TEST_EXPECT(!Survivors.IsEmpty());

        const int64 CancelsBefore = STAT_GET(STAT_Metal_DefragCancels);

        uint32 NumCancelled = 0;
        for (uint32 Attempt = 0; Attempt < 8 && NumCancelled == 0; ++Attempt)
        {
            FRHICommandList CommandList;
            CommandList.BeginFrame();
            CommandList.EndFrame();
            FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
            FRHICommandListExecutor::Get().WaitForCommands();

            for (int32 Index = Survivors.Size() - 1; Index >= 0; --Index)
            {
                if (GetMetalBuffer(Survivors[Index].Buffer.Get())->GetResourceStorage().IsDefragPending())
                {
                    Survivors.RemoveAt(Index);
                    ++NumCancelled;
                }
            }

            FRHICommandListExecutor::Get().WaitForGPU();
        }

        TEST_EXPECT(NumCancelled > 0);
        RunMemoryFrames(2);
        TEST_EXPECT(!MetalDevice->HasPendingDefragMoves());

#if METAL_ENABLE_STATS
        TEST_EXPECT(STAT_GET(STAT_Metal_DefragCancels) >= CancelsBefore + static_cast<int64>(NumCancelled));
#else
        UNREFERENCED_VARIABLE(CancelsBefore);
#endif

        for (const FMemoryProbeBuffer& Survivor : Survivors)
        {
            TEST_EXPECT(ReadbackMatchesPattern(Survivor.Buffer.Get(), Survivor.Index));
        }
    }

    TEST_SECTION("A small residency budget evicts idle allocations and keeps pinned ones");
    {
        constexpr uint64 StandaloneSize = 65ull * 1024ull * 1024ull;
        const FRHIBufferDesc Desc(EBufferFlags::Default | EBufferFlags::ShaderResourceBuffer | EBufferFlags::CopySource | EBufferFlags::CopyDest, sizeof(uint32), StandaloneSize);

        FRHIBufferRef Idle   = RHI::CreateBuffer(Desc);
        FRHIBufferRef Pinned = RHI::CreateBuffer(Desc);
        TEST_EXPECT(Idle != nullptr);
        TEST_EXPECT(Pinned != nullptr);

        if (Idle && Pinned && WriteMemoryPattern(Idle.Get(), 40) && WriteMemoryPattern(Pinned.Get(), 41))
        {
            FMetalResidencyEntry* IdleEntry   = GetMetalBuffer(Idle.Get())->GetResidencyEntry();
            FMetalResidencyEntry* PinnedEntry = GetMetalBuffer(Pinned.Get())->GetResidencyEntry();
            TEST_EXPECT(IdleEntry != nullptr);
            TEST_EXPECT(PinnedEntry != nullptr);

            FRHIShaderResourceViewRef IdleSRV   = RHI::CreateShaderResourceView(Idle.Get(), FRHIShaderResourceViewDesc::CreateBuffer(0, GMemoryPatternWords));
            FRHIShaderResourceViewRef PinnedSRV = RHI::CreateShaderResourceView(Pinned.Get(), FRHIShaderResourceViewDesc::CreateBuffer(0, GMemoryPatternWords));
            const FRHIDescriptorHandle PinnedHandle = (bBindless && PinnedSRV) ? PinnedSRV->GetBindlessHandle() : FRHIDescriptorHandle();

            SetConsoleVariable("MetalRHI.ResidencyBudgetMB", "1");
            RunMemoryFrames(5);

            FMetalResidencyManager& ResidencyManager = MetalDevice->GetResidencyManager();
            TEST_EXPECT(ResidencyManager.GetEvictedBytes() > 0);

            if (IdleEntry && PinnedEntry)
            {
                TEST_EXPECT(!IdleEntry->bResident);
                TEST_EXPECT(!bBindless || PinnedEntry->bResident);
            }

            uint32 Value = 0;
            TEST_EXPECT(IdleSRV && DirectBufferPipeline && DispatchAndReadFirstUint(DirectBufferPipeline.Get(), DirectBufferShader.Get(), nullptr, IdleSRV.Get(), Value));
            TEST_EXPECT_EQ(Value, GetMemoryPatternWord(40, 3));
            TEST_EXPECT(!IdleEntry || IdleEntry->bResident);

            if (bBindless && PinnedHandle.IsValid() && BindlessBufferPipeline)
            {
                FRHIBufferRef Params = CreateIndexParams(PinnedHandle.Index);
                Value = 0;
                TEST_EXPECT(Params && DispatchAndReadFirstUint(BindlessBufferPipeline.Get(), BindlessBufferShader.Get(), Params.Get(), nullptr, Value));
                TEST_EXPECT_EQ(Value, GetMemoryPatternWord(41, 3));
            }

            SetConsoleVariable("MetalRHI.ResidencyBudgetMB", "0");
            RunMemoryFrames(1);
        }
    }

    FRayQueryProbe RayQueryProbe;
    const bool bTraceStructures = RHI::bSupportsInlineRayTracing && CreateRayQueryProbe(RayQueryProbe);
    TEST_EXPECT(!RHI::bSupportsInlineRayTracing || bTraceStructures);

    TEST_SECTION("A defrag pass moves buffers around acceleration structures but never the structures themselves");
    if (bTraceStructures)
    {
        FRayTracingProbeGeometry Geometry;
        TEST_EXPECT(CreateProbeGeometry(true, EAccelerationStructureBuildFlags::None, Geometry));

        const FRHIGeometryAccelerationStructureInstance Instances[] = { MakeProbeInstance(Geometry.Geometry.Get(), 31, 0.0f) };

        FRHISceneAccelerationStructureRef Scene = Geometry.Geometry
            ? RHI::CreateSceneAccelerationStructure(FRHISceneAccelerationStructureDesc(Instances, EAccelerationStructureBuildFlags::None))
            : nullptr;
        TEST_EXPECT(Scene != nullptr);

        if (Scene)
        {
            id<MTLAccelerationStructure> GeometryStructure = GetMetalAccelerationStructure(Geometry.Geometry.Get())->GetMTLAccelerationStructure();
            id<MTLAccelerationStructure> SceneStructure    = GetMetalAccelerationStructure(Scene.Get())->GetMTLAccelerationStructure();
            TEST_EXPECT(GeometryStructure.heap != nil);
            TEST_EXPECT(SceneStructure.heap != nil);

            const uint64 GeometryID = GeometryStructure.gpuResourceID._impl;
            const uint64 SceneID    = SceneStructure.gpuResourceID._impl;

            TArray<FMemoryProbeBuffer> Survivors = CreateFragmentedHeapBuffers(24, BufferSize, false);
            TEST_EXPECT(!Survivors.IsEmpty());

            RunMemoryFrames(8);
            FinalizePendingMoves();
            TEST_EXPECT(!MetalDevice->HasPendingDefragMoves());

            uint32 NumMoved = 0;
            for (const FMemoryProbeBuffer& Survivor : Survivors)
            {
                NumMoved += HasMoved(Survivor) ? 1 : 0;
            }

            TEST_EXPECT(NumMoved > 0);
            TEST_EXPECT(GetMetalAccelerationStructure(Geometry.Geometry.Get())->GetMTLAccelerationStructure() == GeometryStructure);
            TEST_EXPECT(GetMetalAccelerationStructure(Scene.Get())->GetMTLAccelerationStructure() == SceneStructure);
            TEST_EXPECT_EQ(GeometryStructure.gpuResourceID._impl, GeometryID);
            TEST_EXPECT_EQ(SceneStructure.gpuResourceID._impl, SceneID);

            FRayQueryProbeHit Hit = {};
            TEST_EXPECT(TraceProbeRay(RayQueryProbe, Scene.Get(), bBindless, 0.0f, Hit));
            TEST_EXPECT(IsExpectedProbeHit(Hit, 31, 0, 5.0f));
        }
    }

    TEST_SECTION("A small residency budget evicts filler allocations but no acceleration structure");
    if (bTraceStructures)
    {
        constexpr uint64 StandaloneSize = 65ull * 1024ull * 1024ull;
        const FRHIBufferDesc FillerDesc(EBufferFlags::Default | EBufferFlags::ShaderResourceBuffer | EBufferFlags::CopySource | EBufferFlags::CopyDest, sizeof(uint32), StandaloneSize);

        FRayTracingProbeGeometry Geometry;
        TEST_EXPECT(CreateProbeGeometry(true, EAccelerationStructureBuildFlags::None, Geometry));

        const FRHIGeometryAccelerationStructureInstance Instances[] = { MakeProbeInstance(Geometry.Geometry.Get(), 32, 0.0f) };

        FRHISceneAccelerationStructureRef Scene = Geometry.Geometry
            ? RHI::CreateSceneAccelerationStructure(FRHISceneAccelerationStructureDesc(Instances, EAccelerationStructureBuildFlags::None))
            : nullptr;
        FRHIBufferRef Filler = RHI::CreateBuffer(FillerDesc);
        TEST_EXPECT(Scene != nullptr);
        TEST_EXPECT(Filler != nullptr);

        if (Scene && Filler && WriteMemoryPattern(Filler.Get(), 42))
        {
            FMetalResidencyEntry* FillerEntry   = GetMetalBuffer(Filler.Get())->GetResidencyEntry();
            FMetalResidencyEntry* GeometryEntry = GetMetalAccelerationStructure(Geometry.Geometry.Get())->GetResidencyEntry();
            FMetalResidencyEntry* SceneEntry    = GetMetalAccelerationStructure(Scene.Get())->GetResidencyEntry();
            TEST_EXPECT(FillerEntry != nullptr);
            TEST_EXPECT(GeometryEntry != nullptr);
            TEST_EXPECT(SceneEntry != nullptr);

            FMetalResidencyManager& ResidencyManager = MetalDevice->GetResidencyManager();
            const uint64 EvictedBefore = ResidencyManager.GetEvictedBytes();

            SetConsoleVariable("MetalRHI.ResidencyBudgetMB", "1");
            RunMemoryFrames(5);

            TEST_EXPECT(ResidencyManager.GetEvictedBytes() > EvictedBefore);
            TEST_EXPECT(!FillerEntry || !FillerEntry->bResident);
            TEST_EXPECT(!GeometryEntry || GeometryEntry->bResident);
            TEST_EXPECT(!SceneEntry || SceneEntry->bResident);

            FRayQueryProbeHit Hit = {};
            TEST_EXPECT(TraceProbeRay(RayQueryProbe, Scene.Get(), bBindless, 0.0f, Hit));
            TEST_EXPECT(IsExpectedProbeHit(Hit, 32, 0, 5.0f));

            SetConsoleVariable("MetalRHI.ResidencyBudgetMB", "0");
            RunMemoryFrames(1);
        }
    }

    TEST_SECTION("Creating and releasing resources across frames with defrag and eviction on");
    {
        SetConsoleVariable("MetalRHI.ResidencyBudgetMB", "32");

        for (uint32 Iteration = 0; Iteration < 12; ++Iteration)
        {
            TArray<FMemoryProbeBuffer> Buffers = CreateFragmentedHeapBuffers(20, (2ull + (Iteration % 3)) * 1024ull * 1024ull, bBindless);
            TEST_EXPECT(!Buffers.IsEmpty());

            FRHITextureRef Texture = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(
                EFormat::R8G8B8A8_Unorm, 256, 256, 1, 1, ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::RenderTarget));
            if (Texture && bBindless)
            {
                TEST_EXPECT(Texture->GetBindlessSRVHandle().IsValid());
            }

            RunMemoryFrames(1);

            for (int32 Index = Buffers.Size() - 1; Index >= 0; Index -= 2)
            {
                Buffers.RemoveAt(Index);
            }

            RunMemoryFrames(1);
        }

        RunMemoryFrames(2);
        SetConsoleVariable("MetalRHI.ResidencyBudgetMB", "0");
    }

    SetConsoleVariable("MetalRHI.DefragEligibilityDelay", "1");
    RunMemoryFrames(1);

    FShaderCompiler::Destroy();
    TEST_END();
}

static bool ProbePipelinePersistence()
{
    TEST_BEGIN();

    FMetalDeviceRHI* MetalDeviceRHI = FMetalDeviceRHI::Get();
    FMetalDevice*    MetalDevice    = MetalDeviceRHI ? MetalDeviceRHI->GetMetalDevice() : nullptr;
    TEST_EXPECT(MetalDevice != nullptr);
    if (!MetalDevice)
    {
        TEST_END();
    }

    if (!FShaderCompiler::Initialize(Paths::GetAssetDir()))
    {
        TEST_EXPECT(false);
        TEST_END();
    }

    TEST_SECTION("Shaders compiled from the same bytecode share one function");

    const String ComputeSource(
        "SamplerState LinearSampler : register(s0);\n"
        "Texture2D<float4> SourceTex : register(t0);\n"
        "RWTexture2D<float4> DestTex : register(u0);\n"
        "[numthreads(1, 1, 1)]\n"
        "void Main(uint3 DispatchThreadID : SV_DispatchThreadID)\n"
        "{\n"
        "    DestTex[DispatchThreadID.xy] = SourceTex.SampleLevel(LinearSampler, float2(0.5, 0.5), 0);\n"
        "}\n");

    TArray<uint8> ComputeByteCode;
    const FShaderCompileInfo ComputeCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    const bool bComputeCompiled = FShaderCompiler::Get().CompileFromSource(ComputeSource, ComputeCompileInfo, ComputeByteCode);
    TEST_EXPECT(bComputeCompiled);

    FRHIComputeShaderRef ComputeShader;
    if (bComputeCompiled)
    {
        const int64 LibraryHitsBefore = STAT_GET(STAT_Metal_LibraryCacheHits);

        // RHI::CreateComputeShader dedupes identical bytecode above the backend, so the duplicate goes to the device directly
        ComputeShader = RHI::CreateComputeShader(ComputeByteCode);
        FRHIComputeShaderRef DuplicateShader(RHI::Device->CreateComputeShader(ComputeByteCode));
        TEST_EXPECT(ComputeShader != nullptr);
        TEST_EXPECT(DuplicateShader != nullptr);

        if (ComputeShader && DuplicateShader)
        {
            TEST_EXPECT(ComputeShader.Get() != DuplicateShader.Get());
            TEST_EXPECT(GetMetalShader(ComputeShader.Get())->GetMTLFunction() != nil);
            TEST_EXPECT(GetMetalShader(ComputeShader.Get())->GetMTLFunction() == GetMetalShader(DuplicateShader.Get())->GetMTLFunction());
        }

#if METAL_ENABLE_STATS
        TEST_EXPECT(STAT_GET(STAT_Metal_LibraryCacheHits) >= LibraryHitsBefore + 1);
#else
        (void)LibraryHitsBefore;
#endif
    }

    TEST_SECTION("Compute PSOs that differ only in a static sampler share one pipeline");

    if (ComputeShader)
    {
        FRHIStaticSamplerInfo LinearSampler;
        LinearSampler.ShaderRegister   = 0;
        LinearSampler.ShaderVisibility = EShaderStage::Compute;
        LinearSampler.Filter           = ESamplerFilter::MinMagMipLinear;

        FRHIStaticSamplerInfo PointSampler = LinearSampler;
        PointSampler.Filter = ESamplerFilter::MinMagMipPoint;

        FRHIComputePipelineStateDesc LinearDesc;
        LinearDesc.Shader         = ComputeShader.Get();
        LinearDesc.StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>(&LinearSampler, 1);

        FRHIComputePipelineStateDesc PointDesc;
        PointDesc.Shader         = ComputeShader.Get();
        PointDesc.StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>(&PointSampler, 1);

        FRHIComputePipelineStateRef LinearPipeline = RHI::CreateComputePipelineState(LinearDesc);
        FRHIComputePipelineStateRef PointPipeline  = RHI::CreateComputePipelineState(PointDesc);
        TEST_EXPECT(LinearPipeline != nullptr);
        TEST_EXPECT(PointPipeline != nullptr);

        if (LinearPipeline && PointPipeline)
        {
            TEST_EXPECT(LinearPipeline.Get() != PointPipeline.Get());
            TEST_EXPECT(LinearPipeline->GetRHINativeState() != nullptr);
            TEST_EXPECT(LinearPipeline->GetRHINativeState() == PointPipeline->GetRHINativeState());
        }
    }

    TEST_SECTION("Graphics PSOs that differ only in cull and depth state share one pipeline");

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

    FRHIVertexShaderRef VertexShader;
    FRHIPixelShaderRef  PixelShader;
    if (bVertexCompiled && bPixelCompiled)
    {
        VertexShader = RHI::CreateVertexShader(VertexByteCode);
        PixelShader  = RHI::CreatePixelShader(PixelByteCode);
        TEST_EXPECT(VertexShader != nullptr);
        TEST_EXPECT(PixelShader != nullptr);

        FRHIRasterizerStateDesc NoCullDesc;
        NoCullDesc.CullMode = ECullMode::None;

        FRHIRasterizerStateDesc BackCullDesc;
        BackCullDesc.CullMode = ECullMode::Back;

        FRHIDepthStencilStateDesc NoDepthDesc;
        NoDepthDesc.bDepthEnable      = false;
        NoDepthDesc.bDepthWriteEnable = false;

        FRHIDepthStencilStateDesc DepthDesc;
        DepthDesc.bDepthEnable      = true;
        DepthDesc.bDepthWriteEnable = true;
        DepthDesc.DepthFunc         = EComparisonFunc::LessEqual;

        FRHIRasterizerStateRef   NoCullState   = RHI::CreateRasterizerState(NoCullDesc);
        FRHIRasterizerStateRef   BackCullState = RHI::CreateRasterizerState(BackCullDesc);
        FRHIDepthStencilStateRef NoDepthState  = RHI::CreateDepthStencilState(NoDepthDesc);
        FRHIDepthStencilStateRef DepthState    = RHI::CreateDepthStencilState(DepthDesc);
        FRHIBlendStateRef        BlendState    = RHI::CreateBlendState(FRHIBlendStateDesc());

        FRHIGraphicsPipelineStateDesc FirstDesc;
        FirstDesc.VertexShader                                   = VertexShader.Get();
        FirstDesc.PixelShader                                    = PixelShader.Get();
        FirstDesc.DepthStencilState                              = NoDepthState.Get();
        FirstDesc.RasterizerState                                = NoCullState.Get();
        FirstDesc.BlendState                                     = BlendState.Get();
        FirstDesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        FirstDesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        FirstDesc.RasterizerOutputFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
        FirstDesc.RasterizerOutputFormats.DepthStencilFormat     = EFormat::D32_Float;

        FRHIGraphicsPipelineStateDesc SecondDesc = FirstDesc;
        SecondDesc.DepthStencilState = DepthState.Get();
        SecondDesc.RasterizerState   = BackCullState.Get();

        const int64 PSOHitsBefore = STAT_GET(STAT_Metal_PSOCacheHits);

        FRHIGraphicsPipelineStateRef FirstPipeline  = RHI::CreateGraphicsPipelineState(FirstDesc);
        FRHIGraphicsPipelineStateRef SecondPipeline = RHI::CreateGraphicsPipelineState(SecondDesc);
        TEST_EXPECT(FirstPipeline != nullptr);
        TEST_EXPECT(SecondPipeline != nullptr);

        if (FirstPipeline && SecondPipeline)
        {
            TEST_EXPECT(FirstPipeline.Get() != SecondPipeline.Get());
            TEST_EXPECT(FirstPipeline->GetRHINativeState() != nullptr);
            TEST_EXPECT(FirstPipeline->GetRHINativeState() == SecondPipeline->GetRHINativeState());
        }

#if METAL_ENABLE_STATS
        TEST_EXPECT(STAT_GET(STAT_Metal_PSOCacheHits) >= PSOHitsBefore + 1);
#else
        (void)PSOHitsBefore;
#endif
    }

    const String ArchivePath = Paths::GetAssetDir() + "/RHIBootTests_Probe.metalarchive";
    const String SidecarPath = ArchivePath + ".version";
    FPlatformFile::DeleteFile(*ArchivePath);
    FPlatformFile::DeleteFile(*SidecarPath);

    id<MTLFunction> ComputeFunction = ComputeShader ? GetMetalShader(ComputeShader.Get())->GetMTLFunction() : nil;
    if (ComputeFunction)
    {
        SCOPED_AUTORELEASE_POOL();

        auto MakeDescriptor = [ComputeFunction]()
        {
            MTLComputePipelineDescriptor* Descriptor = [[MTLComputePipelineDescriptor new] autorelease];
            Descriptor.computeFunction = ComputeFunction;
            return Descriptor;
        };

        TEST_SECTION("An archive written in Create mode is hit in Use mode");

        {
            FMetalBinaryArchive CreateArchive(MetalDevice);
            TEST_EXPECT(CreateArchive.Initialize(EMetalBinaryArchiveMode::Create, ArchivePath));
            TEST_EXPECT(CreateArchive.CreateComputePipeline(MakeDescriptor()) != nullptr);
            TEST_EXPECT(CreateArchive.Save());
        }

        TEST_EXPECT(FPlatformFile::IsFile(*ArchivePath));
        TEST_EXPECT(FPlatformFile::IsFile(*SidecarPath));

        {
            FMetalBinaryArchive UseArchive(MetalDevice);
            TEST_EXPECT(UseArchive.Initialize(EMetalBinaryArchiveMode::Use, ArchivePath));
            TEST_EXPECT(UseArchive.GetRejectionReason().IsEmpty());

            const int64 ArchiveHitsBefore   = STAT_GET(STAT_Metal_BinaryArchiveHits);
            const int64 ArchiveMissesBefore = STAT_GET(STAT_Metal_BinaryArchiveMisses);
            TEST_EXPECT(UseArchive.CreateComputePipeline(MakeDescriptor()) != nullptr);

#if METAL_ENABLE_STATS
            TEST_EXPECT_EQ(STAT_GET(STAT_Metal_BinaryArchiveHits), ArchiveHitsBefore + 1);
            TEST_EXPECT_EQ(STAT_GET(STAT_Metal_BinaryArchiveMisses), ArchiveMissesBefore);
#else
            (void)ArchiveHitsBefore;
            (void)ArchiveMissesBefore;
#endif
        }

        TEST_SECTION("A sidecar from another OS build is rejected and the pipeline still compiles");

        FMetalBinaryArchiveHeader Header;
        bool bHeaderRead = false;
        {
            TFileRef<IPlatformFile> File = FPlatformFile::OpenForRead(SidecarPath);
            bHeaderRead = File && File->Read(reinterpret_cast<uint8*>(&Header), sizeof(Header)) == static_cast<int32>(sizeof(Header));
        }

        TEST_EXPECT(bHeaderRead);
        if (bHeaderRead)
        {
            const CHAR TamperedBuild[] = "0TAMPER0";
            Memory::Memcpy(Header.OSBuild, TamperedBuild, sizeof(TamperedBuild));

            bool bHeaderWritten = false;
            {
                TFileRef<IPlatformFile> File = FPlatformFile::OpenForWrite(SidecarPath);
                bHeaderWritten = File && File->Write(reinterpret_cast<const uint8*>(&Header), sizeof(Header)) == static_cast<int32>(sizeof(Header));
            }

            TEST_EXPECT(bHeaderWritten);

            FMetalBinaryArchive RejectingArchive(MetalDevice);
            TEST_EXPECT(RejectingArchive.Initialize(EMetalBinaryArchiveMode::Use, ArchivePath));
            TEST_EXPECT(RejectingArchive.GetRejectionReason().Find("OS build") != String::InvalidIndex);
            TEST_EXPECT(RejectingArchive.CreateComputePipeline(MakeDescriptor()) != nullptr);
        }
    }

    id<MTLFunction> VertexFunction = VertexShader ? GetMetalShader(VertexShader.Get())->GetMTLFunction() : nil;
    id<MTLFunction> PixelFunction  = PixelShader ? GetMetalShader(PixelShader.Get())->GetMTLFunction() : nil;
    if (VertexFunction && PixelFunction)
    {
        SCOPED_AUTORELEASE_POOL();

        TEST_SECTION("Append mode saves repeatedly while new pipelines share functions with the loaded archive");

        FPlatformFile::DeleteFile(*ArchivePath);
        FPlatformFile::DeleteFile(*SidecarPath);

        auto MakeDescriptor = [VertexFunction, PixelFunction](MTLPixelFormat Format)
        {
            MTLRenderPipelineDescriptor* Descriptor = [[MTLRenderPipelineDescriptor new] autorelease];
            Descriptor.vertexFunction                  = VertexFunction;
            Descriptor.fragmentFunction                = PixelFunction;
            Descriptor.colorAttachments[0].pixelFormat = Format;
            return Descriptor;
        };

        const MTLPixelFormat Formats[] = { MTLPixelFormatRGBA8Unorm, MTLPixelFormatRGBA16Float, MTLPixelFormatBGRA8Unorm };

        {
            FMetalBinaryArchive CreateArchive(MetalDevice);
            TEST_EXPECT(CreateArchive.Initialize(EMetalBinaryArchiveMode::Create, ArchivePath));
            TEST_EXPECT(CreateArchive.CreateRenderPipeline(MakeDescriptor(Formats[0])) != nullptr);
            TEST_EXPECT(CreateArchive.Save());
        }

        {
            FMetalBinaryArchive AppendArchive(MetalDevice);
            TEST_EXPECT(AppendArchive.Initialize(EMetalBinaryArchiveMode::Append, ArchivePath));
            TEST_EXPECT(AppendArchive.GetRejectionReason().IsEmpty());
            TEST_EXPECT(AppendArchive.CreateRenderPipeline(MakeDescriptor(Formats[0])) != nullptr);
            TEST_EXPECT(AppendArchive.CreateRenderPipeline(MakeDescriptor(Formats[1])) != nullptr);
            TEST_EXPECT(AppendArchive.Save());
            TEST_EXPECT(AppendArchive.CreateRenderPipeline(MakeDescriptor(Formats[2])) != nullptr);
            TEST_EXPECT(AppendArchive.Save());
        }

        {
            FMetalBinaryArchive UseArchive(MetalDevice);
            TEST_EXPECT(UseArchive.Initialize(EMetalBinaryArchiveMode::Use, ArchivePath));
            TEST_EXPECT(UseArchive.GetRejectionReason().IsEmpty());

            const int64 ArchiveHitsBefore   = STAT_GET(STAT_Metal_BinaryArchiveHits);
            const int64 ArchiveMissesBefore = STAT_GET(STAT_Metal_BinaryArchiveMisses);
            for (MTLPixelFormat Format : Formats)
            {
                TEST_EXPECT(UseArchive.CreateRenderPipeline(MakeDescriptor(Format)) != nullptr);
            }

#if METAL_ENABLE_STATS
            TEST_EXPECT_EQ(STAT_GET(STAT_Metal_BinaryArchiveHits), ArchiveHitsBefore + 3);
            TEST_EXPECT_EQ(STAT_GET(STAT_Metal_BinaryArchiveMisses), ArchiveMissesBefore);
#else
            (void)ArchiveHitsBefore;
            (void)ArchiveMissesBefore;
#endif
        }
    }

    FPlatformFile::DeleteFile(*ArchivePath);
    FPlatformFile::DeleteFile(*SidecarPath);

    FShaderCompiler::Destroy();
    TEST_END();
}
#endif

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
        SetConsoleVariable("MetalRHI.BinaryArchiveFileName", "RHIBootTests.metalarchive");
#if PLATFORM_MACOS
        FPlatformMisc::PrepareMetalDebugLayerEnvironment(true);
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
                TEST_EXPECT(ProbeDefaultResourcesAndClears());
                TEST_EXPECT(ProbeMemoryDefragAndResidency());
                TEST_EXPECT(ProbePipelinePersistence());
            }
#endif
            TEST_EXPECT(ProbeAccelerationStructures(ExpectedType));
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

bool RHIBoot_MetalResidencyFallback_Test()
{
    SetConsoleVariable("MetalRHI.ForceResidencyFallback", true);
    const bool bResult = BootRHI(ERHIType::Metal);
    SetConsoleVariable("MetalRHI.ForceResidencyFallback", false);
    return bResult;
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
