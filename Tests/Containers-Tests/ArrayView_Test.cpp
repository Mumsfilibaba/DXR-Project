#include "ArrayView_Test.h"

#if RUN_TARRAYVIEW_TEST
#include "TestUtils.h"

#include <Core/CoreTypes.h>
#include <Core/Containers/Array.h>
#include <Core/Containers/StaticArray.h>
#include <Core/Containers/ArrayView.h>

#include <iostream>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// PrintArrayView

template<typename T>
static void PrintArrayView(const TArrayView<T>& View)
{
    std::cout << "------------------------------" << std::endl;
    for (typename TArrayView<T>::SizeType i = 0; i < View.Size(); i++)
    {
        std::cout << View[i] << std::endl;
    }
    std::cout << "------------------------------" << std::endl;
}

template<typename T>
static void PrintArrayViewRangeBased(const TArrayView<T>& View)
{
    std::cout << "------------------------------" << std::endl;
    for (const T& Element : View)
    {
        std::cout << Element << std::endl;
    }
    std::cout << "------------------------------" << std::endl;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TArrayView_Test

bool TArrayView_Test()
{
    std::cout << std::endl << "----------TArrayView----------" << std::endl << std::endl;
    std::cout << "Testing Constructors" << std::endl;

    TArrayView<uint32> EmptyView;

    TArray<uint32> Arr0 = { 1, 2, 3, 4 };
    TArrayView<uint32> ArrView0 = TArrayView<uint32>(Arr0);

    {
        TArrayView<uint32> _ArrView = MakeArrayView(Arr0);
    }

    TStaticArray<uint32, 4> Arr1 = { 11, 12, 13, 14 };
    TArrayView<uint32> ArrView1 = TArrayView<uint32>(Arr1);

    {
        TArrayView<uint32> _ArrView = MakeArrayView(Arr1);
    }

    uint32 Arr2[] = { 21, 22, 23, 24 };
    TArrayView<uint32> ArrView2 = TArrayView<uint32>(Arr2);

    {
        TArrayView<uint32> _ArrView = MakeArrayView(Arr2);
    }

    uint32* DynamicPtr = new uint32[]{ 31, 32, 33, 34, 35 };
    TArrayView<uint32> ArrView3 = TArrayView<uint32>(DynamicPtr, 5);
    
    {
        TArrayView<uint32> _ArrView = MakeArrayView(DynamicPtr, 5);
    }

    const TArray<uint32> ConstArr0 = { 1, 2, 3, 4 };
    TArrayView<const uint32> ConstArrView0 = TArrayView<const uint32>(ConstArr0);

    std::cout << "Testing At and operator[]" << std::endl;
    PrintArrayView(EmptyView);
    PrintArrayView(ArrView0);
    PrintArrayView(ArrView1);
    PrintArrayView(ArrView2);
    PrintArrayView(ArrView3);

    std::cout << "Testing range-based for-loops" << std::endl;
    PrintArrayViewRangeBased(EmptyView);
    PrintArrayViewRangeBased(ArrView0);
    PrintArrayViewRangeBased(ArrView1);
    PrintArrayViewRangeBased(ArrView2);
    PrintArrayViewRangeBased(ArrView3);

    std::cout << "Testing copy/move constructor" << std::endl;
    TArrayView<uint32> ArrView4 = ArrView1;
    TArrayView<uint32> ArrView5 = Move(ArrView0);

    PrintArrayViewRangeBased(ArrView4);
    PrintArrayViewRangeBased(ArrView5);

    std::cout << "Testing IsEmpty" << std::endl;
    std::cout << "EmptyView=" << std::boolalpha << EmptyView.IsEmpty() << std::endl;
    std::cout << "ArrView0=" << std::boolalpha << ArrView5.IsEmpty() << std::endl;

    std::cout << "Testing Size/SizeInBytes" << std::endl;
    std::cout << "Size: " << ArrView4.Size() << std::endl;
    std::cout << "SizeInBytes: " << ArrView4.SizeInBytes() << std::endl;

    std::cout << "Testing Swap" << std::endl;
    std::cout << "-----------Before----------" << std::endl;
    PrintArrayViewRangeBased(ArrView4);
    PrintArrayViewRangeBased(ArrView5);

    ArrView4.Swap(ArrView5);

    std::cout << "-----------After-----------" << std::endl;
    PrintArrayViewRangeBased(ArrView4);
    PrintArrayViewRangeBased(ArrView5);

    std::cout << "Testing Fill" << std::endl;
    std::cout << "-----------Before----------" << std::endl;
    PrintArrayViewRangeBased(ArrView4);

    ArrView4.Fill(99);

    std::cout << "-----------After-----------" << std::endl;
    PrintArrayViewRangeBased(ArrView4);

    delete[] DynamicPtr;
    SUCCESS();
}

#endif
