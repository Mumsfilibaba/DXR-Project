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

static void ProbeCreateBufferAndTexture()
{
    FRHIBufferRef Buffer = RHI::CreateBuffer(FRHIBufferDesc::CreateVertexBuffer(12, 3));
    LOG_INFO("[BOOT] CreateVertexBuffer %s", Buffer ? "ok" : "FAILED");

    const FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(
        EFormat::R8G8B8A8_Unorm, 4, 4, 1, 1, ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest);
    FRHITextureRef Texture = RHI::CreateTexture(TextureDesc);
    LOG_INFO("[BOOT] CreateTexture2D %s", Texture ? "ok" : "FAILED");
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
        ProbeCreateBufferAndTexture();

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
