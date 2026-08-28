#pragma once
#include <Core/Containers/Array.h>
#include <Application/Elements/CompoundElement.h>

#include "PlaygroundScene.h"

class FBorder;
class FNavButton;
class FScrollBox;

class FPlaygroundShell final : public FCompoundElement
{
public:

    /** @brief How wide the sidebar column is, in pixels. */
    static constexpr int32 SidebarWidth = 176;

public:
    static TSharedPtr<FPlaygroundShell> Create(const FPlaygroundFonts& Fonts, const TArray<FPlaygroundScene>& InScenes);

public:
    FPlaygroundShell();
    virtual ~FPlaygroundShell();

    /**
     * @brief Builds the sidebar and the content host, and shows the first scene.
     *
     * @param Fonts    The faces the sidebar draws with.
     * @param InScenes The scenes to offer, in sidebar order.
     */
    void Initialize(const FPlaygroundFonts& Fonts, const TArray<FPlaygroundScene>& InScenes);

    // FVisualElement Interface
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Shows one scene and marks its sidebar entry.
     *
     * @param SceneIndex The scene to show. An out-of-range index is ignored.
     */
    void SelectScene(int32 SceneIndex);

    /** @return The index of the showing scene into the shell's scenes, or -1 before the first selection. */
    NODISCARD FORCEINLINE int32 GetSelectedScene() const
    {
        return SelectedScene;
    }

private:
    TArray<FPlaygroundScene>       Scenes;
    TArray<TSharedPtr<FNavButton>> NavButtons;
    TSharedPtr<FScrollBox>         ContentHost;
    int32                          SelectedScene;
};
