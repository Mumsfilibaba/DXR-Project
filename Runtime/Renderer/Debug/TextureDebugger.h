#pragma once
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"
#include "ImGuiPlugin/ImGuiPlugin.h"

class FRenderTargetDebugWindow : public IImGuiWidget
{
public:
    FRenderTargetDebugWindow();
    ~FRenderTargetDebugWindow();

    /** @brief - Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Draw() override final;

     /** @brief - Add image for debug drawing */
    void AddTextureForDebugging(const FRHIShaderResourceViewRef& ImageView, const FRHITextureRef& Image, EResourceAccess BeforeState, EResourceAccess AfterState);

    void ClearImages()
    {
        DebugTextures.Clear();
    }

private:
    TArray<FImGuiTexture> DebugTextures;
    int32                 SelectedTextureIndex;
};
