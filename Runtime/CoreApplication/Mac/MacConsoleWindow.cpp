#include "MacConsoleWindow.h"
#include "MacApplication.h"
#include "CocoaConsoleWindow.h"

#include "Core/Mac/Mac.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/Mac/MacRunLoop.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacConsoleWindow

FMacConsoleWindow::FMacConsoleWindow()
    : WindowHandle(nullptr)
	, TextView(nullptr)
	, ScrollView(nullptr)
	, ConsoleColor(nullptr)
{ }

FMacConsoleWindow::~FMacConsoleWindow()
{
	DestroyConsole();
}

void FMacConsoleWindow::CreateConsole()
{
    if (!WindowHandle)
    {
        MakeMainThreadCall(^
        {
            SCOPED_AUTORELEASE_POOL();
            
            const NSUInteger StyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
            
            // TODO: Control with console vars?
            const CGFloat Width  = 640.0f;
            const CGFloat Height = 360.0f;
            
            NSRect ContentRect = NSMakeRect(0.0f, 0.0f, Width, Height);
            
            WindowHandle = [[FCocoaConsoleWindow alloc] init:this ContentRect:ContentRect StyleMask:StyleMask Backing:NSBackingStoreBuffered Defer:NO];
            SetColor(EConsoleColor::White);
            
            NSColor* BackGroundColor = [NSColor colorWithSRGBRed:0.15f green:0.15f blue:0.15f alpha:1.0f];
            
            NSRect ContentFrame = WindowHandle.contentView.frame;
            ScrollView = [[NSScrollView alloc] initWithFrame:ContentFrame];
            ScrollView.borderType = NSNoBorder;
            [ScrollView setHasVerticalScroller:YES];
            [ScrollView setHasHorizontalScroller:NO];
            ScrollView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
            ScrollView.backgroundColor  = BackGroundColor;
            
            NSSize ContentSize = [ScrollView contentSize];
            TextView = [[NSTextView alloc] initWithFrame:NSMakeRect(0.0, 0.0, ContentSize.width, ContentSize.height)];
            [TextView setEditable:NO];
            TextView.minSize = NSMakeSize(0.0f, ContentSize.height);
            TextView.maxSize = NSMakeSize(FLT_MAX, FLT_MAX);
            [TextView setVerticallyResizable:YES];
            [TextView setHorizontallyResizable:NO];
            TextView.autoresizingMask = NSViewWidthSizable;
            TextView.backgroundColor  = BackGroundColor;
            
            NSTextContainer* Container = TextView.textContainer;
            Container.containerSize = NSMakeSize(ContentSize.width, FLT_MAX);
            [Container setWidthTracksTextView:YES];
            
            ScrollView.documentView = TextView;
            
            NSWindowCollectionBehavior Behavior = NSWindowCollectionBehaviorFullScreenAuxiliary
                                                | NSWindowCollectionBehaviorDefault
                                                | NSWindowCollectionBehaviorManaged
                                                | NSWindowCollectionBehaviorParticipatesInCycle;
            
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
        }, true);
    }
}

void FMacConsoleWindow::DestroyConsole()
{
    if (IsVisible())
    {
        MakeMainThreadCall(^
        {
			SCOPED_AUTORELEASE_POOL();
		
			FPlatformApplicationMisc::PumpMessages(true);
			
            NSSafeRelease(WindowHandle);
            NSSafeRelease(ConsoleColor);
			
			DestroyResources();
		}, true);
	}
}

void FMacConsoleWindow::DestroyResources()
{
	SCOPED_AUTORELEASE_POOL();
	
    NSSafeRelease(TextView);
    NSSafeRelease(ScrollView);
}

