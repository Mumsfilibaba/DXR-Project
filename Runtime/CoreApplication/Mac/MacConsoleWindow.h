#pragma once
#include "CocoaConsoleWindow.h"

#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Generic/GenericConsoleWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacConsoleWindow

class COREAPPLICATION_API FMacConsoleWindow final : public FGenericConsoleWindow
{
private:

    FMacConsoleWindow();
    ~FMacConsoleWindow();

public:

	static FMacConsoleWindow* CreateMacConsole();
	
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericConsoleWindow Interface

    virtual void Show(bool bShow) override final;

    virtual void Print(const String& Message) override final;
    
    virtual void PrintLine(const String& Message) override final;

    virtual void Clear() override final;

    virtual void SetTitle(const String& Title) override final;
    
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
	
    CCocoaConsoleWindow*     WindowHandle;
    mutable FCriticalSection WindowCS;

    NSTextView*              TextView;
	NSScrollView*            ScrollView;
	NSDictionary*            ConsoleColor;
};
