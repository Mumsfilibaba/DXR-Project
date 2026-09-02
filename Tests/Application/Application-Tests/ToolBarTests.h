#pragma once

/** @brief Builds a strip of entries and rules, and checks the order and the measurements it produces. */
bool ToolBarComposition_Test();

/** @brief Clicks a button and a toggle, and checks what they fire and what they draw. */
bool ToolBarInteraction_Test();

/** @brief Fuses runs of entries into pills, and checks the gaps close and only the outer ends stay rounded. */
bool ToolBarGroups_Test();

/** @brief Stretches a search field and centres a group with flexible space, and checks both still reach the draw data. */
bool ToolBarFlexibleSpace_Test();

/** @brief Opens a dropdown from the strip, and checks the placement, the switching and the lit entry. */
bool ToolBarDropDown_Test();
