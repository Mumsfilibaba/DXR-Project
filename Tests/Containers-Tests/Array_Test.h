#pragma once
#include "Config.h"

#if RUN_TARRAY_TEST || RUN_TARRAY_BENCHMARKS
#include <Core/CoreTypes.h>

void TArray_Benchmark();
void TArray_Test( int32 Argc, const char** Argv );

#endif
