#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/String.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Misc/IOutputDevice.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FEditorOutputLogWidget final : public IOutputDevice
{
public:
    FEditorOutputLogWidget();
    ~FEditorOutputLogWidget();

    // IOutputDevice
    void Log(const FString& Message) override final;
    void Log(ELogSeverity Severity, const FString& Message) override final;

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
    struct FLogMessage
    {
        FString      Message;
        ELogSeverity Severity = ELogSeverity::Info;
    };

    void DrawFilterBar();
    void DrawLogList();

    bool                    bVisible;
    bool                    bAutoScroll;
    bool                    bScrollToBottom;
    bool                    bFilterInfo;
    bool                    bFilterWarning;
    bool                    bFilterError;

    TStaticArray<CHAR, 256> SearchFilterBuffer;
    TArray<FLogMessage>     Messages;
    FCriticalSection        MessagesCS;
    FDelegateHandle         ImGuiDelegateHandle;
};
