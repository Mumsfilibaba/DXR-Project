#pragma once
#include "Core/Core.h"

class FByteOutputStream
{
public:
    FByteOutputStream()
        : Stream(nullptr)
        , StreamCapacity(0)
        , StreamSize(0)
    {
    }
    
    FByteOutputStream(const FByteOutputStream& Other)
        : Stream(nullptr)
        , StreamCapacity(Other.StreamSize)
        , StreamSize(Other.StreamSize)
    {
        InternalAllocate(Other.StreamSize);
        FMemory::Memcpy(Stream, Other.Stream, Other.StreamSize);
    }
    
    FByteOutputStream(FByteOutputStream&& Other)
        : Stream(Other.Stream)
        , StreamCapacity(Other.StreamCapacity)
        , StreamSize(Other.StreamSize)
    {
        Other.Stream         = nullptr;
        Other.StreamCapacity = 0;
        Other.StreamSize     = 0;
    }
    
    ~FByteOutputStream()
    {
        InternalFree();
    }
    
    template<typename T>
    int32 AddUninitialized()
    {
        const int32 TotalSize = sizeof(T);
        return AppendBytes(TotalSize);
    }
    
    template<typename T>
    int32 AddUninitialized(int32 NumElements)
    {
        const int32 TotalSize = sizeof(T) * NumElements;
        return AppendBytes(TotalSize);
    }

    template<typename T>
    int32 Add(const T& Value)
    {
        return Add(reinterpret_cast<const void*>(&Value), sizeof(T));
    }
        
    template<typename T>
    int32 Add(T* Elements, int32 NumElements)
    {
        const int32 TotalSize = sizeof(T) * NumElements;
        return Add(reinterpret_cast<const void*>(Elements), TotalSize);
    }
    
    int32 Add(const void* Source, int32 Size)
    {
        const int32 PreviousOffset = AppendBytes(Size);
        Write(Source, Size, PreviousOffset);
        return PreviousOffset;
    }
    
    template<typename T>
    int32 Write(const T& Value, int32 WriteOffset)
    {
        return Write(reinterpret_cast<const void*>(&Value), sizeof(T), WriteOffset);
    }
    
    template<typename T>
    int32 Write(T* Elements, int32 NumElements, int32 WriteOffset)
    {
        const int32 TotalSize = sizeof(T) * NumElements;
        return Write(reinterpret_cast<const void*>(Elements), TotalSize, WriteOffset);
    }
    
    int32 Write(const void* Source, int32 Size, int32 InWriteOffset)
    {
        uint8* Dest = Stream + InWriteOffset;
        FMemory::Memcpy(Dest, Source, Size);
        return Size;
    }
    
    int32 WriteOffset() const
    {
        // NOTE: The current write offset is the same as the size of the stream
        // however in the future we might want to have a seperate pointer that points
        // to the location that we should write into
        return StreamSize;
    }
        
    uint8* Data()
    {
        return Stream;
    }
    
    const uint8* Data() const
    {
        return Stream;
    }
    
    int32 Size() const
    {
        return StreamSize;
    }
    
    FByteOutputStream& operator=(const FByteOutputStream& Other)
    {
        if (this != AddressOf(Other))
        {
            if (StreamSize < Other.StreamSize)
            {
                InternalFree();
                InternalAllocate(Other.StreamSize);
                
                StreamCapacity = Other.StreamSize;
                StreamSize     = Other.StreamSize;
            }
            
            FMemory::Memcpy(Stream, Other.Stream, Other.StreamSize);
        }
        
        return *this;
    }
    
    FByteOutputStream& operator=(FByteOutputStream&& Other)
    {
        if (this != AddressOf(Other))
        {
            Stream         = Other.Stream;
            StreamCapacity = Other.StreamCapacity;
            StreamSize     = Other.StreamSize;
            
            Other.Stream         = nullptr;
            Other.StreamCapacity = 0;
            Other.StreamSize     = 0;
        }
        
        return *this;
    }
    
private:
    void Reserve(int32 NewCapacity)
    {
        if (NewCapacity > StreamCapacity)
        {
            uint8* NewStream = reinterpret_cast<uint8*>(FMemory::Malloc(NewCapacity));
            FMemory::Memcpy(NewStream, Stream, StreamSize);
            
            FMemory::Free(Stream);
            Stream = NewStream;

            StreamCapacity = NewCapacity;
        }
    }
    