void FMacConsoleWindow::Show(bool bShow)
{
    TScopedLock Lock(WindowCS);
    
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

void FMacConsoleWindow::Print(const FString& Message)
{
    if (WindowHandle)
    {
        MakeMainThreadCall(^
        {
            SCOPED_AUTORELEASE_POOL();

            NSString* String = Message.GetNSString();
			AppendStringAndScroll(String);
			
            FPlatformApplicationMisc::PumpMessages(true);
        }, true);
    }
}

void FMacConsoleWindow::PrintLine(const FString& Message)
{
    if (WindowHandle)
    {
        MakeMainThreadCall(^
        {
            SCOPED_AUTORELEASE_POOL();

            NSString* String      = Message.GetNSString();
            NSString* FinalString = [String stringByAppendingString:@"\n"];
        
            AppendStringAndScroll(FinalString);
        
            FPlatformApplicationMisc::PumpMessages(true);
        }, true);
    }
}

void FMacConsoleWindow::Clear()
{
    if (WindowHandle)
    {
        MakeMainThreadCall(^
        {
			SCOPED_AUTORELEASE_POOL();
			TextView.string = @"";
        }, true);
    }
}

void FMacConsoleWindow::SetTitle(const FString& InTitle)
{
    if (WindowHandle)
    {
        MakeMainThreadCall(^
        {
            SCOPED_AUTORELEASE_POOL();
            
            NSString* Title = InTitle.GetNSString();
            WindowHandle.title = Title;
        }, true);
    }
}

void FMacConsoleWindow::SetColor(EConsoleColor Color)
{
    TScopedLock Lock(WindowCS);
    
    if (WindowHandle)
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (ConsoleColor)
        {
            [ConsoleColor release];
        }
        
        NSMutableArray* Colors     = [NSMutableArray new];
        NSMutableArray* Attributes = [NSMutableArray new];
        [Attributes addObject:NSForegroundColorAttributeName];
        [Attributes addObject:NSBackgroundColorAttributeName];

        // Add foreground Color
        if (Color == EConsoleColor::White)
        {
            [Colors addObject:[NSColor colorWithSRGBRed:0.85f green:0.85f blue:0.85f alpha:1.0f]];
        }
        else if (Color == EConsoleColor::Red)
        {
            [Colors addObject:[NSColor colorWithSRGBRed:0.85f green:0.0f blue:0.0f alpha:1.0f]];
        }
        else if (Color == EConsoleColor::Green)
        {
            [Colors addObject:[NSColor colorWithSRGBRed:0.0f green:0.85f blue:0.0f alpha:1.0f]];
        }
        else if (Color == EConsoleColor::Yellow)
        {
            [Colors addObject:[NSColor colorWithSRGBRed:0.85f green:0.85f blue:0.0f alpha:1.0f]];
        }
        
        // Add background Color
        [Colors addObject:[NSColor colorWithSRGBRed:0.15f green:0.15f blue:0.15f alpha:1.0f]];
        
        ConsoleColor = [[NSDictionary alloc] initWithObjects:Colors forKeys:Attributes];

        [Colors release];
        [Attributes release];
    }
}

bool FMacConsoleWindow::IsVisible() const
{
    return (WindowHandle != nil);
}

int32 FMacConsoleWindow::GetLineCount() const
{
	if (WindowHandle)
	{
		__block NSUInteger NumberOfLines = 0;
		MakeMainThreadCall(^
		{
			NSString* String = TextView.string;
			
			NSUInteger StringLength = String.length;
			for (NSUInteger LineIndex = 0; LineIndex < StringLength; NumberOfLines++)
			{
				LineIndex = NSMaxRange([String lineRangeForRange:NSMakeRange(LineIndex, 0)]);
			}
		}, true);
		
		return static_cast<int32>(NumberOfLines);
	}
	else
	{
		return -1;
	}
}

void FMacConsoleWindow::OnWindowDidClose()
{
	DestroyResources();
	bIsVisible = false;
    
    // Exit the application, this gives the same behaviour as on Windows
    FPlatformApplicationMisc::RequestExit(0);
}

void FMacConsoleWindow::AppendStringAndScroll(NSString* String)
{
	if (WindowHandle)
	{
		MakeMainThreadCall(^
		{
			SCOPED_AUTORELEASE_POOL();
			
			// TODO: CVar
			const NSUInteger MaxLineCount = 512;
			
            NSAttributedString* AttributedString = nil;
            {
                TScopedLock Lock(WindowCS);
                AttributedString = [[NSAttributedString alloc] initWithString:String attributes:ConsoleColor];
            }
				
			NSTextStorage* Storage = TextView.textStorage;
			[Storage beginEditing];
			
			// Remove lines
			NSUInteger LineCount  = GetLineCount();
			NSString*  TextString = TextView.string;
			if (LineCount >= MaxLineCount)
			{
				NSUInteger LineIndex;
				NSUInteger NumberOfLines = 0;
				NSUInteger StringLength  = TextString.length;
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
			Storage.font = [NSFont fontWithName:@"Courier" size:12.0f];
			
			[Storage endEditing];
			
			// Scroll
			[TextView scrollToEndOfDocument:TextView];
			
			[AttributedString release];
		}, true);
	}
}
