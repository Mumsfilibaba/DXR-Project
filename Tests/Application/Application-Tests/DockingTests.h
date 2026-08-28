#pragma once

/** @brief Measures a tab leaf and a nested split, which is what bounds every splitter drag above it. */
bool DockNodeMinimumSize_Test();

/** @brief Drives the three collapse invariants an emptied tree has to come back to. */
bool DockNodeCollapse_Test();

/** @brief Arranges a splitter and checks where its children and its handles land. */
bool SplitterLayout_Test();

/** @brief Drags a splitter handle, including past the point where a neighbour hits its minimum. */
bool SplitterDrag_Test();

/** @brief Seeds shares and minimums through the description and checks they survive the children arriving. */
bool SplitterSeededDesc_Test();

/** @brief Activates, reorders and closes tabs in a strip. */
bool TabStripReorder_Test();

/** @brief Drags a tab clear of its strip and checks the tear-out threshold. */
bool TabStripTearOut_Test();

/** @brief Docks and undocks panels, checking the tree each operation leaves behind. */
bool DockingAreaDockUndock_Test();

/** @brief Resolves points over a built layout into the panel and the edge a drop would land on. */
bool DockingAreaHitTest_Test();

/** @brief Saves a layout to an ini file and reads it back, comparing the trees. */
bool DockingAreaPersistence_Test();

/** @brief Carries a torn-out tab across two areas and drops it into the second. */
bool DockDragState_Test();
