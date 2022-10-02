#pragma once
#include "Mac.h"

#include "Core/Generic/GenericFile.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacFileHandle

class CORE_API FMacFileHandle 
    : public IFileHandle
{
    static constexpr int64 kMaxReadWriteSize = 1024 * 1024;

public:
    FMacFileHandle(int32 InFileHandle, bool bInReadOnly);
    ~FMacFileHandle() = default;

    virtual bool SeekFromStart(int64 InOffset) override final;
    virtual bool SeekFromCurrent(int64 InOffset) override final;
    virtual bool SeekFromEnd(int64 InOffset);

    virtual int64 Size() const override final;

    virtual int64 Tell() const override final;

    virtual int32 Read(uint8* Dst, uint32 BytesToRead) override final;
    virtual int32 Write(uint8* Src, uint32 BytesToWrite) override final;

    virtual bool Truncate(int64 NewSize) override final;

    virtual bool IsValid() const override final;

    virtual void Close() override final;

private:
    int64 FilePointer = 0;
    int32 FileHandle  = -1;
    bool  bReadOnly;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacFile

struct CORE_API FMacFile 
    : public FGenericFile
{
    static IFileHandle* OpenForRead(const FString& Filename);

    static IFileHandle* OpenForWrite(const FString& Filename);
};