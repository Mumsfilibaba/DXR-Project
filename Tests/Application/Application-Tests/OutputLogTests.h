#pragma once

/** @brief Lays out coloured runs and checks the wrapping and the measurements they produce. */
bool RichTextLayout_Test();

/** @brief Drags a selection across a colour change and reads back what it covers. */
bool RichTextSelection_Test();

/** @brief Searches for a substring spanning runs and lines, and checks the matches found. */
bool RichTextSearch_Test();

/** @brief Feeds the view through IOutputDevice and checks the ring buffer and the runs it builds. */
bool LogViewLogging_Test();

/** @brief Filters by severity and by search text, in both highlight and hide modes. */
bool LogViewFiltering_Test();

/** @brief Checks that the view follows the tail and stops once the reader scrolls away from it. */
bool LogViewAutoScroll_Test();
