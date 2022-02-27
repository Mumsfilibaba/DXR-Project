#include "VulkanViewport.h"

#include "CoreApplication/Interface/PlatformWindow.h"
#include "Core/Debug/Console/ConsoleManager.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ConsoleVariables

TAutoConsoleVariable<int32> GBackbufferCount("vulkan.BackbufferCount", 3);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanViewport

TSharedRef<CVulkanViewport> CVulkanViewport::CreateViewport(CVulkanDevice* InDevice, CVulkanQueue* InQueue, CPlatformWindow* InWindow, EFormat InFormat, uint32 InWidth, uint32 InHeight)
{
    TSharedRef<CVulkanViewport> NewViewport = dbg_new CVulkanViewport(InDevice, InQueue, InWindow, InFormat, InWidth, InHeight);
    if (NewViewport && NewViewport->Initialize())
    {
        return NewViewport;
    }
    
    return nullptr;
}

CVulkanViewport::CVulkanViewport(CVulkanDevice* InDevice, CVulkanQueue* InQueue, CPlatformWindow* InWindow, EFormat InFormat, uint32 InWidth, uint32 InHeight)
    : CRHIViewport(InFormat, InWidth, InHeight)
    , CVulkanDeviceObject(InDevice)
    , Window(::AddRef(InWindow))
    , Surface(nullptr)
    , SwapChain(nullptr)
    , Queue(::AddRef(InQueue))
    , BackBuffer(nullptr)
    , BackBufferView(nullptr)
    , RenderSemaphores()
{
}

CVulkanViewport::~CVulkanViewport()
{
    RenderSemaphores.Clear();
}

bool CVulkanViewport::Initialize()
{
    Surface = CVulkanSurface::CreateSurface(GetDevice(), GetQueue(), Window->GetPlatformHandle());
    if (!Surface)
    {
        VULKAN_ERROR_ALWAYS("Failed to create surface");
        return false;
    }

    SVulkanSwapChainCreateInfo SwapChainCreateInfo;
    SwapChainCreateInfo.BufferCount   = GBackbufferCount.GetInt();
    SwapChainCreateInfo.Extent.width  = Window->GetWidth();
    SwapChainCreateInfo.Extent.width  = Window->GetHeight();
    SwapChainCreateInfo.ColorSpace    = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    SwapChainCreateInfo.Format        = GetColorFormat();
    SwapChainCreateInfo.bVerticalSync = false;

    SwapChain = CVulkanSwapChain::CreateSwapChain(GetDevice(), GetQueue(), GetSurface(), SwapChainCreateInfo);
    if (!SwapChain)
    {
        VULKAN_ERROR_ALWAYS("Failed to create SwapChain");
        return false;
    }

    BackBuffer     = dbg_new TVulkanTexture<CVulkanTexture2D>(Format, Width, Height, 1, 1, 0, SClearValue());
    BackBufferView = dbg_new CVulkanRenderTargetView();

    for (uint32 Index = 0; Index < SwapChainCreateInfo.BufferCount; ++Index)
    {
        CVulkanSemaphore& RenderSemaphore = RenderSemaphores.Emplace(GetDevice());
        if (!RenderSemaphore.Initialize())
        {
            return false;
        }
    }

    return true;
}

bool CVulkanViewport::Resize(uint32 InWidth, uint32 InHeight)
{
    Width  = InWidth;
    Height = InHeight;
    return true;
}

bool CVulkanViewport::Present(bool bVerticalSync)
{
    
    return true;
}

void CVulkanViewport::SetName(const String& InName)
{
    CRHIObject::SetName(InName);
}

CRHIRenderTargetView* CVulkanViewport::GetRenderTargetView() const
{
    return BackBufferView.Get();
}

CRHITexture2D* CVulkanViewport::GetBackBuffer() const
{
    return BackBuffer.Get();
}

bool CVulkanViewport::IsValid() const
{
    return true;
}
