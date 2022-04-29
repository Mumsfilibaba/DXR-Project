#include "MacConsoleWindow.h"
#include "CocoaConsoleWindow.h"
#include "ScopedAutoreleasePool.h"

#include "Core/Threading/Mac/MacRunLoop.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacConsoleWindow

CMacConsoleWindow* CMacConsoleWindow::CreateMacConsole()
{
	return dbg_new CMacConsoleWindow();
}

CMacConsoleWindow::CMacConsoleWindow()
    : Window(nullptr)
	, TextView(nullptr)
	, ScrollView(nullptr)
	, ConsoleColor(nullptr)
{ }

CMacConsoleWindow::~CMacConsoleWindow()
{
	DestroyConsole();
}

void CMacConsoleWindow::CreateConsole()
{
	MakeMainThreadCall(^
	{
		SCOPED_AUTORELEASE_POOL();
		
		const NSUInteger StyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
		
		// TODO: Control with console vars?
		const CGFloat Width  = 640.0f;
		const CGFloat Height = 360.0f;
		
		NSRect ContentRect = NSMakeRect(0.0f, 0.0f, Width, Height);
		
		Window = [[CCocoaConsoleWindow alloc] init:this ContentRect:ContentRect StyleMask:StyleMask Backing:NSBackingStoreBuffered Defer:NO];
		SetColor(EConsoleColor::White);
		
		NSRect ContentFrame = [[Window contentView] frame];
		ScrollView = [[NSScrollView alloc] initWithFrame:ContentFrame];
		[ScrollView setBorderType:NSNoBorder];
		[ScrollView setHasVerticalScroller:YES];
		[ScrollView setHasHorizontalScroller:NO];
		[ScrollView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
		
		TextView = [[NSTextView alloc] initWithFrame:ContentFrame];
		[TextView setEditable:NO];
		[TextView setMinSize:NSMakeSize(0.0f, Height)];
		[TextView setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
		[TextView setVerticallyResizable:YES];
		[TextView setHorizontallyResizable:NO];
		[TextView setAutoresizingMask:NSViewWidthSizable];
		
		NSTextContainer* Container = [TextView textContainer];
		[Container setContainerSize:NSMakeSize(Width, FLT_MAX)];
		[Container setWidthTracksTextView:YES];
		
		[ScrollView setDocumentView:TextView];
		
		[Window setTitle:@"Output Console"];
		[Window setContentView:ScrollView];
		[Window setInitialFirstResponder:TextView];
		[Window setOpaque:YES];
		[Window makeKeyAndOrderFront:Window];

		PlatformApplicationMisc::PumpMessages(true);
	}, true);
}

void CMacConsoleWindow::DestroyConsole()
{
	if (IsVisible())
	{
		MakeMainThreadCall(^
		{
			SCOPED_AUTORELEASE_POOL();
		
			PlatformApplicationMisc::PumpMessages(true);
			
			if (Window)
			{
				[Window release];
				Window = nullptr;
			}
			
			if (ConsoleColor)
			{
				[ConsoleColor release];
				ConsoleColor = nullptr;
			}
			
			DestroyResources();
		}, true);
	}
}

void CMacConsoleWindow::DestroyResources()
{
	SCOPED_AUTORELEASE_POOL();
	
	if (TextView)
	{
		[TextView release];
		TextView = nullptr;
	}
	
	if (ScrollView)
	{
		[ScrollView release];
		ScrollView = nullptr;
	}
}

void CMacConsoleWindow::Show(bool bShow)
{
	if (bIsVisible != bShow)
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

void CMacConsoleWindow::Print(const String& Message)
{  
    if (Window)
    {
        MakeMainThreadCall(^
        {
            SCOPED_AUTORELEASE_POOL();

            NSString* String = [NSString stringWithUTF8String:Message.CStr()];
			AppendStringAndScroll(String);
			
            PlatformApplicationMisc::PumpMessages(true);
        }, true);
    }
}

void CMacConsoleWindow::PrintLine(const String& Message)
{
    if (Window)
    {
        MakeMainThreadCall(^
        {
            SCOPED_AUTORELEASE_POOL();

            NSString* String      = [NSString stringWithUTF8String:Message.CStr()];
            NSString* FinalString = [String stringByAppendingString:@"\n"];
        
            AppendStringAndScroll(FinalString);
        
            PlatformApplicationMisc::PumpMessages(true);
        }, true);
    }
}

void CMacConsoleWindow::Clear()
{
    if (Window)
    {
        MakeMainThreadCall(^
        {
			SCOPED_AUTORELEASE_POOL();
			[TextView setString:@""];
        }, true);
    }
}

void CMacConsoleWindow::SetTitle(const String& InTitle)
{
    if (Window)
    {
        MakeMainThreadCall(^
        {
            SCOPED_AUTORELEASE_POOL();
            
            NSString* Title = [NSString stringWithUTF8String:InTitle.CStr()];
            [Window setTitle:Title];
        }, true);
    }
}

void CMacConsoleWindow::SetColor(EConsoleColor Color)
{
    if (Window)
    {
        MakeMainThreadCall(^
        {
			SCOPED_AUTORELEASE_POOL();
			
			if (ConsoleColor)
			{
				[ConsoleColor release];
			}
			
			NSMutableArray* Colors     = [[NSMutableArray alloc] init];
			NSMutableArray* Attributes = [[NSMutableArray alloc] init];
			[Attributes addObject:NSForegroundColorAttributeName];
			[Attributes addObject:NSBackgroundColorAttributeName];

			// Add foreground Color
			if (Color == EConsoleColor::White)
			{
				[Colors addObject:[NSColor colorWithSRGBRed:1.0f green:1.0f blue:1.0f alpha:1.0f]];
			}
			else if (Color == EConsoleColor::Red)
			{
				[Colors addObject:[NSColor colorWithSRGBRed:1.0f green:0.0f blue:0.0f alpha:1.0f]];
			}
			else if (Color == EConsoleColor::Green)
			{
				[Colors addObject:[NSColor colorWithSRGBRed:0.0f green:1.0f blue:0.0f alpha:1.0f]];
			}
			else if (Color == EConsoleColor::Yellow)
			{
				[Colors addObject:[NSColor colorWithSRGBRed:1.0f green:1.0f blue:0.0f alpha:1.0f]];
			}
			
			// Add background Color
			[Colors addObject:[NSColor colorWithSRGBRed:0.1f green:0.1f blue:0.1f alpha:0.1f]];
			
			ConsoleColor = [[NSDictionary alloc] initWithObjects:Colors forKeys:Attributes];

			[Colors release];
			[Attributes release];
        }, true);
    }
}

int32 CMacConsoleWindow::GetLineCount() const
{
	if (Window)
	{
		__block NSUInteger NumberOfLines = 0;
		MakeMainThreadCall(^
		{
			NSString* String = [TextView string];
			
			NSUInteger StringLength = [String length];
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

void CMacConsoleWindow::OnWindowDidClose()
{
	DestroyResources();
	bIsVisible = false;
}

void CMacConsoleWindow::AppendStringAndScroll(NSString* String)
{
	if (Window)
	{
		MakeMainThreadCall(^
		{
			SCOPED_AUTORELEASE_POOL();
			
			// TODO: CVar
			const NSUInteger MaxLineCount = 196;
			
			NSAttributedString* AttributedString = [[NSAttributedString alloc] initWithString:String attributes:ConsoleColor];
				
			NSTextStorage* Storage = [TextView textStorage];
			[Storage beginEditing];
			
			// Remove lines
			NSUInteger LineCount  = GetLineCount();
			NSString*  TextString = [TextView string];
			if (LineCount >= MaxLineCount)
			{
				NSUInteger LineIndex;
				NSUInteger NumberOfLines = 0;
				NSUInteger StringLength  = [TextString length];
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
			[Storage setFont:[NSFont fontWithName:@"Courier" size:12.0f]];
			
			[Storage endEditing];
			
			// Scroll
			[TextView scrollToEndOfDocument:TextView];
			
			[AttributedString release];
		}, true);
	}
}