    int32 AppendBytes(int32 NumBytes)
    {
        if ((StreamSize + NumBytes) > StreamCapacity)
        {
            const int32 NewCapacity = (StreamSize + NumBytes) << 1;
            Reserve(NewCapacity);
        }
        
        const int32 PreviousOffset = StreamSize;
        StreamSize += NumBytes;
        return PreviousOffset;
    }
    
    void InternalAllocate(int32 NewCapacity)
    {
        Stream = reinterpret_cast<uint8*>(FMemory::Malloc(NewCapacity));
    }
    
    void InternalFree()
    {
        FMemory::Free(Stream);
        
        StreamCapacity = 0;
        StreamSize     = 0;
    }
    
    // ByteStream
    uint8* Stream;
    // Size of the stream available to use
    int32  StreamCapacity;
    // Size of the stream that is used
    int32  StreamSize;
};

class FByteInputStream
{
public:
    FByteInputStream()
        : Stream(nullptr)
        , StreamSize(0)
        , StreamOffset(0)
    {
    }

    FByteInputStream(uint8* InStream, int32 InSize)
        : Stream(InStream)
        , StreamSize(InSize)
        , StreamOffset(0)
    {
    }
    
    FByteInputStream(const FByteInputStream& Other)
        : Stream(nullptr)
        , StreamSize(Other.StreamSize)
        , StreamOffset(Other.StreamSize)
    {
        InternalAllocate(Other.StreamSize);
        FMemory::Memcpy(Stream, Other.Stream, Other.StreamSize);
    }
    
    FByteInputStream(FByteInputStream&& Other)
        : Stream(Other.Stream)
        , StreamSize(Other.StreamSize)
        , StreamOffset(Other.StreamOffset)
    {
        Other.Stream       = nullptr;
        Other.StreamOffset = 0;
        Other.StreamSize   = 0;
    }
    
    ~FByteInputStream()
    {
        InternalFree();
    }
       
    template<typename T>
    int32 Read(T& OutValue)
    {
        return Read(reinterpret_cast<void*>(&OutValue), sizeof(T));
    }
    
    template<typename T>
    int32 Read(T* Elements, int32 NumElements)
    {
        const int32 TotalSize = sizeof(T) * NumElements;
        return Read(reinterpret_cast<void*>(Elements), TotalSize);
    }
    
    int32 Read(void* Dest, int32 Size)
    {
        uint8* Source = Stream + StreamOffset;
        FMemory::Memcpy(Dest, Source, Size);
        StreamOffset += Size;
        return Size;
    }
    
    int32 ReadOffset() const
    {
        return StreamOffset;
    }
    
    const uint8* Data() const
    {
        return Stream;
    }

    const uint8* PeekData(int32 Offset = 0) const
    {
        const int32 TotalOffset = StreamOffset + Offset;
        if (TotalOffset >= StreamSize)
        {
            return nullptr;
        }
        else
        {
            return Stream + TotalOffset;
        }
    }
    
    template<typename T>
    const T* PeekData(int32 Offset = 0) const
    {
        return reinterpret_cast<const T*>(PeekData(Offset));
    }
    
    int32 Size() const
    {
        return StreamSize;
    }
    
    FByteInputStream& operator=(const FByteInputStream& Other)
    {
        if (this != AddressOf(Other))
        {
            if (StreamSize < Other.StreamSize)
            {
                InternalFree();
                InternalAllocate(Other.StreamSize);
                
                StreamOffset = Other.StreamSize;
                StreamSize   = Other.StreamSize;
            }
            
            FMemory::Memcpy(Stream, Other.Stream, Other.StreamSize);
        }
        
        return *this;
    }
    
    FByteInputStream& operator=(FByteInputStream&& Other)
    {
        if (this != AddressOf(Other))
        {
            Stream       = Other.Stream;
            StreamOffset = Other.StreamOffset;
            StreamSize   = Other.StreamSize;
            
            Other.Stream       = nullptr;
            Other.StreamOffset = 0;
            Other.StreamSize   = 0;
        }
        
        return *this;
    }
    
private:
    void InternalAllocate(int32 NewCapacity)
    {
        Stream = reinterpret_cast<uint8*>(FMemory::Malloc(NewCapacity));
    }
    
    void InternalFree()
    {
        FMemory::Free(Stream);
        
        StreamOffset = 0;
        StreamSize   = 0;
    }
    
    // ByteStream
    uint8* Stream;
    // Size of the stream that is used
    int32 StreamSize;
    // Current position in the stream
    int32 StreamOffset;
};
