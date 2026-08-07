#include "Core/Mac/Mac.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "CoreApplication/Mac/MacConsoleOutputDevice.h"
#include "CoreApplication/Mac/MacApplication.h"
#include "CoreApplication/Mac/CocoaConsoleWindow.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

static TAutoConsoleVariable<int32> CVarMaxConsoleLineCount(
    "Mac.OutputConsole.MaxLineCount",
    "Maximum number of lines kept in the output console window before the oldest are trimmed",
    1024,
    64,
    65536,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarConsoleWindowWidth(
    "Mac.OutputConsole.Width",
    "Initial width in points of the output console window, the window stays resizable afterwards",
    800,
    240,
    3840,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarConsoleWindowHeight(
    "Mac.OutputConsole.Height",
    "Initial height in points of the output console window, the window stays resizable afterwards",
    480,
    160,
    2160,
    EConsoleVariableFlags::Default);

static constexpr NSUInteger GNumConsoleColors = 4;

template<typename ObjectType>
static void ReleaseAndClear(ObjectType*& Object)
{
    [Object release];
    Object = nullptr;
}

static EConsoleColor SeverityToColor(ELogSeverity Severity)
{
    if (Severity == ELogSeverity::Info)
    {
        return EConsoleColor::Green;
    }
    else if (Severity == ELogSeverity::Warning)
    {
        return EConsoleColor::Yellow;
    }
    else if (Severity == ELogSeverity::Error)
    {
        return EConsoleColor::Red;
    }

    return EConsoleColor::White;
}

static NSColor* CreateColorForConsoleColor(EConsoleColor Color)
{
    if (Color == EConsoleColor::Red)
    {
        return [NSColor colorWithSRGBRed:0.85f green:0.0f blue:0.0f alpha:1.0f];
    }
    else if (Color == EConsoleColor::Green)
    {
        return [NSColor colorWithSRGBRed:0.0f green:0.85f blue:0.0f alpha:1.0f];
    }
    else if (Color == EConsoleColor::Yellow)
    {
        return [NSColor colorWithSRGBRed:0.85f green:0.85f blue:0.0f alpha:1.0f];
    }

    return [NSColor colorWithSRGBRed:0.85f green:0.85f blue:0.85f alpha:1.0f];
}

FGenericConsoleOutputDevice* FMacConsoleOutputDevice::Create()
{
    return new FMacConsoleOutputDevice();
}

FMacConsoleOutputDevice::FMacConsoleOutputDevice()
    : WindowHandle(nullptr)
    , TextView(nullptr)
    , ScrollView(nullptr)
    , Font(nullptr)
    , BackGroundColor(nullptr)
    , AttributeCache(nullptr)
    , CurrentTextColor(EConsoleColor::White)
    , bFlushScheduled(false)
    , LineCount(0)
{
}

FMacConsoleOutputDevice::~FMacConsoleOutputDevice()
{
    DestroyConsole();
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
    EnqueueLine(Message, CurrentTextColor);
}

void FMacConsoleOutputDevice::Log(ELogSeverity Severity, const String& Message)
{
    EnqueueLine(Message, SeverityToColor(Severity));
}

void FMacConsoleOutputDevice::Flush()
{
    {
        SCOPED_LOCK(PendingCS);
        PendingLines.Clear();

        if (!WindowHandle)
        {
            return;
        }
    }

    FMacThreadManager::Get().MainThreadDispatch(^
    {
        CHECK_COCOA_MAIN_THREAD();
        SCOPED_AUTORELEASE_POOL();

        if (TextView)
        {
            TextView.string = @"";
        }

        LineCount = 0;
    }, NSDefaultRunLoopMode, false);
}

void FMacConsoleOutputDevice::SetTitle(const String& InTitle)
{
    SCOPED_AUTORELEASE_POOL();

    // Cached so a title set before the window exists is not lost, since CreateConsole reads it back
    {
        SCOPED_LOCK(PendingCS);
        Title = InTitle;
    }

    NSString* NewTitle = [InTitle.GetNSString() retain];

    FMacThreadManager::Get().MainThreadDispatch(^
    {
        CHECK_COCOA_MAIN_THREAD();

        if (WindowHandle)
        {
            WindowHandle.title = NewTitle;
        }

        [NewTitle release];
    }, NSDefaultRunLoopMode, false);
}

void FMacConsoleOutputDevice::SetTextColor(EConsoleColor Color)
{
    SCOPED_LOCK(PendingCS);
    CurrentTextColor = Color;
}

void FMacConsoleOutputDevice::OnWindowDidClose()
{
    CHECK_COCOA_MAIN_THREAD();

    {
        SCOPED_LOCK(PendingCS);
        PendingLines.Clear();

        // The window is still executing -close, so it has to outlive this call
        [NSApp removeWindowsItem:WindowHandle];
        [WindowHandle autorelease];
        WindowHandle = nullptr;
    }

    DestroyResources();
}

void FMacConsoleOutputDevice::CreateConsole()
{
    if (WindowHandle)
    {
        return;
    }

    NSString* WindowTitle = nullptr;
    {
        SCOPED_AUTORELEASE_POOL();
        SCOPED_LOCK(PendingCS);
        WindowTitle = [(Title.IsEmpty() ? @"Output Console" : Title.GetNSString()) retain];
    }

    FMacThreadManager::Get().MainThreadDispatch(^
    {
        CHECK_COCOA_MAIN_THREAD();
        SCOPED_AUTORELEASE_POOL();

        if (!Font)
        {
            NSFont* ConsoleFont = [NSFont fontWithName:@"Courier" size:12.0f];
            if (!ConsoleFont)
            {
                ConsoleFont = [NSFont userFixedPitchFontOfSize:12.0f];
            }

            Font = [ConsoleFont retain];
        }

        // Init the backgroundcolor
        if (!BackGroundColor)
        {
            BackGroundColor = [[NSColor colorWithSRGBRed:0.15f green:0.15f blue:0.15f alpha:1.0f] retain];
        }

        MainThreadCreateAttributeCache();

        const NSUInteger StyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;

        const CGFloat Width  = static_cast<CGFloat>(CVarConsoleWindowWidth.GetValue());
        const CGFloat Height = static_cast<CGFloat>(CVarConsoleWindowHeight.GetValue());

        NSRect ContentRect = NSMakeRect(0.0f, 0.0f, Width, Height);

        FCocoaConsoleWindow* NewWindow = [[FCocoaConsoleWindow alloc] init:this ContentRect:ContentRect StyleMask:StyleMask Backing:NSBackingStoreBuffered Defer:NO];

        // AppKit would otherwise release the window on close, on top of the release this class owns
        [NewWindow setReleasedWhenClosed:NO];

        NSRect ContentFrame = NewWindow.contentView.frame;
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

        NewWindow.collectionBehavior = Behavior;

        [NSApp addWindowsItem:NewWindow title:WindowTitle filename:NO];

        NewWindow.title                 = WindowTitle;
        NewWindow.contentView           = ScrollView;
        NewWindow.initialFirstResponder = TextView;
        NewWindow.backgroundColor       = BackGroundColor;

        [NewWindow setOpaque:YES];

        {
            SCOPED_LOCK(PendingCS);
            WindowHandle = NewWindow;
            LineCount    = 0;
        }

        [NewWindow makeKeyAndOrderFront:NewWindow];
        [NewWindow makeFirstResponder:TextView];

        [WindowTitle release];
    }, NSDefaultRunLoopMode, true);
}

void FMacConsoleOutputDevice::DestroyConsole()
{
    // The resources outlive a window the user closed by hand, so both have to be tested
    if (!WindowHandle && !AttributeCache)
    {
        return;
    }

    FMacThreadManager::Get().MainThreadDispatch(^
    {
        CHECK_COCOA_MAIN_THREAD();
        SCOPED_AUTORELEASE_POOL();

        // Closing routes through windowWillClose: and so through OnWindowDidClose
        if (WindowHandle)
        {
            FCocoaConsoleWindow* WindowToClose = WindowHandle;
            [WindowToClose close];

            // A window that was never ordered in gets no windowWillClose:, so finish it by hand
            if (WindowHandle)
            {
                [WindowToClose setDelegate:nil];
                OnWindowDidClose();
            }
        }
        else
        {
            DestroyResources();
        }
    }, NSDefaultRunLoopMode, true);
}

void FMacConsoleOutputDevice::DestroyResources()
{
    CHECK_COCOA_MAIN_THREAD();
    SCOPED_AUTORELEASE_POOL();

    ReleaseAndClear(TextView);
    ReleaseAndClear(ScrollView);
    ReleaseAndClear(AttributeCache);
    ReleaseAndClear(Font);
    ReleaseAndClear(BackGroundColor);

    LineCount = 0;
}

void FMacConsoleOutputDevice::EnqueueLine(const String& Message, EConsoleColor Color)
{
    bool bNeedsFlush = false;
    {
        SCOPED_LOCK(PendingCS);

        if (!WindowHandle)
        {
            return;
        }

        PendingLines.Emplace(FPendingLine{ Message, Color });

        bNeedsFlush     = !bFlushScheduled;
        bFlushScheduled = true;
    }

    // One dispatch per run-loop turn rather than one per line
    if (bNeedsFlush)
    {
        FMacThreadManager::Get().MainThreadDispatch(^
        {
            MainThreadFlushPendingLines();
        }, NSDefaultRunLoopMode, false);
    }
}

void FMacConsoleOutputDevice::MainThreadCreateAttributeCache()
{
    CHECK_COCOA_MAIN_THREAD();

    if (AttributeCache)
    {
        return;
    }

    NSMutableArray* NewCache = [[NSMutableArray alloc] initWithCapacity:GNumConsoleColors];
    for (NSUInteger Index = 0; Index < GNumConsoleColors; Index++)
    {
        NSDictionary* ColorAttributes = @{
            NSForegroundColorAttributeName : CreateColorForConsoleColor(static_cast<EConsoleColor>(Index)),
            NSBackgroundColorAttributeName : BackGroundColor,
            NSFontAttributeName            : Font
        };

        [NewCache addObject:ColorAttributes];
    }

    AttributeCache = NewCache;
}

NSDictionary* FMacConsoleOutputDevice::MainThreadAttributesForColor(EConsoleColor Color) const
{
    CHECK_COCOA_MAIN_THREAD();

    const NSUInteger Index = static_cast<NSUInteger>(Color);
    if (!AttributeCache || Index >= AttributeCache.count)
    {
        return nullptr;
    }

    return [AttributeCache objectAtIndex:Index];
}

void FMacConsoleOutputDevice::MainThreadFlushPendingLines()
{
    CHECK_COCOA_MAIN_THREAD();
    SCOPED_AUTORELEASE_POOL();

    TArray<FPendingLine> Lines;
    {
        SCOPED_LOCK(PendingCS);

        Lines = ::Move(PendingLines);
        PendingLines.Clear();
        bFlushScheduled = false;
    }

    if (Lines.IsEmpty() || !WindowHandle || !TextView || !ScrollView)
    {
        return;
    }

    NSMutableAttributedString* Batch = [[NSMutableAttributedString alloc] init];
    for (const FPendingLine& Line : Lines)
    {
        NSString* NativeString = [NSString stringWithFormat:@"%s\n", *Line.Text];

        NSAttributedString* Attributed = [[NSAttributedString alloc] initWithString:NativeString attributes:MainThreadAttributesForColor(Line.Color)];
        [Batch appendAttributedString:Attributed];
        [Attributed release];
    }

    // Only follow the tail when the view is already there
    NSClipView*   ClipView     = ScrollView.contentView;
    const CGFloat DocumentMaxY = NSMaxY([ScrollView.documentView bounds]);
    const bool    bWasAtBottom = (DocumentMaxY - NSMaxY(ClipView.documentVisibleRect)) <= 1.0;

    NSRange Selection = [TextView selectedRange];

    NSTextStorage* Storage = TextView.textStorage;
    [Storage beginEditing];

    [Storage appendAttributedString:Batch];
    LineCount += Lines.Size();

    const NSUInteger TrimmedCharacters = MainThreadTrimToMaxLines(Storage);

    [Storage endEditing];

    // Slide the selection back by whatever was trimmed off the head
    if (Selection.length > 0)
    {
        if (Selection.location >= TrimmedCharacters)
        {
            Selection.location -= TrimmedCharacters;
        }
        else
        {
            const NSUInteger Removed = TrimmedCharacters - Selection.location;
            Selection.location = 0;
            Selection.length   = (Selection.length > Removed) ? (Selection.length - Removed) : 0;
        }

        [TextView setSelectedRange:Selection];
    }

    // Scroll
    if (bWasAtBottom)
    {
        [TextView scrollToEndOfDocument:TextView];
    }

    [Batch release];
}

NSUInteger FMacConsoleOutputDevice::MainThreadTrimToMaxLines(NSTextStorage* Storage)
{
    CHECK_COCOA_MAIN_THREAD();

    const int32 MaxLineCount = CVarMaxConsoleLineCount.GetValue();
    if (LineCount <= MaxLineCount)
    {
        return 0;
    }

    const int32 LinesToRemove = LineCount - MaxLineCount;

    NSString*        Text       = Storage.string;
    const NSUInteger TextLength = Text.length;

    NSUInteger CutOffset = 0;
    for (int32 Removed = 0; (Removed < LinesToRemove) && (CutOffset < TextLength); Removed++)
    {
        CutOffset = NSMaxRange([Text lineRangeForRange:NSMakeRange(CutOffset, 0)]);
    }

    if (CutOffset == 0)
    {
        return 0;
    }

    [Storage deleteCharactersInRange:NSMakeRange(0, CutOffset)];
    LineCount -= LinesToRemove;

    return CutOffset;
}
