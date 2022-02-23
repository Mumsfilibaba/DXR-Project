#include "VulkanViewport.h"

#include "Core/Debug/Console/ConsoleManager.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ConsoleVariables

TAutoConsoleVariable<int32> GBackbufferCount("vulkan.BackbufferCount", 3);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanViewport

TSharedRef<CVulkanViewport> CVulkanViewport::CreateViewport(CVulkanDevice* InDevice, CVulkanCommandQueue* InQueue, CPlatformWindow* InWindow, EFormat InFormat, uint32 InWidth, uint32 InHeight)
{
    TSharedRef<CVulkanViewport> NewViewport = dbg_new CVulkanViewport(InDevice, InQueue, InWindow, InFormat, InWidth, InHeight);
    if (NewViewport && NewViewport->Initialize())
    {
        return NewViewport;
    }
    
    return nullptr;
}

CVulkanViewport::CVulkanViewport(CVulkanDevice* InDevice, CVulkanCommandQueue* InQueue, CPlatformWindow* InWindow, EFormat InFormat, uint32 InWidth, uint32 InHeight)
    : CRHIViewport(InFormat, InWidth, InHeight)
    , CVulkanDeviceObject(InDevice)
    , Window(::AddRef(InWindow))
    , Surface(nullptr)
    , Queue(::AddRef(InQueue))
    , BackBuffers()
    , BackBufferViews()
    , ImageSemaphores()
    , RenderSemaphores()
{
}

CVulkanViewport::~CVulkanViewport()
{
    ImageSemaphores.Clear();
    RenderSemaphores.Clear();
}

bool CVulkanViewport::Initialize()
{
    Surface = CVulkanSurface::CreateSurface(GetDevice(), Window.Get());
    if (!Surface)
    {
        VULKAN_ERROR_ALWAYS("Failed to create surface");
        return false;
    }

    const uint32 NumBackBuffers = GBackbufferCount.GetInt();

    for (uint32 Index = 0; Index < NumBackBuffers; ++Index)
    {
        CVulkanSemaphore& ImageSemaphore = ImageSemaphores.Emplace(GetDevice());
        if (!ImageSemaphore.Initialize())
        {
            return false;
        }

        CVulkanSemaphore& RenderSemaphore = RenderSemaphores.Emplace(GetDevice());
        if (!RenderSemaphore.Initialize())
        {
            return false;
        }
		
		BackBuffers.Emplace(dbg_new TVulkanTexture<CVulkanTexture2D>(Format, Width, Height, 1, 1, 0, SClearValue()));
		BackBufferViews.Emplace(dbg_new CVulkanRenderTargetView());
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
    return BackBufferViews.LastElement().Get();
}

CRHITexture2D* CVulkanViewport::GetBackBuffer() const
{
    return BackBuffers.LastElement().Get();
}

bool CVulkanViewport::IsValid() const
{
    return true;
}
