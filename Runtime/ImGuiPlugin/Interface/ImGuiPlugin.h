#pragma once
#include "Core/Modules/ModuleManager.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Delegates/MulticastDelegate.h"
#include "RHI/RHIResources.h"

struct ImGuiIO;
struct ImGuiContext;
class FRHICommandList;
class FViewportWidget;

DECLARE_MULTICAST_DELEGATE(FImGuiDrawMulticastDelegate);
typedef FImGuiDrawMulticastDelegate::FDelegate FImGuiDelegate;

struct FImGuiTexture
{
    FImGuiTexture() = default;

    FImGuiTexture(const FRHITextureRef& InImage)
        : ShaderResourceView(MakeSharedRef<FRHIShaderResourceView>(InImage ? InImage->GetShaderResourceView() : nullptr))
        , bEnableBlending(false)
        , bEnableLinearSampler(false)
    {
    }

    FImGuiTexture(const FRHIShaderResourceViewRef& InImageView)
        : ShaderResourceView(InImageView)
        , bEnableBlending(false)
        , bEnableLinearSampler(false)
    {
    }

    FRHITexture* GetTexture() const
    {
        return ShaderResourceView ? static_cast<FRHITexture*>(ShaderResourceView->GetResource()) : nullptr;
    }

    FRHIShaderResourceViewRef ShaderResourceView   = nullptr;
    bool                      bEnableBlending      = false;
    bool                      bEnableLinearSampler = false;
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

    virtual bool InitializeRHI() = 0;
    virtual void ReleaseRHI() = 0;

    virtual bool UpdateFontAtlas() = 0;

    virtual void NewFrame(float DeltaTime) = 0;
    virtual void Tick(float DeltaTime) = 0;
    virtual void Draw(FRHICommandList& CommandList) = 0;

    virtual FDelegateHandle AddBeginFrameDelegate(const FImGuiDelegate& Delegate) = 0;
    virtual FDelegateHandle AddDrawDelegate(const FImGuiDelegate& Delegate) = 0;
    virtual FDelegateHandle AddEndFrameDelegate(const FImGuiDelegate& Delegate) = 0;
    virtual void RemoveBeginFrameDelegate(FDelegateHandle DelegateHandle) = 0;
    virtual void RemoveDrawDelegate(FDelegateHandle DelegateHandle) = 0;
    virtual void RemoveEndFrameDelegate(FDelegateHandle DelegateHandle) = 0;

    virtual void SetMainViewport(const TSharedPtr<FViewportWidget>& InViewport) = 0;

    virtual ImGuiIO* GetImGuiIO() const = 0;
    virtual ImGuiContext* GetImGuiContext() const = 0;
};
