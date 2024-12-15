#pragma once
#include "Core/Platform/CriticalSection.h"
#include "CoreApplication/Mac/CocoaConsoleWindow.h"
#include "CoreApplication/Generic/GenericConsoleOutputDevice.h"

/**
 * @class FMacConsoleOutputDevice
 * @brief A macOS-specific implementation of a console output device.
 *
 * FMacConsoleOutputDevice manages a Cocoa-based console window on macOS for displaying
 * log messages, supporting features like text color, background color, scrolling, and more.
 */
class COREAPPLICATION_API FMacConsoleOutputDevice final : public FGenericConsoleOutputDevice
{
public:

    /**
     * @brief Factory function for creating an FMacConsoleOutputDevice instance.
     *
     * This static function typically either returns a new FMacConsoleOutputDevice or nullptr
     * if a console is unsupported. On macOS, it returns an FMacConsoleOutputDevice that uses
     * a Cocoa NSWindow for displaying console messages.
     *
     * @return A pointer to the newly created console output device or nullptr if unsupported.
     */
    static FGenericConsoleOutputDevice* Create();

public:

    FMacConsoleOutputDevice();
    virtual ~FMacConsoleOutputDevice();

public:

    // FGenericConsoleOutputDevice Interface Overrides
    virtual void Show(bool bShow) override final;

    virtual bool IsVisible() const override final { return (WindowHandle != nullptr); }

    virtual void Log(const FString& Message) override final;

    virtual void Log(ELogSeverity Severity, const FString& Message) override final;

    virtual void Flush() override final;

    virtual void SetTitle(const FString& Title) override final;

    virtual void SetTextColor(EConsoleColor Color) override final;

public:

    /**
     * @brief Callback method invoked when the Cocoa console window closes.
     *
     * Typically called by the window's delegate upon user action to close the window.
     * This method can perform cleanup or mark the console as hidden.
     */
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
    NSTextView*              TextView;
    NSScrollView*            ScrollView;
    NSMutableArray*          Attributes;
    NSMutableArray*          AttributeNames;
    NSFont*                  Font;
    NSColor*                 TextColor;
    NSColor*                 BackGroundColor;
    NSDictionary*            StringAttributes;
};
