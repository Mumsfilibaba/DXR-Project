#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FRendererSettingsWidget
{
public:
    FRendererSettingsWidget();
    ~FRendererSettingsWidget();

    void Draw();

private:
    FDelegateHandle ImGuiDelegateHandle;
};
