#pragma once

#if PLATFORM_MACOS
#include "CocoaConsoleWindow.h"

#include "CoreApplication/Interface/PlatformConsoleWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacConsoleWindow - Mac specific implementation for console window

class COREAPPLICATION_API CMacConsoleWindow final : public CPlatformConsoleWindow
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CPlatformConsoleWindow Interface
    
    static CMacConsoleWindow* CreateWindow();
    
    virtual void Show(bool bShow) override final;

    virtual void Print(const String& Message) override final;
    virtual void PrintLine(const String& Message) override final;

    virtual void Clear() override final;

    virtual void SetTitle(const String& Title) override final;
    virtual void SetColor(EConsoleColor Color) override final;
    
public:
    
    /**
     * Retrieve the number of lines currently written to the consolewindow
     *
     * @return: Returns the number of lines currently written to the console window
     */
    int32 GetLineCount() const;
    
    /**
     * Called when the console window is closed
     */
    void OnWindowDidClose();

private:

    CMacConsoleWindow();
    ~CMacConsoleWindow();
    
    void CreateConsole();
    void DestroyConsole();
    void DestroyResources();
    
    void AppendStringAndScroll(NSString* String);
    
    CCocoaConsoleWindow* Window;
    
    NSTextView* TextView;
    
    NSScrollView* ScrollView;
    NSDictionary* ConsoleColor;
};

#endif
