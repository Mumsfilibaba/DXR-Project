#pragma once

/** @brief Measures a caption against the platform metrics on both a leading-inset and a trailing-inset platform. */
bool TitleBarMetrics_Test();

/** @brief Checks the caption, interactive and maximize rectangles a title bar publishes every arrange. */
bool TitleBarRegions_Test();

/** @brief Drives the application-drawn minimize, maximize and close buttons. */
bool CaptionButtons_Test();

/** @brief Builds a floating window and checks its style, its stacking and what happens when it closes. */
bool FloatingWindow_Test();
