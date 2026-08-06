#include "Core/Mac/Mac.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "CoreApplication/Mac/MacConsoleOutputDevice.h"
#include "CoreApplication/Mac/MacApplication.h"
#include "CoreApplication/Mac/CocoaConsoleWindow.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

FGenericConsoleOutputDevice* FMacConsoleOutputDevice::Create()
{
    return new FMacConsoleOutputDevice();
}

FMacConsoleOutputDevice::FMacConsoleOutputDevice()
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

FMacConsoleOutputDevice::~FMacConsoleOutputDevice()
{
    DestroyConsole();
}

void FMacConsoleOutputDevice::CreateConsole()
{
    if (!WindowHandle)
    {
        FMacThreadManager::Get().MainThreadDispatch(^
        {
            CHECK_COCOA_MAIN_THREAD();
            SCOPED_AUTORELEASE_POOL();

            if (!Font)
            {
                Font = [NSFont fontWithName:@"Courier" size:12.0f];
                [Font retain];
            }
            
            // Init the textcolor (NOTE: This needs to be made before the attributes array is created)
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
            [TextView setSelectable:YES];
            
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
            [WindowHandle makeFirstResponder:TextView];

            if(!GMacApplication)
            {
                do
                {
                    FPlatformApplicationMisc::PumpMessages(true);
                } while(WindowHandle && ![WindowHandle isVisible]);
            }
        }, NSDefaultRunLoopMode, true);
    }
}

void FMacConsoleOutputDevice::DestroyConsole()
{
    if (IsVisible())
    {
        FMacThreadManager::Get().MainThreadDispatch(^
        {
            CHECK_COCOA_MAIN_THREAD();
            SCOPED_AUTORELEASE_POOL();
        
            FPlatformApplicationMisc::PumpMessages(true);
            
            [WindowHandle release];
            DestroyResources();
        }, NSDefaultRunLoopMode, true);
    }
}

void FMacConsoleOutputDevice::DestroyResources()
{
    SCOPED_AUTORELEASE_POOL();
    
    CHECK_COCOA_MAIN_THREAD();
    
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

void FMacConsoleOutputDevice::Show(bool bShow)
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

void FMacConsoleOutputDevice::Log(const String& Message)
{
    SCOPED_LOCK(WindowCS);
    
    if (WindowHandle)
    {
        __block String LocalMessage = Message;

        FMacThreadManager::Get().MainThreadDispatch(^
        {
            CHECK_COCOA_MAIN_THREAD();
            SCOPED_AUTORELEASE_POOL();

            MainThreadAppendStringAndScroll(CreatePrintableString(LocalMessage));
        }, NSDefaultRunLoopMode, false);

        if(!GMacApplication)
        {
            FPlatformApplicationMisc::PumpMessages(true);
        }
    }
}

void FMacConsoleOutputDevice::Log(ELogSeverity Severity, const String& Message)
{
    SCOPED_LOCK(WindowCS);
    
    if (WindowHandle)
    {
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

        // The colour changes bracket the append, so they have to stay inside the same block to keep
        // the message and its colour together
        __block String LocalMessage = Message;

        FMacThreadManager::Get().MainThreadDispatch(^
        {
            CHECK_COCOA_MAIN_THREAD();
            SCOPED_AUTORELEASE_POOL();

            // Set the requested text color
            InternalSetConsoleColor(NewColor);

            MainThreadAppendStringAndScroll(CreatePrintableString(LocalMessage));

            // Return the color the original
            InternalSetConsoleColor(EConsoleColor::White);
        }, NSDefaultRunLoopMode, false);

        if(!GMacApplication)
        {
            FPlatformApplicationMisc::PumpMessages(true);
        }
    }
}

void FMacConsoleOutputDevice::Flush()
{
    SCOPED_LOCK(WindowCS);
    
    if (WindowHandle)
    {
        FMacThreadManager::Get().MainThreadDispatch(^
        {
            CHECK_COCOA_MAIN_THREAD();
            SCOPED_AUTORELEASE_POOL();

            TextView.string = @"";
        }, NSDefaultRunLoopMode, false);

        if(!GMacApplication)
        {
            FPlatformApplicationMisc::PumpMessages(true);
        }
    }
}

void FMacConsoleOutputDevice::SetTitle(const String& InTitle)
{
    SCOPED_LOCK(WindowCS);
    
    if (WindowHandle)
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSString* NewTitle = InTitle.GetNSString();
        [NewTitle retain];
        
        FMacThreadManager::Get().MainThreadDispatch(^
        {
            CHECK_COCOA_MAIN_THREAD();
            SCOPED_AUTORELEASE_POOL();
            
            WindowHandle.title = NewTitle;
            [NewTitle release];
        }, NSDefaultRunLoopMode, true);

        if(!GMacApplication)
        {
            FPlatformApplicationMisc::PumpMessages(true);
        }
    }
}

void FMacConsoleOutputDevice::SetTextColor(EConsoleColor Color)
{
    SCOPED_LOCK(WindowCS);

    // Asynchronous so that a caller holding WindowCS cannot deadlock against a main thread that is
    // itself waiting for WindowCS inside Log. Blocks run in order, so the colour still lands before
    // any message queued after this call.
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        InternalSetConsoleColor(Color);
    }, NSDefaultRunLoopMode, false);
}

void FMacConsoleOutputDevice::InternalSetConsoleColor(EConsoleColor Color)
{
    CHECK_COCOA_MAIN_THREAD();
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

NSAttributedString* FMacConsoleOutputDevice::CreatePrintableString(const String& String)
{
    CHECK_COCOA_MAIN_THREAD();
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
    
    // Create the actual string and return it. Ownership passes to the caller, and
    // MainThreadAppendStringAndScroll is the one that releases it.
    return [[NSAttributedString alloc] initWithString:NativeString attributes:StringAttributes];
}

int32 FMacConsoleOutputDevice::MainThreadGetLineCount() const
{
    CHECK_COCOA_MAIN_THREAD();
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

void FMacConsoleOutputDevice::OnWindowDidClose()
{
    SCOPED_LOCK(WindowCS);
    DestroyResources();
}

void FMacConsoleOutputDevice::MainThreadAppendStringAndScroll(NSAttributedString* AttributedString)
{
    CHECK_COCOA_MAIN_THREAD();
    CHECK(WindowHandle != nil);
    
    SCOPED_AUTORELEASE_POOL();
    
    // TODO: CVar
    const NSUInteger MaxLineCount = 512;
    
    // Only follow the tail when the view is already there
    NSClipView*   ClipView     = ScrollView.contentView;
    const CGFloat DocumentMaxY = NSMaxY([ScrollView.documentView bounds]);
    const bool    bWasAtBottom = (DocumentMaxY - NSMaxY(ClipView.documentVisibleRect)) <= 1.0;
    
    NSRange Selection = [TextView selectedRange];
    
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
        
        // Slide the selection back by whatever was trimmed off the head
        if (Selection.length > 0)
        {
            if (Selection.location >= LineIndex)
            {
                Selection.location -= LineIndex;
            }
            else
            {
                const NSUInteger Removed = LineIndex - Selection.location;
                Selection.location = 0;
                Selection.length   = (Selection.length > Removed) ? (Selection.length - Removed) : 0;
            }
        }
    }
    
    // Add the new String
    [Storage appendAttributedString:AttributedString];
    [Storage endEditing];
    
    if (Selection.length > 0)
    {
        [TextView setSelectedRange:Selection];
    }
    
    // Scroll
    if (bWasAtBottom)
    {
        [TextView scrollToEndOfDocument:TextView];
    }
    
    [AttributedString release];
}
