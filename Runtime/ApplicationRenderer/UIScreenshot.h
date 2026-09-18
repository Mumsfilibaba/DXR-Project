#pragma once
#include "Core/CoreTypes.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Math/IntVector2.h"

class FVisualElement;
class FWindow;

struct APPLICATIONRENDERER_API UIScreenshot
{
    /**
     * @brief Repaints an element offscreen and writes it out.
     *
     * @param Element  The element to draw, which is laid out at Size before drawing rather than at
     *                 whatever size it currently holds.
     * @param Size     The size to lay the element out at, in logical pixels.
     * @param DPIScale The scale to draw at, so a capture can be taken at the density of a display
     *                 that is not attached.
     * @param Filename Where to write, either absolute or relative to the working directory.
     * @return True when the file was written.
     */
    static bool CaptureElement(const TSharedPtr<FVisualElement>& Element, const IntVector2& Size, float DPIScale, const String& Filename);

    /**
     * @brief Repaints a window's content at the window's own size and scale, and writes it out.
     *
     * @param Window   The window whose content to capture.
     * @param Filename Where to write.
     * @return True when the file was written.
     */
    static bool CaptureWindow(const TSharedPtr<FWindow>& Window, const String& Filename);

    /**
     * @brief Captures the first window the application holds, which is the shell in a normal run.
     *
     * @param Filename Where to write.
     * @return True when the file was written.
     */
    static bool CapturePrimaryWindow(const String& Filename);

    /**
     * @brief Asks for a capture of the primary window some number of frames from now.
     *
     * An unattended run needs this because the shell is not worth looking at on its first frame:
     * fonts are still rasterizing, the layout has not settled and no panel has had a tick. Waiting
     * a few frames and then capturing is what makes a scripted run produce the same image a person
     * would see.
     *
     * @param FramesFromNow How many frames to let pass first. Zero or less captures on the next one.
     * @param Filename      Where to write.
     * @param bExitAfter    True to ask the engine to quit once the file is written, which is what an
     *                      unattended run wants so the process does not sit there.
     */
    static void RequestAtFrame(int32 FramesFromNow, const String& Filename, bool bExitAfter);

    /**
     * @brief Services a pending request, taking the capture when its frame has come around.
     *
     * The renderer calls this once a frame, after the frame has been presented, since capturing
     * partway through recording one would submit a second list into the middle of it.
     *
     * The first call also picks up '-UIScreenshot=<path>' from the command line, with an optional
     * '-UIScreenshotFrame=<count>' beside it, which is how a scripted run asks for a capture when
     * there is nobody there to type a console command.
     */
    static void Tick();
};
