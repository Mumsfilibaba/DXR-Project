#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/String.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Misc/IOutputDevice.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FEditorOutputLogWidget final : public IOutputDevice
{
    struct FLogMessage
    {
        FString      Message;
        ELogSeverity Severity = ELogSeverity::Info;
    };

public:
    FEditorOutputLogWidget();
    virtual ~FEditorOutputLogWidget() override;

    // IOutputDevice
    virtual void Log(const FString& Message) override final;
    virtual void Log(ELogSeverity Severity, const FString& Message) override final;

    void Draw();

    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

    bool IsVisible() const
    {
        return bVisible;
    }

private:
    void DrawFilterBar();
    void DrawLogListRichText();

private:
    TStaticArray<CHAR, 256> SearchFilterBuffer;
    TArray<FLogMessage>     Messages;
    FCriticalSection        MessagesCS;
    RichTextViewContext     RichTextCtx;
    FDelegateHandle         ImGuiDelegateHandle;

    bool                    bVisible;
    bool                    bAutoScroll;
    bool                    bScrollToBottom;
    bool                    bFilterInfo;
    bool                    bFilterWarning;
    bool                    bFilterError;
};
