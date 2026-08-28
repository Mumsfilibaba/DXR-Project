#pragma once

/** @brief Projection, ray casting, hit geometry and snapping, against fixed matrices. */
bool GizmoProjection_Test();

/** @brief Which handle a cursor resolves to, and which axes are worth drawing at all. */
bool GizmoHitTest_Test();

/** @brief What a press, a move and a release do to the transform, with and without snapping. */
bool GizmoDrag_Test();

/** @brief Local against world, the operations, and the rules that hold while a drag is in progress. */
bool GizmoModes_Test();
