#pragma once
#include <Core/Containers/Array.h>
#include <Core/Containers/SharedPtr.h>
#include <Core/Containers/String.h>

class FConsole;
class FVisualElement;
struct IFontFace;

struct FPlaygroundFonts
{
    FPlaygroundFonts()
        : Body(nullptr)
        , Heading(nullptr)
        , Monospace(nullptr)
    {
    }

    TSharedPtr<IFontFace> Body;
    TSharedPtr<IFontFace> Heading;
    TSharedPtr<IFontFace> Monospace;
};

struct FPlaygroundScene
{
    FPlaygroundScene()
        : Name()
        , Content(nullptr)
    {
    }

    FPlaygroundScene(const String& InName, const TSharedPtr<FVisualElement>& InContent)
        : Name(InName)
        , Content(InContent)
    {
    }

    String                     Name;
    TSharedPtr<FVisualElement> Content;
};

void CreatePlaygroundScenes(const FPlaygroundFonts& Fonts, TArray<FPlaygroundScene>& OutScenes);

FPlaygroundScene CreateFoundationsScene(const FPlaygroundFonts& Fonts);
FPlaygroundScene CreateVectorsScene(const FPlaygroundFonts& Fonts);
FPlaygroundScene CreateControlsScene(const FPlaygroundFonts& Fonts);
FPlaygroundScene CreateMenusScene(const FPlaygroundFonts& Fonts);
FPlaygroundScene CreateWindowsScene(const FPlaygroundFonts& Fonts);
FPlaygroundScene CreateDockingScene(const FPlaygroundFonts& Fonts);
FPlaygroundScene CreateOutputLogScene(const FPlaygroundFonts& Fonts);
FPlaygroundScene CreateToolBarScene(const FPlaygroundFonts& Fonts);
FPlaygroundScene CreateNodeGraphScene(const FPlaygroundFonts& Fonts);
FPlaygroundScene CreateGizmoScene(const FPlaygroundFonts& Fonts);
FPlaygroundScene CreateEditorElementsScene(const FPlaygroundFonts& Fonts);
FPlaygroundScene CreateConsoleScene(const FPlaygroundFonts& Fonts);

NODISCARD TSharedPtr<FConsole> GetPlaygroundConsole();
FPlaygroundScene CreateTextScene(const FPlaygroundFonts& Fonts);
