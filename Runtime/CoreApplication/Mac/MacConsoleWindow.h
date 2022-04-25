#pragma once

#if PLATFORM_MACOS
#include "CocoaConsoleWindow.h"

#include "CoreApplication/Interface/PlatformConsoleWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac specific implementation for console window

class COREAPPLICATION_API CMacConsoleWindow final : public CPlatformConsoleWindow
{
public:

     /** @brief: Creates a new console */
	static CMacConsoleWindow* Make();
	
	 /** @brief: Show or hide the console window */
	virtual void Show(bool bShow) override final;

	 /** @brief: Prints text to the console window, but does not start a new line */
	virtual void Print(const String& Message) override final;
	
	 /** @brief: Prints a line to the console window */
	virtual void PrintLine(const String& Message) override final;

	 /** @brief: Clear the console window */
	virtual void Clear() override final;

	 /** @brief: Set the title of the console window */
	virtual void SetTitle(const String& Title) override final;
	
	 /** @brief: Set the text-color */
	virtual void SetColor(EConsoleColor Color) override final;
	
	 /** @brief: Retrieve the number of lines currently written to the consolewindow */
	int32 GetLineCount() const;
	
	 /** @brief: Called when the console window is closed */
	void OnWindowDidClose();

private:

    CMacConsoleWindow();
    ~CMacConsoleWindow();
	
	// Create the console window
	void CreateConsole();
	
	// Destroy the console window
	void DestroyConsole();

	 /** @brief: Destroys resources without the window and should only be called on the mainthread */
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
