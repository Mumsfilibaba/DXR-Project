#include "Overlay.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

void FOverlay::Paint(const FRectangle& AssignedBounds)
{
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

FOverlay::FScopedSlotInitilizer FOverlay::AddSlot()
{
    FOverlay::FScopedSlotInitilizer SlotInitializer(MakeUnique<FOverlaySlot>(), Children);
    return ::Move(SlotInitializer);
}