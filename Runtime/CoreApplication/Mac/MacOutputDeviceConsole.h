#pragma once
#include "CocoaConsoleWindow.h"

#include "Core/Platform/CriticalSection.h"
#include "Core/Misc/OutputDeviceConsole.h"

#include "CoreApplication/CoreApplication.h"

class COREAPPLICATION_API FMacOutputDeviceConsole final 
    : public FOutputDeviceConsole
{
public:
    FMacOutputDeviceConsole();
    ~FMacOutputDeviceConsole();

    virtual void Show(bool bShow) override final;

    virtual bool IsVisible() const override final { return (WindowHandle != nullptr); }

    virtual void Log(const FString& Message) override final;
    
    virtual void Log(ELogSeverity Severity, const FString& Message) override final;

    virtual void Flush() override final;

    virtual void SetTitle(const FString& Title) override final;

    virtual void SetTextColor(EConsoleColor Color) override final;

public:
    int32 GetLineCount() const;
    
    void OnWindowDidClose();
	
private:
	void CreateConsole();
	
	void DestroyConsole();
	void DestroyResources();
	
	void AppendStringAndScroll(NSString* String);
	
    FCocoaConsoleWindow*     WindowHandle;
    mutable FCriticalSection WindowCS;

    NSTextView*              TextView;
	NSScrollView*            ScrollView;
	NSDictionary*            ConsoleColor;
};
