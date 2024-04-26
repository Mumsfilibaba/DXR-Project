#include "GenericApplication.h"

TSharedPtr<FGenericApplication> FGenericApplication::Create()
{
    return MakeShared<FGenericApplication>(TSharedPtr<ICursor>(nullptr));
}

FGenericApplication::FGenericApplication(const TSharedPtr<ICursor>& InCursor)
    : Cursor(InCursor)
    , MessageHandler(nullptr)
{
}