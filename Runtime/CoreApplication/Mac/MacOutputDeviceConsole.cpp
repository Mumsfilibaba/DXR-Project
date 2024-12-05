#include "MacOutputDeviceConsole.h"
#include "MacApplication.h"
#include "CocoaConsoleWindow.h"
#include "Core/Mac/Mac.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Mac/MacRunLoop.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

FMacOutputDeviceConsole::FMacOutputDeviceConsole()
    : WindowHandle(nullptr)
    , TextView(nullptr)
    , ScrollView(nullptr)
    , Attributes(nullptr)
    , AttributeNames(nullptr)
    , Font(nullptr)
    , TextColor(nullptr)
    , BackGroundColor(nullptr)
    , StringAttributes(nullptr)
{
}

FMacOutputDeviceConsole::~FMacOutputDeviceConsole()
{
    DestroyConsole();
}

void FMacOutputDeviceConsole::CreateConsole()
{
    if (!WindowHandle)
    {
        SCOPED_AUTORELEASE_POOL();
        
        // Create the font
        if (!Font)
        {
            Font = [NSFont fontWithName:@"Courier" size:12.0f];
            [Font retain];
        }
        
        // Init the textcolor (Note: This needs to be made before the attributes array is created)
        InternalSetConsoleColor(EConsoleColor::White);
        
        // Init the backgroundcolor
        if (!BackGroundColor)
        {
            BackGroundColor = [NSColor colorWithSRGBRed:0.15f green:0.15f blue:0.15f alpha:1.0f];
            [BackGroundColor retain];
        }
        
        // Init the attributes and names used to create an attributed string
        if (!Attributes)
        {
            Attributes = [NSMutableArray new];
            [Attributes addObject:TextColor];
            [Attributes addObject:BackGroundColor];
            [Attributes addObject:Font];
            [Attributes retain];
        }
        
        if (!AttributeNames)
        {
            AttributeNames = [@[NSForegroundColorAttributeName, NSBackgroundColorAttributeName, NSFontAttributeName] mutableCopy];
            [AttributeNames retain];
        }
        
        // Create the window
        ExecuteOnMainThread(^
        {
            SCOPED_AUTORELEASE_POOL();
            
            const NSUInteger StyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
            
            // TODO: Control with console vars?
            const CGFloat Width  = 800.0f;
            const CGFloat Height = 480.0f;
            
            NSRect ContentRect = NSMakeRect(0.0f, 0.0f, Width, Height);
            
            WindowHandle = [[FCocoaConsoleWindow alloc] init:this ContentRect:ContentRect StyleMask:StyleMask Backing:NSBackingStoreBuffered Defer:NO];

            NSRect ContentFrame = WindowHandle.contentView.frame;
            ScrollView            = [[NSScrollView alloc] initWithFrame:ContentFrame];
            ScrollView.borderType = NSNoBorder;
            
            [ScrollView setHasVerticalScroller:YES];
            [ScrollView setHasHorizontalScroller:NO];
            
            ScrollView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
            ScrollView.backgroundColor  = BackGroundColor;
            
            NSSize ContentSize = [ScrollView contentSize];
            TextView = [[NSTextView alloc] initWithFrame:NSMakeRect(0.0, 0.0, ContentSize.width, ContentSize.height)];
            [TextView setEditable:NO];
            
            TextView.minSize = NSMakeSize(0.0f, ContentSize.height);
            TextView.maxSize = NSMakeSize(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
            
            [TextView setVerticallyResizable:YES];
            [TextView setHorizontallyResizable:NO];

            TextView.autoresizingMask = NSViewWidthSizable;
            TextView.backgroundColor  = BackGroundColor;
            
            NSTextContainer* Container = TextView.textContainer;
            Container.containerSize = NSMakeSize(ContentSize.width, TNumericLimits<float>::Max());
            [Container setWidthTracksTextView:YES];
            
            ScrollView.documentView = TextView;
            
            NSWindowCollectionBehavior Behavior =
                NSWindowCollectionBehaviorFullScreenAuxiliary |
                NSWindowCollectionBehaviorDefault |
                NSWindowCollectionBehaviorManaged |
                NSWindowCollectionBehaviorParticipatesInCycle;
            
            WindowHandle.collectionBehavior = Behavior;
            
            NSString* Title = @"Output Console";
            [NSApp addWindowsItem:WindowHandle title:Title filename:NO];
            
            WindowHandle.title                 = Title;
            WindowHandle.contentView           = ScrollView;
            WindowHandle.initialFirstResponder = TextView;
            WindowHandle.backgroundColor       = BackGroundColor;
            
            [WindowHandle setOpaque:YES];
            [WindowHandle makeKeyAndOrderFront:WindowHandle];

            if(!MacApplication)
            {
                do
                {
                    FPlatformApplicationMisc::PumpMessages(true);
                } while(WindowHandle && ![WindowHandle isVisible]);
            }
        }, NSDefaultRunLoopMode, true);
    }
}

void FMacOutputDeviceConsole::DestroyConsole()
{
    if (IsVisible())
    {
        ExecuteOnMainThread(^
        {
            SCOPED_AUTORELEASE_POOL();
        
            FPlatformApplicationMisc::PumpMessages(true);
            
            [WindowHandle release];
            DestroyResources();
        }, NSDefaultRunLoopMode, true);
    }
}

void FMacOutputDeviceConsole::DestroyResources()
{
    SCOPED_AUTORELEASE_POOL();
    
    CHECK(FPlatformThreadMisc::IsMainThread());
    
    [TextView release];
    [ScrollView release];
    [Attributes release];
    [AttributeNames release];
    [Font release];
    
    if (TextColor)
    {
        [TextColor release];
        TextColor = nullptr;
    }
    
    [BackGroundColor release];

    if (StringAttributes)
    {
        [StringAttributes release];
        StringAttributes = nullptr;
    }
}

void FMacOutputDeviceConsole::Show(bool bShow)
{
    SCOPED_LOCK(WindowCS);
    
    if (IsVisible() != bShow)
    {
        if (bShow)
        {
            CreateConsole();
        }
        else
        {
            DestroyConsole();
        }
    }
}

void FMacOutputDeviceConsole::Log(const FString& Message)
{
    SCOPED_LOCK(WindowCS);
    
    if (WindowHandle)
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSAttributedString* AttributedString = CreatePrintableString(Message);
        [AttributedString retain];
        
        ExecuteOnMainThread(^
        {
            SCOPED_AUTORELEASE_POOL();

            MainThreadAppendStringAndScroll(AttributedString);
            [AttributedString release];
        }, NSDefaultRunLoopMode, false);

        if(!MacApplication)
        {
            FPlatformApplicationMisc::PumpMessages(true);
        }
    }
}

