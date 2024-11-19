#pragma once
#include "Core/Core.h"

class FByteOutputStream
{
public:

    /**
     * @brief Default constructor initializes an empty byte stream.
     */
    FByteOutputStream()
        : Stream(nullptr)
        , StreamCapacity(0)
        , StreamSize(0)
    {
    }

    /**
     * @brief Copy constructor creates a deep copy of another byte output stream.
     * @param Other The other FByteOutputStream to copy from.
     */
    FByteOutputStream(const FByteOutputStream& Other)
        : Stream(nullptr)
        , StreamCapacity(Other.StreamSize)
        , StreamSize(Other.StreamSize)
    {
        InternalAllocate(Other.StreamSize);
        FMemory::Memcpy(Stream, Other.Stream, Other.StreamSize);
    }

    /**
     * @brief Move constructor transfers ownership of resources from another byte output stream.
     * @param Other The other FByteOutputStream to move from.
     */
    FByteOutputStream(FByteOutputStream&& Other)
        : Stream(Other.Stream)
        , StreamCapacity(Other.StreamCapacity)
        , StreamSize(Other.StreamSize)
    {
        Other.Stream         = nullptr;
        Other.StreamCapacity = 0;
        Other.StreamSize     = 0;
    }

    /**
     * @brief Destructor frees allocated memory.
     */
    ~FByteOutputStream()
    {
        InternalFree();
    }

    /**
     * @brief Adds uninitialized space for one object of type T to the stream.
     * @tparam T The type of the object.
     * @return The offset at which the uninitialized space begins.
     */
    template<typename T>
    int32 AddUninitialized()
    {
        const int32 TotalSize = sizeof(T);
        return AppendBytes(TotalSize);
    }

    /**
     * @brief Adds uninitialized space for multiple objects of type T to the stream.
     * @tparam T The type of the objects.
     * @param NumElements Number of elements to add.
     * @return The offset at which the uninitialized space begins.
     */
    template<typename T>
    int32 AddUninitialized(int32 NumElements)
    {
        const int32 TotalSize = sizeof(T) * NumElements;
        return AppendBytes(TotalSize);
    }

    /**
     * @brief Adds a single object of type T to the stream.
     * @tparam T The type of the object.
     * @param Value The object to add.
     * @return The offset at which the object was added.
     */
    template<typename T>
    int32 Add(const T& Value)
    {
        return Add(reinterpret_cast<const void*>(&Value), sizeof(T));
    }

    /**
     * @brief Adds multiple objects of type T to the stream.
     * @tparam T The type of the objects.
     * @param Elements Pointer to the array of elements.
     * @param NumElements Number of elements to add.
     * @return The offset at which the objects were added.
     */
    template<typename T>
    int32 Add(T* Elements, int32 NumElements)
    {
        const int32 TotalSize = sizeof(T) * NumElements;
        return Add(reinterpret_cast<const void*>(Elements), TotalSize);
    }

    /**
     * @brief Adds raw data to the stream.
     * @param Source Pointer to the source data.
     * @param Size Size of the data in bytes.
     * @return The offset at which the data was added.
     */
    int32 Add(const void* Source, int32 Size)
    {
        const int32 PreviousOffset = AppendBytes(Size);
        Write(Source, Size, PreviousOffset);
        return PreviousOffset;
    }

    /**
     * @brief Writes a single object of type T to a specific offset in the stream.
     * @tparam T The type of the object.
     * @param Value The object to write.
     * @param WriteOffset The offset at which to write the object.
     * @return The number of bytes written.
     */
    template<typename T>
    int32 Write(const T& Value, int32 WriteOffset)
    {
        return Write(reinterpret_cast<const void*>(&Value), sizeof(T), WriteOffset);
    }

    /**
     * @brief Writes multiple objects of type T to a specific offset in the stream.
     * @tparam T The type of the objects.
     * @param Elements Pointer to the array of elements.
     * @param NumElements Number of elements to write.
     * @param WriteOffset The offset at which to write the elements.
     * @return The number of bytes written.
     */
    template<typename T>
    int32 Write(T* Elements, int32 NumElements, int32 WriteOffset)
    {
        const int32 TotalSize = sizeof(T) * NumElements;
        return Write(reinterpret_cast<const void*>(Elements), TotalSize, WriteOffset);
    }

    /**
     * @brief Writes raw data to a specific offset in the stream.
     * @param Source Pointer to the source data.
     * @param Size Size of the data in bytes.
     * @param InWriteOffset The offset at which to write the data.
     * @return The number of bytes written.
     */
    int32 Write(const void* Source, int32 Size, int32 InWriteOffset)
    {
        uint8* Dest = Stream + InWriteOffset;
        FMemory::Memcpy(Dest, Source, Size);
        return Size;
    }

    /**
     * @brief Gets the current write offset, which is the size of the stream.
     * @return The current write offset.
     */
    int32 WriteOffset() const
    {
        // NOTE: The current write offset is the same as the size of the stream
        // however, in the future we might want to have a separate pointer that points
        // to the location that we should write into
        return StreamSize;
    }

    /**
     * @brief Gets a pointer to the stream data.
     * @return Pointer to the stream data.
     */
    uint8* Data()
    {
        return Stream;
    }

