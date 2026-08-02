#pragma once
#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>

// Which benchmarks the Core-Benchmarks executable runs.
//
// NOTE: there is no build-configuration gate here. Debug timings are misleading,
// so RunCoreBenchmarks only builds Development and Release.
#define RUN_TARRAY_BENCHMARKS    (1)
#define RUN_TASKGRAPH_BENCHMARKS (1)
