#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "CoreApplication/Mac/CocoaConsoleWindow.h"
#include "CoreApplication/PlatformInterface/IPlatformConsoleWindow.h"

class COREAPPLICATION_API FMacConsoleWindow final : public IPlatformConsoleWindow
{
public:
    static IPlatformConsoleWindow* Create();

public:
    FMacConsoleWindow();
    virtual ~FMacConsoleWindow();

    // IPlatformConsoleWindow Interface Overrides
    virtual void Show(bool bShow) override final;
    virtual bool IsVisible() const override final { return (WindowHandle != nullptr); }

    virtual void SetTitle(const String& Title) override final;
    virtual void SetTextColor(EConsoleTextColor Color) override final;

    // IOutputDevice Interface Overrides
    virtual void Log(const String& Message) override final;
    virtual void Log(ELogSeverity Severity, const String& Message) override final;
    virtual void Flush() override final;

    void OnWindowDidClose();

private:
    struct FPendingLine
    {
        String            Text;
        EConsoleTextColor Color;
    };

    void CreateConsole();
    void DestroyConsole();
    void DestroyResources();

    void EnqueueLine(const String& Message, EConsoleTextColor Color);

    void MainThreadCreateAttributeCache();
    NSDictionary* MainThreadAttributesForColor(EConsoleTextColor Color) const;

    void MainThreadFlushPendingLines();
    NSUInteger MainThreadTrimToMaxLines(NSTextStorage* Storage);

    FCocoaConsoleWindow*     WindowHandle;
    NSTextView*              TextView;
    NSScrollView*            ScrollView;
    NSFont*                  Font;
    NSColor*                 BackGroundColor;
    NSArray*                 AttributeCache;
    mutable FCriticalSection WindowCS;
    mutable FCriticalSection PendingCS;
    String                   Title;
    TArray<FPendingLine>     PendingLines;
    EConsoleTextColor        CurrentTextColor;
    bool                     bFlushScheduled;
    int32                    LineCount;
};