void FMacOutputDeviceConsole::Log(ELogSeverity Severity, const FString& Message)
{
    SCOPED_LOCK(WindowCS);
    
    if (WindowHandle)
    {
        SCOPED_AUTORELEASE_POOL();
        
        EConsoleColor NewColor;
        if (Severity == ELogSeverity::Info)
        {
            NewColor = EConsoleColor::Green;
        }
        else if (Severity == ELogSeverity::Warning)
        {
            NewColor = EConsoleColor::Yellow;
        }
        else if (Severity == ELogSeverity::Error)
        {
            NewColor = EConsoleColor::Red;
        }
        else
        {
            NewColor = EConsoleColor::White;
        }
        
        // Set the requested text color
        InternalSetConsoleColor(NewColor);

        NSAttributedString* AttributedString = CreatePrintableString(Message);
        [AttributedString retain];
        
        ExecuteOnMainThread(^
        {
            SCOPED_AUTORELEASE_POOL();

            MainThreadAppendStringAndScroll(AttributedString);
            [AttributedString release];
        }, NSDefaultRunLoopMode, false);

        // Return the color the original
        InternalSetConsoleColor(EConsoleColor::White);

        if(!MacApplication)
        {
            FPlatformApplicationMisc::PumpMessages(true);
        }
    }
}

void FMacOutputDeviceConsole::Flush()
{
    SCOPED_LOCK(WindowCS);
    
    if (WindowHandle)
    {
        ExecuteOnMainThread(^
        {
            SCOPED_AUTORELEASE_POOL();
            TextView.string = @"";
        }, NSDefaultRunLoopMode, false);

        if(!MacApplication)
        {
            FPlatformApplicationMisc::PumpMessages(true);
        }
    }
}

