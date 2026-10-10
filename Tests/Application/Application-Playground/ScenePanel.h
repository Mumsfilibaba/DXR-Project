#pragma once
#include <Core/Containers/String.h>
#include <Application/Elements/CompoundElement.h>

#include "PlaygroundScene.h"

class FScenePanel final : public FCompoundElement
{
public:
    struct FDesc
    {
        FDesc()
            : Title()
            , Description()
            , Content(nullptr)
        {
        }

        FString                    Title;
        FString                    Description;
        FPlaygroundFonts           Fonts;
        TSharedPtr<FVisualElement> Content;
    };

public:
    static TSharedPtr<FScenePanel> Create(const FDesc& Desc);

public:
    FScenePanel();
    virtual ~FScenePanel();

    /**
     * @brief Initializes the panel with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
};

NODISCARD TSharedPtr<FVisualElement> MakeSceneColumn(const FString& Title, const FPlaygroundFonts& Fonts,
    const TArray<TSharedPtr<FVisualElement>>& Panels);
