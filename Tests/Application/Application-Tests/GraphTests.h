#pragma once

/** @brief Adds and removes nodes and links, and checks the rules a connection has to pass. */
bool GraphModelEditing_Test();

/** @brief Runs the layered layout over a diamond and checks the columns and the spacing it produces. */
bool GraphLayoutLayered_Test();

/** @brief Checks the pan and zoom arithmetic, and that a node lands where the view says it should. */
bool GraphCanvasView_Test();

/** @brief Drives selection, node dragging, marquee sweeps and link authoring through the canvas. */
bool GraphCanvasInteraction_Test();

/** @brief Checks that the desc's node style reaches the draw commands, colors, corners and pin outline alike. */
bool GraphNodeStyle_Test();

/** @brief Checks that stacked pins take a row each, in input then output order, and earn a separator. */
bool GraphStackedPins_Test();

/** @brief Checks that a viewer canvas still moves and selects nodes while refusing links and deletions. */
bool GraphViewerMode_Test();
