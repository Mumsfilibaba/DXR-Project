#include "Application/CursorHitCache.h"
#include "Application/Elements/Window.h"

FCursorHitCache::FCursorHitCache()
    : Path()
    , Window()
    , ClientPosition()
    , Generation(0)
    , bIsValid(false)
{
}

FCursorHitCache::~FCursorHitCache() = default;

void FCursorHitCache::Invalidate()
{
    Path.Reset();
    Window.Reset();
    bIsValid = false;
}

const FElementPath& FCursorHitCache::Resolve(const TSharedPtr<FWindow>& InWindow, const IntVector2& InClientPosition)
{
    const uint64 CurrentGeneration = FVisualElement::GetHitTestGeneration();

    const bool bIsCurrent = bIsValid
        && Generation == CurrentGeneration
        && ClientPosition == InClientPosition
        && Window.Get() == InWindow.Get()
        && (!InWindow || Window.IsValid());

    if (bIsCurrent)
    {
        return Path;
    }

    Path.Reset();

    if (InWindow)
    {
        InWindow->HitTest(InClientPosition, Path);
    }

    Window         = InWindow;
    ClientPosition = InClientPosition;
    Generation     = FVisualElement::GetHitTestGeneration();
    bIsValid       = true;
    return Path;
}
