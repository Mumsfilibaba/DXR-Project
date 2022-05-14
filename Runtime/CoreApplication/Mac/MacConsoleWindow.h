#pragma once
#include "CocoaConsoleWindow.h"

#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Generic/GenericConsoleWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacConsoleWindow

class COREAPPLICATION_API CMacConsoleWindow final : public CGenericConsoleWindow
{
private:

    CMacConsoleWindow();
    ~CMacConsoleWindow();

public:

	static CMacConsoleWindow* CreateMacConsole();
	
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericConsoleWindow Interface

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
    mutable CCriticalSection WindowCS;

    NSTextView*              TextView;
	NSScrollView*            ScrollView;
	NSDictionary*            ConsoleColor;
};
