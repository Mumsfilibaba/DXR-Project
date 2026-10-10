#pragma once
#include "LaunchProgram/ProgramLoop.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"

extern const CHAR*        GProgramTitle;
extern TFunction<int32()> GProgramBody;
extern TArray<String>     GProgramArgs;

#define IMPLEMENT_PROGRAM_MAIN(Title, BodyFn) \
    struct FProgramMainRegistrar \
    { \
        FProgramMainRegistrar() \
        { \
            GProgramTitle = (Title); \
            GProgramBody  = (BodyFn); \
        } \
    }; \
    static FProgramMainRegistrar GProgramMainRegistrar;
