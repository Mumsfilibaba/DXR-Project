#pragma once
#include "Defines.h"
#include "Types.h"

#include "Utilities/TUtilities.h"

/*
* Dynamic Array similar to std::vector
*/
template<typename T>
class TArray
{
public:
	typedef T		ValueType;
	typedef Uint32	SizeType;

	/*
	* Constant Iterator
	*/
	class ConstIterator
	{
		friend class TArray;

	public:
		~ConstIterator() = default;

		FORCEINLINE ConstIterator(ValueType* InPtr)
			: Ptr(InPtr)
		{
		}

		FORCEINLINE const ValueType* operator->() const
		{
			return Ptr;
		}

		FORCEINLINE const ValueType& operator*() const
		{
			return *Ptr;
		}

		FORCEINLINE ConstIterator operator++()
		{
			Ptr++;
			return *this;
		}

		FORCEINLINE ConstIterator operator++(Int32)
		{
			ConstIterator Temp = *this;
			Ptr++;
			return Temp;
		}

		FORCEINLINE ConstIterator operator--()
		{
			Ptr--;
			return *this;
		}

		FORCEINLINE ConstIterator operator--(Int32)
		{
			ConstIterator Temp = *this;
			Ptr--;
			return Temp;
		}

		FORCEINLINE ConstIterator operator+(Int32 Offset) const
		{
			ConstIterator Temp = *this;
			return Temp += Offset;
		}

		FORCEINLINE ConstIterator operator-(Int32 Offset) const
		{
			ConstIterator Temp = *this;
			return Temp -= Offset;
		}

		FORCEINLINE ConstIterator& operator+=(Int32 Offset)
		{
			Ptr += Offset;
			return *this;
		}

		FORCEINLINE ConstIterator& operator-=(Int32 Offset)
		{
			Ptr -= Offset;
			return *this;
		}

		FORCEINLINE bool operator==(const ConstIterator& Other) const
		{
			return (Ptr == Other.Ptr);
		}

		FORCEINLINE bool operator!=(const ConstIterator& Other) const
		{
			return (Ptr != Other.Ptr);
		}

		FORCEINLINE bool operator<(const ConstIterator& Other) const
		{
			return Ptr < Other.Ptr;
		}

		FORCEINLINE bool operator<=(const ConstIterator& Other) const
		{
			return Ptr <= Other.Ptr;
		}

		FORCEINLINE bool operator>(const ConstIterator& Other) const
		{
			return Ptr > Other.Ptr;
		}

		FORCEINLINE bool operator>=(const ConstIterator& Other) const
		{
			return Ptr >= Other.Ptr;
		}

	protected:
		ValueType* Ptr;
	};

	/*
	* Standard Iterator
	*/
	class Iterator : public ConstIterator
	{
		friend class TArray;

	public:
		~Iterator() = default;

		FORCEINLINE Iterator(ValueType* InPtr)
			: ConstIterator(InPtr)
		{
		}

		FORCEINLINE ValueType* operator->() const
		{
			return const_cast<ValueType*>(ConstIterator::operator->());
		}

		FORCEINLINE ValueType& operator*() const
		{
			return const_cast<ValueType&>(ConstIterator::operator*());
		}

		FORCEINLINE Iterator operator++()
		{
			ConstIterator::operator++();
			return *this;
		}

		FORCEINLINE Iterator operator++(Int32)
		{
			Iterator Temp = *this;
			ConstIterator::operator++();
			return Temp;
		}

		FORCEINLINE Iterator operator--()
		{
			ConstIterator::operator--();
			return *this;
		}

		FORCEINLINE Iterator operator--(Int32)
		{
			Iterator Temp = *this;
			ConstIterator::operator--();
			return Temp;
		}

		FORCEINLINE Iterator operator+(Int32 Offset) const
		{
			Iterator Temp = *this;
			return Temp += Offset;
		}

		FORCEINLINE Iterator operator-(Int32 Offset) const
		{
			Iterator Temp = *this;
			return Temp -= Offset;
		}

		FORCEINLINE Iterator& operator+=(Int32 Offset)
		{
			ConstIterator::operator+=(Offset);
			return *this;
		}

		FORCEINLINE Iterator& operator-=(Int32 Offset)
		{
			ConstIterator::operator-=(Offset);
			return *this;
		}
	};

