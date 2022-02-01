#include "Variant_Test.h"

#if RUN_TVARIANT_TEST

#include <Core/Containers/Variant.h>
#include <Core/Memory/Memory.h>
#include <Core/Templates/ClassUtilities.h>

#include <iostream>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// STest

void TVariant_Test()
{
    TVariant<std::string, int32> Variant0(int32(0));
    const bool bResult = Variant0.IsType<int32>();

    std::cout << "IsType<std::string>()=" << std::boolalpha << bResult << std::endl;
    return;
}

#endif