#pragma once
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"
#include "ImGuiPlugin/ImGuiPlugin.h"

class FRenderTargetDebugWindow
{
public:
    FRenderTargetDebugWindow();
    ~FRenderTargetDebugWindow();

    /** @brief - Called from ImGuiPlugin. This is where the ImGui-Commands should be called */
    void Draw();

     /** @brief - Add image for debug drawing */
    void AddTextureForDebugging(const FRHIShaderResourceViewRef& ImageView, const FRHITextureRef& Image, EResourceAccess BeforeState, EResourceAccess AfterState);

    void ClearImages()
    {
        DebugTextures.Clear();
    }

private:
    TArray<FImGuiTexture> DebugTextures;
    int32                 SelectedTextureIndex;
    FDelegateHandle       ImGuiDelegateHandle;
};
