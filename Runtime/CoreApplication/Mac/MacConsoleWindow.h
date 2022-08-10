#pragma once
#include "CocoaConsoleWindow.h"

#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Generic/GenericConsoleWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacConsoleWindow

class COREAPPLICATION_API FMacConsoleWindow final 
    : public FGenericConsoleWindow
{
public:
    FMacConsoleWindow();
    ~FMacConsoleWindow();

    virtual void Show(bool bShow) override final;

    virtual void Print(const FString& Message) override final;
    virtual void PrintLine(const FString& Message) override final;

    virtual void Clear() override final;

    virtual void SetTitle(const FString& Title) override final;
    
    virtual void SetColor(EConsoleColor Color) override final;
    
    virtual bool IsVisible() const override final;

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
