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
        String       Message;
        ELogSeverity Severity = ELogSeverity::Info;
    };

public:
    FEditorOutputLogWidget();
    virtual ~FEditorOutputLogWidget() override;

    // IOutputDevice
    virtual void Log(const String& Message) override final;
    virtual void Log(ELogSeverity Severity, const String& Message) override final;

    void Draw();

    bool IsVisible() const
    {
        return bVisible;
    }
    
    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

private:
    void  DrawFilterBar();
    void  DrawLogListRichText();
    void  RebuildRichTextIfDirty();
    void  AppendMessageLine(const FLogMessage& Message);
    bool  IsSeverityVisible(ELogSeverity Severity) const;
    ImU32 GetSeverityColor(ELogSeverity Severity) const;

    TStaticArray<CHAR, 256> SearchFilterBuffer;
    TArray<FLogMessage>     Messages;
    FCriticalSection        MessagesCS;
    FRichTextViewContext    RichTextCtx;
    FDelegateHandle         ImGuiDelegateHandle;
    uint64                  TotalMessagesAdded;
    uint64                  TotalMessagesRemoved;
    TStaticArray<CHAR, 256> BuiltSearchFilter;
    uint64                  BuiltMessagesAdded;
    uint64                  BuiltMessagesRemoved;
    bool                    bBuiltFilterInfo    : 1;
    bool                    bBuiltFilterWarning : 1;
    bool                    bBuiltFilterError   : 1;
    bool                    bAutoScroll         : 1;
    bool                    bFocusSearchField   : 1;
    bool                    bFilterInfo         : 1;
    bool                    bFilterWarning      : 1;
    bool                    bFilterError        : 1;
    bool                    bVisible;
};
