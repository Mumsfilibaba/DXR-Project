#include "RHIBootTests.h"

#include "TestCommon/TestMacros.h"

#include <Core/Misc/ConsoleManager.h>
#include <RHI/RHI.h>
#include <RHI/RHICommandList.h>
#include <RHI/RHIResources.h>

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

static bool BootRHI(ERHIType ExpectedType)
{
    TEST_BEGIN();

    TEST_SECTION("Initialize");

    SetConsoleVariable("RHI.Type", ToString(ExpectedType));
    SetConsoleVariable("RHI.EnableValidation", true);
    SetConsoleVariable("RHI.EnableValidationDebugBreak", false);
    SetConsoleVariable("TaskGraph.EnableRHIThread", false);

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

        if (FRHICommandListExecutor::IsInitialized())
        {
            FRHICommandListExecutor::Get().WaitForGPU();
        }

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
