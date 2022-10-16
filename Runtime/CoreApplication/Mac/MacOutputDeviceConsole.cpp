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
	, ConsoleColor(nullptr)
{ }

FMacOutputDeviceConsole::~FMacOutputDeviceConsole()
{
	DestroyConsole();
}

void FMacOutputDeviceConsole::CreateConsole()
{
    if (!WindowHandle)
    {
        ExecuteOnMainThread(^
        {
            SCOPED_AUTORELEASE_POOL();
            
            const NSUInteger StyleMask = 
                NSWindowStyleMaskTitled | 
                NSWindowStyleMaskClosable | 
                NSWindowStyleMaskResizable | 
                NSWindowStyleMaskMiniaturizable;
            
            // TODO: Control with console vars?
            const CGFloat Width  = 640.0f;
            const CGFloat Height = 360.0f;
            
            NSRect ContentRect = NSMakeRect(0.0f, 0.0f, Width, Height);
            
            WindowHandle = [[FCocoaConsoleWindow alloc] init:this ContentRect:ContentRect StyleMask:StyleMask Backing:NSBackingStoreBuffered Defer:NO];
            SetTextColor(EConsoleColor::White);
            
            NSColor* BackGroundColor = [NSColor colorWithSRGBRed:0.15f green:0.15f blue:0.15f alpha:1.0f];
            
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
			
            NSSafeRelease(WindowHandle);
            NSSafeRelease(ConsoleColor);
			
			DestroyResources();
		}, NSDefaultRunLoopMode, true);
	}
}

void FMacOutputDeviceConsole::DestroyResources()
{
	SCOPED_AUTORELEASE_POOL();
	
    CHECK(FPlatformThreadMisc::IsMainThread());
    
    NSSafeRelease(TextView);
    NSSafeRelease(ScrollView);
}

void FMacOutputDeviceConsole::Show(bool bShow)
{
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
    if (WindowHandle)
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSString* NativeMessage = Message.GetNSString();
        [NativeMessage retain];
        
        ExecuteOnMainThread(^
        {
            SCOPED_AUTORELEASE_POOL();

            NSString* String = [NativeMessage stringByAppendingString:@"\n"];
			AppendStringAndScroll(String);
            [NativeMessage release];
            
            if(!MacApplication)
            {
                FPlatformApplicationMisc::PumpMessages(true);
            }
        }, NSDefaultRunLoopMode, false);
    }
}

void FMacOutputDeviceConsole::Log(ELogSeverity Severity, const FString& Message)
{
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
        
        NSString* NativeMessage = Message.GetNSString();
        [NativeMessage retain];
        
        ExecuteOnMainThread(^
        {
            SCOPED_AUTORELEASE_POOL();

            SetTextColor(NewColor);

            NSString* String = [NativeMessage stringByAppendingString:@"\n"];
			AppendStringAndScroll(String);
            [NativeMessage release];
			
            SetTextColor(EConsoleColor::White);
            
            if(!MacApplication)
            {
                FPlatformApplicationMisc::PumpMessages(true);
            }
        }, NSDefaultRunLoopMode, false);
    }
}

void FMacOutputDeviceConsole::Flush()
{
    if (WindowHandle)
    {
        ExecuteOnMainThread(^
        {
			SCOPED_AUTORELEASE_POOL();
			TextView.string = @"";
            
            if(!MacApplication)
            {
                FPlatformApplicationMisc::PumpMessages(true);
            }
        }, NSDefaultRunLoopMode, false);
    }
}

void FMacOutputDeviceConsole::SetTitle(const FString& InTitle)
{
    if (WindowHandle)
    {
        ExecuteOnMainThread(^
        {
            SCOPED_AUTORELEASE_POOL();
            
            NSString* Title = InTitle.GetNSString();
            WindowHandle.title = Title;
            
            if(!MacApplication)
            {
                FPlatformApplicationMisc::PumpMessages(true);
            }
        }, NSDefaultRunLoopMode, false);
    }
}

void FMacOutputDeviceConsole::SetTextColor(EConsoleColor Color)
{
    if (WindowHandle)
    {
        SCOPED_AUTORELEASE_POOL();
        
        // TScopedLock Lock(WindowCS);
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

int32 FMacOutputDeviceConsole::GetLineCount() const
{
	if (WindowHandle)
	{
		__block NSUInteger NumberOfLines = 0;
		ExecuteOnMainThread(^
		{
			NSString* String = TextView.string;
			
			NSUInteger StringLength = String.length;
			for (NSUInteger LineIndex = 0; LineIndex < StringLength; NumberOfLines++)
			{
				LineIndex = NSMaxRange([String lineRangeForRange:NSMakeRange(LineIndex, 0)]);
			}
		}, NSDefaultRunLoopMode, true);
		
		return static_cast<int32>(NumberOfLines);
	}
	else
	{
		return -1;
	}
}

void FMacOutputDeviceConsole::OnWindowDidClose()
{
	DestroyResources();
    
    // Exit the application, this gives the same behaviour as on Windows
    FPlatformApplicationMisc::RequestExit(0);
}

void FMacOutputDeviceConsole::AppendStringAndScroll(NSString* String)
{
	if (WindowHandle)
	{
		ExecuteOnMainThread(^
		{
			SCOPED_AUTORELEASE_POOL();
			
			// TODO: CVar
			const NSUInteger MaxLineCount = 512;
			
            NSAttributedString* AttributedString = nil;
            {
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
		}, NSDefaultRunLoopMode, true);
	}
}
