#pragma once

#if PLATFORM_MACOS
#include "CocoaConsoleWindow.h"

#include "CoreApplication/Interface/PlatformConsoleWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac specific implementation for console window

class COREAPPLICATION_API CMacConsoleWindow final : public CPlatformConsoleWindow
{
public:

    /* Creates a new console */
	static CMacConsoleWindow* Make();
	
	/* Show or hide the console window */
	virtual void Show(bool bShow) override final;

	/* Prints text to the console window, but does not start a new line */
	virtual void Print(const CString& Message) override final;
	
	/* Prints a line to the console window */
	virtual void PrintLine(const CString& Message) override final;

	/* Clear the console window */
	virtual void Clear() override final;

	/* Set the title of the console window */
	virtual void SetTitle(const CString& Title) override final;
	
	/* Set the text-color */
	virtual void SetColor(EConsoleColor Color) override final;
	
	/* Retrieve the number of lines currently written to the consolewindow */
	int32 GetLineCount() const;
	
	/* Called when the console window is closed */
	void OnWindowDidClose();

private:

    CMacConsoleWindow();
    ~CMacConsoleWindow();
	
	// Create the console window
	void CreateConsole();
	
	// Destroy the console window
	void DestroyConsole();

	/* Destroys resources without the window and should only be called on the mainthread */
	void DestroyResources();
	
	// Appends a new string to the textview and scroll to the bottom
	void AppendStringAndScroll(NSString* String);
	
    // The window
    CCocoaConsoleWindow* Window;
	// Textview
	NSTextView*   TextView;
	// Scollable view
	NSScrollView* ScrollView;
	// Colors
	NSDictionary* ConsoleColor;
};

#endif