void FMacOutputDeviceConsole::SetTitle(const FString& InTitle)
{
    SCOPED_LOCK(WindowCS);
    
    if (WindowHandle)
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSString* NewTitle = InTitle.GetNSString();
        [NewTitle retain];
        
        ExecuteOnMainThread(^
        {
            SCOPED_AUTORELEASE_POOL();
            
            WindowHandle.title = NewTitle;
            [NewTitle release];
        }, NSDefaultRunLoopMode, true);

        if(!MacApplication)
        {
            FPlatformApplicationMisc::PumpMessages(true);
        }
    }
}

void FMacOutputDeviceConsole::SetTextColor(EConsoleColor Color)
{
    SCOPED_LOCK(WindowCS);
    InternalSetConsoleColor(Color);
}

void FMacOutputDeviceConsole::InternalSetConsoleColor(EConsoleColor Color)
{
    SCOPED_AUTORELEASE_POOL();
            
    if (TextColor)
    {
        [TextColor release];
    }
    
    // Add foreground Color
    if (Color == EConsoleColor::White)
    {
        TextColor = [NSColor colorWithSRGBRed:0.85f green:0.85f blue:0.85f alpha:1.0f];
    }
    else if (Color == EConsoleColor::Red)
    {
        TextColor = [NSColor colorWithSRGBRed:0.85f green:0.0f blue:0.0f alpha:1.0f];
    }
    else if (Color == EConsoleColor::Green)
    {
        TextColor = [NSColor colorWithSRGBRed:0.0f green:0.85f blue:0.0f alpha:1.0f];
    }
    else if (Color == EConsoleColor::Yellow)
    {
        TextColor = [NSColor colorWithSRGBRed:0.85f green:0.85f blue:0.0f alpha:1.0f];
    }
    
    [TextColor retain];
}

NSAttributedString* FMacOutputDeviceConsole::CreatePrintableString(const FString& String)
{
    SCOPED_AUTORELEASE_POOL();

    NSString* NativeString = [NSString stringWithFormat:@"%s\n", *String];
    
    // Set the textcolor
    Attributes[0] = TextColor;
    
    // Create a dictionary which can be used to create the string
    if (StringAttributes)
    {
        [StringAttributes release];
    }
    
    StringAttributes = [[NSDictionary alloc] initWithObjects:Attributes forKeys:AttributeNames];
    
    // Create the actual string and return it
    NSAttributedString* AttributedString = [[NSAttributedString alloc] initWithString:NativeString attributes:StringAttributes];
    [AttributedString retain];
    return AttributedString;
}

int32 FMacOutputDeviceConsole::MainThreadGetLineCount() const
{
    CHECK(WindowHandle != nil);
    
    NSString*  String        = TextView.string;
    NSUInteger NumberOfLines = 0;
    NSUInteger StringLength  = String.length;
    for (NSUInteger LineIndex = 0; LineIndex < StringLength; NumberOfLines++)
    {
        LineIndex = NSMaxRange([String lineRangeForRange:NSMakeRange(LineIndex, 0)]);
    }
        
    return static_cast<int32>(NumberOfLines);
}

void FMacOutputDeviceConsole::OnWindowDidClose()
{
    SCOPED_LOCK(WindowCS);
    DestroyResources();
}

void FMacOutputDeviceConsole::MainThreadAppendStringAndScroll(NSAttributedString* AttributedString)
{
    CHECK(WindowHandle != nil);
    
    SCOPED_AUTORELEASE_POOL();
    
    // TODO: CVar
    const NSUInteger MaxLineCount = 512;
    
    NSTextStorage* Storage = TextView.textStorage;
    [Storage beginEditing];
    
    // Remove lines
    NSUInteger LineCount  = MainThreadGetLineCount();
    NSString*  TextString = TextView.string;
    if (LineCount >= MaxLineCount)
    {
        NSUInteger NumberOfLines = 0;
        NSUInteger StringLength  = TextString.length;

        NSUInteger LineIndex;
        for (LineIndex = 0; LineIndex < StringLength; NumberOfLines++)
        {
            LineIndex = NSMaxRange([TextString lineRangeForRange:NSMakeRange(LineIndex, 0)]);
            if (NumberOfLines >= 1)
            {
                break;
            }
        }
        
        NSRange Range = NSMakeRange(0, LineIndex);
        [Storage deleteCharactersInRange:Range];
    }
    
    // Add the new String
    [Storage appendAttributedString:AttributedString];
    [Storage endEditing];
    
    // Scroll
    [TextView scrollToEndOfDocument:TextView];
    
    [AttributedString release];
}
