#pragma once
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

public:

    // FGenericConsoleOutputDevice Interface
    virtual void Show(bool bShow) override final;

    virtual bool IsVisible() const override final { return (WindowHandle != nullptr); }
    
    virtual void Log(const FString& Message) override final;
    
    virtual void Log(ELogSeverity Severity, const FString& Message) override final;
    
    virtual void Flush() override final;
    
    virtual void SetTitle(const FString& Title) override final;
    
    virtual void SetTextColor(EConsoleColor Color) override final;

public:
    void OnWindowDidClose();

private:
    void CreateConsole();
    void DestroyConsole();
    void DestroyResources();
    NSAttributedString* CreatePrintableString(const FString& String);
    void InternalSetConsoleColor(EConsoleColor Color);
    void MainThreadAppendStringAndScroll(NSAttributedString* AttributedString);
    int32 MainThreadGetLineCount() const;
        
    FCocoaConsoleWindow*     WindowHandle;
    mutable FCriticalSection WindowCS;

    NSTextView*     TextView;
    NSScrollView*   ScrollView;
    NSMutableArray* Attributes;
    NSMutableArray* AttributeNames;
    NSFont*         Font;
    NSColor*        TextColor;
    NSColor*        BackGroundColor;
    NSDictionary*   StringAttributes;
};
