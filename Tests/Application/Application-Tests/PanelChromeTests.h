#pragma once

/**
 * @brief Checks that a docked leaf paints one rounded fill and one thin outline.
 *
 * This is the shape of the VS Code look: every panel is its own rounded card rather
 * than a region of one flat surface.
 */
bool PanelChromeGeometry_Test();

/**
 * @brief Checks that the backdrop is the only fill running the whole area, and that the
 * gap between two docked leaves is the width the style asks for and shows the backdrop.
 */
bool PanelChromeGap_Test();

/** @brief Checks that a hovered splitter hints with a thin centered line rather than filling its gutter. */
bool SplitterHintThickness_Test();

/** @brief Checks that the tab strip does not paint its own fill over the panel's rounded top corners. */
bool TabStripBlendsIntoPanel_Test();

/** @brief Checks that the accent stroke is reserved for the focused card and is not the resting one. */
bool PanelChromeFocusStroke_Test();

/**
 * @brief Rasterizes representative shell layouts and writes them to Screenshots/<Set>.
 *
 * This leaves images behind for a human to look at rather than asserting on them, so it
 * passes as long as every snapshot could be written.
 */
bool PanelChromeSnapshots_Test();