    /**
     * @brief Gets a const pointer to the stream data.
     * @return Const pointer to the stream data.
     */
    const uint8* Data() const
    {
        return Stream;
    }

    /**
     * @brief Gets the size of the stream.
     * @return The size of the stream in bytes.
     */
    int32 Size() const
    {
        return StreamSize;
    }

    /**
     * @brief Copy assignment operator.
     * @param Other The other FByteOutputStream to copy from.
     * @return Reference to this instance.
     */
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

    /**
     * @brief Move assignment operator.
     * @param Other The other FByteOutputStream to move from.
     * @return Reference to this instance.
     */
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
    uint8* Stream;        /**< Pointer to the stream data. */
    int32  StreamCapacity;/**< Total allocated capacity of the stream in bytes. */
    int32  StreamSize;    /**< Current size of the stream in bytes. */
};

class FByteInputStream
{
public:

    /**
     * @brief Default constructor initializes an empty byte input stream.
     */
    FByteInputStream()
        : Stream(nullptr)
        , StreamSize(0)
        , StreamOffset(0)
    {
    }

    /**
     * @brief Constructs a byte input stream from existing data.
     * @param InStream Pointer to the input stream data.
     * @param InSize Size of the input stream data in bytes.
     */
    FByteInputStream(uint8* InStream, int32 InSize)
        : Stream(InStream)
        , StreamSize(InSize)
        , StreamOffset(0)
    {
    }

    /**
     * @brief Copy constructor creates a deep copy of another byte input stream.
     * @param Other The other FByteInputStream to copy from.
     */
    FByteInputStream(const FByteInputStream& Other)
        : Stream(nullptr)
        , StreamSize(Other.StreamSize)
        , StreamOffset(Other.StreamSize)
    {
        InternalAllocate(Other.StreamSize);
        FMemory::Memcpy(Stream, Other.Stream, Other.StreamSize);
    }

    /**
     * @brief Move constructor transfers ownership of resources from another byte input stream.
     * @param Other The other FByteInputStream to move from.
     */
    FByteInputStream(FByteInputStream&& Other)
        : Stream(Other.Stream)
        , StreamSize(Other.StreamSize)
        , StreamOffset(Other.StreamOffset)
    {
        Other.Stream       = nullptr;
        Other.StreamOffset = 0;
        Other.StreamSize   = 0;
    }

    /**
     * @brief Destructor frees allocated memory.
     */
    ~FByteInputStream()
    {
        InternalFree();
    }

    /**
     * @brief Reads a single object of type T from the stream.
     * @tparam T The type of the object.
     * @param OutValue Reference to the variable where the read object will be stored.
     * @return The number of bytes read.
     */
    template<typename T>
    int32 Read(T& OutValue)
    {
        return Read(reinterpret_cast<void*>(&OutValue), sizeof(T));
    }

    /**
     * @brief Reads multiple objects of type T from the stream.
     * @tparam T The type of the objects.
     * @param Elements Pointer to the array where the read elements will be stored.
     * @param NumElements Number of elements to read.
     * @return The number of bytes read.
     */
    template<typename T>
    int32 Read(T* Elements, int32 NumElements)
    {
        const int32 TotalSize = sizeof(T) * NumElements;
        return Read(reinterpret_cast<void*>(Elements), TotalSize);
    }

    /**
     * @brief Reads raw data from the stream.
     * @param Dest Pointer to the destination buffer.
     * @param Size Number of bytes to read.
     * @return The number of bytes read.
     */
    int32 Read(void* Dest, int32 Size)
    {
        uint8* Source = Stream + StreamOffset;
        FMemory::Memcpy(Dest, Source, Size);
        StreamOffset += Size;
        return Size;
    }

    /**
     * @brief Gets the current read offset in the stream.
     * @return The current read offset in bytes.
     */
    int32 ReadOffset() const
    {
        return StreamOffset;
    }

    /**
     * @brief Gets a const pointer to the stream data.
     * @return Const pointer to the stream data.
     */
    const uint8* Data() const
    {
        return Stream;
    }

    /**
     * @brief Peeks at data in the stream without advancing the read offset.
     * @param Offset The offset from the current read position.
     * @return Const pointer to the data at the specified offset, or nullptr if out of bounds.
     */
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

    /**
     * @brief Peeks at data of type T in the stream without advancing the read offset.
     * @tparam T The type of the data to peek.
     * @param Offset The offset from the current read position.
     * @return Const pointer to the data of type T at the specified offset, or nullptr if out of bounds.
     */
    template<typename T>
    const T* PeekData(int32 Offset = 0) const
    {
        return reinterpret_cast<const T*>(PeekData(Offset));
    }

    /**
     * @brief Gets the size of the stream.
     * @return The size of the stream in bytes.
     */
    int32 Size() const
    {
        return StreamSize;
    }

    /**
     * @brief Copy assignment operator.
     * @param Other The other FByteInputStream to copy from.
     * @return Reference to this instance.
     */
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

    /**
     * @brief Move assignment operator.
     * @param Other The other FByteInputStream to move from.
     * @return Reference to this instance.
     */
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
    int32 StreamSize;
    int32 StreamOffset;
};
