#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "CoreApplication/Mac/CocoaConsoleWindow.h"
#include "CoreApplication/Generic/GenericConsoleOutputDevice.h"

class COREAPPLICATION_API FMacConsoleOutputDevice final : public FGenericConsoleOutputDevice
{
public:
    static FGenericConsoleOutputDevice* Create();

public:
    FMacConsoleOutputDevice();
    virtual ~FMacConsoleOutputDevice();

    // FGenericConsoleOutputDevice Interface Overrides
    virtual void Show(bool bShow) override final;
    virtual bool IsVisible() const override final { return (WindowHandle != nullptr); }
    virtual void Log(const String& Message) override final;
    virtual void Log(ELogSeverity Severity, const String& Message) override final;
    virtual void Flush() override final;
    virtual void SetTitle(const String& Title) override final;
    virtual void SetTextColor(EConsoleColor Color) override final;

    void OnWindowDidClose();

private:
    struct FPendingLine
    {
        String        Text;
        EConsoleColor Color;
    };

    void CreateConsole();
    void DestroyConsole();
    void DestroyResources();

    void EnqueueLine(const String& Message, EConsoleColor Color);

    void MainThreadCreateAttributeCache();
    NSDictionary* MainThreadAttributesForColor(EConsoleColor Color) const;

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
    EConsoleColor            CurrentTextColor;
    bool                     bFlushScheduled;
    int32                    LineCount;
};
