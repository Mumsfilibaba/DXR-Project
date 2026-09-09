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

/** @brief Reorders a tab in a built area and checks the tree, a rebuild and a save all follow it. */
bool DockingAreaTabReorder_Test();

/** @brief Carries a torn-out tab across two areas and drops it into the second. */
bool DockDragState_Test();

/** @brief Drops a tab clear of every area and drives the host window that spawns through to closing again. */
bool DockWindowManagerTearOut_Test();

/** @brief Drives the decorator a tear-out puts up, through hiding over a target and both ways a drop commits. */
bool DockDecoratorDrag_Test();

/** @brief Runs the same tear-out with the retained-target trial on, where there is no RHI to snapshot into. */
bool DockDecoratorSnapshot_Test();

/** @brief Drives the picture of the panel a target draws over the rectangle a drop would land in. */
bool DockDropPreview_Test();

/** @brief Drags a host window by its own title bar over an area and drops it there. */
bool DockHostNativeDrag_Test();

/** @brief Round-trips a multi-window layout file, and reads a version 1 file as one window. */
bool DockLayoutFileMultiWindow_Test();
