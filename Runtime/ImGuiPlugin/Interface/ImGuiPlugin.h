#pragma once
#include "Core/Modules/ModuleManager.h"
#include "Core/Containers/SharedPtr.h"
#include "RHI/RHIResources.h"

struct ImGuiIO;
struct ImGuiContext;
class FRHICommandList;
class FViewport;

struct IImGuiWidget
{
    virtual ~IImGuiWidget() = default;

    virtual void Draw() = 0;
};

struct FImGuiTexture
{
    FImGuiTexture() = default;

    FImGuiTexture(const FRHITextureRef& InImage, EResourceAccess InBefore, EResourceAccess InAfter)
        : View(MakeSharedRef<FRHIShaderResourceView>(InImage ? InImage->GetShaderResourceView() : nullptr))
        , Texture(InImage)
        , BeforeState(InBefore)
        , AfterState(InAfter)
    {
    }

    FImGuiTexture(const FRHIShaderResourceViewRef& InImageView, const FRHITextureRef& InImage, EResourceAccess InBefore, EResourceAccess InAfter)
        : View(InImageView)
        , Texture(InImage)
        , BeforeState(InBefore)
        , AfterState(InAfter)
    {
    }

    FRHITextureRef            Texture;
    FRHIShaderResourceViewRef View;
    EResourceAccess           BeforeState;
    EResourceAccess           AfterState;

    bool bAllowBlending = false;
    bool bSamplerLinear = false;
};

struct IImguiPlugin : public FModuleInterface
{
    // This ensures that the plugin is actually loaded
    static bool IsEnabled()
    {
        return FModuleManager::Get().IsModuleLoaded("ImGuiPlugin");
    }

    // Use IsEnabled before calling get to ensure that there is a valid interface
    static IImguiPlugin& Get()
    {
        return FModuleManager::Get().GetModuleRef<IImguiPlugin>("ImGuiPlugin");
    }

    virtual ~IImguiPlugin() = default;

    virtual bool InitializeRenderer() = 0;
    virtual void ReleaseRenderer()    = 0;

    virtual void Tick(float Delta) = 0;
    virtual void TickRenderer(FRHICommandList& CommandList) = 0;

    virtual void AddWidget(const TSharedPtr<IImGuiWidget>& InWidget)    = 0;
    virtual void RemoveWidget(const TSharedPtr<IImGuiWidget>& InWidget) = 0;
    
    virtual void SetMainViewport(FViewport* InViewport) = 0;

    virtual ImGuiIO*      GetImGuiIO()      const = 0;
    virtual ImGuiContext* GetImGuiContext() const = 0;
};