#include <Core/Containers/SharedPtr.h>
#include <Core/Misc/ConsoleManager.h>
#include <Application/Application.h>
#include <Application/Console/Console.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Button.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Style/UIStyle.h>

#include "PlaygroundScene.h"
#include "ScenePanel.h"

static TSharedPtr<FConsole> GConsole;

static TAutoConsoleVariable<float> GPlaygroundSpin(
    "Playground.SpinSpeed",
    "How fast the playground pretends to spin, which nothing reads and the console can still find",
    1.0f,
    EConsoleVariableFlags::Default);

TSharedPtr<FConsole> GetPlaygroundConsole()
{
    return GConsole;
}

FPlaygroundScene CreateConsoleScene(const FPlaygroundFonts& Fonts)
{
    FConsole::FDesc ConsoleDesc;
    ConsoleDesc.Font = Fonts.Monospace;

    GConsole = FConsole::Create(ConsoleDesc);

    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FButton::FDesc ButtonDesc;
        ButtonDesc.Text = "Drop the console down";
        ButtonDesc.Font = Fonts.Body;
        ButtonDesc.OnClicked = FOnClicked::CreateLambda([]()
        {
            if (GConsole)
            {
                GConsole->SetIsOpen(true);
                FApplication::Get().SetFocusElement(GConsole->GetInput());
            }
        });

        FScenePanel::FDesc Desc;
        Desc.Title       = "Console";
        Desc.Description = "The existing console, hung over the whole window rather than dropped into this page, because it captures every key while it is open. The tilde key toggles it wherever you are, and this button does the same thing. Type a few letters for the completion list, use the arrow keys to walk it, and set a variable to watch it echo the new value. Playground.SpinSpeed is here to be found.";
        Desc.Fonts       = Fonts;
        Desc.Content     = FButton::Create(ButtonDesc);
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene("Console", MakeSceneColumn("Console", Fonts, Panels));
}