	/*
	* TArray API
	*/
public:
	FORCEINLINE TArray() noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
	}

	FORCEINLINE TArray(SizeType InSize, const ValueType& Value = ValueType()) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		Assign(InSize, Value);
	}

	FORCEINLINE TArray(Iterator Begin, Iterator End) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		Assign(Begin, End);
	}

	FORCEINLINE TArray(std::initializer_list<ValueType> IList) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		Assign(IList);
	}

	FORCEINLINE TArray(const TArray& Other) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		Assign(Other.begin(), Other.end());
	}

	FORCEINLINE TArray(TArray&& Other) noexcept
		: Data(Other.Data)
		, Size(Other.Size)
		, Capacity(Other.Capacity)
	{
		Other.Data		= nullptr;
		Other.Size		= 0;
		Other.Capacity	= 0;
	}

	FORCEINLINE ~TArray()
	{
		Clear();
		Capacity = 0;

		if (Data)
		{
			delete[] Data;
			Data = nullptr;
		}
	}

	FORCEINLINE void Clear() noexcept
	{
		// Call destructor 
		for (Uint32 Index = 0; Index < Size; Index++)
		{
			Data[Index].~ValueType();
		}

		Size = 0;
	}

	FORCEINLINE void Assign(SizeType InSize, const ValueType& Value = ValueType()) noexcept
	{
		// Clear before assigning, calls destructors properly
		Clear();

		// Reserve enough space
		Reserve(InSize);

		// Set objects
		for (SizeType Index = 0; Index < InSize; Index++)
		{
			// Copy, cannot move since ValueType(ValueType&&) probably resets the 'value' object 
			Data[Index] = Value;
		}

		Size = InSize;
	}

	FORCEINLINE void Assign(Iterator Begin, Iterator End) noexcept
	{
		// Clear before assigning, calls destructors properly
		Clear();

		// Reserve enough space
		const SizeType Count = static_cast<SizeType>(End.Ptr - Begin.Ptr);
		Reserve(Count);

		// Set objects
		SizeType Index = 0;
		for (Iterator It = Begin; It != End; It++)
		{
			Data[Index] = *It;
			Index++;
		}

		Size = Count;
	}

	FORCEINLINE void Assign(std::initializer_list<ValueType> IList) noexcept
	{
		// Clear before assigning, calls destructors properly
		Clear();

		// Reserve enough space 
		Reserve(static_cast<SizeType>(IList.size()));

		// Move all objects
		SizeType Index = 0;
		for (const ValueType& Object : IList)
		{
			Data[Index] = Move(Object);
			Index++;
		}

		Size = static_cast<SizeType>(IList.size());
	}

	FORCEINLINE void Resize(SizeType InSize, const ValueType& Value = ValueType()) noexcept
	{
		if (InSize == Size)
		{
			return;
		}

		if (InSize == 0)
		{
			Clear();
		}
		else if (InSize > Size)
		{
			// Reserve enough space
			if (InSize >= Capacity)
			{
				Reserve(InSize + (Capacity / 2));
			}

			// Create new objects
			for (Uint32 Index = Size; Index < InSize; Index++)
			{
				Data[Index] = Value;
			}
		}
		else
		{
			// Destroy objects
			for (Uint32 Index = InSize; Index < Size; Index++)
			{
				Data[Index].~ValueType();
			}
		}

		Size = InSize;
	}

	FORCEINLINE void Reserve(SizeType InCapacity) noexcept
	{
		if (InCapacity <= Capacity)
		{
			return;
		}

		// Allocate new memory
		ValueType* TempData = new ValueType[InCapacity];

		// Move data to the new memory
		for (SizeType Index = 0; Index < Size; Index++)
		{
			TempData[Index] = Move(Data[Index]);
		}

		// Release and set memory
		if (Data)
		{
			delete[] Data;
		}

		Data = TempData;

		// Set capacity
		Capacity = InCapacity;
	}

	FORCEINLINE Iterator PushBack(const ValueType& Element) noexcept
	{
		// Reserve Space
		if (Size >= Capacity)
		{
			Reserve(Capacity + (Capacity / 2));
		}

		// Push element
		Data[Size++] = Element;
		return end();
	}

	FORCEINLINE Iterator PushBack(ValueType&& Element) noexcept
	{
		// Reserve Space
		if (Size >= Capacity)
		{
			Reserve(Capacity + (Capacity / 2));
		}

		// Push element
		Data[Size++] = Move(Element);
		return end();
	}

	template<typename... TArgs>
	FORCEINLINE Iterator EmplaceBack(TArgs&&... Args) noexcept
	{
		// Reserve Space
		if (Size >= Capacity)
		{
			Reserve(Capacity + (Capacity / 2));
		}

		// Push element
		new(&Data[Size++]) T(Forward<TArgs>(Args)...);
		return end();
	}

	template<typename... TArgs>
	FORCEINLINE Iterator Emplace(ConstIterator Pos, TArgs&&... Args) noexcept
	{
		// Special case
		if (Pos == end())
		{
			return EmplaceBack(Forward<TArgs>(Args)...);
		}

		// Emplace new element
		Iterator PosAfterMove = MakeSpace(Pos, 1);
		new (PosAfterMove.Ptr) ValueType(Forward<TArgs>(Args)...);
		Size++;

		return PosAfterMove;
	}

	FORCEINLINE Iterator Insert(ConstIterator Pos, const ValueType& Value) noexcept
	{
		// Special case
		if (Pos == end())
		{
			return PushBack(Value);
		}

		// Insert new element		
		Iterator PosAfterMove = MakeSpace(Pos, 1);
		(*PosAfterMove) = Value;
		Size++;

		return PosAfterMove;
	}

	FORCEINLINE Iterator Insert(ConstIterator Pos, ValueType&& Value) noexcept
	{
		// Special case
		if (Pos == end())
		{
			return PushBack(Value);
		}

		// Insert new element
		Iterator PosAfterMove = MakeSpace(Pos, 1);
		(*PosAfterMove) = Move(Value);
		Size++;

		return PosAfterMove;
	}

	FORCEINLINE Iterator Insert(ConstIterator Pos, std::initializer_list<ValueType> IList) noexcept
	{
		// Insert new element
		const SizeType Count = static_cast<SizeType>(IList.size());
		Iterator PosAfterMove = MakeSpace(Pos, Count);

		Iterator It = PosAfterMove;
		for (const ValueType& Value : IList)
		{
			(*It) = Move(Value);
			It++;
		}

		Size += Count;
		return PosAfterMove;
	}

	FORCEINLINE Iterator PopBack() noexcept
	{
		Size--;
		Data[Size].~ValueType();

		return end();
	}

	FORCEINLINE Iterator Erase(Iterator Pos) noexcept
	{
		return Erase(static_cast<ConstIterator>(Pos));
	}

	FORCEINLINE Iterator Erase(ConstIterator Pos) noexcept
	{
		// Special case
		if (Pos == end())
		{
			return PopBack();
		}

		// Call destructor
		(*Pos).~ValueType();

		// Move elements
		const SizeType Index = static_cast<SizeType>(Pos.Ptr - begin().Ptr);

		Iterator From = begin() + Index;
		for (Iterator It = From; (It != end()); It++)
		{
			From++;
			(*It) = Move(*From);
		}

		Size--;
		return begin() + Index;
	}

	FORCEINLINE Iterator Erase(Iterator First, Iterator Last) noexcept
	{
		return Erase(static_cast<ConstIterator>(First), static_cast<ConstIterator>(Last));
	}

	FORCEINLINE Iterator Erase(ConstIterator First, ConstIterator Last) noexcept
	{
		// Call destructors
		for (ConstIterator It = First; (It != Last); It++)
		{
			(*It).~ValueType();
		}

		// Move elements
		const SizeType Index	= static_cast<SizeType>(First.Ptr - begin().Ptr);
		const SizeType Offset	= static_cast<SizeType>(Last.Ptr - First.Ptr);

		Iterator To = begin() + Index;
		for (Iterator It = To + Offset; (It != end()); It++)
		{
			(*To) = Move(*It);
			To++;
		}

		Size -= Offset;
		return begin() + Index;
	}

	FORCEINLINE void Swap(TArray& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			ValueType*	TempPtr			= Data;
			SizeType	TempSize		= Size;
			SizeType	TempCapacity	= Capacity;

			Data		= Other.Data;
			Size		= Other.Size;
			Capacity	= Other.Capacity;

			Other.Data		= TempPtr;
			Other.Size		= TempSize;
			Other.Capacity	= TempCapacity;
		}
	}

	FORCEINLINE void ShrinkToFit() noexcept
	{
		if (Capacity > Size)
		{
			// Allocate new data and move it
			ValueType* TempData = new ValueType[Size];
			for (Uint32 Index = 0; Index < Size; Index++)
			{
				TempData[Index] = Move(Data[Index]);
			}

			// Delete old data
			delete[] Data;
			Data = TempData;

			// Set new capacity
			Capacity = Size;
		}
	}

	FORCEINLINE bool IsEmpty() const noexcept
	{
		return (Size == 0);
	}

	FORCEINLINE ValueType& GetFront() noexcept
	{
		VALIDATE(Size > 0);
		return Data[0];
	}

	FORCEINLINE const ValueType& GetFront() const noexcept
	{
		VALIDATE(Size > 0);
		return Data[0];
	}

	FORCEINLINE ValueType& GetBack() noexcept
	{
		VALIDATE(Size > 0);
		return Data[Size - 1];
	}

	FORCEINLINE const ValueType& GetBack() const noexcept
	{
		VALIDATE(Size > 0);
		return Data[Size - 1];
	}

	FORCEINLINE ValueType* GetData() noexcept
	{
		return Data;
	}

	FORCEINLINE const ValueType* GetData() const noexcept
	{
		return Data;
	}

	FORCEINLINE SizeType GetSize() const noexcept
	{
		return Size;
	}

	FORCEINLINE SizeType GetCapacity() const noexcept
	{
		return Capacity;
	}

	FORCEINLINE ValueType& GetElementAt(SizeType Index) noexcept
	{
		VALIDATE(Index < Size);
		return Data[Index];
	}

	FORCEINLINE const ValueType& GetElementAt(SizeType Index) const noexcept
	{
		VALIDATE(Index < Size);
		return Data[Index];
	}

	FORCEINLINE TArray& operator=(const TArray& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			// Call clear so that all destructors gets called
			Clear();

			// Reserve enough space
			Reserve(Other.Capacity);

			// Copy
			Size = Other.Size;
			for (Uint32 Index = 0; Index < Size; Index++)
			{
				Data[Index] = Other.Data[Index];
			}
		}

		return *this;
	}

	FORCEINLINE TArray& operator=(TArray&& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Data		= Other.Data;
			Size		= Other.Size;
			Capacity	= Other.Capacity;

			Other.Data		= nullptr;
			Other.Size		= 0;
			Other.Capacity	= 0;
		}

		return *this;
	}

	FORCEINLINE TArray& operator=(std::initializer_list<ValueType> IList) noexcept
	{
		// Clear the old data
		Clear();

		// Assign it to the IList
		Assign(IList);

		return *this;
	}

	FORCEINLINE ValueType& operator[](SizeType Index) noexcept
	{
		return GetElementAt(Index);
	}

	FORCEINLINE const ValueType& operator[](SizeType Index) const noexcept
	{
		return GetElementAt(Index);
	}

	/*
	* STL iterator functions
	*/
public:
	FORCEINLINE Iterator begin() noexcept
	{
		return Iterator(Data);
	}

	FORCEINLINE Iterator end() noexcept
	{
		return Iterator(Data + Size);
	}

	FORCEINLINE ConstIterator begin() const noexcept
	{
		return ConstIterator(Data);
	}

	FORCEINLINE ConstIterator end() const noexcept
	{
		return ConstIterator(Data + Size);
	}

private:
	FORCEINLINE Iterator MakeSpace(ConstIterator Pos, SizeType InSize) noexcept
	{
		// Reserve Space
		const SizeType NewSize	= Size + InSize;
		const SizeType Offset	= InSize - 1;
		const SizeType Index	= static_cast<SizeType>(Pos.Ptr - begin().Ptr);
		if (NewSize >= Capacity)
		{
			Reserve(NewSize + (Capacity / 2));
		}

		// Move all objects
		Iterator Begin	= begin() + Index;
		Iterator End	= Begin + Offset;
		Iterator From	= end();
		for (Iterator It = (From + Offset); (It != End); It--)
		{
			From--;
			(*It) = Move(*From);
		}

		// Return new pos
		return Begin;
	}

	ValueType* Data;
	SizeType Size;
	SizeType Capacity;
};