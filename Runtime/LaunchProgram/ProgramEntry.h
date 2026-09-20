#pragma once
#include "LaunchProgram/ProgramLoop.h"

extern const CHAR*          GProgramTitle;
extern TFunction<int32()> GProgramBody;

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
